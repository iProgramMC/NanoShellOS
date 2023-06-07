#  crt2/Makefile
#  Copyright (C) 2023 iProgramInCpp
#  Makefile for the NanoShell C Runtime Library

CC = clang
LD = ld
AS = nasm
AR = ar

# Description:
# libnanoshell - The shared library that implements most of nanoshell's libc.
# libnanoentry - The static library that implements the entry point of all programs.

# Note: Libnanoshell also includes libgcc.

# Objects included with libnanoshell.
LIBN_C_FILES = \
	src/a_assert.c \
	src/a_cc.c     \
	src/a_env.c    \
	src/a_error.c  \
	src/a_file.c   \
	src/a_math.c   \
	src/a_mem.c    \
	src/a_printf.c \
	src/a_sort.c   \
	src/a_string.c \
	src/a_time.c   \
	src/a_ver.c    \
	src/a_video.c  \
	src/a_window.c \
	src/calls.c
LIBN_ASM_FILES = \
	src/math.asm

# Objects included with libnanoentry.
LIBE_C_FILES = \
	src/entry.c
LIBE_ASM_FILES = \
	src/crt0.asm

CRT_STUB = src/zstub.c

LGCC=../tools/libgcc-i686.a
	
LIBN_O_FILES = $(patsubst src/%,build/libnanoshell/%.o,$(LIBN_C_FILES) $(LIBN_ASM_FILES))
LIBE_O_FILES = $(patsubst src/%,build/libnanoentry/%.o,$(LIBE_C_FILES) $(LIBE_ASM_FILES))

CFLAGS=-I include/ -I src/ -ffreestanding -target i686-elf -g -fno-exceptions -Wall -Wextra -std=c99 -mno-sse -mno-sse2 -fpic
ASFLAGS=-f elf32
ARFLAGS=rcs

LIBN_TARGET = lib/libnanoshell.so
LIBE_TARGET = lib/libnanoentry.a

all: libnanoshell libnanoentry

clean:
	rm -rf build
	rm -rf lib

# Build LibNanoshell
libnanoshell: $(LIBN_O_FILES)
	@mkdir -p $(dir $@)
	@echo "[LibNanoShell] Linking"
	@mkdir -p $(dir $(LIBN_TARGET))
	@$(LD) -shared -T lib_i386.ld -g -nostdlib -zmax-page-size=0x1000 -o $(LIBN_TARGET) $(LIBN_O_FILES) $(LGCC)
	
# Build LibNanoentry
libnanoentry: $(LIBE_O_FILES)
	@mkdir -p $(dir $@)
	@echo "[LibNanoEntry] Archiving"
	@$(AR) $(ARFLAGS) $(LIBE_TARGET) $^
	
# Build LibNanoshell object files
build/libnanoshell/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "[LibNanoShell] Assembling $<"
	@$(AS) $(ASFLAGS) -o $@ $^
	
build/libnanoshell/%.c.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "[LibNanoShell] Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@
	
# Build Libnanoentry object files.
build/libnanoentry/%.c.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "[LibNanoEntry] Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

build/libnanoentry/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "[LibNanoEntry] Assembling $<"
	@$(AS) $(ASFLAGS) -o $@ $^
	
