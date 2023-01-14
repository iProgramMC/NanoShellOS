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

size_t g_windowDepthBufferSzBytes = 0;

void KillWindowDepthBuffer ()
{
	if (g_windowDepthBuffer)
	{
		MmFreeK(g_windowDepthBuffer);
		g_windowDepthBuffer = NULL;
		g_windowDepthBufferSzBytes = 0;
	}
}
void InitWindowDepthBuffer ()
{
	KillWindowDepthBuffer();
	
	g_windowDepthBufferSzBytes = sizeof (short) * GetScreenSizeX() * GetScreenSizeY();
	g_windowDepthBuffer = MmAllocateK(g_windowDepthBufferSzBytes);
	
	ResetWindowDrawOrder ();
}
void SetWindowDepthBuffer (int windowIndex, int x, int y)
{
	if (x < 0 || y < 0 || x >= GetScreenSizeX() || y >= GetScreenSizeY()) return;
	g_windowDepthBuffer[GetScreenSizeX() * y + x] = windowIndex;
}
short GetWindowIndexInDepthBuffer (int x, int y)
{
	if (x < 0 || y < 0 || x >= GetScreenSizeX() || y >= GetScreenSizeY()) return -1;
	short test = g_windowDepthBuffer[GetScreenSizeX() * y + x];
	return test;
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

//TODO: make this really work
void FillDepthBufferWithWindowIndex (Rectangle r, /*uint32_t* framebuffer, */int index, bool *bObscuredOut, bool *bForeMostOut)
{
	*bObscuredOut = true;
	*bForeMostOut = true;
	
	int hx = GetScreenSizeX(), hy = GetScreenSizeY();
	int idxl = r.left, idxr = r.right;
	if (idxl < 0) idxl = 0;
	if (idxr < 0) idxr = 0;
	if (idxl >= hx) return;//completely OOB
	if (idxr >= hx) idxr = hx;
	int gap = idxr - idxl;
	if (gap <= 0) return;//it will never be bigger than zero if it is now
	//int gapdiv2 = gap / 2;
	idxl += hx * r.top;
	idxr += hx * r.top;
	for (int y = r.top; y < r.bottom; y++)
	{
		if (y >= hy) break;//no point.
		if (y < 0) continue;
		
		for (int j = idxl; j != idxr; j++)
			// If this pixel is not occupied by a window yet
			if (g_windowDepthBuffer[j] < 0)
			{
				// occupy it ourselves, and let the WM know that we're not totally occluded
				if (*bObscuredOut)
				{
					*bObscuredOut = false;
				}
				g_windowDepthBuffer[j] = index;
			}
			// It's occupied by the foremost window at that pixel, so this window is no longer considered 'foremost'
			else
			{
				if (*bForeMostOut)
					*bForeMostOut = false;
			}
		
		idxl += hx;
		idxr += hx;
	}
}

void UpdateDepthBuffer (void)
{
	memset_ints (g_windowDepthBuffer, 0xFFFFFFFF, g_windowDepthBufferSzBytes/4);
	
	// Go front to back instead of back to front.
	bool bFilledIn[WINDOWS_MAX] = { 0 };
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		short j = g_windowDrawOrder[i];
		if (j == -1) continue;
		if (!g_windows[j].m_used) continue;
		
		if (g_windows[j].m_hidden) continue;
		
		if (!bFilledIn[j])
		{
			bFilledIn[j] = true;
			FillDepthBufferWithWindowIndex (g_windows[j].m_rect, j, &g_windows[j].m_bObscured, &g_windows[j].m_bForemost);
			/*
			SLogMsg("Window '%s' is %s and %s",
				g_windows[j].m_title,
				g_windows[j].m_bObscured ? "obscured" : "not obscured",
				g_windows[j].m_bForemost ? "foremost" : "not foremost"
			);
			*/
		}
	}
}

void RefreshRectangle(Rectangle rect, Window* pWindowToExclude)
{
	// pWnd - the window that's being undrawn
	VBEData* backup = g_vbeData;
	VidSetVBEData(NULL);
	
	UpdateDepthBuffer();
	
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
