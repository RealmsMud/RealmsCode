#!/bin/sh
docker run -d -v /home/realms/realms/:/home/realms/realms/ -p 3333:3333 --security-opt seccomp=unconfined --name realms realms-image
#docker run -d -v /realms/:/home/realms/realms/ -p 3333:3333 --security-opt seccomp=unconfined --name realms realms-image
