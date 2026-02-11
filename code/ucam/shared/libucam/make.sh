#!/bin/bash

usage()                                  
{                                             
    echo "Usage:  ./make.sh [CHIP]"
	echo "CHIP:HI3516EV300,HI3519V101,HI3519A,HI3559V100,SSC9381,SSC333,SSC337,RV1126"
	echo "Usage:  ./make.sh HI3516EV300"
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

# Check for svn and a svn repo.
if rev=`LANG= LC_ALL= LC_MESSAGES=C svn info 2>/dev/null | grep '^Last Changed Rev'`; then
	rev=`echo $rev | awk '{print $NF}'`
	#printf -- '-svn:%s\n' "$rev"
	# All done with svn
	#return
fi

if [ ! -n "$rev" ] ;then
	echo "get svn version error!"
	rev=`git rev-parse HEAD`
fi

if [ ! -n "$rev" ] ;then
  echo "get svn/git version error!"
  exit 1
fi

BaseDir=$PWD

VERSION=1.2.07

LIBUCAM_VERSION=V$VERSION
LIBUCAM_VERSION_BCD=0x$(echo $VERSION | sed 's/\.//g')

printf -- '-make:%s svn/git:%s\n' "$1" "$rev"

make CHIP="$1" clean
check_result "clean ucam_lib"

make CHIP="$1" SVN_GIT_VERSION=$rev UCAM_VERSION=$LIBUCAM_VERSION UCAM_VERSION_BCD=$LIBUCAM_VERSION_BCD
check_result "make ucam_lib"

#cd $BaseDir/test
#make CHIP="$1" clean
#check_result "clean ucam_test"
#make CHIP="$1"
#check_result "make ucam_test"
cd $BaseDir


UCAM_LIB_CHIP=$(echo $1 | tr A-Z a-z)
version_output=../vhd_sdk/libucam/$UCAM_LIB_CHIP/lib_ucam

if [ -d $version_output ]; then
	rm $version_output -rf
fi
mkdir -p $version_output
check_result "mkdir version_output"
cp libvhducam.a $version_output
cp libvhducam.so $version_output
cp $BaseDir/ucam/include $version_output -rf

if [ -d $version_output/../alsa ]; then
	rm $version_output/../alsa -rf
fi
mkdir -p $version_output/../alsa

cp $BaseDir/alsa/out/lib/${1,,}/* $version_output/../alsa -rf
cp $BaseDir/alsa/out/include $version_output/../alsa -rf

cp libvhducam.so ~/nfs/

