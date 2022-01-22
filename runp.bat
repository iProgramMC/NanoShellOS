@rem Run physical script 

@echo off

set backupPath=%path%
set NSPath=%CD%
copy kernel.bin F:\boot\kernel.bin /y
cd /d c:\Program Files\qemu
set path=%path%;%NSPath%

qemu-system-i386 -m 16M -drive file=\\.\PHYSICALDRIVE2,format=raw -display sdl -debugcon stdio -d cpu_reset -monitor telnet:127.0.0.1:55555,server,nowait
: --accel whpx
rem -s -S 

rem go back
cd /d %NSPath%

set path=%backupPath%
