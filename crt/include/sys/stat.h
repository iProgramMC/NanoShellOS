// sys/mman.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __SYS__STAT_H
#define __SYS__STAT_H

#include <nanoshell/stat_types.h>

int FiStatAt  (int dd,         const char*pfn,  StatResult* pres);
int FiStat    (const char*pfn, StatResult* pres);
int FiLinkStat(const char*pfn, StatResult* pres);
int FiFDStat  (int fd, StatResult* pres);

int    chmod(const char *, mode_t);
int    fchmod(int, mode_t);
int    fstat(int, struct stat *);
int    lstat(const char *, struct stat *);
int    mkdir(const char *, mode_t);
int    stat(const char *, struct stat *);
//int    mkfifo(const char *, mode_t);
//int    mknod(const char *, mode_t, dev_t);
//mode_t umask(mode_t);

#endif//__SYS__STAT_H