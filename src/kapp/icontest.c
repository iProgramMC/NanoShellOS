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
		case EVENT_PAINT:
			//draw until ICON_COUNT:
			for (int i = ICON_NULL+1; i < ICON_COUNT; i++)
			{
				int x = i & 7, y = i >> 3;
				RenderIconForceSize((IconType)i, x*32 + 10, y*32 + 15, 16);
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
	Window* pWindow = CreateWindow ("Icon Test", 300, 200, 320, 240, IconTestProc, 0);
	
	if (!pWindow)
		DebugLogMsg("Hey, the window couldn't be created");
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}