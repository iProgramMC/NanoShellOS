/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Manager Depth Buffer Module
******************************************/
#include "wi.h"

short* g_windowDepthBuffer = NULL; //must be allocated
short g_windowDrawOrder[WINDOWS_MAX];
int g_windowDrawOrderSize;
int g_windowDrawOrderLayerEnds[6]; // [i] = Where the layer 'i+1' starts.

void ResetWindowDrawOrder()
{
    for (int i = 0; i < (int)ARRAY_COUNT(g_windowDrawOrderLayerEnds); i++)
        g_windowDrawOrderLayerEnds[i] = 0;
    for (int i = 0; i < (int)ARRAY_COUNT(g_windowDrawOrder); i++)
        g_windowDrawOrder[i] = -1;
    g_windowDrawOrderSize = 0;
}

SAI int GetLayer(Window* pWindow)
{
	if (pWindow->m_flags & WF_FOREGRND) return 2;
	if (pWindow->m_flags & WF_BACKGRND) return 0;
	return 1;
}

void AddWindowToDrawOrder(short index)
{
    int layer = GetLayer(&g_windows[index]);
    // Move everything from the next layer and beyond by one.
    int start = g_windowDrawOrderLayerEnds[layer];

    // Increase the size of the draw order array.
    g_windowDrawOrderSize++;

    // Move everything.
    memmove(&g_windowDrawOrder[start + 1], &g_windowDrawOrder[start], sizeof(g_windowDrawOrder[0]) * (g_windowDrawOrderSize - start - 1));
    for (int i = layer; i < (int)ARRAY_COUNT(g_windowDrawOrderLayerEnds); i++)
    {
        g_windowDrawOrderLayerEnds[i]++;
    }

    // Set the new order index.
    g_windowDrawOrder[start] = index;
}

void RemovePlaceFromDrawOrder(int place)
{
    int layer = GetLayer(&g_windows[g_windowDrawOrder[place]]);

    // memmove everything from 'place' onwards
    memmove(&g_windowDrawOrder[place], &g_windowDrawOrder[place + 1], sizeof(g_windowDrawOrder[0]) * (g_windowDrawOrderSize - place - 1));

    g_windowDrawOrder[--g_windowDrawOrderSize] = -1;

    for (int i = layer; i < (int)ARRAY_COUNT(g_windowDrawOrderLayerEnds); i++)
    {
        g_windowDrawOrderLayerEnds[i]--;
    }
}

void RemoveWindowFromDrawOrder(int index)
{
    for (int i = 0; i < g_windowDrawOrderSize; i++)
    {
        if (g_windowDrawOrder[i] == index)
            RemovePlaceFromDrawOrder(i);
    }
}

void MovePreExistingWindowToFront(short windowIndex)
{
	RemoveWindowFromDrawOrder(windowIndex);
	AddWindowToDrawOrder(windowIndex);
}

Window* ShootRayAndGetWindow(int x, int y)
{
	Point p = { x, y };
	for (int i = g_windowDrawOrderSize; i >= 0; i--)
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
	
	//redraw the background, if needed
	Rectangle* pStart = NULL, *pEnd = NULL;
	WmSplitRectangle(rect, pWindowToExclude, &pStart, &pEnd);
	
	for (Rectangle* pRect = pStart; pRect != pEnd; pRect++)
	{
		RedrawBackground(*pRect);
	}
	
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
			windowDrawList[i]->m_vbeData.m_drs->m_bIgnoreAndDrawAll = true;
			RenderWindow(windowDrawList[i]);
		}
	}
	
	if (RectangleOverlap(&rect, TooltipGetRect()))
		TooltipDraw();
	
	g_vbeData = backup;
}
