// nanoshell/setjmp_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _SETJMP_TYPES_H
#define _SETJMP_TYPES_H
// https://github.com/jezze/subc
typedef struct
{
	void *esp, *eax, *ebp;
}
JumpBufferTag;

typedef JumpBufferTag JumpBuffer[1], jmp_buf[1];

#endif//_SETJMP_TYPES_H
