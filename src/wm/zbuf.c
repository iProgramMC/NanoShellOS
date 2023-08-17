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
	if (pWindow->m_flags & WF_FOREGRND) return 3;
	if (pWindow->m_flags & WF_BACKGRND) return 1;
	if (pWindow->m_flags & WF_BACKGND2) return 0;
	return 2;
}

void DebugDumpDrawOrder()
{
	SLogMsgNoCr("DrawOrderDebug: ");
	
	for (int i = 0; i < g_windowDrawOrderSize; i++)
	{
		for (int x = 0; x < (int)ARRAY_COUNT(g_windowDrawOrderLayerEnds); x++)
			if (g_windowDrawOrderLayerEnds[x] == i)
				SLogMsgNoCr("|%d|", x);
		SLogMsgNoCr("%d ", g_windowDrawOrder[i]);
	}
	
	SLogMsg("  End");
}

void AddWindowToDrawOrderUnsafe(short index)
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

void RemovePlaceFromDrawOrderUnsafe(int place)
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

void RemoveWindowFromDrawOrderUnsafe(int index)
{
    for (int i = 0; i < g_windowDrawOrderSize; i++)
    {
        if (g_windowDrawOrder[i] == index)
		{
            RemovePlaceFromDrawOrderUnsafe(i);
			i--;
		}
    }
}

void MovePreExistingWindowToFront(short windowIndex)
{
	cli;
	RemoveWindowFromDrawOrderUnsafe(windowIndex);
	AddWindowToDrawOrderUnsafe(windowIndex);
	sti;
}

void AddWindowToDrawOrder(short windowIndex)
{
	cli;
	AddWindowToDrawOrderUnsafe(windowIndex);
	sti;
}

void RemovePlaceFromDrawOrder(int place)
{
	cli;
	RemovePlaceFromDrawOrderUnsafe(place);
	sti;
}

void RemoveWindowFromDrawOrder(int index)
{
	cli;
	RemoveWindowFromDrawOrderUnsafe(index);
	sti;
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
		
		if (RectangleContains(&g_windows[order].m_fullRect, &p))
		{
			return &g_windows[order];
		}
	}
	
	return NULL;
}

void DisjointRectSetAdd (DsjRectSet* pSet, Rectangle rect);
void RefreshRectangle(Rectangle rect, Window* pWindowToExclude)
{
	KeVerifyInterruptsEnabled;
	
	if (!IsWindowManagerTask())
	{
		WindowAction act;
		act.bInProgress = true;
		act.nActionType = WACT_UNDRAW_RECT;
		act.pWindow = pWindowToExclude;
		act.rect    = rect;
		ActionQueueAdd(act);
		// TODO: maybe we should wait for it to complete?
		return;
	}
	
	// pWnd - the window that's being undrawn
	VBEData* backup = g_vbeData;
	VidSetVBEData(NULL);
	
	/*
	LockAcquire (&g_BackgdLock);
	
	//redraw the background, if needed
	Rectangle* pStart = NULL, *pEnd = NULL;
	WmSplitRectWithWindows(rect, pWindowToExclude, &pStart, &pEnd, false);
	
	for (Rectangle* pRect = pStart; pRect != pEnd; pRect++)
	{
		RedrawBackground(*pRect);
	}
	
	WmSplitDone();
	
	LockFree (&g_BackgdLock);
	*/
	
	// draw the windows below it, in their z-order.
	int sz = 0;
	Window* windowDrawList[WINDOWS_MAX];
	
	sz = 0;
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		int drawOrder = g_windowDrawOrder[i];
		if (drawOrder < 0) continue;
		
		Window* pWindow = &g_windows[drawOrder];
		
		if (RectangleOverlap (&pWindow->m_fullRect, &rect) && pWindow != pWindowToExclude)
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
		
		if (windowDrawList[i]->m_flags & WF_MINIMIZE)
		{
			//TODO?
		}
		else
		{
			Rectangle rint, *pWndRect = &windowDrawList[i]->m_fullRect;
			if (!RectangleIntersect(&rint, &rect, pWndRect))
				continue;
			rint.left   -= pWndRect->left;
			rint.top    -= pWndRect->top;
			rint.right  -= pWndRect->left;
			rint.bottom -= pWndRect->top;
			
			DisjointRectSetAdd(windowDrawList[i]->m_vbeData.m_drs, rint);
			RenderWindow(windowDrawList[i]);
		}
	}
	
	g_vbeData = backup;
}
