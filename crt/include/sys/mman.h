// sys/mman.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __SYS__MMAN_H
#define __SYS__MMAN_H

#include <nanoshell/mman_types.h>

void* mmap(void *, size_t, int, int, int, off_t);
int   munmap(void *, size_t);

#endif//__SYS__MMAN_H