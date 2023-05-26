// nanoshell/lock_types.h
// Copyright (C) 2023 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _NANOSHELL_LOCKTYPES__H
#define _NANOSHELL_LOCKTYPES__H

typedef struct
{
	volatile bool  m_held;
	volatile void* m_task_owning_it;
	volatile void* m_return_addr;
}
SafeLock;

#endif//_NANOSHELL_LOCKTYPES__H
