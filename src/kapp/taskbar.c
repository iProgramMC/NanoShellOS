/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Launcher Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <vfs.h>
#include <elf.h>

#define TASKBAR_WIDTH (GetScreenWidth())
#define TASKBAR_HEIGHT TITLE_BAR_HEIGHT + 14 // padding around button: 4 px, padding around text: 2 px
#define TASKBAR_BUTTON_WIDTH 60
#define TASKBAR_BUTTON_HEIGHT TITLE_BAR_HEIGHT + 8
#define TASKBAR_TIME_THING_WIDTH 60

//hack.
#undef  TITLE_BAR_HEIGHT
#define TITLE_BAR_HEIGHT 11

enum {
	TASKBAR_HELLO = 0x1,
	TASKBAR_START_TEXT,
	TASKBAR_TIME_TEXT,
};

void LaunchLauncher()
{
	int errorCode = 0;
	Task* pTask;
	
	//create the program manager task.
	errorCode = 0;
	pTask = KeStartTask(LauncherEntry, 0, &errorCode);
	DebugLogMsg("Created launcher task. pointer returned:%x, errorcode:%x", pTask, errorCode);
}

void UpdateTaskbar (Window* pWindow)
{
	char buffer[1024];
	
	//TODO: Window buttons.
	
	// FPS
	sprintf(buffer, "<-- Click this button to start.  FPS: %d     ", GetWindowManagerFPS());
	SetLabelText(pWindow, TASKBAR_START_TEXT, buffer);
	
	// Time
	TimeStruct* time = TmReadTime();
	sprintf(buffer, "%02d:%02d:%02d", time->hours, time->minutes, time->seconds);
	SetLabelText(pWindow, TASKBAR_TIME_TEXT, buffer);
}

void CALLBACK TaskbarProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE: {
			Rectangle r;
			
			RECT (r, 4, 2, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_BUTTON, r, "Start", TASKBAR_HELLO, 0, 0);
			RECT (r, 8 + TASKBAR_BUTTON_WIDTH, 8, TASKBAR_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_TEXT, r, "<-- Click this button to start.", TASKBAR_START_TEXT, 0, WINDOW_BACKGD_COLOR);
			RECT (r, GetScreenWidth() - 2 - TASKBAR_TIME_THING_WIDTH, 8, TASKBAR_TIME_THING_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_TEXT, r, "?", TASKBAR_TIME_TEXT, 0, WINDOW_BACKGD_COLOR);
			
			break;
		}
		case EVENT_UPDATE: {
			UpdateTaskbar(pWindow);
			break;
		}
		case EVENT_PAINT: {
			
			break;
		}
		case EVENT_COMMAND: {
			switch (parm1)
			{
				case TASKBAR_HELLO:
					LaunchLauncher();
					break;
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void TaskbarEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = TASKBAR_WIDTH-1, wh = TASKBAR_HEIGHT;//, sh = GetScreenHeight();
	int wx = 0, wy = 0;//(sh - wh)+2;
	
	Window* pWindow = CreateWindow ("Desktop", wx, wy, ww, wh, TaskbarProgramProc, WF_NOCLOSE | WF_NOTITLE);
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	pWindow->m_iconID = ICON_DESKTOP2;
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	int timeout = 0;
	while (HandleMessages (pWindow))
	{
		if (timeout == 0)
		{
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			WindowRegisterEvent(pWindow, EVENT_PAINT,  0, 0);
			timeout = 100;
		}
		timeout--;
	}
#endif
}
