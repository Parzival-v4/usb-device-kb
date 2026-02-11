#!/bin/bash 

check_result() {
    if [ $? -ne 0 ]; then
      echo "ERROR: Fail to $1 "
      exit -1
    fi
}

# Check for svn and a svn repo.
if rev=`LANG= LC_ALL= LC_MESSAGES=C svn info 2>/dev/null | grep '^Last Changed Rev'`; then
	rev=`echo $rev | awk '{print $NF}'`
	printf -- '-svn:%s\n' "$rev"
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

VERSION=1.2.27

LIBUCAM_VERSION=V$VERSION
LIBUCAM_VERSION_BCD=0x$(echo $VERSION | sed 's/\.//g')
BaseDir=$PWD
#version_output=/home/$(whoami)/work_space/version_output/rk3588_libucam/

if [ $1 == VS816 ];then	
	version_output=~/work_space/version_output/lib_ucam/trunk/
else
	version_output=/data/$(whoami)/work_space/version_output/lib_ucam/trunk/
fi

if [ -n "$2" ];then
	version_output=$2
fi

#sed -i "s\VHD_LIBUCAM_VERSION .*\VHD_LIBUCAM_VERSION \"$LIBUCAM_VERSION\"\g" $BaseDir/ucam/include/ucam/ucam.h
#check_result "sed set VHD_LIBUCAM_VERSION"

if [ -d $version_output/$LIBUCAM_VERSION ]; then
	rm $version_output/$LIBUCAM_VERSION -rf
fi
mkdir -p $version_output/$LIBUCAM_VERSION

#cp $version_output/template/ $version_output/$LIBUCAM_VERSION/ -rf
#check_result "cp template dir"



make_libucam(){
	cd $BaseDir
	make CHIP=$1 clean
	check_result "clean ucam_lib"
	make CHIP=$1 SVN_GIT_VERSION=$rev UCAM_VERSION=$LIBUCAM_VERSION UCAM_VERSION_BCD=$LIBUCAM_VERSION_BCD
	check_result "make $1 ucam_lib"

	cp $version_output/template/${1,,} $version_output/$LIBUCAM_VERSION/ -rf
	check_result "cp template dir"

	mkdir -p $version_output/$LIBUCAM_VERSION/${1,,}/lib_ucam/static
	mkdir -p $version_output/$LIBUCAM_VERSION/${1,,}/lib_ucam/dynamic

	cp libvhducam.a $version_output/$LIBUCAM_VERSION/${1,,}/lib_ucam/static
	check_result "cp libvhducam.a"
	cp libvhducam.so $version_output/$LIBUCAM_VERSION/${1,,}/lib_ucam/dynamic
	check_result "cp libvhducam.so"
	rm $version_output/$LIBUCAM_VERSION/${1,,}/lib_ucam/include -rf
	cp $BaseDir/ucam/include $version_output/$LIBUCAM_VERSION/${1,,}/lib_ucam/ -rf
	check_result "cp ucam/include"
	
	
	cp -rf $BaseDir/alsa/out/lib/${1,,}/* $version_output/$LIBUCAM_VERSION/${1,,}/alsa/
	check_result "cp alsa *"
	rm $version_output/$LIBUCAM_VERSION/${1,,}/alsa/include -rf
	cp $BaseDir/alsa/out/include $version_output/$LIBUCAM_VERSION/${1,,}/alsa -rf
	check_result "cp alsa include"
}

if [ -n "$1" ];then
  	make_libucam $1
	check_result "make $1"
  	chmod 777 $version_output/$LIBUCAM_VERSION/ -R
	printf -- 'build with -svn/git:%s\n' "$rev"
	cd $BaseDir
	exit 0
fi

#make_libucam CV5
#check_result "make_cv5"

#make_libucam HI3519V101
#check_result "make_hi3519v101"
#make_libucam HI3559V100
#check_result "make_hi3559v100"
#make_libucam HI3519A
#check_result "make_hi3519a"
#make_libucam HI3516EV300
#check_result "make_hi3516ev300"
make_libucam SSC9381
check_result "make_ssc9381"
make_libucam SSC333
check_result "make_ssc333"
#make_libucam SSC337
#check_result "make_ssc337"

# make_libucam SSC9211
# check_result "make_ssc9211"

make_libucam SSC268
check_result "make_ssc268"

#make_libucam SSC369
#check_result "make_ssc369"

make_libucam RV1126
check_result "make_rv1126"


chmod 777 $version_output/$LIBUCAM_VERSION/ -R
printf -- 'build with -svn/git:%s\n' "$rev"
