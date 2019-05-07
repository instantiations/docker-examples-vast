#!/bin/sh

cd /opt/app

export LANG=en_US.iso88591
export VAST_ROOT="/opt/vast92"
export LD_LIBRARY_PATH="${VAST_ROOT}/bin:$LD_LIBRARY_PATH"
$VAST_ROOT/bin/esnx -no_break -msd -mcd -i./seasideTrafficLight-unix64.icx -ini:./seasideTrafficLight-unix64.ini

cp *.sdf /opt/log
