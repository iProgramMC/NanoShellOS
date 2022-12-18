// unistd.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _UNISTD_H
#define _UNISTD_H

#include <nanoshell/unistd_types.h>

// POSIX file access functions
int     open  (const char* path, int oflag);
int     close (int fd);
size_t  read  (int fd,       void* buf,    unsigned int nbyte);
size_t  write (int fd, const void* buf,    unsigned int nbyte);
int     lseek (int fd,       int   offset,          int whence);
int     tellf (int fd);
int     tellsz(int fd);

// NanoShell specifics that have yet to have POSIX names assigned to them. (TODO)
int     FiOpenDir  (const char* pFileName);
int     FiCloseDir (int dd);
DirEnt* FiReadDir  (int dd);
int     FiSeekDir  (int dd,          int loc);
int     FiRewindDir(int dd);
int     FiTellDir  (int dd);
int     FiStatAt   (int dd,         const char*pfn,  StatResult* pres);
int     FiStat     (const char*pfn, StatResult* pres);
int     FiChDir    (const char*pfn);
const char* FiGetCwd();
const char* ErrNoStr(int errno);

#endif//_UNISTD_H