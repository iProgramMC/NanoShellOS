@rem 
@rem Main build script
@rem

@rem I'm not sure we even need this?

@echo off

echo NanoShell Operating System Build Script
echo (C) 2021 iProgramInCpp
echo.
echo Please wait.

: Print current time:
echo "%date% %time%" > build/icons/__time.h

: Touch the main.c file, so the date is always up-to-date.
echo. >> src/main.c

set backupPath=%path%
set path=%cd%
set laptopshit=C:\Program Files (x86)\GnuWin32\bin
cd /d %path%
set NSPath=%path%
set path=%backupPath%;%NSPath%\tools\i686-gcc\bin;%NSPath%\tools\nasm;%NSPath%\tools;%laptopshit%

:make initramdisk
make

set path=%backupPath%

:pause
:call run.bat