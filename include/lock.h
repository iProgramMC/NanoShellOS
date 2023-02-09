//  ***************************************************************
//  lock.h - Creation date: 26/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _LOCK_H
#define _LOCK_H

typedef struct
{
	volatile bool  m_held;
	volatile void* m_task_owning_it;
	volatile void* m_return_addr;
}
SafeLock;
void LockAcquire (SafeLock *pLock);
void LockFree (SafeLock *pLock);

#endif

