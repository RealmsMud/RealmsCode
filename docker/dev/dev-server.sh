#!/bin/sh
# Run the freshly built MUD from the out-of-tree build dir inside the dev sandbox.
docker exec -it realms-dev sh -c \
    "LD_LIBRARY_PATH=/build:/build/_deps/dpp-build/library /build/RealmsCode $*"
