/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

          Task Scheduler  module
******************************************/
#include <task.h>
#include <memory.h>
#include <string.h>
#include <print.h>

// The kernel task is task 0.  Other tasks are 1-indexed.
// This means g_runningTasks[0] is unused.

__attribute__((aligned(16)))
Task g_runningTasks[C_MAX_TASKS];


static int s_lastRunningTaskIndex = 1;

static int s_currentRunningTask = -1;
static CPUSaveState g_kernelSaveState;
__attribute__((aligned(16)))
static int          g_kernelFPUState[128];
static VBEData*     g_kernelVBEContext = NULL;
static Heap*        g_kernelHeapContext = NULL;
static Console*     g_kernelConsoleContext = NULL;

extern Heap*        g_pHeap;
extern Console*     g_currentConsole; //logmsg

bool g_forceKernelTaskToRunNext = false;

void ForceKernelTaskToRunNext(void)
{
	g_forceKernelTaskToRunNext = true;
}

void KeFindLastRunningTaskIndex(void)
{
	for (int i = C_MAX_TASKS - 1; i > 0; i--)
	{
		if (g_runningTasks[i].m_bExists)
		{
			s_lastRunningTaskIndex = i + 1;
			return;
		}
	}
	s_lastRunningTaskIndex = 1;
}

// Requests a re-schedule.
void KeTaskDone(void)
{
	asm ("int $0x81\n\t");
}

void KeTaskDebugDump(void)
{
	cli;
	bool any_tasks = false;
	LogMsg("Listing tasks.");
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		Task* t = g_runningTasks + i;
		if (!t->m_bExists) continue;
		any_tasks = true;
		LogMsg("- %d  F:%x  AL:%d AF:%s AS:%s", i, t->m_pFunction, t->m_authorLine, t->m_authorFunc, t->m_authorFile);
	}
	if (!any_tasks)
		LogMsg("No tasks currently running.");
	sti;
}
void KeTaskAssignTag(Task* pTask, const char* pTag)
{
	if (!pTask)
		return;
	int e = strlen(pTag);
	if (e >= 30)
		e = 30;
	memcpy (pTask->m_tag, pTag, e + 1);
	char tempbuf[33];
	sprintf(tempbuf, "[%s]", pTask->m_tag);
	strcpy (pTask->m_tag, tempbuf);
}
const char* KeTaskGetTag(Task* pTask)
{
	if (!pTask)
		return "<kernel task>";
	return pTask->m_tag;
}

// This function (in asm/task.asm) prepares the initial task for
// execution.
extern void KeTaskStartup();
extern uint32_t g_curPageDirP; //memory.c
extern VBEData* g_vbeData, g_mainScreenVBEData;
void KeConstructTask (Task* pTask)
{
	pTask->m_state.esp = ((int)pTask->m_pStack + C_STACK_BYTES_PER_TASK) & ~0xF; //Align to 4 bits
	pTask->m_state.ebp = 0;
	pTask->m_state.eip = (int)KeTaskStartup;
	pTask->m_state.eax = (int)pTask;
	// fill in the other registers with garbage, so we know that it's executed ok
	pTask->m_state.ebx = 0x01234567;
	pTask->m_state.ecx = 0x7A7053E1;
	pTask->m_state.edx = 0xCE999999;
	pTask->m_state.esi = 0xAAAAAAAA;
	pTask->m_state.edi = 0xBBBBBBBB;
	pTask->m_state.cs  = 0x8;//same as our CS
	pTask->m_state.eflags = 0x297; //same as our own EFL register
	pTask->m_state.cr3 = g_curPageDirP; //same as our own CR3 register
	
	ZeroMemory (pTask->m_fpuState, sizeof(pTask->m_fpuState));
	
	//clear the stack
	ZeroMemory (pTask->m_pStack, C_STACK_BYTES_PER_TASK);
	
	// push the iretd worthy registers on the stack:
	pTask->m_state.esp -= sizeof(int) * 5;
	memcpy ((void*)(pTask->m_state.esp), &pTask->m_state.eip, sizeof(int)*3);
	
	pTask->m_pVBEContext = &g_mainScreenVBEData;
	pTask->m_pCurrentHeap = g_pHeap;//default kernel heap.
	pTask->m_pConsoleContext = g_currentConsole;
}

Task* KeStartTaskD(TaskedFunction function, int argument, int* pErrorCodeOut, const char* authorFile, const char* authorFunc, int authorLine)
{
	cli; //must do this, because otherwise we can expect an interrupt to come in and load our unfinished structure
	
	int i = 1;
	for (; i < C_MAX_TASKS; i++)
	{
		if (!g_runningTasks[i].m_bExists)
		{
			break;
		}
	}
	
	if (i == C_MAX_TASKS)
	{
		*pErrorCodeOut = TASK_ERROR_TOO_MANY_TASKS;
		sti;
		return NULL;
	}
	
	void *pStack = MmAllocate(C_STACK_BYTES_PER_TASK);
	if (pStack)
	{
		//Setup our new task here:
		Task* pTask = &g_runningTasks[i];
		pTask->m_bExists = true;
		pTask->m_pFunction = function;
		pTask->m_pStack = pStack;
		pTask->m_bFirstTime = true;
		pTask->m_authorFile = authorFile;
		pTask->m_authorFunc = authorFunc;
		pTask->m_authorLine = authorLine;
		pTask->m_argument   = argument;
		pTask->m_bMarkedForDeletion = false;
		
		char buffer[32];
		sprintf(buffer, "<task no. %d>", i);
		
		KeConstructTask(pTask);
		
		if (pErrorCodeOut)
			*pErrorCodeOut = TASK_SUCCESS;
		sti;
		
		KeFindLastRunningTaskIndex ();
		
		return pTask;
	}
	else
	{
		*pErrorCodeOut = TASK_ERROR_STACK_ALLOC_FAILED;
		sti;
		return NULL;
	}
}
static void KeResetTask(Task* pTask, bool killing, bool interrupt)
{
	if (!interrupt) cli; //must do this, because otherwise we can expect an interrupt to come in and load our unfinished structure
	if (pTask == KeGetRunningTask())
	{
		//SLogMsg("Marked current task for execution (KeResetTask)");
		pTask->m_bMarkedForDeletion = true;
		sti;//if we didn't restore interrupts here would be our death point
		while (1) hlt;
	}
	else
	{
		//SLogMsg("Deleting task %x (KeResetTask, killing:%d)", pTask, killing);
		if (killing && pTask->m_pStack)
		{
			//SLogMsg("Freeing this task's stack");
			MmFree(pTask->m_pStack);
		}
		pTask->m_pStack = NULL;
		
		//SLogMsg("Resetting stuff about it...");
		pTask->m_bFirstTime = false;
		pTask->m_bExists    = false;
		pTask->m_pFunction  = NULL;
		pTask->m_authorFile = NULL;
		pTask->m_authorFunc = NULL;
		pTask->m_authorLine = 0;
		pTask->m_argument   = 0;
		pTask->m_featuresArgs = false;
		pTask->m_bMarkedForDeletion = false;
		if (!interrupt) sti;//if we didn't restore interrupts here would be our death point
	}
}
bool KeKillTask(Task* pTask)
{
	if (pTask == KeGetRunningTask())
		KeExit();
	
	cli;
	if (pTask == NULL)
	{
		sti; return false;
	}
	if (!pTask->m_bExists)
	{
		sti; return false;
	}
	KeResetTask(pTask, true, false);
	sti;
	return true;
}
Task* KeGetRunningTask()
{
	if (s_currentRunningTask == -1) return NULL;
	return &g_runningTasks[s_currentRunningTask];
}
void KiTaskSystemInitialize()
{
	for (int i = 0; i < C_MAX_TASKS; i++)
		KeResetTask(g_runningTasks + i, false, true);
}

CPUSaveState* g_saveStateToRestore1 = NULL;
extern void KeStartedNewKernelTask();
extern void KeStartedNewTask();
void KeRestoreKernelTask()
{
	g_saveStateToRestore1 = &g_kernelSaveState;
	KeStartedNewKernelTask();
}
void KeRestoreStandardTask(Task* pTask)
{
	g_saveStateToRestore1 = &pTask->m_state;
	KeStartedNewTask();
}
void KeTaskStartupC(Task* pTask)
{
	//call the main function of the thread
	pTask->m_pFunction (pTask->m_argument);
	
	//after we're done, kill the task
	KeExit();
}
__attribute__((noreturn))
void KeExit()
{
	if (!KeGetRunningTask())
	{
		LogMsg("Stopping system!?");
		KeStopSystem();
	}
	
	//SLogMsg("Marked current task for execution (KeExit)");
	KeGetRunningTask()->m_bMarkedForDeletion = true;
	while (1) hlt;
}

void KeFxSave(int *fpstate)
{
	asm("fxsave (%0)" :: "r"(fpstate));
}
void KeFxRestore(int *fpstate)
{
	asm("fxrstor (%0)" :: "r"(fpstate));
}
void KeSwitchTask(CPUSaveState* pSaveState)
{
	Task* pTask = KeGetRunningTask();
	//Please note that tasking code does not use the FPU, so we should be safe just saving it here.
	if (pTask)
	{
		memcpy (& pTask -> m_state, pSaveState, sizeof(CPUSaveState));
		KeFxSave (pTask -> m_fpuState);
		pTask->m_pVBEContext = g_vbeData;
		pTask->m_pCurrentHeap = g_pHeap;
		pTask->m_pConsoleContext = g_currentConsole;
	}
	else
	{
		memcpy (&g_kernelSaveState, pSaveState, sizeof(CPUSaveState));
		KeFxSave (g_kernelFPUState); //perhaps we won't use this.
		g_kernelVBEContext = g_vbeData;
		g_kernelHeapContext = g_pHeap;
		g_kernelConsoleContext = g_currentConsole;
	}
	ResetToKernelHeap();
	
	// If using RTC, also flush register C
	//WritePort (0x70, 0x0C);
	//ReadPort  (0x71);
	
	if (!pTask) //switching away from kernel task?
	{
		for (int i = 0; i < C_MAX_TASKS; i++)
		{
			if (g_runningTasks[i].m_bMarkedForDeletion)
			{
				KeResetTask(&g_runningTasks[i], true, true);
			}
		}
	}
	
	Task* pNewTask = NULL;
	if (g_forceKernelTaskToRunNext)
	{
		s_currentRunningTask = -1;
		g_forceKernelTaskToRunNext = false;
	}
	else
	{
		int i = s_currentRunningTask + 1;
		int task = s_lastRunningTaskIndex;
		for (; i < task; i++)
		{
			if (g_runningTasks[i].m_bExists)
				break;
		}
		
		if (i < task)
		{
			pNewTask = g_runningTasks + i;
			s_currentRunningTask = i;
		}
		else
			s_currentRunningTask = -1;
	}
	
	if (pNewTask)
	{
		//first, restore this task's FPU registers:
		KeFxRestore(pNewTask->m_fpuState);
		g_vbeData = pNewTask->m_pVBEContext;
		g_currentConsole = pNewTask->m_pConsoleContext;
		UseHeap (pNewTask->m_pCurrentHeap);
		KeRestoreStandardTask(pNewTask);
	}
	else
	{
		//Kernel task
		//first, restore the kernel task's FPU registers:
		KeFxRestore(g_kernelFPUState);
		g_vbeData = g_kernelVBEContext;
		g_currentConsole = g_kernelConsoleContext;
		UseHeap (g_kernelHeapContext);
		KeRestoreKernelTask();
	}
}

