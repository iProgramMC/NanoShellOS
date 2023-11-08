// sys/types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __SYS__TYPES_H
#define __SYS__TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef long mode_t;
typedef long nlink_t;
typedef long uid_t;
typedef long gid_t;
typedef long dev_t;
typedef long id_t;
typedef long pid_t;
typedef long tid_t;
typedef long blksize_t;
typedef long blkcnt_t;
typedef long off_t;
typedef ptrdiff_t ssize_t;
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
typedef unsigned long ino_t;

#endif//__SYS__TYPES_H