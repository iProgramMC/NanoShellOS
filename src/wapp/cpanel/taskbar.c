/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

      Control panel - Taskbar Applet
******************************************/

#include <wbuiltin.h>

enum
{
	TASKBAR_POPUP_DATECHECK = 1,
	TASKBAR_POPUP_TIMECHECK,
	TASKBAR_POPUP_APPLY,
	TASKBAR_POPUP_CANCEL,
};

#define TSKBR_POPUP_WIDTH 200
#define TSKBR_POPUP_HEITE 140

extern bool g_bShowDate, g_bShowTimeSeconds;
void TaskbarSetProperties(bool bShowDate, bool bShowTimeSecs);

void CALLBACK CplTaskbarWndProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			pWindow->m_iconID = ICON_HOME;
			
			Rectangle r;
			
			RECT(r, 10, 10, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show the date on the task bar", TASKBAR_POPUP_DATECHECK, g_bShowDate, 0);
			
			RECT(r, 10, 30, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show the seconds on the clock on the task bar", TASKBAR_POPUP_TIMECHECK, g_bShowTimeSeconds, 0);
			
			RECT(r, (TSKBR_POPUP_WIDTH - 160) / 2,     TSKBR_POPUP_HEITE - 30, 75, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", TASKBAR_POPUP_CANCEL, 0, 0);
			RECT(r, (TSKBR_POPUP_WIDTH - 160) / 2 + 80,TSKBR_POPUP_HEITE - 30, 75, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "OK",     TASKBAR_POPUP_APPLY,  0, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
				case TASKBAR_POPUP_CANCEL:
					DestroyWindow(pWindow);
					break;
				case TASKBAR_POPUP_APPLY:
					TaskbarSetProperties(CheckboxGetChecked(pWindow, TASKBAR_POPUP_DATECHECK), CheckboxGetChecked(pWindow, TASKBAR_POPUP_TIMECHECK));
					DestroyWindow(pWindow);
					break;
			}
			break;
		}
		case EVENT_PAINT:
		{
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void CplTaskbar(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Task Bar",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		TSKBR_POPUP_WIDTH,
		TSKBR_POPUP_HEITE,
		CplTaskbarWndProc,
		WF_NOMINIMZ
	);
}
