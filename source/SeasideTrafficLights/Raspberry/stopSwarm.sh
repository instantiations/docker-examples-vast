#!/bin/bash

readonly STACK_NAME=seaside-debian-slim

docker stack rm $STACK_NAME
declare TIMEOUT=0
readonly WAIT_INTERVAL=1
readonly LIMIT=120

echo -n 'waiting for network to shutdown'
while [ "$(docker network ls | grep --count "$STACK_NAME")" -gt 0 ] && [ $TIMEOUT -lt $LIMIT ]; do
    echo -n .
    sleep $WAIT_INTERVAL
    (( TIMEOUT+=WAIT_INTERVAL ))
done
if [ $TIMEOUT -ge $LIMIT ]; then
    echo ' failed' 1>&2
    exit 1
else
    echo ' ok'
fi

