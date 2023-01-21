#!/bin/bash
#note : I've aliased this shell script as 'tapp'.

if [ -z "$1" ]
then
	echo "usage: $0 <application to try>"
	exit
fi

# make it
if ! make -C apps/$1; then
	echo "Make failed!"
	exit
fi

# copy it
cp apps/$1/$1.nse fs/Bin/$1.nse

# refresh the initrd
rm build/initrd.tar

# and build the other stuff
./makab.sh
