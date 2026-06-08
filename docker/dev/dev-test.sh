#!/bin/sh
# Build the RealmsTests target and run it inside the dev sandbox (realms-dev -> /build).
# Args pass through to the gtest binary.
#   dev-test.sh                          build + run the full suite
#   dev-test.sh --gtest_filter='Md5.*'   build + run a subset
docker exec -it realms-dev sh -c '
    cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON \
    && cmake --build /build -j${PARALLEL:-12} --target RealmsTests \
    && LD_LIBRARY_PATH=/build:/build/_deps/dpp-build/library /build/RealmsTests "$@"
' sh "$@"
