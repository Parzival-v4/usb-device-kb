#!/bin/bash 
usage()                                  
{                                             
    echo "Usage:  ./make_release_to_git.sh [CHIP]"
	echo "CHIP:VS816 CV5 RK3588"
	echo "Usage:  ./make.sh VS816"
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

CHIP=$1
BaseDir=$PWD
COMPILE_INFO="`whoami`@`hostname` `date +%Y-%m-%d\ %H:%M:%S`"

LIB_GIT_TREE="$(git rev-parse --abbrev-ref HEAD)"
LIB_GIT_REV=`git rev-parse HEAD`
GIT_PRODUCT=$CHIP
if [ $CHIP == "X3" ]; then
	GIT_PRODUCT=X3X5
fi

TO_GIT_URL=ssh://git@192.168.1.204:33/ucam_sdk/release/${GIT_PRODUCT,,}/libucam.git

version_output=$BaseDir/git_release
if [ -d $version_output ]; then
	rm $version_output -rf
fi

echo "release to git url:${TO_GIT_URL}"
git clone ${TO_GIT_URL} git_release
check_result "git clone"

cd git_realease
git checkout interim
check_result "git checkout interim"
git pull origin interim
cd -

build_usb_ko="N"
read -p "build ../gadget_vhd usb ko? (y/N)" temp
if [ "$temp" == "Y" ] || [ "$temp" == "y" ]; then
	build_usb_ko="Y"
fi
echo build_usb_ko:"$build_usb_ko"

KO_GIT_TREE="null"
KO_GIT_REV="null"
function make_release_ko(){
	echo "build usb ko ..."
    cd $BaseDir/../gadget_vhd
    check_result "cd gadget_vhd"
    KO_GIT_TREE="$(git rev-parse --abbrev-ref HEAD)"
    KO_GIT_REV=`git rev-parse HEAD`

    ./make_release.sh $CHIP
    check_result "usb_ko make_release"

    rm -rf $version_output/${CHIP,,}/ko
    cp -rf ko_release/$KO_GIT_TREE/${CHIP,,}/ko $version_output/${CHIP,,}/
    check_result "cp usb_ko"
    cd $BaseDir
}

function make_release_lib(){
	echo "build usb lib ..."
    cd $BaseDir
    ./make_release.sh $CHIP
    check_result "lib make_release"

    rm -rf $version_output/${CHIP,,}/alsa
    rm -rf $version_output/${CHIP,,}/lib_ucam
    if [ ! -d $version_output/${CHIP,,} ]; then
	    mkdir -p $version_output/${CHIP,,}
    fi
    #cp -rf lib_release/$LIB_GIT_TREE/${CHIP,,}/alsa $version_output/${CHIP,,}/
    #echo "cp -rf lib_release/$LIB_GIT_TREE/${CHIP,,}/alsa $version_output/${CHIP,,}/"
    #check_result "cp lib alsa"
    cp -rf lib_release/$LIB_GIT_TREE/${CHIP,,}/lib_ucam $version_output/${CHIP,,}/
    check_result "cp lib ucam"
}


if [ "$build_usb_ko" == "Y" ]; then
    make_release_ko
    check_result "make_release_ko"
fi

make_release_lib
check_result "make_release_lib"

cd $BaseDir
echo "step 1:check make log"
echo "step 2:cd git_release"
echo "step 3:git status, check"
echo "step 4:edit 版本日志.txt"
echo "step 5:git commit"
echo "step 6:git push"
