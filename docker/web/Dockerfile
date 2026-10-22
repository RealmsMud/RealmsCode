ARG PHP_IMAGE=realms-php:8.4.27-apache-trixie
FROM ${PHP_IMAGE}

RUN php -r 'if (PHP_VERSION_ID < 80427 || PHP_VERSION_ID >= 80500) { fwrite(STDERR, "PHP 8.4.27 or later in the 8.4 branch is required\n"); exit(1); }' && \
    php -r 'foreach (["curl", "dom", "mbstring", "session", "SimpleXML", "xml", "zlib"] as $extension) { if (!extension_loaded($extension)) { fwrite(STDERR, "Missing PHP extension: $extension\n"); exit(1); } }' && \
    cp "$PHP_INI_DIR/php.ini-production" "$PHP_INI_DIR/php.ini" && \
    a2enmod rewrite ssl access_compat alias setenvif && \
    a2dissite 000-default && \
    ln -s /etc/apache2/sites-available/realms.conf /etc/apache2/sites-enabled/realms.conf && \
    ln -s /etc/apache2/sites-available/realms-ssl.conf /etc/apache2/sites-enabled/realms-ssl.conf

COPY php.ini /usr/local/etc/php/conf.d/realms.ini

EXPOSE 47 48
