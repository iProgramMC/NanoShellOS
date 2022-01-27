call clean

: Build hello-world application
cd application/hellowindow
call build
copy Chart.nse ..\..\fs\Chart.nse
cd ../..
cd application/version
call build
copy ver.nse ..\..\fs\Version.nse
cd ../..
cd application/helloworld
call build
copy HelloWorld.nse ..\..\fs\HelloWorld.nse
cd ../..
cd application/listtest
call build
copy list.nse ..\..\fs\ListTest.nse
cd ../..
cd application/oregon
call build
copy oregon.nse ..\..\fs\oregon.nse
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
