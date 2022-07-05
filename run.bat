@rem Run script 

@echo off

set backupPath=%path%
set NSPath=%CD%
cd /d c:\Program Files\qemu
set path=%path%;%NSPath%

qemu-system-i386.exe                            ^
-device sb16                                    ^
-d cpu_reset                                    ^
-m 256M                                         ^
-boot d                                         ^
-cdrom %nspath%\build\image.iso                 ^
-debugcon stdio                                 ^
-display sdl                                    ^
-accel tcg                                      ^
-drive id=disk,file=%nspath%\vdisk.vdi,if=none  ^
-device ahci,id=ahci                            ^
-device ide-hd,drive=disk,bus=ahci.0            ^
-monitor telnet:127.0.0.1:55555,server,nowait

: -s -S
: -serial COM7
: -kernel %nspath%/kernel.bin 
: -debugcon stdio
: -monitor telnet:127.0.0.1:55555,server,nowait
:
:qemu-system-i386 -m 16M -drive file=\\.\PHYSICALDRIVE1,format=raw
rem -s -S 

rem go back
cd /d %NSPath%

set path=%backupPath%
