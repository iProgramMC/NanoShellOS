// unistd.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _UNISTD_H
#define _UNISTD_H

#include <nanoshell/unistd_types.h>

// POSIX file access functions
int open  (const char *pfn, int oflag);
int close (int fd);
int read  (int fd,       void* buf,    unsigned int nbyte);
int write (int fd, const void* buf,    unsigned int nbyte);
int lseek (int fd,       int   offset,          int whence);
int tellf (int fd);
int tellsz(int fd); // not standard
int chdir (const char *pfn);
int fchdir(int fd);
int unlink(const char *pfn);

char* getcwd(char* buf, size_t sz);

// NanoShell specifics
const char* FiGetCwd();

#endif//_UNISTD_H