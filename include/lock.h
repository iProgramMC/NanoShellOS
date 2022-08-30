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
	bool  m_held;
	void* m_task_owning_it;
}
SafeLock;
void LockAcquire (SafeLock *pLock);
void LockFree (SafeLock *pLock);

#endif

