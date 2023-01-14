/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Manager Depth Buffer Module
******************************************/
#include "wi.h"


short* g_windowDepthBuffer = NULL; //must be allocated
short  g_windowDrawOrder[WINDOWS_MAX];

void ResetWindowDrawOrder()
{
	memset(g_windowDrawOrder, 0xFF, sizeof(g_windowDrawOrder));
}

void AddWindowToDrawOrder(short windowIndex)
{
	for (size_t i = 0; i < ARRAY_COUNT (g_windowDrawOrder); i++)
	{
		if (g_windowDrawOrder [i] == windowIndex)
			g_windowDrawOrder [i] =  -1;
	}
	memcpy (g_windowDrawOrder, g_windowDrawOrder+1, sizeof(g_windowDrawOrder)-sizeof(short));
	g_windowDrawOrder[WINDOWS_MAX-1] = windowIndex;
}

void MovePreExistingWindowToFront(short windowIndex)
{
	//where is our window located?
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		if (g_windowDrawOrder[i] == windowIndex)
		{
			g_windowDrawOrder[i] = -1;
			//move everything after it back
			memcpy (g_windowDrawOrder + i, g_windowDrawOrder + i + 1, sizeof (short) * (WINDOWS_MAX - i - 1));
			g_windowDrawOrder[WINDOWS_MAX-1] = windowIndex;
			
			return;
		}
	}
}

Window* ShootRayAndGetWindow(int x, int y)
{
	Point p = { x, y };
	for (int i = WINDOWS_MAX-1; i >= 0; i--)
	{
		short order = g_windowDrawOrder[i];
		if (order < 0) continue;
		if (!g_windows[order].m_used)   continue;
		if ( g_windows[order].m_hidden) continue;
		
		if (RectangleContains(&g_windows[order].m_rect, &p))
		{
			return &g_windows[order];
		}
	}
	
	return NULL;
}

void RefreshRectangle(Rectangle rect, Window* pWindowToExclude)
{
	// pWnd - the window that's being undrawn
	VBEData* backup = g_vbeData;
	VidSetVBEData(NULL);
	
	LockAcquire (&g_BackgdLock);
	
	//redraw the background and all the things underneath:
	RedrawBackground(rect);
	
	LockFree (&g_BackgdLock);
	
	// draw the windows below it, in their z-order.
	int sz = 0;
	Window* windowDrawList[WINDOWS_MAX];
	
	sz = 0;
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		int drawOrder = g_windowDrawOrder[i];
		if (drawOrder < 0) continue;
		
		Window* pWindow = &g_windows[drawOrder];
		
		if (RectangleOverlap (&pWindow->m_rect, &rect) && pWindow != pWindowToExclude)
			windowDrawList[sz++] = pWindow;
	}
	
	// We've added the windows to the list, so draw them. We don't need to worry
	// about windows above them, as the way we're drawing them makes it so pixels
	// over the window aren't overwritten.
	//DebugLogMsg("Drawing %d windows below this one", sz);
	for (int i = sz - 1; i >= 0; i--) 
	{
		if (windowDrawList[i]->m_hidden) continue;
		// Hrm... we should probably use the new VidBitBlit for this
		
		if (windowDrawList[i]->m_minimized)
		{
			//TODO?
		}
		else
		{
			VidBitBlit (
				g_vbeData,
				rect.left,
				rect.top,
				rect.right  - rect.left,
				rect.bottom - rect.top,
				&windowDrawList[i]->m_vbeData,
				rect.left - windowDrawList[i]->m_rect.left,
				rect.top  - windowDrawList[i]->m_rect.top,
				BOP_SRCCOPY
			);
		}
	}
	
	if (RectangleOverlap(&rect, TooltipGetRect()))
		TooltipDraw();
	
	g_vbeData = backup;
}
