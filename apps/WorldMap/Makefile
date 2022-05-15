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
STRIP=strip

FEATURES=\
	-DUSE_CONSOLE    \
	-DUSE_VIDEO      \
	-DUSE_MISC       \
	-DUSE_MEMORY     \
	-DUSE_WINDOW

CFLAGS=-I $(INC_DIR) -I $(LII_DIR) $(FEATURES) -ffreestanding -target i686-elf -O2 -fno-exceptions -Wall -Wextra -std=c99 -mno-sse2
LDFLAGS=-T link.ld -g -nostdlib -zmax-page-size=0x1000
ASFLAGS=-f elf32

# Compile the application
APP_TARGET=WorldMap.nse

APP_C_FILES=$(shell find $(SRC_DIR) -type f -name '*.c')
APP_S_FILES=$(shell find $(SRC_DIR) -type f -name '*.asm')
LIB_C_FILES=$(shell find $(LIS_DIR) -type f -name '*.c')
LIB_S_FILES=$(shell find $(LIS_DIR) -type f -name '*.asm')
APP_O_FILES=$(patsubst $(SRC_DIR)/%,$(BLD_DIR)/%.o,$(APP_C_FILES) $(APP_S_FILES))
LIB_O_FILES=$(patsubst $(LIS_DIR)/%,$(BLD_DIR)/%.o,$(LIB_C_FILES) $(LIB_S_FILES))

all: application


application: $(APP_TARGET)

$(APP_TARGET): $(APP_O_FILES) $(LIB_O_FILES)
	@echo "linking: $@"
	@echo $(APP_O_FILES) $(LIB_O_FILES)
	@$(LD) $(LDFLAGS) -o $@ $^
	@$(STRIP) $@

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
