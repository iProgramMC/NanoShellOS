/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

          Task Scheduler  module
******************************************/
#include <task.h>
#include <memory.h>
#include <process.h>
#include <string.h>
#include <print.h>
#include <misc.h>

// The kernel task is task 0.  Other tasks are 1-indexed.
// This means g_runningTasks[0] is unused.

__attribute__((aligned(16)))
Task g_runningTasks[C_MAX_TASKS];

extern bool g_interruptsAvailable;

static int s_lastRunningTaskIndex = 1;

static int s_currentRunningTask = -1;
static CPUSaveState g_kernelSaveState;

static Process*       g_pProcess = NULL;

__attribute__((aligned(16)))
static int            g_kernelFPUState[128];
static VBEData*       g_kernelVBEContext = NULL;
static Heap*          g_kernelHeapContext = NULL;
static Console*       g_kernelConsoleContext = NULL;
static const uint8_t* g_kernelFontContext = NULL;
static char           g_kernelCwd[PATH_MAX+2];
static uint32_t       g_kernelSysCallNum; //honestly, kind of useless since the kernel task will never be an ELF trying to call into the system :^)

extern Heap*          g_pHeap;
extern Console*       g_currentConsole; //logmsg
extern const uint8_t* g_pCurrentFont;
extern char           g_cwd[PATH_MAX+2];

extern bool           g_interruptsAvailable;

bool g_forceKernelTaskToRunNext = false;


void ForceKernelTaskToRunNext(void)
{
	g_forceKernelTaskToRunNext = true;
}

void KeKillThreadByPID (int proc)
{
	if (proc < 0 || proc >= (int)ARRAY_COUNT (g_runningTasks)) return;
	
	if (!g_runningTasks[proc].m_bExists) return;
	
	if (!KeKillTask(&g_runningTasks[proc]))
	{
		LogMsg("Can't kill task!");
	}
	else LogMsg("Killed task with pid %d", proc);
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
bool gIdleOnDoneTask = false, gDisableIdle = false;
void KeTaskDone(void)
{
	if (gIdleOnDoneTask)
		hlt;
	else
		// actually request a re-schedule.
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
void KeFxSave(int *fpstate);
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
	
	//seg registers
	pTask->m_state.cs  = SEGMENT_KECODE;//same as our CS
	pTask->m_state.ds  = SEGMENT_KEDATA;
	pTask->m_state.es  = SEGMENT_KEDATA;
	pTask->m_state.fs  = SEGMENT_KEDATA;
	pTask->m_state.gs  = SEGMENT_KEDATA;
	pTask->m_state.ss  = SEGMENT_KEDATA;
	
	pTask->m_state.eflags = 0x297; //same as our own EFL register
	pTask->m_state.cr3 = g_curPageDirP; //same as our own CR3 register
	
	ZeroMemory (pTask->m_fpuState, sizeof(pTask->m_fpuState));
	
	//clear the stack
	ZeroMemory (pTask->m_pStack, C_STACK_BYTES_PER_TASK);
	
	// push the iretd worthy registers on the stack:
	pTask->m_state.esp -= sizeof(int) * 5;
	memcpy ((void*)(pTask->m_state.esp), &pTask->m_state.eip, sizeof(int)*3);
	
	pTask->m_pVBEContext     = &g_mainScreenVBEData;
	pTask->m_pCurrentHeap    = g_pHeap;//default kernel heap.
	pTask->m_pConsoleContext = g_currentConsole;
	pTask->m_pFontContext    = g_pCurrentFont;
	strcpy (pTask->m_cwd, g_cwd); // inherit from parent
	
	memset   (pTask->m_fpuState, 0, sizeof (pTask->m_fpuState));
	KeFxSave (pTask->m_fpuState);
}

void MmFreeUnsafeK(void *ptr);
void ExOnThreadExit (Process* pProc, Task* pTask);

Task* KeStartTaskExUnsafeD(TaskedFunction function, int argument, int* pErrorCodeOut, void *pProcVoid, const char* authorFile, const char* authorFunc, int authorLine)
{
	Process *pProc = (Process*)pProcVoid;
	// Pre-allocate the stack, since it depends on interrupts being on
	void *pStack = MmAllocateK(C_STACK_BYTES_PER_TASK);
	if (!pStack)
	{
		*pErrorCodeOut = TASK_ERROR_STACK_ALLOC_FAILED;
		return NULL;
	}
	
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
		
		// Pre-free the pre-allocated stack. Don't need it lying around
		if (pStack)
			MmFreeUnsafeK(pStack);
		
		return NULL;
	}
	
	if (pStack)
	{
		//Setup our new task here:
		Task* pTask = &g_runningTasks[i];
		memset (pTask, 0, sizeof *pTask);
		pTask->m_bExists = true;
		pTask->m_pFunction = function;
		pTask->m_pStack = pStack;
		pTask->m_bFirstTime = true;
		pTask->m_authorFile = authorFile;
		pTask->m_authorFunc = authorFunc;
		pTask->m_authorLine = authorLine;
		pTask->m_argument   = argument;
		pTask->m_bMarkedForDeletion = false;
		pTask->m_pProcess = pProc;
		
		Heap* pBkp = g_pHeap;
		
		// This replacement is purely symbolic: while it won't use the
		// page directory of this heap, it will apply it to the new task.
		if (pProc)
			g_pHeap = &pProc->sHeap;
		
		char buffer[32];
		sprintf(buffer, "<task no. %d>", i);
		
		KeConstructTask(pTask);
		
		// Restore Old Heap
		g_pHeap = pBkp;
		
		if (pErrorCodeOut)
			*pErrorCodeOut = TASK_SUCCESS;
		
		// Update last running task index. Makes task scheduling faster
		KeFindLastRunningTaskIndex ();
		
		return pTask;
	}
	else
	{
		*pErrorCodeOut = TASK_ERROR_STACK_ALLOC_FAILED;
		return NULL;
	}
}
Task* KeStartTaskExD(TaskedFunction function, int argument, int* pErrorCodeOut, void *pProcVoid, const char* authorFile, const char* authorFunc, int authorLine)
{
	cli; //must do this, because otherwise we can expect an interrupt to come in and load our unfinished structure
	Task* pResult = KeStartTaskExUnsafeD (function, argument, pErrorCodeOut, pProcVoid, authorFile, authorFunc, authorLine);
	sti;
	return pResult;
}
Task* KeStartTaskD(TaskedFunction function, int argument, int* pErrorCodeOut, const char* authorFile, const char* authorFunc, int authorLine)
{
	return KeStartTaskExD(function, argument, pErrorCodeOut, NULL, authorFile, authorFunc, authorLine);
}

static void KeResetTask(Task* pTask, bool killing, bool interrupt)
{
	if (!interrupt) cli; //must do this, because otherwise we can expect an interrupt to come in and load our unfinished structure
	if (pTask == KeGetRunningTask())
	{
		pTask->m_bMarkedForDeletion = true;
		if (!interrupt)
		{
			 LogMsg("KEResetTask: WTF?");
			SLogMsg("KEResetTask: WTF?");
			KeStopSystem ();
		}
		sti;//if we didn't restore interrupts here would be our death point
		while (1) hlt;
	}
	else
	{
		//SLogMsg("Deleting task %x (KeResetTask, killing:%d)", pTask, killing);
		if (killing && pTask->m_pStack)
		{
			MmFreeUnsafeK(pTask->m_pStack);
		}
		pTask->m_pStack = NULL;
		
		pTask->m_bFirstTime = false;
		pTask->m_bExists    = false;
		pTask->m_pFunction  = NULL;
		pTask->m_authorFile = NULL;
		pTask->m_authorFunc = NULL;
		pTask->m_authorLine = 0;
		pTask->m_reviveAt   = 0;
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
	
	if (pTask->m_pProcess)
		ExOnThreadExit ((Process*)pTask->m_pProcess, pTask);
	
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
	
	Task* pTask = KeGetRunningTask();
	
	if (pTask->m_pProcess)
		ExOnThreadExit ((Process*)pTask->m_pProcess, pTask);
	
	//SLogMsg("Marked current task for execution (KeExit)");
	pTask->m_bMarkedForDeletion = true;
	while (1)
		KeTaskDone ();
}
#ifdef USE_SSE_FXSAVE
void KeFxSave(int *fpstate)
{
	asm("fxsave (%0)" :: "r"(fpstate));
}
void KeFxRestore(int *fpstate)
{
	asm("fxrstor (%0)" :: "r"(fpstate));
}
#else
void KeFxSave(int *fpstate)
{
	asm("fwait\n"
		"fsave (%0)" :: "r"(fpstate));
}
void KeFxRestore(int *fpstate)
{
	asm("fwait\n"
		"frstor (%0)" :: "r"(fpstate));
}
#endif

void KeCheckDyingTasks()
{
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		if (g_runningTasks[i].m_bMarkedForDeletion)
		{
			KeResetTask(&g_runningTasks[i], true, true);
		}
	}
}	

// Every 10 milliseconds the task switches continue
///int g_TaskSwitchingAggressiveness = 10;

void ResetToKernelHeapUnsafe();
void UseHeapUnsafe (Heap* pHeap);
void ExCheckDyingProcesses();

uint32_t* const pSysCallNum = (uint32_t*)0xC0007CFC;//for easy reading; the pointer itself is constant

void KeSwitchTask(CPUSaveState* pSaveState)
{
	g_pProcess = NULL;
	
	Task* pTask = KeGetRunningTask();
	//Please note that tasking code does not use the FPU, so we should be safe just saving it here.
	if (pTask)
	{
		memcpy (& pTask -> m_state, pSaveState, sizeof(CPUSaveState));
		memcpy   (pTask -> m_cwd,   g_cwd,      sizeof(g_cwd));
		KeFxSave (pTask -> m_fpuState);
		pTask->m_pVBEContext     = g_vbeData;
		pTask->m_pCurrentHeap    = g_pHeap;
		pTask->m_pConsoleContext = g_currentConsole;
		pTask->m_pFontContext    = g_pCurrentFont;
		pTask->m_sysCallNum      = *pSysCallNum;
	}
	else
	{
		memcpy (&g_kernelSaveState, pSaveState, sizeof(CPUSaveState));
		memcpy   (g_kernelCwd,      g_cwd,      sizeof(g_cwd));
		KeFxSave (g_kernelFPUState); //perhaps we won't use this.
		g_kernelVBEContext     = g_vbeData;
		g_kernelHeapContext    = g_pHeap;
		g_kernelConsoleContext = g_currentConsole;
		g_kernelFontContext    = g_pCurrentFont;
		g_kernelSysCallNum     = *pSysCallNum;
	}
	ResetToKernelHeapUnsafe();
	
	if (!pTask) //switching away from kernel task?
	{
		KeCheckDyingTasks();
		ExCheckDyingProcesses();
	}
	
	int g_tick_count = GetTickCount();
	
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
			Task* p = &g_runningTasks[i];
			if (p->m_bExists)
			{
				// Is the task scheduled to come back online?
				if (p->m_reviveAt < g_tick_count && !p->m_bSuspended)
					break;
			}
		}
		
		if (i < task)
		{
			pNewTask = g_runningTasks + i;
			s_currentRunningTask = i;
		}
		else
		{
			//kernel task will always come back no matter what
			s_currentRunningTask = -1;
		}
	}
	
	if (pNewTask)
	{
		//first, restore this task's FPU registers:
		KeFxRestore(pNewTask->m_fpuState);
		memcpy (g_cwd, pNewTask->m_cwd, sizeof (g_cwd));
		g_vbeData        = pNewTask->m_pVBEContext;
		g_currentConsole = pNewTask->m_pConsoleContext;
		g_pCurrentFont   = pNewTask->m_pFontContext;
		*pSysCallNum     = pNewTask->m_sysCallNum;
		
		g_pProcess = (Process*)pNewTask->m_pProcess;
		
		UseHeapUnsafe (pNewTask->m_pCurrentHeap);
		KeRestoreStandardTask(pNewTask);
	}
	else
	{
		//Kernel task
		//first, restore the kernel task's FPU registers:
		KeFxRestore(g_kernelFPUState);
		memcpy (g_cwd, g_kernelCwd, sizeof (g_cwd));
		g_vbeData = g_kernelVBEContext;
		g_currentConsole = g_kernelConsoleContext;
		g_pCurrentFont = g_kernelFontContext;
		*pSysCallNum     = g_kernelSysCallNum;
		
		g_pProcess = NULL;
		
		UseHeapUnsafe (g_kernelHeapContext);
		KeRestoreKernelTask();
	}
}

void LockAcquire (SafeLock *pLock) // An attempt at a safe lock
{
	while (true)
	{
		// Clear interrupts: we need the following to be atomic
		cli;
		
		// If the lock's value is false (i.e. it has been freed) then we can grab it.
		if (pLock->m_held == false)
		{
			// So grab it.
			pLock->m_held = true;
			pLock->m_task_owning_it = KeGetRunningTask ();
			sti;
			return;
		}
		
		// Restore interrupts and let other threads run
		sti;
		KeTaskDone();
	}
}

void LockFree (SafeLock *pLock)
{
	if (pLock->m_task_owning_it == KeGetRunningTask())
	{
		// The lock is ours: free it
		pLock->m_task_owning_it =  NULL;
		pLock->m_held = false;
	}
	else
		SLogMsg("Cannot release lock %x held by task %x as task %x", pLock, pLock->m_task_owning_it, KeGetRunningTask ());
}

