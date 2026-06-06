#!/bin/sh
# Incremental out-of-tree build inside the running dev sandbox. First run configures and
# builds everything (incl. _deps); later runs recompile only changed translation units.
# Pass extra args through to the build (e.g. a target name).
docker exec realms-dev sh -c \
    "cmake -S /src -B /build && cmake --build /build -j\${PARALLEL:-12} $*"
