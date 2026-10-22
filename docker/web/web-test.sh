#!/bin/sh
set -eu

exec python3 -c 'import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

script_dir = Path(sys.argv[1]).resolve().parent
if len(sys.argv) > 3:
    raise SystemExit("Usage: web-test.sh [candidate-image]")
image = sys.argv[2] if len(sys.argv) == 3 else os.environ.get("REALMS_WEB_IMAGE", "realms-web:php8.4.27")
for program in ("docker", "curl", "openssl"):
    if not shutil.which(program):
        raise SystemExit("Required command is missing: " + program)

deadline = time.monotonic() + 110
container_id = None
cidfile = None
root = None
test_failed = False

def interrupted(signum, frame):
    raise RuntimeError("Smoke test interrupted")

signal.signal(signal.SIGTERM, interrupted)
signal.signal(signal.SIGINT, interrupted)

def run(args, check=True, env=None):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise RuntimeError("Smoke test exceeded its time budget")
    result = subprocess.run(args, capture_output=True, text=True, timeout=remaining, env=env)
    if check and result.returncode:
        raise RuntimeError("Command failed: " + args[0] + "\n" + result.stderr.strip())
    return result

def request(url, *extra):
    return run(["curl", "--silent", "--show-error", "--connect-timeout", "2", "--max-time", "5", *extra, url])

try:
    run(["docker", "image", "inspect", image])
    root = Path(tempfile.mkdtemp(prefix="realms-web-test-")).resolve()
    web = root / "www"
    data = root / "data"
    config = root / "config"
    for directory in (web, data, config):
        directory.mkdir()
    cidfile = root / "container.id"
    runtime_probe = """<?php
header("Content-Type: application/json");
echo json_encode([
    "root" => basename(__DIR__),
    "version" => PHP_VERSION_ID,
    "sapi" => PHP_SAPI,
    "xml" => function_exists("xml_parser_create"),
    "curl" => function_exists("curl_init"),
    "zlib" => function_exists("ob_gzhandler"),
    "upload_progress" => ini_get("session.upload_progress.enabled"),
    "memory_limit" => ini_get("memory_limit"),
    "expose_php" => ini_get("expose_php"),
]);
"""
    for name in ("public", "private"):
        document_root = web / name
        document_root.mkdir()
        document_root.joinpath("index.php").write_text(runtime_probe)
        document_root.joinpath("short.php").write_text("""<? echo "short-tags-ok"; ?>""")
        document_root.joinpath(name + "-only.txt").write_text(name + "-root")
    for filename in ("realms.conf", "realms-ssl.conf", "ports.conf"):
        shutil.copyfile(script_dir / "test" / filename, config / filename)
    run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
         "-subj", "/CN=localhost", "-keyout", str(config / "server.key"),
         "-out", str(config / "server.crt")])
    env = os.environ.copy()
    env.update({
        "REALMS_WEB_ROOT": str(web),
        "REALMS_WEB_DATA": str(data),
        "REALMS_WEB_CONFIG": str(config),
        "REALMS_WEB_NAME": root.name,
        "REALMS_WEB_IMAGE": image,
        "REALMS_WEB_BIND_ADDRESS": "127.0.0.1",
        "REALMS_WEB_HTTP_PORT": "0",
        "REALMS_WEB_HTTPS_PORT": "0",
        "REALMS_WEB_CIDFILE": str(cidfile),
    })
    launcher = ["sh", str(script_dir / "web-run.sh")]
    key = config / "server.key"
    hidden_key = config / "server.key.saved"
    key.rename(hidden_key)
    missing = run(launcher, check=False, env=env)
    hidden_key.rename(key)
    if missing.returncode == 0 or "server.key" not in missing.stderr or cidfile.exists():
        raise RuntimeError("Launcher did not reject missing runtime configuration")
    launched = run(launcher, env=env)
    container_id = launched.stdout.strip()
    if not re.fullmatch("[0-9a-f]{64}", container_id):
        raise RuntimeError("Launcher did not return a container ID")
    inspect = json.loads(run(["docker", "inspect", container_id]).stdout)[0]
    expected = {
        "/home/realms/www": (web, True),
        "/home/realms/realms": (data, True),
        "/etc/apache2/sites-available/realms.conf": (config / "realms.conf", False),
        "/etc/apache2/sites-available/realms-ssl.conf": (config / "realms-ssl.conf", False),
        "/etc/apache2/ports.conf": (config / "ports.conf", False),
        "/etc/ssl/certs/server.crt": (config / "server.crt", False),
        "/etc/ssl/private/server.key": (config / "server.key", False),
    }
    mounts = {mount["Destination"]: mount for mount in inspect["Mounts"]}
    for destination, (source, writable) in expected.items():
        mount = mounts.get(destination, {})
        if mount.get("Type") != "bind" or mount.get("Source") != str(source) or mount.get("RW") != writable:
            raise RuntimeError("Unexpected runtime mount: " + destination)
    ports = inspect["NetworkSettings"]["Ports"]
    urls = []
    for port, scheme, document_root in (("48/tcp", "http", "public"), ("47/tcp", "https", "private")):
        bindings = ports.get(port)
        if not bindings or any(binding["HostIp"] != "127.0.0.1" for binding in bindings):
            raise RuntimeError("Fixture port is not restricted to loopback")
        host_port = bindings[0]["HostPort"]
        urls.append((scheme + "://localhost:" + host_port, host_port, document_root))
    ready_until = min(deadline, time.monotonic() + 30)
    while True:
        probe = run(["curl", "--silent", "--connect-timeout", "1", "--max-time", "2",
                     "--fail", urls[0][0] + "/index.php"], check=False)
        if probe.returncode == 0:
            break
        if time.monotonic() >= ready_until:
            raise RuntimeError("Apache did not become ready")
        time.sleep(0.25)
    for base, port, document_root in urls:
        options = ["--resolve", "localhost:" + port + ":127.0.0.1"]
        if base.startswith("https"):
            options += ["--cacert", str(config / "server.crt")]
        headers_path = root / "response-headers"
        result = json.loads(request(base + "/index.php", *options, "--fail",
                                    "--dump-header", str(headers_path)).stdout)
        if str(result["expose_php"]).lower() not in ("", "0", "off"):
            raise RuntimeError("PHP version exposure is enabled")
        if any(line.lower().startswith("x-powered-by:") for line in headers_path.read_text().splitlines()):
            raise RuntimeError("Apache exposed the PHP version header")
        if not 80427 <= result["version"] < 80500 or result["sapi"] != "apache2handler":
            raise RuntimeError("Apache is not serving patched PHP 8.4")
        if not all(result.get(extension) is True for extension in ("xml", "curl", "zlib")):
            raise RuntimeError("A required PHP extension is missing")
        if str(result["upload_progress"]).lower() not in ("", "0", "off"):
            raise RuntimeError("Session upload progress is enabled")
        memory_limit = re.fullmatch(r"([0-9]+)([KMG]?)", str(result["memory_limit"]), re.IGNORECASE)
        if memory_limit is None or int(memory_limit[1]) <= 0:
            raise RuntimeError("PHP memory limit is not positive and finite")
        if request(base + "/short.php", *options, "--fail").stdout != "short-tags-ok":
            raise RuntimeError("PHP short tags are not enabled")
        if result["root"] != document_root:
            raise RuntimeError("Protocol and port reached the wrong document root")
        marker = request(base + "/" + document_root + "-only.txt", *options, "--fail").stdout
        if marker != document_root + "-root":
            raise RuntimeError("Document root marker mismatch")
        other_root = "private" if document_root == "public" else "public"
        missing = request(base + "/" + other_root + "-only.txt", *options,
                          "--output", str(root / "missing-body"), "--write-out", "%{http_code}")
        if missing.stdout != "404" or other_root + "-root" in (root / "missing-body").read_text():
            raise RuntimeError("A document root exposed the other root fixture")

    print("PASS: patched PHP 8.4 through Apache HTTP/TLS, extensions, short tags, runtime mounts, missing config rejection, and public/private port routing.")
except Exception as error:
    test_failed = True
    print("FAIL: " + str(error), file=sys.stderr)
    sys.exit(1)
finally:
    if not re.fullmatch("[0-9a-f]{64}", container_id or "") and cidfile is not None and cidfile.is_file():
        container_id = cidfile.read_text().strip()
    if test_failed and container_id and re.fullmatch("[0-9a-f]{64}", container_id):
        remaining = min(2, deadline - time.monotonic())
        if remaining > 0:
            try:
                logs = subprocess.run(["docker", "logs", "--tail", "40", container_id],
                                      capture_output=True, text=True, timeout=remaining)
                print((logs.stdout + logs.stderr)[-12000:], file=sys.stderr)
            except (subprocess.TimeoutExpired, OSError):
                pass
    cleanup_failed = False
    if container_id and re.fullmatch("[0-9a-f]{64}", container_id):
        try:
            removed = subprocess.run(["docker", "rm", "-f", container_id], capture_output=True, text=True, timeout=8)
            if removed.returncode:
                cleanup_failed = True
                print("Container cleanup failed: " + container_id, file=sys.stderr)
        except (subprocess.TimeoutExpired, OSError):
            cleanup_failed = True
            print("Container cleanup failed or timed out: " + container_id, file=sys.stderr)
    if root is not None:
        shutil.rmtree(root)
    if cleanup_failed:
        sys.exit(1)
' "$0" "$@"
