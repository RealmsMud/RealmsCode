FROM phusion/baseimage

# Use baseimage-docker's init process.
ENTRYPOINT ["/sbin/my_init"]

# Update
RUN apt-get update && apt-get upgrade -y -o Dpkg::Options::="--force-confold" && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# ...put your own build instructions here...
RUN apt-get update && apt-get install -y --no-install-recommends \
    apache2 \
    php7.0 \
    libapache2-mod-php7.0 && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN a2enmod php7.0 && \
    a2enmod rewrite && \
    a2enmod ssl && \
    rm -f /etc/apache2/sites-enabled/000-default.conf


# Manually set up the apache environment variables
ENV APACHE_RUN_USER www-data
ENV APACHE_RUN_GROUP www-data
ENV APACHE_LOG_DIR /var/log/apache2
ENV APACHE_LOCK_DIR /var/lock/apache2
ENV APACHE_PID_FILE /var/run/apache2.pid

COPY conf/realms*.conf /etc/apache2/sites-available/
COPY conf/ports.conf /etc/apache2/
COPY conf/server.crt /etc/ssl/certs/
COPY conf/server.key /etc/ssl/private/

RUN ln -s /etc/apache2/sites-available/realms.conf /etc/apache2/sites-enabled/realms.conf && \
    ln -s /etc/apache2/sites-available/realms-ssl.conf /etc/apache2/sites-enabled/realms-ssl.conf

EXPOSE 47 48
