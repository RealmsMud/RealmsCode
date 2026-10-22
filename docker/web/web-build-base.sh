#!/bin/sh
set -eu

upstream_commit=e3eaff845f7a9f7259c31da54dce0320b47a3bbd
archive_sha256=ee08891952ec34f146caee3e34bc56e31aa16e79fd6eada3d2d1b0fd189b8b37
temporary_dir=$(mktemp -d)
trap 'rm -rf "$temporary_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

curl --fail --show-error --silent --location --retry 1 \
    "https://codeload.github.com/docker-library/php/tar.gz/$upstream_commit" \
    --output "$temporary_dir/php-docker.tar.gz"
if command -v sha256sum >/dev/null 2>&1; then
    actual_sha256=$(sha256sum "$temporary_dir/php-docker.tar.gz")
else
    actual_sha256=$(shasum -a 256 "$temporary_dir/php-docker.tar.gz")
fi
actual_sha256=${actual_sha256%% *}
if [ "$actual_sha256" != "$archive_sha256" ]; then
    printf '%s\n' "Official PHP Docker recipe archive checksum mismatch" >&2
    exit 1
fi

mkdir "$temporary_dir/context"
tar -xzf "$temporary_dir/php-docker.tar.gz" --strip-components=4 \
    -C "$temporary_dir/context" "php-$upstream_commit/8.4/trixie/apache"
docker build --pull -t "${REALMS_PHP_IMAGE:-realms-php:8.4.27-apache-trixie}" \
    "$@" "$temporary_dir/context"
