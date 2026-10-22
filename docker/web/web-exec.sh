#!/bin/sh
set -eu

if [ "$#" -eq 0 ]; then
    set -- /bin/bash
fi

exec docker exec -it "${REALMS_WEB_NAME:-realms-web}" env TERM="${TERM:-xterm}" "$@"
