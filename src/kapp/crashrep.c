/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Problem Reporting applet
******************************************/
#include <wbuiltin.h>
#include <task.h>
#include <debug.h>
#include <idt.h>
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

//The argument is assumed to be a pointer to a Registers* structure.  It gets freed automatically by the task.
void CrashReportWindow( int argument )
{
	CrashInfo* pCrashInfo = (CrashInfo*)argument;
	
	char string [8192];
	DumpRegistersToString (string, &pCrashInfo->m_regs);
	
	char otherString[512], 
	     //anotherString[4096], 
	     smallerString[16];
	sprintf(
		otherString,
		"This task has performed an illegal operation and will be shut down.\n\n"
		"If the problem persists, contact the program vendor.\n\n"
	);
	
	int index = 0;
	while (pCrashInfo->m_stackTrace[index] != 0)
	{
		sprintf(smallerString, "\n- 0x%x", pCrashInfo->m_stackTrace[index]);
		strcat (string, smallerString);
		index++;
	}
	
	int winwidth = 500, winheight = 260;
	
	Window* pWindow = CreateWindow (
		pCrashInfo->m_tag, 
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
		LogMsg("Could not create crash report window!");
		LogMsg("Some task crashed, here is its register dump:");
		LogMsg("%s", string);
		return;
	}
	
	pWindow->m_iconID = ICON_NANOSHELL_LETTERS;
	
	//add some controls to the window before handing control
	Rectangle r;
	RECT(r, 10, 10 + TITLE_BAR_HEIGHT, 32, 32);
	AddControl (pWindow, CONTROL_ICON, r, NULL, 1, ICON_AMBULANCE, 0);
	
	RECT(r, 50, 10 + TITLE_BAR_HEIGHT, winwidth - 50, 100);
	AddControl (pWindow, CONTROL_TEXTHUGE, r, NULL, 2, 0, TEXTSTYLE_WORDWRAPPED);
	SetHugeLabelText(pWindow, 2, otherString);
	
	RECT(r, 50, winheight/4, winwidth - 60, (winheight-TITLE_BAR_HEIGHT-80));
	AddControl (pWindow, CONTROL_TEXTINPUT, r, NULL, 4, 5, 0);
	SetTextInputText(pWindow, 4, string);
	
	RECT(r, (winwidth - 50) / 2, (winheight - 30), 50, 20);
	AddControl (pWindow, CONTROL_BUTTON, r, "Close", 3, 0, 0);
	
	while (HandleMessages (pWindow));
}

void CrashReporterCheck()
{
	if (KeDidATaskCrash())
	{
		//OMG! A task died? Call the ambulance!!!
		
		// Allocate the registers so we can pass them onto the new task.
		CrashInfo* pCrashInfo = MmAllocate (sizeof (CrashInfo)), crashInfo = *KeGetCrashedTaskInfo();
		memcpy (pCrashInfo, &crashInfo, sizeof(CrashInfo));
		
		// Let the kernel know that we have processed its crash report.
		KeAcknowledgeTaskCrash();
		
		// Create a CrashReportWindow instance.
		int error_code = 0;
		Task *pTask = KeStartTask (CrashReportWindow, (int)pCrashInfo, &error_code);
		
		if (!pTask)
		{
			LogMsg("Could not create crash report task.");
			LogMsg("Some task crashed, here is its register dump:");
			DumpRegisters(&pCrashInfo->m_regs);
			MmFree(pCrashInfo);
		}
		
		//Done.
	}
}
