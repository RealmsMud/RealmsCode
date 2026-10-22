#!/bin/sh
set -eu

web_root=${REALMS_WEB_ROOT:-/home/realms/realms/www}
game_data=${REALMS_WEB_DATA:-/home/realms/realms}
config_dir=${REALMS_WEB_CONFIG:-/home/realms/realms/RealmsCode/docker/web/conf}
bind_address=${REALMS_WEB_BIND_ADDRESS:-}
http_port=${REALMS_WEB_HTTP_PORT:-48}
https_port=${REALMS_WEB_HTTPS_PORT:-47}

if [ -n "$bind_address" ]; then
    http_port=$bind_address:$http_port
    https_port=$bind_address:$https_port
fi

for directory in "$web_root" "$game_data" "$config_dir"; do
    if [ ! -d "$directory" ]; then
        printf 'Required directory is missing: %s\n' "$directory" >&2
        exit 1
    fi
done

web_root=$(CDPATH= cd -- "$web_root" && pwd)
game_data=$(CDPATH= cd -- "$game_data" && pwd)
config_dir=$(CDPATH= cd -- "$config_dir" && pwd)

for filename in realms.conf realms-ssl.conf ports.conf server.crt server.key; do
    if [ ! -f "$config_dir/$filename" ] || [ ! -r "$config_dir/$filename" ]; then
        printf 'Required configuration file is missing or unreadable: %s\n' "$config_dir/$filename" >&2
        exit 1
    fi
done

set --
if [ -n "${REALMS_WEB_CIDFILE:-}" ]; then
    set -- --cidfile "$REALMS_WEB_CIDFILE"
fi

exec docker run -d \
    "$@" \
    --mount "type=bind,src=$web_root,dst=/home/realms/www" \
    --mount "type=bind,src=$game_data,dst=/home/realms/realms" \
    --mount "type=bind,src=$config_dir/realms.conf,dst=/etc/apache2/sites-available/realms.conf,readonly" \
    --mount "type=bind,src=$config_dir/realms-ssl.conf,dst=/etc/apache2/sites-available/realms-ssl.conf,readonly" \
    --mount "type=bind,src=$config_dir/ports.conf,dst=/etc/apache2/ports.conf,readonly" \
    --mount "type=bind,src=$config_dir/server.crt,dst=/etc/ssl/certs/server.crt,readonly" \
    --mount "type=bind,src=$config_dir/server.key,dst=/etc/ssl/private/server.key,readonly" \
    -p "$http_port:48" \
    -p "$https_port:47" \
    --name "${REALMS_WEB_NAME:-realms-web}" \
    "${REALMS_WEB_IMAGE:-realms-web:php8.4.27}"
