@rem 
@rem Main build script
@rem

@rem I'm not sure we even need this?

@echo off

echo NanoShell Operating System Build Script
echo (C) 2021 iProgramInCpp
echo.
echo Please wait.
set backupPath=%path%
set path=%cd%
set laptopshit=C:\Program Files (x86)\GnuWin32\bin
cd /d %path%
set NSPath=%path%
set path=%backupPath%;%NSPath%\tools\i686-gcc\bin;%NSPath%\tools\nasm;%NSPath%\tools;%laptopshit%

"C:\Program Files (x86)\GnuWin32\bin\make.exe" initramdisk
"C:\Program Files (x86)\GnuWin32\bin\make.exe"

set path=%backupPath%

:pause
:call run.bat