#  crt/Makefile
#  Copyright (C) 2022 iProgramInCpp
#  Makefile for the NanoShell C Runtime Library

CC = clang
LD = ld
AS = nasm
AR = ar

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
	src/calls.c    \
	src/entry.c
	
LIBN_ASM_FILES = \
	src/math.asm

CRT1_ASM_FILES = \
	src/crt0.asm

CRT_STUB = src/zstub.c
	
LIBN_O_FILES = $(patsubst src/%,build_libnanoshell/%.o,$(LIBN_C_FILES) $(LIBN_ASM_FILES))
CRT1_O_FILES = $(patsubst src/%,build_crt1/%.o,$(CRT1_ASM_FILES))
CRTS_O_FILES = $(patsubst src/%,build_crts/%.o,$(CRTS_C_FILES))

CFLAGS=-I include/ -I src/ -ffreestanding -target i686-elf -g -fno-exceptions -Wall -Wextra -std=c99 -mno-sse -mno-sse2
ASFLAGS=-f elf32
ARFLAGS=rcs
CRT1_LDFLAGS=-relocatable

LIBN_TARGET = lib/libnanoshell.a
CRT1_TARGET = lib/crt1.o
CRTI_TARGET = lib/crti.o
CRTN_TARGET = lib/crtn.o

ROOT_PATH = ../fs

all: libnanoshell crt1 crti

update: all
	@echo "Copying the NanoShell C library to the root FS at '$(ROOT_PATH)'"
	cp $(LIBN_TARGET) $(ROOT_PATH)/User/Library/$(notdir $(LIBN_TARGET))
	cp $(CRT1_TARGET) $(ROOT_PATH)/User/Library/$(notdir $(CRT1_TARGET))
	cp $(CRTI_TARGET) $(ROOT_PATH)/User/Library/$(notdir $(CRTI_TARGET))
	cp $(CRTN_TARGET) $(ROOT_PATH)/User/Library/$(notdir $(CRTN_TARGET))
	rm -rf ../build/initrd.tar
	rm -rf ../build/image.iso
	
updinc:
	cp -r include/* $(ROOT_PATH)/User/Include

clean:
	rm -rf build_libnanoshell
	rm -rf build_crt1
	rm -rf lib

libnanoshell: $(LIBN_O_FILES)
	@echo "Archiving libnanoshell"
	@mkdir -p $(dir $(LIBN_TARGET))
	@$(AR) $(ARFLAGS) $(LIBN_TARGET) $^
	
build_libnanoshell/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "[LibNanoshell] Assembling $<"
	@$(AS) $(ASFLAGS) -o $@ $^
	
build_libnanoshell/%.c.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "[LibNanoshell] Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

crt1: $(CRT1_O_FILES)
	@echo "Linking crt1"
	@mkdir -p $(dir $(CRT1_TARGET))
	@$(LD) $(CRT1_LDFLAGS) -o $(CRT1_TARGET) -T crt1.ld $^

build_crt1/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "[CRT1] Assembling $<"
	@$(AS) $(ASFLAGS) -o $@ $^

crti: src/zstub.c
	@echo "Compiling stub object"
	@mkdir -p $(dir $(CRTI_TARGET))
	@$(CC) $(CFLAGS) -c $(CRT_STUB) -o $(CRTI_TARGET)
	@cp $(CRTI_TARGET) $(CRTN_TARGET)
	
	
