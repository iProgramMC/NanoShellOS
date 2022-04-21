#!/bin/sh
echo "Getting rid of previous application versions..."
rm fs/Bin -rf
mkdir fs/Bin

echo "Building all applications..."
cd apps

# for each directory in apps/
for d in * ; do
	#if it is a dir
	if [ -d "$d" ]; then
		#go there
		echo "Building $d"
		cd "$d"
		
		make
		
		cp "$d.nse" "../../fs/Bin/$d.nse"
		
		cd ..
	fi
done

cd ..