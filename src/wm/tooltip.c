/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Window Manager Tooltip Module
******************************************/
#include "wi.h"

Tooltip g_tooltip;

void TooltipDismiss()
{
	//if we're not showing anything, get outta here
	if (!g_tooltip.m_shown) return;
	
	g_tooltip.m_shown = false;
	RefreshRectangle(g_tooltip.m_rect, NULL);
}

void TooltipDraw()
{
	//if we're not showing anything, get outta here
	if (!g_tooltip.m_shown) return;
	
	VBEData* pBackup = g_vbeData;
	
	VidSetVBEData (&g_mainScreenVBEData);
	
	VidFillRect(TOOLTIP_BACKGD_COLOR, g_tooltip.m_rect.left, g_tooltip.m_rect.top, g_tooltip.m_rect.right - 2, g_tooltip.m_rect.bottom - 2);
	VidDrawRect(TOOLTIP_TEXT_COLOR,   g_tooltip.m_rect.left, g_tooltip.m_rect.top, g_tooltip.m_rect.right - 2, g_tooltip.m_rect.bottom - 2);
	
	VidTextOut(g_tooltip.m_text, g_tooltip.m_rect.left + 3, g_tooltip.m_rect.top + 3, TOOLTIP_TEXT_COLOR, TRANSPARENT);
	
	VidSetVBEData (pBackup);
}

void TooltipShow(const char * text, int x, int y)
{
	//dismiss the old one
	TooltipDismiss();
	
	strncpy (g_tooltip.m_text, text, 511);
	
	//calculate the rectangle
	int width = 0, height = 0;
	VidTextOutInternal(text, 0, 0, 0, 0, true, &width, &height);
	
	g_tooltip.m_shown = true;
	
	g_tooltip.m_rect.left   = x;
	g_tooltip.m_rect.top    = y;
	g_tooltip.m_rect.right  = g_tooltip.m_rect.left + width  + 6;
	g_tooltip.m_rect.bottom = g_tooltip.m_rect.top  + height + 6;
	
	//render it initially
	TooltipDraw();
}

Rectangle* TooltipGetRect()
{
	return &g_tooltip.m_rect;
}
