// nanoshell/stdio_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library

#ifndef __NANOSHELL_STDIO_TYPES_H
#define __NANOSHELL_STDIO_TYPES_H

#include <sys/types.h>

#define EOF (-1)

typedef struct
{
	int fd;
	uint8_t ungetc_buf[4];
	int     ungetc_buf_sz;
	bool    eof;
	bool    error;
}
FILE;

#endif//__NANOSHELL_STDIO_TYPES_H
