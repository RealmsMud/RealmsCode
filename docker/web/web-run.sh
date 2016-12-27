#!/bin/sh
docker run -d -v /home/realms/realms/www/:/home/realms/www/ -v /home/realms/realms/:/home/realms/realms/ -p 47:47 -p 48:48 --name realms-web realms-web
