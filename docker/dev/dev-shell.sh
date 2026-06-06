#!/bin/sh
# Interactive shell into the running dev sandbox.
docker exec -it realms-dev env TERM=xterm script -q -c "/bin/bash" /dev/null
