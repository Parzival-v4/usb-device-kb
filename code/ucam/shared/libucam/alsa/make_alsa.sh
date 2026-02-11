#!/bin/bash

check_result() {
    if [ $? -ne 0 ]; then
      echo "ERROR: Fail to $1 "
      exit -1
    fi
}
BaseDir=$PWD
AlsaDir=$BaseDir/alsa-lib-1.1.7
OSDRV_CROSS=arm-himix100-linux-gcc
HOST=arm-himix100-linux
STRIP=$HOST-strip

cd $AlsaDir
./configure --host=$HOST CC=$OSDRV_CROSS --disable-mixer --with-configdir=/home/audio/alsa/ --enable-static=no --enable-shared=yes
check_result "configure"
cd $AlsaDir
make clean
make
check_result "make"
$STRIP $AlsaDir/src/.libs/*.so