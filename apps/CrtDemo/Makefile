# Makefile for NanoShell CRT application
# March 11, 2022

SRC_DIR=src
INC_DIR=include
BLD_DIR=build
LIS_DIR=../../crt/src
LII_DIR=../../crt/include

CC=clang
LD=ld
AS=nasm

CFLAGS=-I $(INC_DIR) -I $(LII_DIR) -ffreestanding -target i686-elf -g -fno-exceptions -Wall -Wextra -std=c99 -mno-sse -mno-sse2
LDFLAGS=-T link.ld -g -nostdlib -zmax-page-size=0x1000
ASFLAGS=-f elf32

# Compile the application
APP_TARGET=CrtDemo.nse

APP_C_FILES=$(shell find $(SRC_DIR) -type f -name '*.c')
APP_S_FILES=$(shell find $(SRC_DIR) -type f -name '*.asm')
LIB_C_FILES=$(shell find $(LIS_DIR) -type f -name '*.c')
LIB_S_FILES=$(shell find $(LIS_DIR) -type f -name '*.asm')
APP_O_FILES=$(patsubst $(SRC_DIR)/%,$(BLD_DIR)/%.o,$(APP_C_FILES) $(APP_S_FILES))
LIB_O_FILES=$(patsubst $(LIS_DIR)/%,$(BLD_DIR)/%.o,$(LIB_C_FILES) $(LIB_S_FILES))

all: application

clean:
	rm -rf $(BLD_DIR)/*
	rm $(APP_TARGET)

application: $(APP_TARGET)

$(APP_TARGET): $(APP_O_FILES) $(LIB_O_FILES)
	@echo "linking: $@"
	@echo $(APP_O_FILES) $(LIB_O_FILES)
	@$(LD) $(LDFLAGS) -o $@ $^

# Build Library
$(BLD_DIR)/%.asm.o: $(LIS_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "assembling: $<"
	@$(AS) $(ASFLAGS) -o $@ $^

$(BLD_DIR)/%.c.o: $(LIS_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@
	
# Build Source
$(BLD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "assembling: $<"
	@$(AS) $(ASFLAGS) -o $@ $^

$(BLD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@
