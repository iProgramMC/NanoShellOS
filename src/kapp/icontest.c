/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Icon Test Application module
******************************************/

#include <wbuiltin.h>

void CALLBACK IconTestProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			RECT(r, 10, TITLE_BAR_HEIGHT + 10, 300, 50);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Test?", 1000, 0, 0);
			break;
		}
		case EVENT_PAINT:
			//draw until ICON_COUNT:
			for (int i = ICON_NULL+1; i < ICON_COUNT; i++)
			{
				int x = i & 15, y = i >> 4;
				RenderIconForceSize((IconType)i, x*32 + 10, y*32 + TITLE_BAR_HEIGHT+52, 32);
			}
			/*RenderIcon(ICON_CABINET, 10, 20);*/
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void IconTestTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("Icon test", 300, 200, 540, 400, IconTestProc, 0);
	pWindow->m_iconID = ICON_INFO;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}