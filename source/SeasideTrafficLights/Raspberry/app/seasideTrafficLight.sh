#!/bin/sh

cd /opt/app

# If we don't specify this LANG or any other compatible one
# we would get quite some test failures
# Some Linux are fine just with $LANG but some others (like Raspbian) do already
# export $LC_ALL so if we don't override it, VA will still pick 
# up the default encoding. So, just in case, we expor all variables. 
export LANG=en_US.iso88591; export LANGUAGE=en_US.iso88591; export LC_ALL=en_US.iso88591

export VAST_ROOT="/opt/vast92"
export LD_LIBRARY_PATH="${VAST_ROOT}/bin:$LD_LIBRARY_PATH"
$VAST_ROOT/bin/esnx -no_break -msd -mcd -i./seasideTrafficLight-unix.icx -ini:./seasideTrafficLight-unix.ini

cp *.sdf /opt/log/
