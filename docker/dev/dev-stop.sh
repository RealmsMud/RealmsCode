#!/bin/sh
# Stop the MUD running in the dev sandbox (started by dev-server.sh).
docker exec realms-dev pkill -x RealmsCode
