ARG DPP_VERSION=9.0.15
ARG DPP_FILENAME=libdpp-${DPP_VERSION}-linux-x64.deb

FROM ubuntu:20.04 as BUILD
ARG DPP_VERSION
ARG DPP_FILENAME
ENV TZ=US
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV DPP_URL=https://github.com/brainboxdotcc/DPP/releases/download/v${DPP_VERSION}/${DPP_FILENAME}


# Update
RUN apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends wget gnupg2 ca-certificates && \
    # LLVM/Clang 10
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=DontWarn apt-key add - && \
    echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-12 main" | tee /etc/apt/sources.list.d/llvm.list && \
    # CMake
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=DontWarn apt-key add - && \
    echo "deb https://apt.kitware.com/ubuntu/ focal main" | tee /etc/apt/sources.list.d/cmake.list && \
    # TZ Stupidity
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
    apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    make \
    clang-12 \
    lldb-12 \
    lld-12 \
    gcc \
    g++ \
    gdb \
    libsodium23 \
    libopus0 \
    libxml2-dev \
    libboost-python-dev \
    libboost-filesystem-dev \
    libpython3.8-dev \
    python3-dev \
    libaspell-dev \
    libpspell-dev  \
    aspell \
    aspell-en \
    zlib1g-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    ln -s /usr/bin/clang-12 /usr/bin/clang && \
    ln -s /usr/bin/clang++-12 /usr/bin/clang++ && \
    wget ${DPP_URL} && \
    dpkg -i ${DPP_FILENAME}


WORKDIR /build

COPY . .

# Build the mud now
ARG PARALLEL=12
ARG LEAK
RUN cmake . && make -j ${PARALLEL}

FROM ubuntu:20.04 as RUN

ARG DPP_VERSION
ARG DPP_FILENAME

# Update
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY --from=BUILD /${DPP_FILENAME} .

RUN apt-get update && apt-get install -y --no-install-recommends \
    libxml2 \
    python3 \
    libpython3.8 \
    libboost-python1.71.0 \
    libboost-filesystem1.71.0 \
    libboost-system1.71.0 \
    libsodium23 \
    libopus0 \
    aspell \
    zlib1g  \
    gdb && \
    dpkg -i ${DPP_FILENAME} && \
    rm ${DPP_FILENAME} && \
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
COPY --from=BUILD /build/leak.supp .

ENV ASAN_OPTIONS="detect_leaks=1"
ENV LSAN_OPTIONS="suppressions=leak.supp"
CMD ["/mud/RealmsCode"]

