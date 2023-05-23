/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

      Window Manager Shutdown Module
******************************************/
#include "wi.h"

bool g_shutdownSentDestroySignals = false;
bool g_shutdownWaiting 			  = false;
bool g_shutdownRequest 			  = false;
bool g_shutdownWantReb 			  = false;
bool g_shutdownSentCloseSignals   = false;
bool g_shutdownProcessing		  = false;
bool g_shutdownDoneAll			  = false;

#define SAVING_DATA_POPUP_WIDTH  (260)
#define SAVING_DATA_POPUP_HEIGHT (24)

typedef struct
{
	int return_code;
	char title[WINDOW_TITLE_MAX];
	Window* pWindowToKill;
}
UserKillWndControlBlock;

void CALLBACK ShuttingDownWindowCallback (Window* pWindow, int messageType, int parm1, int parm2)
{
	DefaultWindowProc (pWindow, messageType, parm1, parm2);
	if (messageType == EVENT_CREATE)
	{
		Rectangle r;
		RECT(r, 0, 0, SAVING_DATA_POPUP_WIDTH, SAVING_DATA_POPUP_HEIGHT);
		
		AddControl(pWindow, CONTROL_TEXTCENTER, r, "Please wait while unsaved data is written to disk.", 100, 0, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED);
	}
}

// Further shut down processing after closing all the windows.
void FurtherShutdownProcessing()
{
	Window *pWindow = CreateWindow(
		"Shutting down",
		(GetScreenSizeX() - SAVING_DATA_POPUP_WIDTH) / 2, (GetScreenSizeY() - SAVING_DATA_POPUP_HEIGHT) / 2,
		SAVING_DATA_POPUP_WIDTH, SAVING_DATA_POPUP_HEIGHT,
		ShuttingDownWindowCallback, WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ | WF_NOTITLE | WF_SYSPOPUP
	);
	pWindow->m_bWindowManagerUpdated = true;
	
	KeOnShutDownSaveData();
	
	DestroyWindow (pWindow);
}

void ShutdownProcessing(UNUSED int parameter)
{
	KeTaskAssignTag(KeGetRunningTask(), "Shutting down");
	g_shutdownProcessing = true;
	
	// Request an End Task to all applications.
	
	// reverse to shut down the taskbar last.
	while (true)
	{
		bool bReady = true;
		for (int i = WINDOWS_MAX-1; i >= 0; i--)
		{
			if (g_windows[i].m_used)
			{
				bReady = false;
				if (!(g_windows[i].m_flags & WI_MESSGBOX))
				{
					WindowRegisterEvent (g_windows + i, EVENT_DESTROY, 0, 0);
				}
				
				int half_seconds_to_wait;
			check_wait1:
				half_seconds_to_wait = 20;
				bool shutdownOK = false;
				while (half_seconds_to_wait--)
				{
					if (!g_windows[i].m_used)
					{
						shutdownOK = true;
						break; // move on
					}
					
					WaitMS(500);
				}
				
				if (!shutdownOK)
				{
					SLogMsg("Asking user if they want to shutdown this application...");
					int result = MessageBox(
						NULL,
						"This NanoShell application cannot respond to the End Task\n"
						"request.  It may be busy, waiting for a response from you,\n"
						"or it may have stopped executing.\n"
						"\n"
						"o Press 'Ignore' to cancel the shut down and return to\n"
						"  NanoShell.\n\n"
						"o Press 'Abort' to close this application immediately.\n"
						"  You will lose any unsaved information in this application.\n\n"
						"o Press 'Retry' to wait for this application to shut itself\n"
						"  down.",
						g_windows[i].m_title,
						MB_ABORTRETRYIGNORE | ICON_WARNING << 16
					);
					
					if (result == MBID_ABORT)
					{
						SLogMsg("Shutting it down!");
						if (g_windows[i].m_used)
						{
							if (!g_windows[i].m_bWindowManagerUpdated)
							{
								if (g_windows[i].m_pOwnerThread)
									KeKillTask (g_windows[i].m_pOwnerThread);
								if (g_windows[i].m_pSubThread)
									KeKillTask (g_windows[i].m_pSubThread);
							}
							
							NukeWindow (&g_windows[i]);
						}
					}
					else if (result == MBID_IGNORE)
					{
						SLogMsg("Shutdown cancelled.");
						g_shutdownDoneAll    = false;
						g_shutdownProcessing = false;
						g_shutdownRequest    = false;
						return;
					}
					else goto check_wait1;
				}
			}
		}
		if (bReady)
		{
			// Lookin' good!
			SLogMsg("All windows have shutdown gracefully! Going to flush all caches and shut down now...");
			
			FurtherShutdownProcessing();
			
			g_shutdownDoneAll    = true;
			g_shutdownProcessing = false;
			g_shutdownRequest    = false;
			return;
		}
	}
}

Window* g_pShutdownMessage = NULL;

void WindowManagerOnShutdownTask (__attribute__((unused)) int useless)
{
	if (g_shutdownWantReb || MessageBox (NULL, "It is now safe to shut down your computer.", "Shutdown Computer", MB_RESTART | ICON_SHUTDOWN << 16) == MBID_OK)
	{
		KeRestartSystem();
	}
}

void WindowManagerOnShutdown(void)
{
	//create a task
	UNUSED int useless = 0;
	Task* task = KeStartTask(WindowManagerOnShutdownTask, 0, &useless);
	KeDetachTask(task);
	KeUnsuspendTask(task);
}

void WindowManagerShutdown(bool wants_restart_too)
{
	if (g_shutdownProcessing)
	{
		MessageBox(NULL, "The window station is shutting down.", "Error", MB_OK | ICON_ERROR << 16);
		return;
	}
	g_shutdownRequest = true;
	g_shutdownWantReb = wants_restart_too;
}
