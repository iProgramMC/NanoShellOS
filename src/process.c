/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Process Executive module
******************************************/
#include <process.h>
#include <time.h>
#include "mm/memoryi.h"

// note: Process ID 0 is no longer used.

SafeLock gProcessLock;

Process gProcesses[C_MAX_PROCESS];

UserHeap* MuiGetCurrentHeap();
void MuiUseHeap(UserHeap* pHeap);
bool MuiKillHeap(UserHeap* pHeap);
void MuiResetHeap();

Task* KeStartTaskExUnsafeD(TaskedFunction function, int argument, int* pErrorCodeOut, void *pProcVoid, const char* authorFile, const char* authorFunc, int authorLine);

Process* ExMakeUpAProcess()
{
	for (size_t i = 1; i != ARRAY_COUNT (gProcesses); i++)
	{
		if (!gProcesses[i].bActive)
			return &gProcesses[i];
	}
	return NULL;
}

void ReleaseCursorsBy(Process* pProc);

void ExDisposeProcess(Process *pProc)
{
	// Assert that all threads are dead
	if (pProc->nTasks != 0)
	{
		// They are not.  We need to try again
		pProc->bWillDie = false;
		return;
	}
	
	if (pProc->bAttached)
	{
		// wait until it's detached. Also, signal every thread waiting for us.
		KeUnsuspendTasksWaitingForProc(pProc);
		return;
	}
	
	if (pProc->sResourceTable.m_pResources)
	{
		MhFree(pProc->sResourceTable.m_pResources);
	}
	
	// Release the cursors we've uploaded...
	ReleaseCursorsBy(pProc);
	
	// If the current heap is this process' heap (which we hope it isn't),
	// disposing of this heap will automatically switch to the kernel heap :^)
	MuiKillHeap(pProc->pHeap);
	
	// Deactivate this process
	pProc->bActive = false;
	
	// Free the tasks array.
	MhFree(pProc->sTasks);
	
	// If there are any tasks waiting for us to terminate, unsuspend those now
	KeUnsuspendTasksWaitingForProc(pProc);
	
	// If we have a sym tab loaded:
	if (pProc->pSymTab)
	{
		MhFree(pProc->pSymTab);
		pProc->pSymTab = NULL;
		pProc->nSymTabEntries = 0;
	}
	if (pProc->pStrTab)
	{
		MhFree(pProc->pStrTab);
		pProc->pStrTab = NULL;
	}
}

void ExCheckDyingProcesses(Process *pProcToAvoid)
{
	for (size_t i = 1; i != ARRAY_COUNT (gProcesses); i++)
	{
		if (&gProcesses[i] == pProcToAvoid) continue;
		if (!gProcesses[i].bActive) continue;
		if (!gProcesses[i].bWillDie) continue;
		if (gProcesses[i].bWaitingForCrashAck) continue;
		
		ExDisposeProcess(&gProcesses[i]);
	}
}

void ExKillProcess(Process *pProc)
{
	Task* pThisTask = KeGetRunningTask();
	
	bool triedToKillSelf = false;
	
	for (int i = 0; i < pProc->nTasks; i++)
	{
		Task* pTask = pProc->sTasks[i];
		//TODO: this is a special case, if trying to kill this process
		if (pThisTask == pTask)
		{
			//Special case. Let us finish killing the other tasks first
			triedToKillSelf = true;
		}
		else
		{
			KeKillTask (pTask);
		}
	}
	
	//TODO
	if (triedToKillSelf)
	{
		// All the required processing for the death of this process is done,
		// we just need to kill this task
		KeExit();
	}
}

void ExOnThreadExit (Process* pProc, Task* pTask)
{
	// Locate the task
	for (int i = 0; i < pProc->nTasks; i++)
	{
		if (pProc->sTasks[i] == pTask)
		{
			// Remove it
			memcpy (&pProc->sTasks[i], &pProc->sTasks[i+1], sizeof (Task*) * (pProc->nTasks - i - 1));
			pProc->nTasks--;
			
			if (pProc->nTasks == 0)
			{
				// Let the process free its stuff first
				if (pProc->OnDeath)
				{
					MuiUseHeap (pProc->pHeap);
					
					pProc->OnDeath(pProc);
					pProc->OnDeath = NULL;
					
					MuiResetHeap ();
				}
				
				pProc->bWillDie = true;//it is useless now
			}
			
			return;
		}
	}
}

bool ExAddThreadToProcess(Process* pProc, Task* pTask)
{
	if (pProc->nTasks + 1 > pProc->nTaskCapacity)
	{
		int newCap = pProc->nTaskCapacity * 2;
		
		Task** pTasksNew = MhReAllocate(pProc->sTasks, newCap * sizeof(Task*));
		if (!pTasksNew)
			return false;
		
		pProc->sTasks = pTasksNew;
		pProc->nTaskCapacity = newCap;
	}
	
	pProc->sTasks[pProc->nTasks++] = pTask;
	return true;
}

Process* ExGetRunningProc()
{
	if (KeGetRunningTask())
		return (Process*)KeGetRunningTask()->m_pProcess;
	else
		return NULL;
}

Process* ExGetProcessByRID(uint64_t rid)
{
	for (size_t i = 0; i < ARRAY_COUNT(gProcesses); i++)
	{
		Process* pProc = &gProcesses[i];
		if (pProc->bActive && pProc->nIdentifier == rid)
		{
			return pProc;
		}
	}
	return NULL;
}

Process* ExCreateProcess (TaskedFunction pTaskedFunc, int nParm, const char *pIdent, int nHeapSize, int *pErrCode, void* pDetail)
{
	LockAcquire (&gProcessLock);
	KeVerifyInterruptsEnabled;
	cli;
	
	Process* pProc = ExMakeUpAProcess();
	if (!pProc)
	{
		*pErrCode = EX_PROC_TOO_MANY_PROCESSES;
		sti;
		LockFree (&gProcessLock);
		return NULL;
	}
	
	memset (pProc, 0, sizeof *pProc);
	
	// Create a new heap
	if (nHeapSize < 128)
		nHeapSize = 128;
	
	pProc->pHeap = MuCreateHeap();
	if (!pProc->pHeap)
	{
		*pErrCode = EX_PROC_CANT_MAKE_HEAP;
		LockFree (&gProcessLock);
		return NULL;
	}
	
	// Use the heap, so that it can be used in the task
	UserHeap* pBkp = MuiGetCurrentHeap();
	MuiUseHeap (pProc->pHeap);
	
	strcpy (pProc->sIdentifier, pIdent);
	
	// Setup the process structure.
	pProc->bActive  = true;
	pProc->bWillDie = false;
	pProc->bAttached = true;
	pProc->nTasks = 0;
	pProc->nTaskCapacity = 4096 / sizeof(Task*);
	pProc->sTasks = MhAllocate(4096, NULL);
	
	// Create the task itself
	Task* pTask = KeStartTaskExUnsafeD(pTaskedFunc, nParm, pErrCode, pProc, pProc->sIdentifier, "[Process]", 0);
	if (!pTask)
	{
		SLogMsg("Uh oh!");
		
		pProc->bActive = false;
		MhFree(pProc->sTasks);
		pProc->sTasks = NULL;
		pProc->nTaskCapacity = 0;
		
		sti;
		//error code was already set
		LockFree (&gProcessLock);
		return NULL;
	}
	
	pProc->nIdentifier = ReadTSC();
	pProc->pDetail   = pDetail;
	pProc->pSymTab   = pProc->pStrTab = NULL;
	pProc->nSymTabEntries = 0;
	pProc->sResourceTable.m_pResources = NULL;
	pProc->sResourceTable.m_nResources = 0;
	pProc->nWindows = 0;
	
	KeTaskAssignTag(pTask, pProc->sIdentifier);
	
	KeUnsuspendTaskUnsafe(pTask);
	KeDetachTask(pTask);
	
	MuiUseHeap (pBkp);
	
	KeVerifyInterruptsDisabled;
	sti;
	
	LockFree (&gProcessLock);
	
	return pProc;
}

void ExDetachProcess(Process* pProc)
{
	pProc->bAttached = false;
}

void WaitProcessInternal(void* pProcessToWait);
void ExJoinProcess(Process* pProc)
{
	cli;
	if (!pProc->bAttached)
	{
		sti;
		return;
	}
	
	// check if the process is a zombie yet:
	if (pProc->bWillDie)
	{
		// yeah. detach and return
		pProc->bAttached = false;
		sti;
		return;
	}
	
	// the process won't die, should wait until it does.
	WaitProcessInternal(pProc);
}

void ExSetProgramInfo(const ProgramInfo* pProgInfo)
{
	ExGetRunningProc()->pProgInfo = *pProgInfo;
	
	// dump the program info
#if 0
	
	SLogMsg("ExSetProgramInfo has the following data (%p):", pProgInfo);
	SLogMsg("Subsystem: %d", pProgInfo->m_info.m_subsystem);
	SLogMsg("Version Number: %x", pProgInfo->m_info.m_Version.Data);
	SLogMsg("App Name: %s", pProgInfo->m_info.m_AppName);
	SLogMsg("App Auth: %s", pProgInfo->m_info.m_AppAuthor);
	SLogMsg("App Copr: %s", pProgInfo->m_info.m_AppCopyright);
	SLogMsg("Pro Name: %s", pProgInfo->m_info.m_ProjName);
	
#endif
}

bool ExLoadResourceTable(void *pResourceTableData)
{
	if (!ExGetRunningProc()) return false;
	
	// so we can do byte level math with this. It'll be basic, don't worry.
	uint8_t* pResourceTableDataBytes = (uint8_t*)pResourceTableData;
	
	// Step 1. Count the resources.
	Resource* pResources = (Resource*)(pResourceTableDataBytes + 4), *pResource;
	
	int nResources = *(int*)pResourceTableDataBytes;
	
	SLogMsg("Nr of resources: %d", nResources);
	
	ResourceTable* pRT = &ExGetRunningProc()->sResourceTable;
	
	pResource = pResources;
	pRT->m_pResources = MmAllocate(sizeof(Resource**) * nResources);
	pRT->m_nResources = nResources;
	
	for (int i = 0; i < nResources; i++)
	{
		pRT->m_pResources[i] = pResource;
		
		pResource = (Resource*)((uint8_t*)(pResource) + pResource->m_size + sizeof(Resource));
	}
	
	return true;
}

Resource* ExLookUpResource(int id)
{
	if (!ExGetRunningProc()) return NULL;
	
	return RstLookUpResource(&ExGetRunningProc()->sResourceTable, id);
}

typedef struct
{
	int  nProcessID;
	char sIdentifier[250];
	int  nTasks;
	int  nWindows;
}
ProcessInfo;

void ExProcessDebugDump()
{
	ProcessInfo* pInfo = MmAllocate(sizeof(ProcessInfo) * C_MAX_PROCESS);
	int nProcesses = 0;
	
	cli;
	
	for (int i = 0; i < C_MAX_PROCESS; i++)
	{
		ProcessInfo procInfo;
		
		if (!gProcesses[i].bActive) continue;
		
		strcpy(procInfo.sIdentifier, gProcesses[i].sIdentifier);
		procInfo.nTasks     = gProcesses[i].nTasks;
		procInfo.nWindows   = gProcesses[i].nWindows;
		procInfo.nProcessID = i;
		
		pInfo[nProcesses++] = procInfo;
	}
	
	sti;
	
	LogMsg(" PID | NT | NW | ImageName");
	
	for (int i = 0; i < nProcesses; i++)
	{
		ProcessInfo* pi = pInfo + i;
		LogMsg("%4d | %2d | %2d | %s", pi->nProcessID, pi->nTasks, pi->nWindows, pi->sIdentifier);
	}
	
	MmFree(pInfo);
}
