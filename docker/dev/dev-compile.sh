#!/bin/sh
# Out-of-tree build inside the running dev sandbox (realms-dev -> /build).
#   dev-compile.sh                incremental build
#   dev-compile.sh clean          clean our code only (deps stay built) - fast recompile prep
#   dev-compile.sh full-clean     clean everything incl. fetched deps (dpp/crow/json/...)
#   dev-compile.sh rebuild        clean our code + build
#   dev-compile.sh full-rebuild   clean everything + build (deps recompiled too)
#   dev-compile.sh <target>       build a single target (e.g. RealmsTests)
docker exec realms-dev sh -c '
    OURS="/build/CMakeFiles/*.dir"   # our targets only; deps live under /build/_deps/*-build
    case "$1" in
        clean)        rm -rf $OURS && cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON ;;
        full-clean)   cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON && cmake --build /build --target clean ;;
        rebuild)      rm -rf $OURS && cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON && cmake --build /build -j${PARALLEL:-12} ;;
        full-rebuild) cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON && cmake --build /build -j${PARALLEL:-12} --clean-first ;;
        "")           cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON && cmake --build /build -j${PARALLEL:-12} ;;
        *)            cmake -S /src -B /build -DREALMS_BUILD_TESTS=ON && cmake --build /build -j${PARALLEL:-12} --target "$1" ;;
    esac
' sh "$1"
