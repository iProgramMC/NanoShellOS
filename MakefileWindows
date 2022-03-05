# 
#   Makefile for my OS project
#
#   2020-2021 iProgramInCpp
#

SRC_DIR=src
INC_DIR=include
BUILD_DIR=build
ISO_DIR=$(BUILD_DIR)/iso_root

CC=clang
LD=ld
AS=nasm

CFLAGS=-I $(INC_DIR) -ffreestanding -target i686-elf -g -O2 -Wall -Wextra -std=c99 -DRANDOMIZE_MALLOCED_MEMORY -mno-sse -mno-sse2
LDFLAGS=-T link.ld -g -nostdlib -zmax-page-size=0x1000
ASFLAGS=-f elf32

# Compile the kernel

KERNEL_TARGET=$(BUILD_DIR)/kernel.bin
INITRD_TARGET=$(BUILD_DIR)/initrd.tar
IMAGE_TARGET=$(BUILD_DIR)/image.iso

KERNEL_C_FILES=$(shell find $(SRC_DIR) -type f -name '*.c')
KERNEL_AS_FILES=$(shell find $(SRC_DIR) -type f -name '*.asm')
KERNEL_O_FILES=$(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%.o,$(KERNEL_C_FILES) $(KERNEL_AS_FILES))

all: kernel

limine:
	git clone https://github.com/limine-bootloader/limine -b latest-binary --depth=1
	make -C limine

kernel: $(KERNEL_TARGET)

initrd: $(INITRD_TARGET)

image: limine $(IMAGE_TARGET)

run: image
	qemu-system-i386 -cdrom $(IMAGE_TARGET)

clean:
	rm -rf $(BUILD_DIR)/*

$(KERNEL_TARGET): $(KERNEL_O_FILES)
	@echo "Linking $@"
	@$(LD) $(LDFLAGS) -o $@ $^

$(INITRD_TARGET):
	@tar -cf $@ -C fs .

$(IMAGE_TARGET): $(KERNEL_TARGET) $(INITRD_TARGET)
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)
	@cp $^ limine.cfg limine/limine.sys limine/limine-cd.bin $(ISO_DIR)
	@xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --protective-msdos-label $(ISO_DIR) -o $@ 2>/dev/null
	@limine/limine-install $@ 2>/dev/null
	@rm -rf $(ISO_DIR)

$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling $<"
	@$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@
