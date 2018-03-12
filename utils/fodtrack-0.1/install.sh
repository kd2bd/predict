#!/bin/bash
#
# install.sh - small script for installing fodtrack
#
# test if we are root
#
whoami=`whoami`
if [ $whoami != "root" ]; then
	echo "Sorry $whoami, you need to be root to install this program"
	exit
fi

echo Compiling fodtrack...

gcc -O2 fodtrack.c -o fodtrack

echo Installing fodtrack...

cp fodtrack /usr/local/bin
rm fodtrack
cp fodtrack.conf /etc
mkdir -p /usr/local/man/man8
cp fodtrack.8 /usr/local/man/man8
mkdir -p /usr/local/man/man5
cp fodtrack.conf.5 /usr/local/man/man5



