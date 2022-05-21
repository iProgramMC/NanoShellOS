/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Process Executive module
******************************************/
#include <process.h>

SafeLock gProcessLock;

Process gProcesses[64];

void UseHeapUnsafe  (Heap* pHeap);
void FreeHeapUnsafe (Heap* pHeap);
void ResetToKernelHeapUnsafe();

Task* KeStartTaskExUnsafeD(TaskedFunction function, int argument, int* pErrorCodeOut, void *pProcVoid, const char* authorFile, const char* authorFunc, int authorLine);
bool AllocateHeapUnsafeD (Heap* pHeap, int size, const char* callerFile, int callerLine);

Process* ExMakeUpAProcess()
{
	for (size_t i = 0; i != ARRAY_COUNT (gProcesses); i++)
	{
		if (!gProcesses[i].bActive)
			return &gProcesses[i];
	}
	return NULL;
}

extern Heap *g_pHeap;

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
	FreeHeapUnsafe (&pProc->sHeap);
	
	// Deactivate this process
	pProc->bActive = false;
	
	// If there are any tasks waiting for us to terminate, unsuspend those now
	KeUnsuspendTasksWaitingForProc(pProc);
}

void ExCheckDyingProcesses(Process *pProcToAvoid)
{
	for (size_t i = 0; i != ARRAY_COUNT (gProcesses); i++)
	{
		if (&gProcesses[i] == pProcToAvoid) continue;
		if (!gProcesses[i].bActive) continue;
		if (!gProcesses[i].bWillDie) continue;
		
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
					UseHeap (&pProc->sHeap);
					
					pProc->OnDeath(pProc);
					pProc->OnDeath = NULL;
					
					ResetToKernelHeap ();
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

Process* ExCreateProcess (TaskedFunction pTaskedFunc, int nParm, const char *pIdent, int nHeapSize, int *pErrCode)
{
	LockAcquire (&gProcessLock);
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
	
	if (!AllocateHeapUnsafeD (&pProc->sHeap, nHeapSize, pIdent, 10000))
	{
		*pErrCode = EX_PROC_CANT_MAKE_HEAP;
		LockFree (&gProcessLock);
		return NULL;
	}
	
	// Use the heap, so that it can be used in the task
	Heap* pBkp = g_pHeap;
	UseHeapUnsafe (&pProc->sHeap);
	
	strcpy (pProc->sIdentifier, pIdent);
	
	// Create the task itself
	Task* pTask = KeStartTaskExUnsafeD(pTaskedFunc, nParm, pErrCode, pProc, pProc->sIdentifier, "[Process]", 0);
	if (!pTask)
	{
		SLogMsg("crap");
		sti;
		//error code was already set
		LockFree (&gProcessLock);
		return NULL;
	}
	
	// Setup the process structure.
	pProc->bActive  = true;
	pProc->bWillDie = false;
	pProc->nTasks = 1;
	pProc->sTasks[0] = pTask;
	
	UseHeapUnsafe (pBkp);
	
	LockFree (&gProcessLock);
	sti;
	return pProc;
}

