/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Problem Reporting applet
******************************************/
#include <wbuiltin.h>
#include <task.h>
#include <process.h>
#include <debug.h>
#include <idt.h>
#include <elf.h>
#include <vfs.h>

bool KeDidATaskCrash();
void KeAcknowledgeTaskCrash();//sigh.
CrashInfo* KeGetCrashedTaskInfo();

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
//The argument is assumed to be a pointer to a Registers* structure.  It gets freed automatically by the task.
void CrashReportWindow( int argument )
{
	CrashInfo* pCrashInfo = (CrashInfo*)argument;
	
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

void WmOnTaskCrashed(Task *pTask);

void CrashReporterCheck()
{
	if (KeDidATaskCrash())
	{
		// A task died? Call the ambulance!!!
		cli;
		CrashInfo crashInfo = *KeGetCrashedTaskInfo();
		sti;
		
		// Allocate the registers so we can pass them onto the new task.
		CrashInfo* pCrashInfo = MmAllocate (sizeof (CrashInfo));
		
		// Close the file resources opened by this task.
		FiReleaseResourcesFromTask(crashInfo.m_pTaskKilled);
		
		// Close the windows that have been opened by this task.
		//WmOnTaskCrashed(crashInfo.m_pTaskKilled);
		
		if (!pCrashInfo)
		{
			ILogMsg("Could not create crash report task.");
			ILogMsg("Some task crashed, here is its register dump:");
			DumpRegisters(&crashInfo.m_regs);
			return;
		}
		
		memcpy (pCrashInfo, &crashInfo, sizeof(CrashInfo));
		
		// Let the kernel know that we have processed its crash report.
		KeAcknowledgeTaskCrash();
		
		// Create a CrashReportWindow instance.
		int error_code = 0;
		Task *pTask = KeStartTask (CrashReportWindow, (int)pCrashInfo, &error_code);
		
		if (!pTask)
		{
			ILogMsg("Could not create crash report task.");
			ILogMsg("Some task crashed, here is its register dump:");
			DumpRegisters(&pCrashInfo->m_regs);
			MmFree(pCrashInfo);
		}
		else
		{
			KeUnsuspendTask(pTask);
		}
		
		// Kill the process
		if (crashInfo.m_pTaskKilled->m_pProcess)
		{
			Process *p = (Process*)crashInfo.m_pTaskKilled->m_pProcess;
			p->bWaitingForCrashAck = false;
			ExKillProcess(p);
		}
		else
		{
			// Just kill the task.
			KeKillTask(crashInfo.m_pTaskKilled);
		}
	}
	
	// Check for low memory
}

extern bool g_windowManagerRunning;
extern Task *g_pWindowMgrTask;

void CrashReporterCheckNoWindow()
{
	if (KeDidATaskCrash())
	{
		// OMG! A task died? Call the ambulance!!!
		
		// Allocate the registers so we can pass them onto the new task.
		CrashInfo* pCrashInfo, crashInfo = *KeGetCrashedTaskInfo();
		
		if (crashInfo.m_pTaskKilled != g_pWindowMgrTask)
		{
			if (g_windowManagerRunning) return;
		}
		
		pCrashInfo = &crashInfo;
		
		// Let the kernel know that we have processed its crash report.
		KeAcknowledgeTaskCrash();
		
		// Print the crash output
		ILogMsg("Task %x (tag: '%s') has crashed, here're its details:", pCrashInfo->m_pTaskKilled, pCrashInfo->m_tag);
		Process *p = (Process*)crashInfo.m_pTaskKilled->m_pProcess;
		KeLogExceptionDetails(BC_EX_DEBUG, &pCrashInfo->m_regs, crashInfo.m_pTaskKilled->m_pProcess);
		
		// Kill the process
		if (crashInfo.m_pTaskKilled->m_pProcess)
		{
			p->bWaitingForCrashAck = false;
			ExKillProcess(p);
		}
		else
		{
			// Just kill the task.
			KeKillTask(crashInfo.m_pTaskKilled);
		}
	}
	
	// Check for low memory
}
