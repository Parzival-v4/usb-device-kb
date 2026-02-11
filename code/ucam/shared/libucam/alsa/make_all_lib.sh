#!/bin/bash
###
 # @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 # 
 # @Author      : QinHaiCheng
 # 
 # @Date        : 2021-05-25 10:26:28
 # @FilePath    : \lib_ucam\alsa\make_all_lib.sh
 # @Description : 
### 
 
check_result() {
    if [ $? -ne 0 ]; then
      echo "ERROR: Fail to $1 "
      exit -1
    fi
}

BaseDir=$PWD

if [ -d $BaseDir/out/lib ]; then
	rm -rf $BaseDir/out/lib
fi

mkdir -p $BaseDir/out/lib

if [ -d $BaseDir/out/include ]; then
	rm -rf $BaseDir/out/include
fi

make_libalsa(){
	CHIP=$1
	CROSS_COMPILE=$3
	cd $BaseDir
	sh make.sh $2 $CROSS_COMPILE
	check_result "make  $1 alsa"
	if [ -d $BaseDir/out/lib/${CHIP} ]; then
		rm $BaseDir/out/lib/${CHIP}
	fi
	mkdir -p $BaseDir/out/lib/${CHIP}
	mkdir -p $BaseDir/out/lib/${CHIP}/static
	mkdir -p $BaseDir/out/lib/${CHIP}/dynamic
	cp $BaseDir/alsa-lib-1.1.7/out/*.a $BaseDir/out/lib/${CHIP}/static/
	check_result "cp *.a"
	cp $BaseDir/alsa-lib-1.1.7/out/*.so* $BaseDir/out/lib/${CHIP}/dynamic/
	check_result "cp *.so"
	
}

make_libalsa_safe(){
	CHIP=$1
	CROSS_COMPILE=$3
	cd $BaseDir
	sh make_safe.sh $2 $CROSS_COMPILE
	check_result "make_safe  $1 alsa"
	if [ -d $BaseDir/out/lib/${CHIP} ]; then
		rm $BaseDir/out/lib/${CHIP}
	fi
	mkdir -p $BaseDir/out/lib/${CHIP}
	mkdir -p $BaseDir/out/lib/${CHIP}/static
	mkdir -p $BaseDir/out/lib/${CHIP}/dynamic
	cp $BaseDir/alsa-lib-1.1.7/out/*.a $BaseDir/out/lib/${CHIP}/static/
	check_result "cp *.a"
	cp $BaseDir/alsa-lib-1.1.7/out/*.so* $BaseDir/out/lib/${CHIP}/dynamic/
	check_result "cp *.so"
	
}

#make_libalsa gk7205v300 "arm-gcc6.3-linux-uclibceabi"
#check_result "gk7205v300"

#export PATH=/usr/local/cortex-a76-2023.04-gcc12.2-linux5.15/bin:$PATH
#export ARCH=arm64
#export CROSS_COMPILE="aarch64-linux-gnu-"
#make_libalsa cv5 "aarch64-linux-gnu"
#check_result "cv5"
#exit 0

# export PATH=/opt/vs-linux/x86-arm/gcc-linaro-7.5.0-aarch64-linux-gnu/bin:$PATH
# export ARCH=arm64
# export CROSS_COMPILE="aarch64-linux-gnu-"
# make_libalsa_safe vs816 "aarch64-linux-gnu"
# check_result "vs816"
# exit 0

# export PATH=/data/hgy/bin/toolchain/aarch64-ca53-linux-gnueabihf-8.4.01/bin:$PATH
# export ARCH=arm64
# export CROSS_COMPILE="aarch64-ca53-linux-gnu-"
# make_libalsa nt98530 "aarch64-ca53-linux-gnu"
# check_result "nt98530"
# exit 0


make_libalsa hi3516ev300 "arm-himix100-linux"
check_result "hi3516ev300"


make_libalsa hi3519v101 "arm-hisiv500-linux"
check_result "hi3519v101"

make_libalsa hi3519a "arm-himix200-linux"
check_result "hi3519a"

make_libalsa ssc333 "arm-buildroot-linux-uclibcgnueabihf" "/opt/sigmastar/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf"
check_result "ssc333"

make_libalsa ssc9211 "arm-buildroot-linux-uclibcgnueabihf" "/opt/sigmastar/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf"
check_result "ssc9211"

make_libalsa ssc337 "arm-buildroot-linux-uclibcgnueabihf" "/opt/sigmastar/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf"
check_result "ssc337"

make_libalsa ssc9381 "arm-linux-gnueabihf" "/opt/sigmastar/gcc-sigmastar-9.1.0-2019.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-9.1.0"
check_result "ssc9381"

make_libalsa ssc268 "arm-linux-gnueabihf" "/opt/sigmastar/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-sigmastar-9.1.0"
check_result "ssc268"

make_libalsa ssc369 "aarch64-linux-gnu" "/opt/sigmastar/gcc-10.2.1-20210303-sigmastar-glibc-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-sigmastar-10.2.1"
check_result "ssc369"

make_libalsa rv1126 "arm-linux-gnueabihf" "/opt/rockchip/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf"
check_result "rv1126"

cp -rf $BaseDir/alsa-lib-1.1.7/include $BaseDir/out/
check_result "cp include"

chmod 777 $BaseDir/out/lib -R


