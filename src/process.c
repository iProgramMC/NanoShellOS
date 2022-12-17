/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Process Executive module
******************************************/
#include <process.h>
#include <misc.h>
#include "mm/memoryi.h"

SafeLock gProcessLock;

Process gProcesses[64];

UserHeap* MuiGetCurrentHeap();
void MuiUseHeap(UserHeap* pHeap);
bool MuiKillHeap(UserHeap* pHeap);
void MuiResetHeap();

Task* KeStartTaskExUnsafeD(TaskedFunction function, int argument, int* pErrorCodeOut, void *pProcVoid, const char* authorFile, const char* authorFunc, int authorLine);

Process* ExMakeUpAProcess()
{
	for (size_t i = 0; i != ARRAY_COUNT (gProcesses); i++)
	{
		if (!gProcesses[i].bActive)
			return &gProcesses[i];
	}
	return NULL;
}

void ExDisposeProcess(Process *pProc)
{
	// Assert that all threads are dead
	if (pProc->nTasks != 0)
	{
		// They are not.  We need to try again
		pProc->bWillDie = false;
		return;
	}
	
	// If the current heap is this process' heap (which we hope it isn't),
	// disposing of this heap will automatically switch to the kernel heap :^)
	MuiKillHeap(pProc->pHeap);
	
	// Deactivate this process
	pProc->bActive = false;
	
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
	if (pProc->pFdTable)
	{
		MhFree(pProc->pFdTable);
		pProc->pFdTable = NULL;
	}
	if (pProc->pDdTable)
	{
		MhFree(pProc->pDdTable);
		pProc->pDdTable = NULL;
	}
}

void ExCheckDyingProcesses(Process *pProcToAvoid)
{
	for (size_t i = 0; i != ARRAY_COUNT (gProcesses); i++)
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

bool ExAddTaskToProcess(Process* pProc, Task* pTask)
{
	SLogMsg("ExAddTaskToProcess");
	if (pProc->nTasks >= (int)ARRAY_COUNT(pProc->sTasks))
		return false;
	
	pProc->sTasks[pProc->nTasks++] = pTask;
	return true;
}

//ugh..
void ExKillProcessesByFileHandle(uint32_t handle, bool bDirTable)
{
	bool bTryingToKillSelf = false;
	for (size_t i = 0; i < ARRAY_COUNT(gProcesses); i++)
	{
		Process* pProc = &gProcesses[i];
		if (!pProc->bActive) continue;
		
		uint32_t* table = bDirTable ? pProc->pDdTable : pProc->pFdTable;
		
		if (!pProc->pFdTable) continue;
		for (int j = 0; j < C_MAX_FDS_PER_TABLE; j++)
		{
			if (table[j] == handle)
			{
				if (pProc == ExGetRunningProc())
					bTryingToKillSelf = false;
				else
					ExKillProcess(pProc);
			}
		}
	}
	
	//make sure to kill ourself last
	if (bTryingToKillSelf)
	{
		ExKillProcess(ExGetRunningProc());
	}
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
	
	// Create the task itself
	Task* pTask = KeStartTaskExUnsafeD(pTaskedFunc, nParm, pErrCode, pProc, pProc->sIdentifier, "[Process]", 0);
	if (!pTask)
	{
		SLogMsg("Uh oh!");
		sti;
		//error code was already set
		LockFree (&gProcessLock);
		return NULL;
	}
	
	// Setup the process structure.
	pProc->bActive  = true;
	pProc->bWillDie = false;
	pProc->nIdentifier = ReadTSC();
	pProc->pDetail   = pDetail;
	pProc->pSymTab   = pProc->pStrTab = NULL;
	pProc->nSymTabEntries = 0;
	pProc->pFdTable = MhAllocate(sizeof(uint32_t) * C_MAX_FDS_PER_TABLE, NULL);
	pProc->pDdTable = MhAllocate(sizeof(uint32_t) * C_MAX_FDS_PER_TABLE, NULL);
	memset(pProc->pFdTable, 0xFF, sizeof(uint32_t) * C_MAX_FDS_PER_TABLE);
	memset(pProc->pDdTable, 0xFF, sizeof(uint32_t) * C_MAX_FDS_PER_TABLE);
	
	KeUnsuspendTaskUnsafe(pTask);
	
	MuiUseHeap (pBkp);
	
	KeVerifyInterruptsDisabled;
	sti;
	
	LockFree (&gProcessLock);
	
	return pProc;
}

