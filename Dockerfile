FROM phusion/baseimage

# Set correct environment variables.

EXPOSE 3333

ENV HOME /home/realms/
RUN useradd jason

# Use baseimage-docker's init process.
CMD ["/sbin/my_init"]

# ...put your own build instructions here...
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold"
RUN apt-get install -y clang libxml2-dev cmake make libboost-python-dev libboost-filesystem-dev zlib1g-dev \
                             python3-dev libaspell-dev libpspell-dev gdb

#sudo apt-get update && sudo apt-get install c

# Clean up APT when done.
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*



