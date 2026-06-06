#!/bin/sh
# Start the long-lived dev sandbox. Source is bind-mounted read-write at /src; the
# out-of-tree build dir (/build, holding .o files + _deps) is the persistent named volume
# realms-build so compiled objects survive container/host restarts.
#
# Host game data dir via first positional arg, then $REALMS_DATA, and finally the default
#   sh dev-up.sh /Users/jason/realms/realms
#   REALMS_DATA=/Users/jason/realms/realms sh dev-up.sh
set -e

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
REALMS_DATA="${1:-${REALMS_DATA:-$REPO/../realms-data}}"

if [ ! -d "$REALMS_DATA" ]; then
    echo "warning: data dir '$REALMS_DATA' does not exist; creating it empty (the MUD will" >&2
    echo "         fail to load config until real game data lives there)." >&2
    mkdir -p "$REALMS_DATA"
fi

docker volume inspect realms-build >/dev/null 2>&1 || docker volume create realms-build

# Replace any existing sandbox so re-running picks up new args/mounts.
if [ -n "$(docker ps -aq -f name='^realms-dev$')" ]; then
    echo "Replacing existing realms-dev container..."
    docker rm -f realms-dev >/dev/null
fi

docker run -d \
    -v "$REPO":/src \
    -v realms-build:/build \
    -v "$REALMS_DATA":/home/realms/realms \
    -p 3333:3333 \
    --security-opt seccomp=unconfined \
    --name realms-dev \
    realms-dev
