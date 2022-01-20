# 
#   Makefile for my OS project
#
#   2020-2021 iProgramInCpp
#
#
#

# TODO: Make only the object files that need making

# Include directory
IDIR=./include
BDIR=./build

# C Compiler and flags
CC=i686-elf-gcc
CFLAGS_BEG=-DTEST

CFLAGS=-I$(IDIR) -I$(BDIR) -ffreestanding -g -O2 -Wall -Wextra -fno-exceptions -std=c99 -DRANDOMIZE_MALLOCED_MEMORY

# Special flags for linker
CLFLAGS_BEG=-T ./link.ld 
CLFLAGS_MID=-ffreestanding -g -nostdlib
CLFLAGS_END=-lgcc

# Assembler and flags
AS=./tools/nasm/nasm
AFLAGS=-felf32

# Icon converter
ICC=./tools/icc/icontest
FSMAKER=./tools/fsmaker

BUILD=build
SRC=src
ICONS=icons
FS=fs
BUICO=build/icons

INITRD=nanoshell_initrd

# Convert the icons

PNG_FILES=$(wildcard $(ICONS)/*.png)
PNG_H_FILES := $(patsubst $(BUILD)/$(ICONS)/%.h, $(BUILD)/%.h, $(foreach file,$(PNG_FILES),$(BUILD)/$(ICONS)/$(file:.png=.h)))

$(BUICO)/%.h: $(ICONS)/%.png
	$(ICC) $< $@

# Compile the kernel

C_MAIN_FILES=$(wildcard $(SRC)/*.c)
C_KAPP_FILES=$(wildcard $(SRC)/kapp/*.c)
C__FS__FILES=$(wildcard $(SRC)/fs/*.c)
ASSEMB_FILES=$(wildcard $(SRC)/asm/*.asm)

O_FILES := $(patsubst $(BUILD)/$(SRC)/%.o, $(BUILD)/%.o, $(foreach file,$(C_MAIN_FILES),$(BUILD)/$(file:.c=.o))) \
		   $(patsubst $(BUILD)/$(SRC)/%.o, $(BUILD)/%.o, $(foreach file,$(C_KAPP_FILES),$(BUILD)/$(file:.c=.o))) \
		   $(patsubst $(BUILD)/$(SRC)/%.o, $(BUILD)/%.o, $(foreach file,$(C__FS__FILES),$(BUILD)/$(file:.c=.o))) \
		   $(patsubst $(BUILD)/$(SRC)/%.o, $(BUILD)/%.o, $(foreach file,$(ASSEMB_FILES),$(BUILD)/$(file:.asm=.o)))

TARGET := kernel.bin

default: $(PNG_H_FILES) $(O_FILES)
	$(info Linking...)
	$(CC) $(CLFLAGS_BEG) -o $(TARGET) $(CLFLAGS_MID) $(O_FILES) $(CLFLAGS_END)
		
# Kernel src files
$(BUILD)/%.o: $(SRC)/%.asm
	$(AS) $(AFLAGS) $< -o $@
	
$(BUILD)/%.o: $(SRC)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)


initramdisk:
	$(FSMAKER) $(FS) $(INITRD)

# Make Clean
clean: 
		$(RM) $(BUILD)/*
		$(RM) ./kernel.bin