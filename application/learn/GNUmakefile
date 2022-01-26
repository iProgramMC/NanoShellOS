# 
#   Makefile for user application
#
#   2021-2022 iProgramInCpp
#

# TODO: Make only the object files that need making

# Include directory
IDIR=./include
IDIRLIB=../lib/include

# C Compiler and flags
CC=i686-elf-gcc
CFLAGS_BEG=-DTEST
CFLAGS=-I$(IDIR) -I$(IDIRLIB) -ffreestanding -g -Wall -Wextra -fno-exceptions -std=c99

# Special flags for linker
CLFLAGS_BEG=-T ./link.ld 
CLFLAGS_MID=-ffreestanding -g -nostdlib
CLFLAGS_END=-lgcc

# Assembler and flags
AS=nasm
AFLAGS=-felf32

BUILD=build
SRC=src
LIBSRC=../lib/src

C_FILES_USER=$(wildcard $(SRC)/*.c)
S_FILES_USER=$(wildcard $(SRC)/asm/*.asm)
C_FILES_LIB =$(wildcard $(LIBSRC)/*.c)
S_FILES_LIB =$(wildcard $(LIBSRC)/*.asm)

C_FILES = $(C_FILES_USER) $(C_FILES_LIB)
S_FILES = $(S_FILES_USER) $(S_FILES_LIB)

O_FILES := $(patsubst $(BUILD)/$(SRC)/%.o, $(BUILD)/%.o, $(foreach file,$(C_FILES_USER),$(BUILD)/$(file:.c=.o))) \
		   $(patsubst $(BUILD)/$(SRC)/%.o, $(BUILD)/%.o, $(foreach file,$(S_FILES_USER),$(BUILD)/$(file:.asm=.o))) \
		   $(patsubst $(BUILD)/$(LIBSRC)/%.o, $(BUILD)/%.o, $(foreach file,$(C_FILES_LIB),$(BUILD)/$(file:.c=.o))) \
		   $(patsubst $(BUILD)/$(LIBSRC)/%.o, $(BUILD)/%.o, $(foreach file,$(S_FILES_LIB),$(BUILD)/$(file:.asm=.o)))

TARGET := learn.nse

default: $(O_FILES)
	$(CC) $(CLFLAGS_BEG) -o $(TARGET) $(CLFLAGS_MID) $(O_FILES) $(CLFLAGS_END)
		
# Kernel src files
$(BUILD)/%.o: $(SRC)/%.asm
	$(AS) $(AFLAGS) $< -o $@
$(BUILD)/%.o: $(SRC)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)
$(BUILD)/%.o: $(LIBSRC)/%.asm
	$(AS) $(AFLAGS) $< -o $@
$(BUILD)/%.o: $(LIBSRC)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)


# Make Clean
clean: 
		$(RM) $(BUILD)/*
		$(RM) ./learn.nse