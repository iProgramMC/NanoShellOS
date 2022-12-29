// nanoshell/stdlib_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library

#ifndef __NANOSHELL_STDLIB_TYPES_H
#define __NANOSHELL_STDLIB_TYPES_H

#include <sys/types.h>

#define RAND_MAX (0x7FFFFFFF)

typedef int(*ComparisonFunc) (const void*, const void*);
typedef int(*ComparisonReentrantFunc) (const void*, const void*, void*);

#endif//__NANOSHELL_STDLIB_TYPES_H