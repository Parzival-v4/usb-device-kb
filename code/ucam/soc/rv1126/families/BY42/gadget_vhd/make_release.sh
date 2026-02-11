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

WEBCAM_VERSION=V1.1.66
BaseDir=$PWD

sed -i "s\WEBCAM_VERSION .*\WEBCAM_VERSION \"$WEBCAM_VERSION\"\g" $BaseDir/include/uvc_config.h
check_result "sed set WEBCAM_VERSION"

GIT_TREE="$(git rev-parse --abbrev-ref HEAD)"

COMPILE_INFO="`whoami`@`hostname` `date +%Y-%m-%d\ %H:%M:%S`"

version_out_path=$BaseDir/ko_release/$GIT_TREE/

if [ -d $version_out_path ]; then
	rm $version_out_path -rf
fi

make_ko(){
	LIB_CHIP=$(echo $1 | tr A-Z a-z)
	sh ./make.sh $1
	check_result "make ko"
  	mkdir -p $version_out_path/$LIB_CHIP/ko
  	check_result "mkdir release"
	cp $BaseDir/ko/*.ko $version_out_path/$LIB_CHIP/ko
  	check_result "cp ko"
}

if [ -n "$1" ];then
	make_ko $1
	check_result "make $1"
    echo "build CHIP:$1 usb_ko:$WEBCAM_VERSION git tree:$GIT_TREE commit:$rev by:$COMPILE_INFO"
	exit 0
fi

#make_ko CV5
#check_result "make_cv5"

#make_ko HI3519V101
#check_result "make_hi3519v101"
#make_ko HI3519A
#check_result "make_hi3519a"
#make_ko HI3516EV300
#check_result "make_hi3516ev300"
#make_ko GK7205V300
#check_result "make_gk7205v300"
make_ko SSC9381
check_result "make_ssc9381"
make_ko SSC333
check_result "make_ssc333"
#make_ko SSC337
#check_result "make_ssc337"
#make_ko SSC9211
#check_result "make_ssc9211"
make_ko SSC268
check_result "make_ssc268"
#make_ko SSC369
#check_result "make_ssc369"
make_ko RV1126
check_result "make_rv1126"

echo "build CHIP:all usb_ko:$WEBCAM_VERSION git tree:$GIT_TREE commit:$rev by:$COMPILE_INFO"
