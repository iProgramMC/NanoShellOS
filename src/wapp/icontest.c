/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Icon Test Application module
******************************************/

#include <wbuiltin.h>
#include <wmenu.h>
#include <image.h>

/*
static int lastTick1 = 0;
static int lastTick2 = 0;
static int lastTick3 = 0;
*/

Image* GetIconImage(IconType type, int sz);

void CALLBACK IconTestProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		/*
		case EVENT_USER:
		{
			int nowTick = GetTickCount();
			SLogMsg("Update Timer 1. Diff = %d", nowTick - lastTick1);
			lastTick1 = nowTick;
			break;
		}
		case EVENT_USER + 1:
		{
			int nowTick = GetTickCount();
			SLogMsg("Update Timer 2. Diff = %d", nowTick - lastTick2);
			lastTick2 = nowTick;
			break;
		}
		case EVENT_USER + 2:
		{
			int nowTick = GetTickCount();
			SLogMsg("Update Timer 3. Diff = %d", nowTick - lastTick3);
			lastTick3 = nowTick;
			break;
		}
		case EVENT_CREATE:
		{
			AddTimer(pWindow, 500, EVENT_USER);
			AddTimer(pWindow, 250, EVENT_USER + 1);
			AddTimer(pWindow, 100, EVENT_USER + 2);
			break;
		}
		*/
		
		case EVENT_CREATE:
		{
			//Image* pImg = GetIconImage(ICON_EXPERIMENT, 32);
			
			//int id = UploadCursor(pImg, 16, 16);
			//pWindow->m_data = (void*)id;
			
			//ChangeCursor(pWindow, id);
			
			break;
		}
		case EVENT_DESTROY:
		{
			//int id = (int)pWindow->m_data;
			//ChangeCursor(pWindow, CURSOR_DEFAULT);
			//ReleaseCursor(id);
			
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
		}
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
			
			Rectangle r;
			RECT(r, 10, 10 + TITLE_BAR_HEIGHT, 100, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Hang (5 sec)", 1002, 0, 0);
			
			RECT(r, 10, 40 + TITLE_BAR_HEIGHT, 100, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Hang (10 sec)", 1000, 0, 0);
			
			RECT(r, 10, 70 + TITLE_BAR_HEIGHT, 100, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Hang (30 sec)", 1001, 0, 0);
			
			RECT(r, 10, 100 + TITLE_BAR_HEIGHT, 100, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Show Tooltip", 1003, 0, 0);
			
			RECT(r, 10, 130 + TITLE_BAR_HEIGHT, 100, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Crash Now!", 1004, 0, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
				case 1000:
					SLogMsg("Hanging 10 sec!");
					WaitMS(10000);
					break;
				case 1001:
					SLogMsg("Hanging 30 sec!");
					WaitMS(30000);
					break;
				case 1002:
					SLogMsg("Hanging 5 sec!");
					WaitMS(5000);
					break;
				case 1004:
					SLogMsg("Gonna crash now!");
					*((uint32_t*)0xFFFFFFF8) = 0x01234567;
					break;
				case 1003:
				{
					Point p = GetMousePos();
					TooltipShow("This is a testing tooltip!\n\nLook ma, I'm on another line!!\nThis is awesome!", p.x, p.y + 30);
					break;
				}
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
	Window* pWindow = CreateWindow ("Icon test", 300, 200, 540, 540, IconTestProc, WF_ALWRESIZ);
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