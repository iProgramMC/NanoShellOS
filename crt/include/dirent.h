// dirent.h
// Copyright (C) 2023 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _DIRENT_H
#define _DIRENT_H

#include <nanoshell/dirent_types.h>

// NanoShell specifics that have yet to have POSIX names assigned to them. (TODO)
int FiOpenDir  (const char* pFileName);
int FiCloseDir (int dd);
int FiReadDir  (DirEnt* pDirEnt, int dd);
int FiSeekDir  (int dd,          int loc);
int FiRewindDir(int dd);
int FiTellDir  (int dd);
int FiChDir    (const char*pfn);
const char* FiGetCwd();
const char* ErrNoStr(int errno);

// Standard C functions
DIR* opendir(const char* dirname);
int  closedir(DIR *dirp);
void rewinddir(DIR* dirp);
int  telldir(DIR* dirp);
void seekdir(DIR* dirp, int told);
struct dirent* readdir(DIR *dirp);

#endif//_DIRENT_H