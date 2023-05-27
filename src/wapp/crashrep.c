/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Problem Reporting applet
******************************************/
#include <idt.h>
#include <elf.h>
#include <icon.h>
#include <print.h>
#include <string.h>
#include <window.h>
#include <widget.h>
#include "../mm/memoryi.h"

extern bool g_windowManagerRunning;
CrashInfo *g_AEEQueueFirst, *g_AEEQueueLast;

int g_CrashInfoKey;

// This "key" is used in WaitObject to let the task handler know that we need
// to wait for a crash.

int* CrashInfoGetKey()
{
	return &g_CrashInfoKey;
}

void CrashInfoAdd(CrashInfo* pInfo)
{
	CrashInfo* pMem = MhAllocate(sizeof *pMem, NULL);
	*pMem = *pInfo;
	pMem->m_next = NULL;
	
	// append to end
	if (g_AEEQueueLast)
		g_AEEQueueLast->m_next = pMem;
	
	g_AEEQueueLast = pMem;
	
	if (!g_AEEQueueFirst)
		g_AEEQueueFirst = pMem;
	
	KeUnsuspendTasksWaitingForObject(CrashInfoGetKey());
}

CrashInfo* CrashInfoPop()
{
	CrashInfo *pPopped = g_AEEQueueFirst;
	
	if (!pPopped)
		return pPopped;
	
	g_AEEQueueFirst = g_AEEQueueFirst->m_next;
	if (!g_AEEQueueFirst)
		g_AEEQueueLast = NULL;
	
	return pPopped;
}

void CALLBACK CrashReportWndProc( Window* pWindow, int messageType, int parm1, int parm2 )
{
	switch (messageType)
	{
		case EVENT_COMMAND:
			if (parm1 == 3)
			{
				DestroyWindow (pWindow);
			}
			break;
		default:
			DefaultWindowProc ( pWindow, messageType, parm1, parm2 );
			break;
	}
}

const char* GetBugCheckReasonText(BugCheckReason reason);

void CrashReporterFinalize(CrashInfo* pCrashInfo);

//The argument is assumed to be a pointer to a Registers* structure.  It gets freed automatically by the task.
void CrashReportWindow(CrashInfo* pCrashInfo)
{
	char string [8192];
	string[0] = 0;
	DumpRegistersToString (string, &pCrashInfo->m_regs);
	
	Process *p = (Process*)pCrashInfo->m_pTaskKilled->m_pProcess;
	
	char otherString[512], 
	     //anotherString[4096], 
	     smallerString[512];
	sprintf(
		otherString,
		"This task has performed an illegal operation and will be shut down.\n\n"
		"A restart of the computer is strongly recommended. Save your work and restart.\n\n"
	);
	
	sprintf(string + strlen (string), "\nError %x (%s)\nStack Trace:", pCrashInfo->m_nErrorCode, GetBugCheckReasonText (pCrashInfo->m_nErrorCode));
	
	int index = 0;
	while (pCrashInfo->m_stackTrace[index] != 0)
	{
		ElfSymbol * pSym = ExLookUpSymbol (p, pCrashInfo->m_stackTrace[index]);
		if (pSym)
			snprintf(smallerString, sizeof smallerString, "\n- 0x%x\t%s", pCrashInfo->m_stackTrace[index], pSym->m_pName);
		else
			snprintf(smallerString, sizeof smallerString, "\n- 0x%x", pCrashInfo->m_stackTrace[index]);
		strcat (string, smallerString);
		index++;
	}
	
	int winwidth = 500, winheight = 260;
	
	char winTitle[500];
	sprintf(winTitle, "Application Execution Error - %s", pCrashInfo->m_tag);
	
	Window* pWindow = CreateWindow (
		winTitle, 
		(GetScreenWidth()  - winwidth)  / 2,
		(GetScreenHeight() - winheight) / 2,
		winwidth,
		winheight,
		CrashReportWndProc,
		WF_NOMINIMZ
	);
	
	CrashReporterFinalize(pCrashInfo);
	
	MmFree (pCrashInfo);
	
	if (!pWindow)
	{
		ILogMsg("Could not create crash report window!");
		ILogMsg("Some task crashed, here is its register dump:");
		ILogMsg("%s", string);
		return;
	}
	
	pWindow->m_iconID = ICON_NANOSHELL_LETTERS;
	
	//add some controls to the window before handing control
	Rectangle r;
	RECT(r, 10, 10, 32, 32);
	AddControl (pWindow, CONTROL_ICON, r, NULL, 1, ICON_AMBULANCE, 0);
	
	RECT(r, 50, 10, winwidth - 50, 100);
	AddControl (pWindow, CONTROL_TEXTHUGE, r, NULL, 2, WINDOW_TEXT_COLOR, TEXTSTYLE_WORDWRAPPED);
	SetHugeLabelText(pWindow, 2, otherString);
	
	int ylol = (winheight) / 4;
	RECT(r, 50, ylol, winwidth - 60, (winheight-ylol-40));
	AddControl (pWindow, CONTROL_TEXTINPUT, r, NULL, 4, 5, 0);
	SetTextInputText(pWindow, 4, string);
	
	RECT(r, (winwidth - 50) / 2, (winheight - 30), 50, 20);
	AddControl (pWindow, CONTROL_BUTTON, r, "Damn", 3, 0, 0);
	
	while (HandleMessages (pWindow));
}

void CrashReporterFinalize(CrashInfo* pCrashInfo)
{
	if (!pCrashInfo->m_pTaskKilled) return;
	
	// Kill the process, which also includes our task
	Process* pProc = pCrashInfo->m_pTaskKilled->m_pProcess;
	if (pProc)
	{
		pProc->bWaitingForCrashAck = false;
		ExKillProcess(pProc);
	}
	else
	{
		// Just kill the task.
		KeKillTask(pCrashInfo->m_pTaskKilled);
	}
}

void CrashReporterProcessCrash(CrashInfo* pCrashInfo)
{
	if (!g_windowManagerRunning)
	{
		// If the window manager isn't running currently, simply log the crash to the screen.
		ILogMsg("Task %x (tag: '%s') has crashed, here're its details:", pCrashInfo->m_pTaskKilled, pCrashInfo->m_tag);
		
		Process *pProc = (Process*)pCrashInfo->m_pTaskKilled->m_pProcess;
		
		KeLogExceptionDetails(pCrashInfo->m_nErrorCode, &pCrashInfo->m_regs, pProc);
		
		CrashReporterFinalize(pCrashInfo);
	}
	else
	{
		// Create a crash report window.
		CrashReportWindow(pCrashInfo);
	}
	
}

void CrashReporterTask(UNUSED int arg)
{
	CrashInfo* pInfo = NULL;
	
	while (true)
	{
		// clear interrupts to avoid a race condition where a crash could come in
		// while we check....
		cli;
		
		pInfo = CrashInfoPop();
		
		if (!pInfo)
		{
			// this turns on interrupts again
			WaitObject(CrashInfoGetKey());
			continue;
		}
		
		sti;
		
		// we have a crash. Process it!
		CrashReporterProcessCrash(pInfo);
		
		// and free it:
		MmFree(pInfo);
	}
}

void CrashReporterInit()
{
	// Launch a task to watch for crashes.
	int errCode = 0;
	Task* pTask = KeStartTask(CrashReporterTask, 0, &errCode);
	if (!pTask)
	{
		LogMsg("Error: cannot spawn crash reporter task: %x", errCode);
		KeStopSystem();
	}
	
	KeTaskAssignTag(pTask, "CrashReporter");
	KeUnsuspendTask(pTask);
	KeDetachTask(pTask);
	
}
