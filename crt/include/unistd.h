// unistd.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _UNISTD_H
#define _UNISTD_H

#include <nanoshell/unistd_types.h>

// POSIX file access functions
int open  (const char* path, int oflag);
int close (int fd);
int read  (int fd,       void* buf,    unsigned int nbyte);
int write (int fd, const void* buf,    unsigned int nbyte);
int lseek (int fd,       int   offset,          int whence);
int tellf (int fd);
int tellsz(int fd);

#endif//_UNISTD_H