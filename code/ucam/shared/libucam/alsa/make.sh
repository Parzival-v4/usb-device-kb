#!/bin/bash
 
usage()                                  
{                                             
    echo "Usage:  ./make.sh [gcc]"
	echo "Usage:  ./make.sh arm-himix100-linux"
}

check_result() {
    if [ $? -ne 0 ]; then
      echo "ERROR: Fail to $1 "
      exit -1
    fi
}

if [ ! -n "$1" ];then
  usage
  exit 1
fi

BaseDir=$PWD
AlsaDir=$BaseDir/alsa-lib-1.1.7
HOST=$1

if [ ! -n "$2" ] ;then
  OSDRV_CROSS=$1-gcc
  STRIP=$1-strip
else
  OSDRV_CROSS=$2-gcc
  STRIP=$2-strip
fi

if [ -d $AlsaDir/out ]; then
	rm -rf $AlsaDir/out
fi

mkdir -p $AlsaDir/out

cd $AlsaDir
./configure --host=$HOST --target=$HOST CC=$OSDRV_CROSS --with-configdir=/home/audio/alsa/ --enable-static=no --enable-shared=yes --with-debug=no LDFLAGS="-lm" 
check_result "configure"
cd $AlsaDir
make clean
make
check_result "make shared"
$STRIP $AlsaDir/src/.libs/*.so*
cp $AlsaDir/src/.libs/*.so* $AlsaDir/out
check_result "cp .so"

make clean
./configure --host=$HOST --target=$HOST CC=$OSDRV_CROSS --with-configdir=/home/audio/alsa/ --enable-static=yes --enable-shared=no --with-debug=no LDFLAGS="-lm" 
check_result "configure"
cd $AlsaDir
make clean
make
check_result "make static"
cp $AlsaDir/src/.libs/*.a $AlsaDir/out
check_result "cp .a"