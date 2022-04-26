/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Icon Test Application module
******************************************/

#include <wbuiltin.h>
#include <wmenu.h>
#include <image.h>

Image* GetIconImage(IconType type, int sz);

void CALLBACK IconTestProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			//draw until ICON_COUNT:
			int icons_per_width = (pWindow->m_rect.right - pWindow->m_rect.left - 40) / 32;
			if (icons_per_width == 0) icons_per_width = 1;
			for (int i = ICON_NULL+1; i < ICON_COUNT; i++)
			{
				int x = i % icons_per_width, y = i / icons_per_width;
				RenderIconForceSize((IconType)i, x*32 + 10, y*32 + TITLE_BAR_HEIGHT+8/*+((pWindow->m_rect.bottom - pWindow->m_rect.top) - (400 - 82))*/, 32);
			}
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void IconTestTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("Icon test", 300, 200, 540, 400, IconTestProc, WF_ALWRESIZ);
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