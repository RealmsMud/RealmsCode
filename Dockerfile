FROM ubuntu:18.04 as BUILD

# Update
RUN apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends wget gnupg2 ca-certificates && \
    # LLVM/Clang 10
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key 2>/dev/null | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=DontWarn apt-key add - && \
    echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-10 main" | tee /etc/apt/sources.list.d/llvm.list && \
    # CMake
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=DontWarn apt-key add - && \
    echo "deb https://apt.kitware.com/ubuntu/ bionic main" | tee /etc/apt/sources.list.d/cmake.list && \
    apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    make \
    clang-10 \
    lldb-10 \
    lld-10 \
    gcc \
    g++ \
    gdb \
    libxml2-dev \
    libboost-python-dev \
    libboost-filesystem-dev \
    python3-dev \
    libaspell-dev \
    libpspell-dev \
    aspell \
    zlib1g-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    ln -s /usr/bin/clang-10 /usr/bin/clang && \
    ln -s /usr/bin/clang++-10 /usr/bin/clang++

WORKDIR /build

COPY . .

ARG PARALLEL=12
ARG LEAK
RUN cmake . && make -j ${PARALLEL}

FROM ubuntu:18.04 as RUN

# Update
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN apt-get update && apt-get install -y --no-install-recommends \
    libxml2 \
    python3 \
    libpython3.6 \
    libboost-python1.65 \
    libboost-filesystem1.65 \
    aspell \
    zlib1g && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ARG username=jason

RUN useradd ${username}
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

