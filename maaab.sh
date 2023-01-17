#!/bin/sh
# script to make all apps, remove initrd and make image
# Make All Apps And Build . SHell

sh make_all.sh

rm build/initrd.tar

make image 

sh run.sh

