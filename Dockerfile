FROM phusion/baseimage

# Set correct environment variables.

EXPOSE 3333

ENV HOME /home/realms/
RUN useradd jason

# Use baseimage-docker's init process.
CMD ["/sbin/my_init"]

# ...put your own build instructions here...
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold"
RUN apt-get install -y --no-install-recommends install \
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

#sudo apt-get update && sudo apt-get install c

# Clean up APT when done.
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*



