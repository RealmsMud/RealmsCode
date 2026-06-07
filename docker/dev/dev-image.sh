#!/bin/sh
# Build the dev sandbox image (toolchain only, no source baked in).
docker build --target dev -t realms-dev "$(dirname "$0")/../.."
