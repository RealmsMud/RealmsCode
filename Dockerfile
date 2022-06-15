FROM ubuntu:22.04 as BUILD
ENV TZ=US
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++


# Update
RUN apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends wget gnupg2 ca-certificates && \
#    # LLVM/Clang
#    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=DontWarn apt-key add - && \
#    echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main" | tee /etc/apt/sources.list.d/llvm.list && \
#    # CMake
#    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=DontWarn apt-key add - && \
#    echo "deb https://apt.kitware.com/ubuntu/ focal main" | tee /etc/apt/sources.list.d/cmake.list && \
    # TZ Stupidity
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
    apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    make \
    git \
    clang-14 \
    lldb-14 \
    lld-14 \
    gcc \
    g++ \
    gdb \
    libsodium23 \
    libopus0 \
    libxml2-dev \
    libssl3 \
    libssl-dev \
    libboost-filesystem-dev \
    libboost-date-time-dev \
    libpython3.10 \
    libpython3.10-dev \
    python3-dev \
    libaspell-dev \
    libpspell-dev  \
    aspell \
    aspell-en \
    zlib1g-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    ln -s /usr/bin/clang-14 /usr/bin/clang && \
    ln -s /usr/bin/clang++-14 /usr/bin/clang++

WORKDIR /build

COPY . .

# Build the mud now
ARG PARALLEL=12
ARG LEAK
RUN cmake . && make -j ${PARALLEL}

FROM ubuntu:22.04 as RUN

# Update
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends \
    libxml2 \
    python3 \
    libssl3 \
    clang-14 \
    lldb-14 \
    lld-14 \
    libpython3.10 \
    libboost-python1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-system1.74.0 \
    libboost-date-time1.74.0 \
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

COPY --from=BUILD /build/RealmsCode .
COPY --from=BUILD /build/List .
COPY --from=BUILD /build/Updater .

# Temporary Workaround
COPY --from=BUILD /build/libRealmsLib.so .
COPY --from=build /build/_deps/dpp-build/libdpp.so.2.9.2 .
COPY --from=BUILD /build/MyLSan.supp .

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

# Temporary Workaround
ENV LD_LIBRARY_PATH=./

ENV ASAN_OPTIONS="detect_odr_violation=0,detect_leaks=0"
ENV LSAN_OPTIONS="LSAN_OPTIONS=suppressions=../MyLSan.supp"

CMD ["/mud/RealmsCode"]

