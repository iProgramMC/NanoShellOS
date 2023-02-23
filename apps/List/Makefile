# Should you want to ignore certain sources, you'll have to add all the other ones to `APP_C_FILES`.

APPLICATION_NAME = List
SRC_DIR=src
APP_C_FILES=$(shell find $(SRC_DIR) -type f -name '*.c')
APP_S_FILES=$(shell find $(SRC_DIR) -type f -name '*.asm')

include ../CommonMakefile
