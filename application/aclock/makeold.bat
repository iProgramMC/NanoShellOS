@echo off

set kpath=Z:\NanoshellV3
set path=%path%;%kpath%\tools\i686-gcc\bin;%kpath%\tools\nasm;

echo compiling
i686-elf-gcc -c lib.c -o lib.o -I %kpath%\tools\i686-gcc\include -I . -ffreestanding -g -Wall -Wextra -fno-exceptions -std=c99
i686-elf-gcc -c main.c -o main.o -I %kpath%\tools\i686-gcc\include -I . -ffreestanding -g -Wall -Wextra -fno-exceptions -std=c99
nasm -felf32 liba.asm -o liba.o

echo linking
i686-elf-gcc -T link.ld -o main.elf -ffreestanding -g -nostdlib main.o lib.o liba.o -lgcc

echo Done!