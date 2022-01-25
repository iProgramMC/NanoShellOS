call clean

: Build hello-world application
cd application/hellowindow
call build
copy Chart.nse ..\..\fs\chart.nse
cd ../..
cd application/version
call build
copy ver.nse ..\..\fs\ver.nse
cd ../..
cd application/helloworld
call build
copy HelloWorld.nse ..\..\fs\HelloWorld.nse
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
