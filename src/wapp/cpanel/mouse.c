/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

       Control panel - Mouse Applet
******************************************/

#include <wbuiltin.h>

#define MOUSE_POPUP_WIDTH 200
#define MOUSE_POPUP_HEITE 70

extern bool g_ps2MouseAvail;

int  GetMouseSpeedMultiplier();
void SetMouseSpeedMultiplier(int spd);

enum
{
	MOUSEP_SPEED_SCROLL = 1000,
	MOUSEP_KEYBOARD_CONTROL_MOUSE,
};

void CALLBACK CplMouseWndProc(Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			pWindow->m_iconID = ICON_MOUSE;//TODO
			
			//add a button
			Rectangle r;
			RECT(r,8,8,MOUSE_POPUP_WIDTH-16,35);
			AddControl(pWindow, CONTROL_SURROUND_RECT, r, "Mouse tracking speed", 1, 0, 0);
			{
				//add stuff inside the rect.
				//this scope has no actual reason for its existence other than to mark that stuff we add here goes inside the rect above.
				
				RECT(r, 16,  24, 32, 20);
				AddControl(pWindow, CONTROL_TEXT, r, "Slow", 2, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
				RECT(r, MOUSE_POPUP_WIDTH - 40, 24, 32, 20);
				AddControl(pWindow, CONTROL_TEXT, r, "Fast", 3, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
				RECT(r, 50,  22, MOUSE_POPUP_WIDTH - 100, 1);
				AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, MOUSEP_SPEED_SCROLL, (0)<<16|(4), (1)<<16|(GetMouseSpeedMultiplier()));
			}
			
			break;
		}
		case EVENT_COMMAND:
			DestroyWindow(pWindow);
			break;
		case EVENT_RELEASECURSOR:
			SetMouseSpeedMultiplier(GetScrollBarPos(pWindow, MOUSEP_SPEED_SCROLL));
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void CplMouse(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Mouse",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		MOUSE_POPUP_WIDTH,
		MOUSE_POPUP_HEITE,
		CplMouseWndProc,
		WF_NOMINIMZ
	);
}
