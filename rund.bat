@rem Run script 

@echo off

set backupPath=%path%
set NSPath=%CD%
cd /d d:\Program Files\qemu
set path=%path%;%NSPath%

qemu-system-i386.exe -d cpu_reset -m 16M -serial stdio -kernel %nspath%/kernel.bin -s -S
rem -s -S 

rem go back
cd /d %NSPath%

set path=%backupPath%
