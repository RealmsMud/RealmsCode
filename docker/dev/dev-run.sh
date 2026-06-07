#!/bin/sh
# Run the prod image. Host game data dir via first positional arg, then $REALMS_DATA, and finally
# the default. e.g.:
#   sh dev-run.sh /Users/jason/realms/realms
set -e

REALMS_DATA="${1:-${REALMS_DATA:-/home/realms/realms}}"

# Replace any existing container so re-running doesn't conflict on the name.
if [ -n "$(docker ps -aq -f name='^realms$')" ]; then
    echo "Replacing existing realms container..."
    docker rm -f realms >/dev/null
fi

docker run -d -v "$REALMS_DATA":/home/realms/realms -p 3333:3333 --security-opt seccomp=unconfined --name realms realms-image
#docker run -d -v /realms/:/home/realms/realms/ -p 3333:3333 --security-opt seccomp=unconfined --name realms realms-image
