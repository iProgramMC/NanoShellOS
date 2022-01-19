/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Task Scheduler module header file
******************************************/
#ifndef _TASK_H
#define _TASK_H

#include <main.h>
#include <debug.h>
#include <video.h>
#include <memory.h>
#include <console.h>

#define C_MAX_TASKS 1024//reduced, old was 1024
#define C_STACK_BYTES_PER_TASK 32768 //plenty, but can change later if needed.

/***********************************************************

	A Task is a called function which runs independently 
	of the rest of the kernel. When the task is started 
	(with KeStartTask), a little helper function gets 
	called, which calls the task's m_function.
	When the m_function returns, the helper function
	automatically kills the task.
	(KeKillTask can also be used to kill a task prematurely)
	
************************************************************/

typedef void (*TaskedFunction) (int arg);

// Of course this does not save memory - only register states.
// The memory area should stay the same anyway across taskswitches.

// Normally we'd use  the Registers struct, but, in order to not 
// break shit, we need to push the ESP first, and pop it last.

// This is why we use CPUSaveState instead. (as we did in NanoShell2)
typedef struct CPUSaveState
{
	int cr3;
	int eax, ebx, ecx, edx,
	    esi, edi, ebp, esp,
		eip, cs, eflags;
}
CPUSaveState;

// Task structure definition:
typedef struct
{
	//! TODO: Maybe we could use a linked list instead? Hmm. 
	//  For now I'll just use a hardcoded array.
	
	bool           m_bExists;  //true if this task has been initialized
	TaskedFunction m_pFunction;
	
	CPUSaveState   m_state;
	
	__attribute__((aligned(16)))
	int			   m_fpuState[128];
	
	void *         m_pStack;   //this task's stack (This pointer is equivalent to the peak of the stack.)
	bool           m_featuresArgs;
	int            m_argument;
	bool           m_bFirstTime;
	bool           m_bMarkedForDeletion;
	
	const char*    m_authorFile, 
	          *    m_authorFunc;
	int 		   m_authorLine;
	
	VBEData*       m_pVBEContext;
	Heap *         m_pCurrentHeap;
	Console*       m_pConsoleContext;
	
	char 		   m_tag[33];
}
Task;

enum {
	TASK_SUCCESS = 0x10000000,
	TASK_ERROR_TOO_MANY_TASKS,
	TASK_ERROR_STACK_ALLOC_FAILED,
	TASK_ERROR_END,
} TaskError;

/***********************************************************
    Allows you to spawn a new task. Returns an error code 
	through errorCodeOut. You can pass in arguments via
	pPassedVarlist.
    
	The function needs to have a "void (int)" header.
	pPassedArgument may be null, if the TaskedFunction does
	not require any arguments.
    
	errorCodeOut returns TASK_SUCCESS if the Task returned
	is not NULL. errorCodeOut can also be NULL, if we don't
	actually care (although, this is recommended against)
***********************************************************/
Task* KeStartTaskD(TaskedFunction function, int argument, int *pErrorCodeOut, const char* a, const char* b, int c);
#define KeStartTask(function, argument, errorPtr) \
        KeStartTaskD(function, argument, errorPtr, __FILE__, __FUNCTION__, __LINE__)

/***********************************************************
    Allows you to kill the task passed into itself.
	N.B.: KeKillTask(KeGetRunningTask()) calls KeExit,
	if the task that's currently exiting requests to be
	killed.  If you're done with your task please 
	use KeExit().
	
	  Returns TRUE if task was killed successfully.
	  Execution is guaranteed to not continue if 
	  you killed KeGetRunningTask().
***********************************************************/
bool KeKillTask(Task* pTask);

/***********************************************************
    Gets the currently running task.
	    Returns NULL if this is the kernel task.
***********************************************************/
Task* KeGetRunningTask();

/***********************************************************
    Sets the task's tag.
***********************************************************/
void KeTaskAssignTag(Task* pTask, const char* pTag);

/***********************************************************
    Sets the task's tag.
***********************************************************/
const char* KeTaskGetTag(Task* pTask);

/***********************************************************
    Queues the current task for deletion, and yields.
	Execution will not continue beyond the call to this
	function.  If the current task is the kernel task,
	the kernel will halt (use KeStopSystem())
***********************************************************/
__attribute__((noreturn))
void KeExit();

/***********************************************************
    Internal function to initialize the task scheduler.
***********************************************************/
void KiTaskSystemInitialize();

/***********************************************************
    Internal function to list all tasks to debug output.
***********************************************************/
void KeTaskDebugDump();


#endif//_TASK_H