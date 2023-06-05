// errno.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __ERRNO_H
#define __ERRNO_H

#include <nanoshell/error_nums.h>

int  seterrno(int en);
int  geterrno(int en);
int* geterrnoptr();
#define errno (*geterrnoptr())

void perror(const char* fmt, ...);

#endif//__ERRNO_H