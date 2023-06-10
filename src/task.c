/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

          Task Scheduler  module
******************************************/
#include <task.h>
#include <process.h>
#include <string.h>
#include <print.h>
#include <time.h>
#include <vfs.h>

// The kernel task is task 0.  Other tasks are 1-indexed.
// This means g_runningTasks[0] is unused.

__attribute__((aligned(16)))
Task g_runningTasks[C_MAX_TASKS];

// Let it be accessible from sysmon
uint64_t g_kernelCpuTimeTotal;

extern bool g_interruptsAvailable;

static int s_lastRunningTaskIndex = 1;

static int s_currentRunningTask = -1;
static CPUSaveState g_kernelSaveState;

static Process*       g_pProcess = NULL;

static uint64_t       g_kernelLastSwitchTime;
__attribute__((aligned(16)))
static int            g_kernelFPUState[128];
static VBEData*       g_kernelVBEContext = NULL;
static UserHeap*      g_kernelHeapContext = NULL;
static Console*       g_kernelConsoleContext = NULL;
static void*          g_kernelFontContext = NULL;
static uint32_t       g_kernelFontIDContext = 0;
static char           g_kernelCwd[PATH_MAX+2];
static void*          g_kernelCwdNode;
static uint32_t       g_kernelSysCallNum; //honestly, kind of useless since the kernel task will never be an ELF trying to call into the system :^)

extern UserHeap*      g_pCurrentUserHeap;
extern Console*       g_currentConsole; //logmsg
extern void*          g_pCwdNode;
extern void*          g_pCurrentFont;
extern uint32_t       g_nCurrentFontID;
extern char           g_cwdStr[PATH_MAX+2];

extern bool           g_interruptsAvailable;

uint64_t g_onePitIntAgo, g_twoPitIntsAgo;

static ThreadStats s_kernelThreadStats;

void MuiUseHeap(UserHeap* pHeap);
void MuiResetHeap(void);

SAI void KeReviveTask (Task *pTask)
{
	pTask->m_bSuspended     = false;
	pTask->m_suspensionType = SUSPENSION_NONE;
}

void KeUnsuspendTaskUnsafe(Task* pTask)
{
	KeReviveTask(pTask);
}

void KeUnsuspendTask(Task* pTask)
{
	KeVerifyInterruptsEnabled;
	cli;
	KeReviveTask(pTask);
	sti;
}

bool KeKillThreadByPID (int proc)
{
	if (proc < 0 || proc >= (int)ARRAY_COUNT (g_runningTasks)) return false;
	
	if (!g_runningTasks[proc].m_bExists) return false;
	
	return KeKillTask(&g_runningTasks[proc]);
}

// stupid hack... but I think that's what other OSes do (issue a SIGTERM 
// or SIGHUP to the running process using this terminal)
void KeKillThreadsByConsole(Console *pConsole)
{
	for (int i = C_MAX_TASKS - 1; i > 0; i--)
	{
		if (!g_runningTasks[i].m_bExists) continue;
		
		if (s_currentRunningTask != i && g_runningTasks[i].m_pConsoleContext == pConsole)
			KeKillTask(&g_runningTasks[i]);
	}
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

Task* KeGetThreadByRID(uint64_t rid)
{
	for (int i = C_MAX_TASKS - 1; i > 0; i--)
	{
		if (g_runningTasks[i].m_bExists && g_runningTasks[i].m_nIdentifier == rid)
			return &g_runningTasks[i];
	}
	return NULL;
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
	KeVerifyInterruptsEnabled;
	cli;
	bool any_tasks = false;
	ILogMsg("Listing tasks.");
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		Task* t = g_runningTasks + i;
		if (!t->m_bExists) continue;
		any_tasks = true;
		ILogMsg("- %d  F:%x  AL:%d AF:%s AS:%s", i, t->m_pFunction, t->m_authorLine, t->m_authorFunc, t->m_authorFile);
	}
	if (!any_tasks)
		ILogMsg("No tasks currently running.");
	
	KeVerifyInterruptsDisabled;
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

extern VBEData* g_vbeData, g_mainScreenVBEData;
void KeFxSave(int *fpstate);

uint32_t* MhGetKernelPageDirectory();

void KeConstructTask (Task* pTask)
{
	KeVerifyInterruptsDisabled;
	
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
	
	if (MuGetCurrentHeap())
	{
		pTask->m_state.cr3 = MuGetCurrentHeap()->m_nPageDirectory; //same as our own CR3 register
	}
	else
	{
		pTask->m_state.cr3 = (uintptr_t)MhGetKernelPageDirectory() - KERNEL_BASE_ADDRESS;
	}
	
	ZeroMemory (pTask->m_fpuState, sizeof(pTask->m_fpuState));
	
	//clear the stack
	ZeroMemory (pTask->m_pStack, C_STACK_BYTES_PER_TASK);
	
	// push the iretd worthy registers on the stack:
	pTask->m_state.esp -= sizeof(int) * 5;
	memcpy ((void*)(pTask->m_state.esp), &pTask->m_state.eip, sizeof(int)*3);
	
	pTask->m_pVBEContext     = &g_mainScreenVBEData;
	pTask->m_pCurrentHeap    = g_pCurrentUserHeap;//default kernel heap.
	pTask->m_pConsoleContext = g_currentConsole;
	pTask->m_pFontContext    = g_pCurrentFont;
	strcpy (pTask->m_cwd, "");
	pTask->m_cwdNode         = g_pCwdNode;
	
	// Add another reference to the CWD node:
	FsAddReference(g_pCwdNode);
	
	memset   (pTask->m_fpuState, 0, sizeof (pTask->m_fpuState));
	KeFxSave (pTask->m_fpuState);
}

void ExOnThreadExit (Process* pProc, Task* pTask);

Task* KeStartTaskExUnsafeD(TaskedFunction function, int argument, int* pErrorCodeOut, void *pProcVoid, const char* authorFile, const char* authorFunc, int authorLine)
{
	Process *pProc = (Process*)pProcVoid;
	// Pre-allocate the stack, since it depends on interrupts being on
	void *pStack = MmAllocateID(C_STACK_BYTES_PER_TASK);
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
			MmFreeID(pStack);
		
		return NULL;
	}
	
	if (pStack)
	{
		//Setup our new task here:
		Task* pTask = &g_runningTasks[i];
		memset (pTask, 0, sizeof *pTask);
		
		// This replacement is purely symbolic: while it won't use the
		// page directory of this heap, it will apply it to the new task.
		if (pProc)
		{
			if (!ExAddThreadToProcess(pProc, pTask))
			{
				*pErrorCodeOut = TASK_ERROR_CANT_ADD_TO_PROC;
				
				MmFreeID(pStack);
				
				return NULL;
			}
		}
		
		pTask->m_pFunction  = function;
		pTask->m_bAttached  = true;
		pTask->m_pStack     = pStack;
		pTask->m_bFirstTime = true;
		pTask->m_authorFile = authorFile;
		pTask->m_authorFunc = authorFunc;
		pTask->m_authorLine = authorLine;
		pTask->m_argument   = argument;
		pTask->m_bMarkedForDeletion = false;
		pTask->m_pProcess = pProc;
		pTask->m_nIdentifier = ReadTSC();
		
		// Task is suspended by default. Use KeUnsuspendTask to unsuspend a task.
		pTask->m_suspensionType = SUSPENSION_TOTAL;
		pTask->m_bSuspended     = true;
		
		// This replacement is purely symbolic: while it won't use the
		// page directory of this heap, it will apply it to the new task.
		if (pProc)
		{
			MuiUseHeap(pProc->pHeap);
		}
		
		UserHeap* pBkp = MuGetCurrentHeap();
		
		char buffer[32];
		sprintf(buffer, "<task no. %d>", i);
		
		KeConstructTask(pTask);
		
		// Restore Old Heap
		MuiUseHeap(pBkp);
		
		if (pErrorCodeOut)
			*pErrorCodeOut = TASK_SUCCESS;
		
		pTask->m_bExists = true;
		
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
	KeVerifyInterruptsEnabled;
	cli; //must do this, because otherwise we can expect an interrupt to come in and load our unfinished structure
	Task* pResult = KeStartTaskExUnsafeD (function, argument, pErrorCodeOut, pProcVoid, authorFile, authorFunc, authorLine);
	sti;
	return pResult;
}
Task* KeStartTaskD(TaskedFunction function, int argument, int* pErrorCodeOut, const char* authorFile, const char* authorFunc, int authorLine)
{
	return KeStartTaskExD(function, argument, pErrorCodeOut, ExGetRunningProc(), authorFile, authorFunc, authorLine);
}

// TODO: A lot of the code is the same. Maybe we should not repeat ourselves.
void KeUnsuspendTasksWaitingForProc(void *pProc)
{
	// let everyone know that this process is gone
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		register Task *pCurTask = &g_runningTasks[i];
		if (!pCurTask->m_bExists) continue;
		
		if (!pCurTask->m_bSuspended) continue;
		if (pCurTask->m_suspensionType != SUSPENSION_UNTIL_PROCESS_EXPIRY) continue;
		
		if (pCurTask->m_pWaitedTaskOrProcess == pProc)
		{
			// Unsuspend this process, they're done waiting!
			KeReviveTask(pCurTask);
		}
	}
}

// TODO: A lot of the code is the same. Maybe we should not repeat ourselves.
void KeUnsuspendTasksWaitingForObject(void *pProc)
{
	// let everyone know that this process is gone
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		register Task *pCurTask = &g_runningTasks[i];
		if (!pCurTask->m_bExists) continue;
		
		if (!pCurTask->m_bSuspended) continue;
		if (pCurTask->m_suspensionType != SUSPENSION_UNTIL_OBJECT_EVENT) continue;
		
		if (pCurTask->m_pWaitedTaskOrProcess == pProc)
		{
			// Unsuspend this process, they're done waiting!
			KeReviveTask(pCurTask);
		}
	}
}

void KeUnsuspendTasksWaitingForPipeWrite(void *pProc)
{
	// let everyone know that this process is gone
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		register Task *pCurTask = &g_runningTasks[i];
		if (!pCurTask->m_bExists) continue;
		
		if (!pCurTask->m_bSuspended) continue;
		if (pCurTask->m_suspensionType != SUSPENSION_UNTIL_PIPE_WRITE) continue;
		
		if (pCurTask->m_pPipeWaitingToWrite == pProc)
		{
			// Unsuspend this process, they're done waiting!
			KeReviveTask(pCurTask);
		}
	}
}

void KeUnsuspendTasksWaitingForPipeRead(void *pProc)
{
	// let everyone know that this process is gone
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		register Task *pCurTask = &g_runningTasks[i];
		if (!pCurTask->m_bExists) continue;
		
		if (!pCurTask->m_bSuspended) continue;
		if (pCurTask->m_suspensionType != SUSPENSION_UNTIL_PIPE_READ) continue;
		
		if (pCurTask->m_pPipeWaitingToRead == pProc)
		{
			// Unsuspend this process, they're done waiting!
			KeReviveTask(pCurTask);
		}
	}
}

void KeUnsuspendTasksWaitingForWM()
{
	// let everyone know that this process is gone
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		register Task *pCurTask = &g_runningTasks[i];
		if (!pCurTask->m_bExists) continue;
		
		if (!pCurTask->m_bSuspended) continue;
		if (pCurTask->m_suspensionType != SUSPENSION_UNTIL_WM_UPDATE) continue;
		
		// Unsuspend this process, they're done waiting!
		KeReviveTask(pCurTask);
	}
}

void WmOnTaskDied(Task *pTask);

void KeDetachTask(Task* pTask)
{
	pTask->m_bAttached = false;
}

void KeResetTask_ReleaseFileNode(void* parm)
{
	if (!parm)
		// likely we're at init, just return
		return;
	
	FileNode* pFN = parm;
	FsReleaseReference(pFN);
}

static void KeResetTask(Task* pTask, bool killing, bool interrupt)
{
	if (!interrupt)
	{
		KeVerifyInterruptsEnabled;
		cli; //must do this, because otherwise we can expect an interrupt to come in and load our unfinished structure
	}
	else
	{
		KeVerifyInterruptsDisabled;
	}
	
	WmOnTaskDied(pTask);
	
	// let everyone know that this task is gone
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		register Task *pCurTask = &g_runningTasks[i];
		if (!pCurTask->m_bExists) continue;
		
		if (!pCurTask->m_bSuspended) continue;
		if (pCurTask->m_suspensionType != SUSPENSION_UNTIL_TASK_EXPIRY) continue;
		
		if (pCurTask->m_pWaitedTaskOrProcess == pTask)
		{
			// Unsuspend this process, they're done waiting!
			KeReviveTask(pCurTask);
		}
	}
	
	if (pTask == KeGetRunningTask())
	{
		pTask->m_bMarkedForDeletion = true;
		if (!interrupt)
		{
			ILogMsg("KEResetTask: WTF?");
			SLogMsg("KEResetTask: WTF?");
			KeStopSystem ();
		}
		KeVerifyInterruptsDisabled;
		sti;//if we didn't restore interrupts here would be our death point
		KeTaskDone();
		while (1) hlt;
	}
	else
	{
		if (killing && pTask->m_pStack)
		{
			MmFreeID(pTask->m_pStack);
		}
		pTask->m_pStack = NULL;
		
		pTask->m_bFirstTime = false;
		pTask->m_pFunction  = NULL;
		pTask->m_authorFile = NULL;
		pTask->m_authorFunc = NULL;
		pTask->m_authorLine = 0;
		pTask->m_reviveAt   = 0;
		pTask->m_argument   = 0;
		pTask->m_featuresArgs = false;
		pTask->m_bMarkedForDeletion = false;
		
		// release the reference to our CWD soon:
		KeAddDeferredCall(KeResetTask_ReleaseFileNode, pTask->m_cwdNode);
		pTask->m_cwdNode = NULL;
		
		if (pTask->m_bAttached)
		{
			// suspend it as SUSPENSION_ZOMBIE:
			SLogMsg("Thread %d is now a zombie.", pTask - g_runningTasks);
			pTask->m_bSuspended     = true;
			pTask->m_suspensionType = SUSPENSION_ZOMBIE;
		}
		else
		{
			// simply get rid of it.
			pTask->m_bExists = false;
		}
		
		if (!interrupt)
		{
			KeVerifyInterruptsDisabled;
			sti;
		}
	}
}

void WmOnTaskCrashed(Task *pTask);

bool KeKillTask(Task* pTask)
{
	if (!pTask) return false;
	
	if (pTask == KeGetRunningTask())
		KeExit();
	
	if (pTask->m_pProcess)
		ExOnThreadExit ((Process*)pTask->m_pProcess, pTask);
	
	if (!pTask->m_bExists)
		return false;
	
	// Close the file resources opened by this task.
	FiReleaseResourcesFromTask(pTask);
	
	// Close the windows that have been opened by this task.
	WmOnTaskCrashed(pTask);
	
	KeVerifyInterruptsEnabled;
	cli;
	if (!pTask->m_bExists)
	{
		sti; return false;
	}
	
	KeResetTask(pTask, true, true);
	
	KeVerifyInterruptsDisabled;
	
	sti;
	return true;
}

Task* KeGetRunningTask()
{
	if (s_currentRunningTask == -1) return NULL;
	return &g_runningTasks[s_currentRunningTask];
}

void KiTaskSystemInit()
{
	for (int i = 0; i < C_MAX_TASKS; i++)
		KeResetTask(g_runningTasks + i, false, true);
}

CPUSaveState* g_saveStateToRestore1 = NULL;
extern void KeStartedNewKernelTask();
extern void KeStartedNewTask();
extern void KeOnExitInterrupt();
void KeRestoreKernelTask()
{
	KeOnExitInterrupt();
	g_saveStateToRestore1 = &g_kernelSaveState;
	KeStartedNewKernelTask();
}
void KeRestoreStandardTask(Task* pTask)
{
	KeOnExitInterrupt();
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
		ILogMsg("Stopping system!?");
		KeStopSystem();
	}
	
	Task* pTask = KeGetRunningTask();
	
	// Close the file resources opened by this task.
	FiReleaseResourcesFromTask(pTask);
	
	// Close the windows that have been opened by this task.
	WmOnTaskCrashed(pTask);
	
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

void KeCheckDyingTasks(Task* pTaskToAvoid)
{
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		//if it's a zombie, and we're not switching away from it (i.e. using its resources)...
		if (g_runningTasks[i].m_bMarkedForDeletion && &g_runningTasks[i] != pTaskToAvoid)
			//end it. end it good.
			KeResetTask(&g_runningTasks[i], true, true);
	}
}	

// Every 10 milliseconds the task switches continue
///int g_TaskSwitchingAggressiveness = 10;

void ExCheckDyingProcesses();
Process* ExGetRunningProc();

uint32_t* const pSysCallNum = (uint32_t*)0xC0007CFC;//for easy reading; the pointer itself is constant

void WaitTask (Task* pTaskToWait)
{
	if (!pTaskToWait) return;
	Task *pTask = KeGetRunningTask();
	
	if (!pTask->m_bAttached)
	{
		SLogMsg("ERROR: WaitTask can't wait for a detached thread.");
		return;
	}
	
	// check if it's a zombie right now:
	cli;
	
	if (pTask->m_bSuspended && pTask->m_suspensionType == SUSPENSION_ZOMBIE)
	{
		pTask->m_bAttached = false;
		KeResetTask(pTask, true, true);
	}
	
	sti;
	
	if (pTask == pTaskToWait)
	{
		// No point in waiting like this.
		
		// TODO: Kill the task instead. There's no point in running it further if it
		// waits for itself, consumes a task spot other tasks could occupy for no reason
		return;
	}
	
	pTask->m_pWaitedTaskOrProcess = pTaskToWait;
	pTask->m_suspensionType       = SUSPENSION_UNTIL_TASK_EXPIRY;
	pTask->m_bSuspended           = true;
	
	while (pTask->m_bSuspended) KeTaskDone();
}

void WaitPipeWrite (void* pPipe)
{
	Task *pTask = KeGetRunningTask();
	
	pTask->m_pPipeWaitingToWrite  = pPipe;
	pTask->m_suspensionType       = SUSPENSION_UNTIL_PIPE_WRITE;
	pTask->m_bSuspended           = true;
	
	while (pTask->m_bSuspended) KeTaskDone();
}

void WaitPipeRead (void* pPipe)
{
	Task *pTask = KeGetRunningTask();
	
	pTask->m_pPipeWaitingToRead   = pPipe;
	pTask->m_suspensionType       = SUSPENSION_UNTIL_PIPE_READ;
	pTask->m_bSuspended           = true;
	
	while (pTask->m_bSuspended) KeTaskDone();
}

void WaitProcessInternal(void* pProcessToWait)
{
	Task *pTask = KeGetRunningTask();
	
	pTask->m_pWaitedTaskOrProcess = pProcessToWait;
	pTask->m_suspensionType       = SUSPENSION_UNTIL_PROCESS_EXPIRY;
	pTask->m_bSuspended           = true;
	
	sti;
	
	while (pTask->m_bSuspended) KeTaskDone();
}

void WaitProcess (void* pProc)
{
	ExJoinProcess((Process*)pProc);
}

void WaitObject(void* pObject)
{
	Task *pTask = KeGetRunningTask();
	pTask->m_suspensionType       = SUSPENSION_UNTIL_OBJECT_EVENT;
	pTask->m_pWaitedTaskOrProcess = pObject;
	pTask->m_bSuspended           = true;
	
	if (KeCheckInterruptsDisabled())
		sti;
	
	while (pTask->m_bSuspended) KeTaskDone();
}

void WaitMS (int ms)
{
	if (ms <= 0) return;
	int tickCountToStop = GetTickCount() + ms;
	
	Task* pTask = KeGetRunningTask();
	if (pTask)
	{
		pTask->m_suspensionType = SUSPENSION_UNTIL_TIMER_EXPIRY;
		pTask->m_bSuspended     = true;
		pTask->m_reviveAt       = tickCountToStop;
	}
	while (GetTickCount() < tickCountToStop)
	{
		KeTaskDone();
	}
}

void WaitUntilWMUpdate()
{
	Task *pTask = KeGetRunningTask();
	
	//TODO: What if the Window Manager calls this?
	
	pTask->m_suspensionType       = SUSPENSION_UNTIL_WM_UPDATE;
	pTask->m_bSuspended           = true;
	
	while (pTask->m_bSuspended) KeTaskDone();
}

SAI bool IsTaskSuspended(Task *pTask, int tick_count)
{
	switch (pTask->m_suspensionType)
	{
		case SUSPENSION_NONE: // No Suspension
			return false;
		case SUSPENSION_UNTIL_TIMER_EXPIRY:
			if (tick_count > pTask->m_reviveAt)
			{
				KeReviveTask(pTask);
				return false;
			}
			return true;
		default:
			if (!pTask->m_bSuspended)
			{
				KeReviveTask(pTask);
				return false;
			}
			return true;
	}
}

SAI int GetNextTask()
{
	int g_tick_count = GetTickCountUnsafe();
	
	int i = s_currentRunningTask + 1;
	int task = s_lastRunningTaskIndex;
	
	// look for a task we can switch to...
	for (; i < task; i++)
	{
		Task* p = &g_runningTasks[i];
		if (p->m_bExists)
		{
			// Is the task scheduled to come back online?
			if (!IsTaskSuspended (p, g_tick_count))
				return i;
		}
	}
	
	// haven't found any tasks there, back to the beginning...
	for (i = 0; i < s_currentRunningTask; i++)
	{
		Task* p = &g_runningTasks[i];
		if (p->m_bExists)
		{
			// Is the task scheduled to come back online?
			if (!IsTaskSuspended (p, g_tick_count))
				return i;
		}
	}
	
	// resort to running an idle task
	return -1;
}

// Saves the internal context of the currently running thread. This can be stuff such as
// the VBE context, console context, font context, system call number, FX registers etc.
void KeSaveTaskInternalContext(CPUSaveState* pSaveState)
{
	Task* pTask = KeGetRunningTask();
	//Please note that tasking code does not use the FPU, so we should be safe just saving it here.
	if (pTask)
	{
		memcpy (& pTask -> m_state, pSaveState, sizeof(CPUSaveState));
		memcpy   (pTask -> m_cwd,   g_cwdStr,   sizeof(g_cwdStr));
		KeFxSave (pTask -> m_fpuState);
		pTask->m_pVBEContext     = g_vbeData;
		pTask->m_pCurrentHeap    = g_pCurrentUserHeap;
		pTask->m_pConsoleContext = g_currentConsole;
		pTask->m_pFontContext    = g_pCurrentFont;
		pTask->m_pFontIDContext  = g_nCurrentFontID;
		pTask->m_sysCallNum      = *pSysCallNum;
		pTask->m_cwdNode         = g_pCwdNode;
	}
	else
	{
		memcpy (&g_kernelSaveState, pSaveState, sizeof(CPUSaveState));
		memcpy   (g_kernelCwd,      g_cwdStr,   sizeof(g_cwdStr));
		KeFxSave (g_kernelFPUState); //perhaps we won't use this.
		g_kernelVBEContext     = g_vbeData;
		g_kernelHeapContext    = g_pCurrentUserHeap;
		g_kernelConsoleContext = g_currentConsole;
		g_kernelFontContext    = g_pCurrentFont;
		g_kernelFontIDContext  = g_nCurrentFontID;
		g_kernelSysCallNum     = *pSysCallNum;
		g_kernelCwdNode        = g_pCwdNode;
	}
	
	// Revert to a standard context.
	MuiResetHeap();
	g_currentConsole = &g_debugConsole;
	g_vbeData        = &g_mainScreenVBEData;
}

void KeRestoreTaskInternalContext()
{
	// note: this may have changed since the KeSaveTaskInternalContext within the function
	Task* pNewTask = KeGetRunningTask();
	
	if (pNewTask)
	{
		KeFxRestore(pNewTask->m_fpuState);
		memcpy (g_cwdStr, pNewTask->m_cwd, sizeof (g_cwdStr));
		g_pCwdNode       = pNewTask->m_cwdNode;
		g_vbeData        = pNewTask->m_pVBEContext;
		g_currentConsole = pNewTask->m_pConsoleContext;
		g_pCurrentFont   = pNewTask->m_pFontContext;
		g_nCurrentFontID = pNewTask->m_pFontIDContext;
		*pSysCallNum     = pNewTask->m_sysCallNum;
		g_pProcess = (Process*)pNewTask->m_pProcess;
		MuiUseHeap (pNewTask->m_pCurrentHeap);
	}
	else
	{
		KeFxRestore(g_kernelFPUState);
		memcpy (g_cwdStr, g_kernelCwd, sizeof (g_cwdStr));
		g_pCwdNode       = g_kernelCwdNode;
		g_vbeData        = g_kernelVBEContext;
		g_currentConsole = g_kernelConsoleContext;
		g_pCurrentFont   = g_kernelFontContext;
		g_nCurrentFontID = g_kernelFontIDContext;
		*pSysCallNum     = g_kernelSysCallNum;
		g_pProcess = NULL;
		MuiUseHeap (g_kernelHeapContext);
	}
}

void ExCheckDyingProcesses(void* pProcToAvoid);

void KeSwitchTask(bool bCameFromPIT, CPUSaveState* pSaveState)
{
	register uint64_t tsc = ReadTSC();
	if (bCameFromPIT)
	{
		g_twoPitIntsAgo = g_onePitIntAgo;
		g_onePitIntAgo  = tsc;
	}
	g_pProcess = NULL;
	
	KeSaveTaskInternalContext(pSaveState);
	
	Task* pTask = KeGetRunningTask();
	
	void *pProc = NULL;
	if (pTask) pProc = pTask->m_pProcess;
	
	KeCheckDyingTasks(pTask);
	ExCheckDyingProcesses(pProc);
	
	Task* pNewTask = NULL;
	
	int i = GetNextTask();
	
	if (i > 0)
	{
		pNewTask = g_runningTasks + i;
		s_currentRunningTask = i;
	}
	else
	{
		//kernel task will always come back no matter what
		s_currentRunningTask = -1;
	}
	
	if (pTask)
	{
		// Before switching, log the amount of time passed
		uint64_t totalTime = tsc - pTask->m_lastSwitchTime;
		pTask->m_cpuTimeTotal += totalTime;
	}
	else
	{
		uint64_t totalTime = tsc - g_kernelLastSwitchTime;
		g_kernelCpuTimeTotal += totalTime;
	}
	
	KeRestoreTaskInternalContext();
	
	if (pNewTask)
	{
		pNewTask->m_lastSwitchTime = tsc;
		KeRestoreStandardTask(pNewTask);
	}
	else
	{
		g_kernelLastSwitchTime = tsc;
		KeRestoreKernelTask();
	}
}

void LockAcquire (SafeLock *pLock) // An attempt at a safe lock
{
	int tries = 0;
	while (true)
	{
		// Clear interrupts: we need the following to be atomic
		KeVerifyInterruptsEnabled;
		cli;
		
		// If the lock's value is false (i.e. it has been freed) then we can grab it.
		if (pLock->m_held == false)
		{
			// So grab it.
			pLock->m_held = true;
			pLock->m_task_owning_it = KeGetRunningTask ();
			pLock->m_return_addr    = __builtin_return_address(0);
			KeVerifyInterruptsDisabled;
			sti;
			return;
		}
		
		tries++;
		
		if (tries == 10000)
		{
			SLogMsg("Tried to acquire a lock 10000 times. Is something wrong?  Task Owner: %p", pLock->m_return_addr);
			PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
		}
		
		// Restore interrupts and let other threads run
		KeVerifyInterruptsDisabled;
		sti;
		KeTaskDone();
	}
}

void LockFree (SafeLock *pLock)
{
	if (pLock->m_task_owning_it == KeGetRunningTask())
	{
		// The lock is ours: free it
		KeVerifyInterruptsEnabled;
		cli;
		pLock->m_task_owning_it =  NULL;
		pLock->m_held = false;
		sti;
	}
	else
	{
		SLogMsg("Cannot release lock %x held by task %x as task %x", pLock, pLock->m_task_owning_it, KeGetRunningTask ());
		PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
	}
}

void KeTaskTest()
{
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		Task *pTask = &g_runningTasks[i];
		if (!pTask->m_bExists) continue;
		
		//to get a rough idea of what a task is doing
		SLogMsg("TASK %p  EIP:%p  ESP:%p  %s:%d", pTask, pTask->m_state.eip, pTask->m_state.esp, pTask->m_authorFile, pTask->m_authorLine);
	}
}

ThreadStats* KeGetTaskStats(Task* task)
{
	if (!task)
		return &s_kernelThreadStats;
	
	return &task->m_threadStats;
}

ThreadStats* KeGetThreadStats()
{
	return KeGetTaskStats(KeGetRunningTask());
}
