FROM phusion/baseimage

# Use baseimage-docker's init process.
ENTRYPOINT ["/sbin/my_init"]

# Update
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# ...put your own build instructions here...
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash-completion \
    build-essential \
    cmake \
    cmake-curses-gui \
    coreutils \
    clang \
    gcc \
    g++ \
    gdb \
    git \
    libxml2-dev \
    libboost-python-dev \
    libboost-filesystem-dev \
    make \
    python3 \
    python3-dev \
    libaspell-dev \
    libpspell-dev \
    zlib1g-dev \
    aspell && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Set correct environment variables.
EXPOSE 3333

ENV HOME /home/realms/

RUN useradd jason



