#!/bin/sh

usage() {
	echo "Usage: $1 [--CHIP] ./make.sh HI3519A"
	echo "CHIP:HI3519V101,HI3519A, HI3516EV300, MT6797, HI3559, SSC9381, SSC333, SSC337"
	exit 1
}

if [ ! -n "$1" ] ;then
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

printf -- '-make:%s svn/git:%s\n' "$1" "$rev"

mkdir -p ko

make CHIP=$1 clean
make CHIP=$1 SVN_GIT_VERSION=$rev

cp ko /home/$(whoami)/nfs/ -rf
#rm /home/$(whoami)/nfs/ko/*.sh
#rm /home/$(whoami)/nfs/ko/vbus_gpio.ko
#rm /home/$(whoami)/nfs/ko/g_dfu.ko
