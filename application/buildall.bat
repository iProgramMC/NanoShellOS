call cleanall

: Build hello-world application
cd clock
call build
copy Clock.nse ..\..\fs\Clock.nse
cd ..
cd hellowindow
call build
copy Chart.nse ..\..\fs\Chart.nse
cd ..
cd version
call build
copy ver.nse ..\..\fs\Version.nse
cd ..
cd helloworld
call build
copy HelloWorld.nse ..\..\fs\HelloWorld.nse
cd ..
cd listtest
call build
copy list.nse ..\..\fs\ListTest.nse
cd ..
cd oregon
call build
copy oregon.nse ..\..\fs\oregon.nse
cd ..

