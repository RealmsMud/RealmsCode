FROM ubuntu:26.04 AS base
ENV TZ=US
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

# Update
RUN apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends wget gnupg2 ca-certificates && \
    # TZ Stupidity
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
    apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    make \
    git \
    clang-21 \
    lldb-21 \
    lld-21 \
    libclang-rt-21-dev \
    gcc \
    g++ \
    gdb \
    libsodium23 \
    libopus0 \
    libxml2-dev \
    libssl3t64 \
    libssl-dev \
    libasio-dev \
    libboost-filesystem-dev \
    libboost-date-time-dev \
    libboost-regex-dev \
    libpython3.14 \
    libpython3.14-dev \
    python3-dev \
    libaspell-dev \
    libpspell-dev  \
    aspell \
    aspell-en \
    zlib1g-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    ln -s /usr/bin/clang-21 /usr/bin/clang && \
    ln -s /usr/bin/clang++-21 /usr/bin/clang++

# Prod build: bakes the source in and compiles from scratch.
FROM base AS build

WORKDIR /build

COPY . .

# Build the mud now
ARG PARALLEL=12
ARG LEAK
RUN cmake . && make -j ${PARALLEL}

# Dev sandbox: toolchain only, no source baked in. Source is bind-mounted at /src and the
# out-of-tree build dir (/build, holding .o files + _deps) is a persistent named volume,
# both supplied at `docker run` time. Stays alive to be exec'd into for incremental builds.
# See docker/dev/*.sh.
FROM base AS dev

WORKDIR /build
ARG PARALLEL=12
CMD ["sleep", "infinity"]

FROM ubuntu:26.04 AS run

# Update
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends \
    libxml2-16 \
    python3 \
    libssl3t64 \
    clang-21 \
    lldb-21 \
    lld-21 \
    libpython3.14 \
    libboost-python1.90.0 \
    libboost-filesystem1.90.0 \
    libboost-date-time1.90.0 \
    libboost-regex1.90.0 \
    libsodium23 \
    libopus0 \
    aspell \
    zlib1g \
    locales \
    locales-all \
    gdb && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ARG username=jason

RUN useradd ${username}

# We want the SRC, but not all the build objs, for GDB to work
WORKDIR /build
COPY . .

WORKDIR /mud

# Set correct environment variables.
EXPOSE 3333
ENV HOME /home/realms/

COPY --from=build /build/RealmsCode .
COPY --from=build /build/List .
COPY --from=build /build/Updater .

# Temporary Workaround
COPY --from=build /build/libRealmsLib.so .
COPY --from=build /build/_deps/dpp-build/library/libdpp.so.* .
COPY --from=build /build/MyLSan.supp .

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

# Temporary Workaround
ENV LD_LIBRARY_PATH=./

ENV ASAN_OPTIONS="detect_odr_violation=0,detect_leaks=0"
ENV LSAN_OPTIONS="LSAN_OPTIONS=suppressions=../MyLSan.supp"

CMD ["/mud/RealmsCode"]
