#!/bin/sh
# script to make all apps, remove initrd and make image
# Make All Apps And Build . SHell

if ! make image; then
	echo "Make failed!"
	exit
fi

sh run.sh

