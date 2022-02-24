call clean

: Build hello-world application
cd application/hellowindow
call build
copy win.nse ..\..\fs\win.nse
cd ../..

: Build cabinet application
:cd application/cabinet
:call build
:copy cab.nse ..\..\fs\cab.nse
:cd ../..

: Make the initrd
make initramdisk

: Build the kernel
call build
