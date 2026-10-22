#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec docker build \
    --build-arg "PHP_IMAGE=${REALMS_PHP_IMAGE:-realms-php:8.4.27-apache-trixie}" \
    -t "${REALMS_WEB_IMAGE:-realms-web:php8.4.27}" "$script_dir"
