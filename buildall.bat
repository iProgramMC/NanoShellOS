call clean

: Build hello-world application
cd application
call buildall.bat
cd ..

: Make the initrd
make initramdisk

: Build the kernel
call build
