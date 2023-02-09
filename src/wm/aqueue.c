/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Manager Action Queue Module
******************************************/
#include "wi.h"

static WindowAction s_internal_action_queue[4096];
static int          s_internal_action_queue_head,
                    s_internal_action_queue_tail;
static SafeLock     s_internal_action_queue_lock;

SafeLock* ActionQueueGetSafeLock()
{
	return &s_internal_action_queue_lock;
}

bool ActionQueueWouldOverflow()
{
	if (s_internal_action_queue_tail == 0)
		return s_internal_action_queue_head == 4095;
	return s_internal_action_queue_head == s_internal_action_queue_tail - 1;
}

WindowAction* ActionQueueAdd(WindowAction action)
{
	while (ActionQueueWouldOverflow())
		KeTaskDone();
	
	LockAcquire(&s_internal_action_queue_lock);
	
	WindowAction *pAct = &s_internal_action_queue[s_internal_action_queue_head];
	*pAct = action;
	
	s_internal_action_queue_head = (s_internal_action_queue_head + 1) % 4096;
	
	LockFree(&s_internal_action_queue_lock);
	
	return pAct;
}

WindowAction* ActionQueueGetFront()
{
	return &s_internal_action_queue[s_internal_action_queue_tail];
}

void ActionQueuePop()
{
	s_internal_action_queue_tail = (s_internal_action_queue_tail + 1) % 4096;
}

void ActionQueueWaitForFrontToFinish()
{
	while (ActionQueueGetFront()->bInProgress);
}

bool ActionQueueEmpty()
{
	return s_internal_action_queue_tail == s_internal_action_queue_head;
}

