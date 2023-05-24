// nanoshell/lock.h
// Copyright (C) 2023 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _NANOSHELL_LOCK__H
#define _NANOSHELL_LOCK__H

#include <nanoshell/lock_types.h>

void LockAcquire (SafeLock *pLock);
void LockFree (SafeLock *pLock);

#endif//_NANOSHELL_LOCK__H
