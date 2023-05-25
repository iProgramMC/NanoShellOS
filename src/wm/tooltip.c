/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Window Manager Tooltip Module
******************************************/
#include "wi.h"

Window* g_pTooltipWindow;
SafeLock g_tooltipLock;

void TooltipDismiss()
{
	LockAcquire(&g_tooltipLock);
	if (!g_pTooltipWindow)
	{
		LockFree(&g_tooltipLock);
		return;
	}
	
	DestroyWindow(g_pTooltipWindow);
	g_pTooltipWindow = NULL;
	
	LockFree(&g_tooltipLock);
	return;
}

void TooltipWindowProc(Window* pWindow, int eventType, int parm1, int parm2)
{
	switch (eventType)
	{
		case EVENT_USER:
		{
			char* ptr = (char*)parm1;
			
			Rectangle rect = GetWindowClientRect(pWindow, false);
			rect.left   += 2;
			rect.top    += 2;
			rect.right  -= 2;
			rect.bottom -= 2;
			
			AddControl(pWindow, CONTROL_TEXTHUGE, rect, NULL, 1, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			SetHugeLabelText(pWindow, 1, ptr);
			
			MmFree(ptr);
			
			CallControlCallback(pWindow, 1, EVENT_PAINT, 0, 0);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			break;
	}
}

void TooltipShow(const char * text, int x, int y)
{
	//dismiss the old one
	TooltipDismiss();
	
	LockAcquire(&g_tooltipLock);
	
	//calculate the rectangle
	size_t sz = strlen(text);
	char* ptr = MmAllocate(sz * 2 + 1);
	
	int height = WrapText(ptr, text, GetScreenWidth() / 3);
	int width  = 0, height2 = 0;
	VidTextOutInternal(ptr, 0, 0, 0, 0, true, &width, &height2);
	
	g_pTooltipWindow = CreateWindow("Tooltip Window", x, y, width + 4, height + 4, TooltipWindowProc, WF_NOTITLE | WF_SYSPOPUP | WF_FOREGRND);
	
	if (!g_pTooltipWindow)
	{
		LockFree(&g_tooltipLock);
		MmFree(ptr);
		return;
	}
	
	WindowRegisterEvent(g_pTooltipWindow, EVENT_USER, (int)ptr, 0);
	
	g_pTooltipWindow->m_bWindowManagerUpdated = true;
	
	LockFree(&g_tooltipLock);
}

Rectangle* TooltipGetRect()
{
	return &g_pTooltipWindow->m_fullRect;
}
