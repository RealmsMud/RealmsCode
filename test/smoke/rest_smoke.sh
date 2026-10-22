#!/bin/sh
#
# rest_smoke.sh - live smoke test for the account/creation REST API.
#
# Run against a freshly-built, freshly-started server (config.xml needs a non-empty
# <JwtSecret>; see docker/dev/dev-server.sh or the macOS native build):
#
#   sh test/smoke/rest_smoke.sh
#
# Walks POST /api/account/create|login|legacy-login|claim and the full
# /api/creation/start -> step* -> view -> commit wizard, following
# view.options[0].value at each step (fixed values only for the "stats" and "name"
# steps). Tamper checks (out-of-order path, replaying a completed path) and a
# banned-account 403 are covered too. Login throttle is NOT covered here (see
# test/api-smoke.sh's RUN_THROTTLE section).
#
# Env vars:
#   API_BASE   base URL                (default http://localhost:8080)
#   DATA_DIR   REALMS_BASE_PATH the server is running against, for the banned-account
#              seed file and for restoring player/Abc.xml (default ~/realms/realms)
#
# Same helper style as test/api-smoke.sh (status/expect/auth, python3 for JSON).
# Cleanup (accounts/players created here, plus the banned-account seed and
# player/Abc.xml) runs even on failure.
set -u

API_BASE="${API_BASE:-http://localhost:8080}"
DATA_DIR="${DATA_DIR:-$HOME/realms/realms}"
BODY="$(mktemp)"
CUR="$(mktemp)"
PATHFILE="$(mktemp)"
JTOOL="$(mktemp)"
ABC_BACKUP="$(mktemp)"

pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  PASS  %s\n' "$1"; }
no()  { fail=$((fail+1)); printf '  FAIL  %s\n' "$1"; }

# status METHOD PATH [curl args...] -> prints HTTP code; body written to $BODY.
status() {
    method="$1"; path="$2"; shift 2
    curl -s -o "$BODY" -w '%{http_code}' -X "$method" "$@" "$API_BASE$path"
}

# expect EXPECTED METHOD PATH DESC [curl args...]
expect() {
    exp="$1"; method="$2"; path="$3"; desc="$4"; shift 4
    got="$(status "$method" "$path" "$@")"
    if [ "$got" = "$exp" ]; then ok "$desc ($method $path -> $got)"
    else no "$desc ($method $path -> got $got, want $exp)"; fi
}

auth() { printf 'Authorization: Bearer %s' "$1"; }
json_ct() { printf 'Content-Type: application/json'; }
jget() { python3 "$JTOOL" get "$1" "$2"; }
rand6() { python3 -c "import random,string;print(''.join(random.choice(string.ascii_lowercase) for _ in range(6)))"; }

cleanup() {
    [ -n "${SMK_ACCT:-}" ] && rm -f "$DATA_DIR/accounts/$SMK_ACCT.json"
    [ -n "${SMK_CHAR:-}" ] && rm -f "$DATA_DIR/player/$SMK_CHAR.xml" "$DATA_DIR/player/backup/$SMK_CHAR.bak.xml"
    rm -f "$DATA_DIR/accounts/Smkbanned.json"
    if [ -s "$ABC_BACKUP" ]; then
        cp "$ABC_BACKUP" "$DATA_DIR/player/Abc.xml"
        echo "restored player/Abc.xml"
    fi
    rm -f "$BODY" "$CUR" "$PATHFILE" "$JTOOL" "$ABC_BACKUP"
}
trap cleanup EXIT INT TERM

cat > "$JTOOL" <<'PYEOF'
# jtool.py -- tiny JSON helper for rest_smoke.sh. All data flows through files/argv,
# never through shell-interpolated python source, to avoid quoting bugs.
import json
import sys


def load(path):
    with open(path) as f:
        return json.load(f)


def dump(obj, path):
    with open(path, "w") as f:
        json.dump(obj, f)


def get(path, dotted):
    d = load(path)
    for k in dotted.split("."):
        if isinstance(d, list):
            d = d[int(k)] if k.lstrip("-").isdigit() and -len(d) <= int(k) < len(d) else None
        elif isinstance(d, dict):
            d = d.get(k)
        else:
            d = None
        if d is None:
            break
    if d is None:
        print("")
    elif isinstance(d, (dict, list)):
        print(json.dumps(d))
    else:
        print(d)


def append_step(path_file, step, value):
    try:
        arr = load(path_file)
    except (FileNotFoundError, json.JSONDecodeError):
        arr = []
    arr.append({"step": step, "value": value})
    dump(arr, path_file)


def swap01(infile, outfile):
    arr = load(infile)
    if len(arr) >= 2:
        arr[0], arr[1] = arr[1], arr[0]
    dump(arr, outfile)


if __name__ == "__main__":
    cmd = sys.argv[1]
    if cmd == "get":
        get(sys.argv[2], sys.argv[3])
    elif cmd == "append":
        append_step(sys.argv[2], sys.argv[3], sys.argv[4])
    elif cmd == "swap01":
        swap01(sys.argv[2], sys.argv[3])
    else:
        sys.exit(f"unknown jtool command: {cmd}")
PYEOF

if [ -f "$DATA_DIR/player/Abc.xml" ]; then
    cp "$DATA_DIR/player/Abc.xml" "$ABC_BACKUP"
else
    echo "WARN: $DATA_DIR/player/Abc.xml not found; claim/legacy-login checks may fail" >&2
fi

SMK_ACCT="Smk$(rand6)"
SMK_PW="SmkGood123"
SMK_CHAR="Smk$(rand6)"

echo "== account/create =="
expect 400 POST /api/account/create "non-string name -> 400" \
    -H "$(json_ct)" -d '{"name":123,"pw":"whatever12"}'
expect 400 POST /api/account/create "short password -> 400" \
    -H "$(json_ct)" -d "{\"name\":\"$SMK_ACCT\",\"pw\":\"ab1\"}"
expect 200 POST /api/account/create "create succeeds -> 200" \
    -H "$(json_ct)" -d "{\"name\":\"$SMK_ACCT\",\"pw\":\"$SMK_PW\"}"
ACCT_TOKEN="$(jget "$BODY" token)"
[ -n "$ACCT_TOKEN" ] && ok "create returned a token" || no "create response had no token"
expect 409 POST /api/account/create "duplicate create -> 409" \
    -H "$(json_ct)" -d "{\"name\":\"$SMK_ACCT\",\"pw\":\"$SMK_PW\"}"

echo "== account/login =="
expect 404 POST /api/account/login "unknown account -> 404" \
    -H "$(json_ct)" -d '{"name":"nosuchsmkacct","pw":"whatever12"}'
expect 401 POST /api/account/login "wrong password -> 401" \
    -H "$(json_ct)" -d "{\"name\":\"$SMK_ACCT\",\"pw\":\"definitely-wrong\"}"
expect 200 POST /api/account/login "correct password -> 200" \
    -H "$(json_ct)" -d "{\"name\":\"$SMK_ACCT\",\"pw\":\"$SMK_PW\"}"
LOGIN_TOKEN="$(jget "$BODY" token)"
[ -n "$LOGIN_TOKEN" ] && ok "login returned a token" || no "login response had no token"

if [ -z "$ACCT_TOKEN" ]; then
    echo "  SKIP  creation walk (no account token from create)"
else
    echo "== creation wizard walk (account: $SMK_ACCT) =="
    printf '[]' > "$PATHFILE"
    expect 200 GET /api/creation/start "creation start -> 200" -H "$(auth "$ACCT_TOKEN")"
    cp "$BODY" "$CUR"

    walk_ok=1
    i=0
    while [ "$i" -lt 30 ]; do
        i=$((i + 1))
        donef="$(jget "$CUR" done)"
        [ "$donef" = "True" ] && break
        step="$(jget "$CUR" view.step)"
        inputKind="$(jget "$CUR" view.inputKind)"
        if [ "$step" = "name" ]; then
            value="$SMK_CHAR"
        elif [ "$inputKind" = "stats" ]; then
            value="12 12 12 10 10"
        else
            value="$(jget "$CUR" view.options.0.value)"
        fi

        pathjson="$(cat "$PATHFILE")"
        code="$(status POST /api/creation/step -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" \
                -d "{\"path\":$pathjson,\"value\":\"$value\"}")"
        if [ "$code" != "200" ]; then
            no "creation step '$step' (value '$value') -> got $code, want 200"
            walk_ok=0
            break
        fi
        cp "$BODY" "$CUR"
        if [ "$(jget "$CUR" ok)" != "True" ]; then
            no "creation step '$step' rejected: $(jget "$CUR" error.message)"
            walk_ok=0
            break
        fi
        python3 "$JTOOL" append "$PATHFILE" "$step" "$value"
        donef="$(jget "$CUR" done)"
        [ "$donef" = "True" ] && break
    done

    if [ "$walk_ok" -eq 1 ] && [ "$(jget "$CUR" done)" = "True" ]; then
        ok "creation wizard reached done after $i step(s)"
    else
        no "creation wizard did not reach done within $i step(s)"
    fi

    echo "== creation view/tamper =="
    finalpath="$(cat "$PATHFILE")"
    expect 200 POST /api/creation/view "view of completed path -> 200" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d "{\"path\":$finalpath}"
    [ "$(jget "$BODY" done)" = "True" ] && ok "view reports done" || no "view does not report done"

    SWAPPED="$(mktemp)"
    python3 "$JTOOL" swap01 "$PATHFILE" "$SWAPPED"
    swappedpath="$(cat "$SWAPPED")"
    rm -f "$SWAPPED"
    expect 400 POST /api/creation/step "out-of-order path -> 400" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d "{\"path\":$swappedpath,\"value\":\"x\"}"
    expect 409 POST /api/creation/step "completed path replayed to step -> 409" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d "{\"path\":$finalpath,\"value\":\"x\"}"

    echo "== creation commit =="
    expect 200 POST /api/creation/commit "commit -> 200" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d "{\"path\":$finalpath}"
    committed_name="$(jget "$BODY" name)"
    [ "$committed_name" = "$SMK_CHAR" ] && ok "commit body has the chosen name" || no "commit body name mismatch: $committed_name"
    [ -n "$(jget "$BODY" level)" ] && ok "commit body has level" || no "commit body missing level"
    [ -n "$(jget "$BODY" class)" ] && ok "commit body has class" || no "commit body missing class"
    expect 409 POST /api/creation/commit "commit again -> 409 (name taken)" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d "{\"path\":$finalpath}"
fi

echo "== account/legacy-login =="
expect 404 POST /api/account/legacy-login "unknown character -> 404" \
    -H "$(json_ct)" -d '{"name":"nosuchsmkchar","pw":"whatever12"}'
expect 401 POST /api/account/legacy-login "wrong password -> 401" \
    -H "$(json_ct)" -d '{"name":"Abcdefg","pw":"definitely-wrong"}'
expect 200 POST /api/account/legacy-login "correct password -> 200" \
    -H "$(json_ct)" -d '{"name":"Abcdefg","pw":"abc123"}'
[ -n "$(jget "$BODY" token)" ] && ok "legacy-login returned a token" || no "legacy-login response had no token"

if [ -z "$ACCT_TOKEN" ]; then
    echo "  SKIP  claim checks (no account token from create)"
else
    echo "== account/claim =="
    expect 401 POST /api/account/claim "wrong character password -> 401" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d '{"charName":"Abc","charPassword":"definitely-wrong"}'
    expect 200 POST /api/account/claim "claim Abc/lc2002 -> 200" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d '{"charName":"Abc","charPassword":"lc2002"}'
    [ "$(jget "$BODY" claimed)" = "Abc" ] && ok "claim body names Abc" || no "claim body missing/wrong 'claimed'"
    expect 409 POST /api/account/claim "already-claimed -> 409" \
        -H "$(json_ct)" -H "$(auth "$ACCT_TOKEN")" -d '{"charName":"Abc","charPassword":"lc2002"}'
fi

echo "== banned account =="
mkdir -p "$DATA_DIR/accounts"
cat > "$DATA_DIR/accounts/Smkbanned.json" <<EOF
{"accountName":"Smkbanned","password":"SmkGood123","banned":true,"banReason":"smoke test","characterNames":[],"email":"","version":"2.63a"}
EOF
expect 403 POST /api/account/login "banned account -> 403" \
    -H "$(json_ct)" -d '{"name":"Smkbanned","pw":"SmkGood123"}'

echo
echo "==== $pass passed, $fail failed ===="
[ "$fail" -eq 0 ]
