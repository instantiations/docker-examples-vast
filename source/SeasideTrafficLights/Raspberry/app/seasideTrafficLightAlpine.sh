#!/bin/sh

cd /opt/app

export PATH=/usr/glibc-compat/bin:$PATH
export LD_LIBRARY_PATH=/usr/glibc-compat/lib:$LD_LIBRARY_PATH

./seasideTrafficLight.sh

