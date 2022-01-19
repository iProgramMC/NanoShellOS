@rem Run script 

@echo off

set NSPath=%CD%
set BackupPath = %path%
set path=%path%;%NSPath%;%NSPath%\tools\i686-gcc\bin

i686-elf-gdb kernel.bin
set path = %BackupPath%
