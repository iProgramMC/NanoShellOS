# 
#   Makefile for my OS project
#
#   2020-2021 iProgramInCpp
#

SRC_DIR=src
INC_DIR=include
BUILD_DIR=build

CC=clang
LD=ld
AS=nasm

CFLAGS=-I $(INC_DIR) -I $(BUILD_DIR) -ffreestanding -mno-sse -mno-sse2 -target i686-elf -g -O2 -Wall -Wextra -std=c99 -DRANDOMIZE_MALLOCED_MEMORY
LDFLAGS=-T link.ld -zmax-page-size=0x1000 -g -nostdlib
ASFLAGS=-f elf32

# Compile the kernel

KERNEL_TARGET=$(BUILD_DIR)/kernel.bin
KERNEL_C_FILES=$(shell find $(SRC_DIR) -type f -name '*.c')
KERNEL_AS_FILES=$(shell find $(SRC_DIR) -type f -name '*.asm')
KERNEL_O_FILES=$(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%.o,$(KERNEL_C_FILES) $(KERNEL_AS_FILES))

$(KERNEL_TARGET): $(KERNEL_O_FILES)
	$(info Linking...)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -rf $(BUILD_DIR)/*