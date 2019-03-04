#!/bin/sh

# cd /home/pi/SeasideTrafficLight
cd /opt/app

export LANG=en_US.iso88591
export VAST_ROOT="/opt/vast92"
export LD_LIBRARY_PATH="${VAST_ROOT}/bin"
$LD_LIBRARY_PATH/esnx -no_break -msd -mcd -i./seasideTrafficLight-unix.icx -ini:./seasideTrafficLight-unix.ini

cp *.sdf /opt/log