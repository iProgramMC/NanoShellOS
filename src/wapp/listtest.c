/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Icon Test Application module
******************************************/

#include <wbuiltin.h>

#define LISTTEST_WIDTH  600
#define LISTTEST_HEIGHT 400

void CALLBACK ListTestProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
			break;
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			RECT(r, 8, 8 + TITLE_BAR_HEIGHT, LISTTEST_WIDTH - 16, LISTTEST_HEIGHT - 16 - TITLE_BAR_HEIGHT);
			
			AddControlEx(pWindow, CONTROL_TABLEVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, 1, 0, 0);
			
			/*RECT (r, 20, 20, 1, 240-40);
			//goes from 0-99
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, 2, ((0<<16)|100), 50);
			
			RECT (r, 40, 20, 1, 240-40);
			//goes from 0-9
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, 2, ((0<<16)|10), 5);
			
			RECT (r, 60, 20, 240-40, 1);
			//goes from 0-99
			AddControl (pWindow, CONTROL_HSCROLLBAR, r, NULL, 2, ((0<<16)|100), 50);
			
			RECT (r, 60, 40, 240-40, 1);
			//goes from 0-9
			AddControl (pWindow, CONTROL_HSCROLLBAR, r, NULL, 2, ((0<<16)|10), 5);*/
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void ListTestTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("List Test", 300, 200, LISTTEST_WIDTH, LISTTEST_HEIGHT, ListTestProc, WF_ALWRESIZ);
	pWindow->m_iconID = ICON_TEXT_FILE;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
