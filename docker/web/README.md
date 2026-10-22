# Realms web image

PHP 8.4.27 Apache image. Application files are deployed separately.

## Build and test

Run from the repository root on the target host architecture. Requires Docker,
curl, tar, and sha256sum or shasum; tests also require Python 3 and openssl.

```sh
sh docker/web/web-build-base.sh
sh docker/web/web-build.sh
sh docker/web/web-test.sh
```

The base helper builds a pinned official PHP recipe and verifies its checksum.
Set `REALMS_PHP_IMAGE` consistently for both build scripts to override the base
tag. Set `REALMS_WEB_IMAGE` consistently for build, test, and run.
The smoke test uses a temporary container and certificate on loopback ports;
it checks HTTP/TLS and configuration, then removes its resources.

## Runtime configuration

Provide existing application/data directories and these readable files under
`REALMS_WEB_CONFIG`: `realms.conf`, `realms-ssl.conf`, `ports.conf`,
`server.crt`, and `server.key`. Configuration and TLS files mount read-only.

| Variable | Default |
|-|-|
| REALMS_WEB_IMAGE | realms-web:php8.4.27 |
| REALMS_WEB_NAME | realms-web |
| REALMS_WEB_ROOT | /home/realms/realms/www |
| REALMS_WEB_DATA | /home/realms/realms |
| REALMS_WEB_CONFIG | /home/realms/realms/RealmsCode/docker/web/conf |
| REALMS_WEB_BIND_ADDRESS | Empty: all host interfaces |
| REALMS_WEB_HTTP_PORT | 48 |
| REALMS_WEB_HTTPS_PORT | 47 |

Application and game data mount writable at `/home/realms/www` and
`/home/realms/realms`. The launcher does not change ownership.
HTTP port 48 serves `public`; HTTPS port 47 serves `private`.
The supplied virtual hosts disable `.htaccess` through `AllowOverride None`.

PHP uses `php.ini-production` plus `docker/web/php.ini`: short tags enabled,
version headers hidden, upload progress disabled. Legacy `conf/php.ini` is not
mounted. Certificates and application files are excluded from the image.

## Run

```sh
sh docker/web/web-run.sh
sh docker/web/web-exec.sh
```

For staging, use `REALMS_WEB_BIND_ADDRESS=127.0.0.1`, unused ports, disposable
data copies, and a distinct `REALMS_WEB_NAME` if the old container still exists.
`web-exec.sh` also accepts a command; `REALMS_WEB_CIDFILE` records the launched ID.

The PHP application passed 18 synthetic and 18 snapshot test groups against
this image with an emulated game interface. Production inventory handlers are
disabled in RealmsCode master. Before public startup, validate the production
TLS/configuration, worker permissions, and actual game-server integration.
Keep the stopped PHP 7 service offline if validation fails.
