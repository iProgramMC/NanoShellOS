/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

       Window Rectangle Stack Module
******************************************/
#include "wi.h"

// If r2 is entirely contained within r1
bool RectangleFullOverlap(Rectangle *r1, Rectangle *r2)
{
	return (r1->left <= r2->left && r1->top <= r2->top && r1->right >= r2->right && r1->bottom >= r2->bottom);
}

#define C_RECTANGLE_STACK_SIZE (16384)

// Note: The order does NOT matter, everything will be used anyway. Analysis of complexity:
// O(1): Adding a single element
// O(1): Removing a single element
// O(n): Lookup

Rectangle* g_WmRectangleStack = NULL;
int g_WmRectangleStackIndex;

void WmCreateRectangleStack()
{
	g_WmRectangleStack = (Rectangle*)WmCAllocate(C_RECTANGLE_STACK_SIZE * sizeof(Rectangle));
}

void WmFreeRectangleStack()
{
	SAFE_DELETE(g_WmRectangleStack);
	g_WmRectangleStack = NULL;
}

static void WmClearRectangleStack()
{
	g_WmRectangleStackIndex = 0;
}

static void WmRemoveRectangleInStack(int spot)
{
	g_WmRectangleStack[spot] = g_WmRectangleStack[--g_WmRectangleStackIndex];
}

//note: the rectangle MUST be from the stack.
static void WmRemoveRectangleFromStack(Rectangle* rect)
{
	WmRemoveRectangleInStack((int)(rect - g_WmRectangleStack));
}

static int WmAddRectangle(Rectangle* rect)
{
	if (rect->left > rect->right || rect->top > rect->bottom)
	{
		SLogMsg("INVALID RECT!! Left %d, Right %d, Top %d, Bottom %d", rect->left, rect->right, rect->top, rect->bottom);
		return -1;
	}
	
	if (rect->left == rect->right || rect->top == rect->bottom)
		return -1;
	
	if (g_WmRectangleStackIndex >= C_RECTANGLE_STACK_SIZE)
		return -1;

	g_WmRectangleStack[g_WmRectangleStackIndex] = *rect;
	return g_WmRectangleStackIndex++;
}

static void WmSplitAlongHoriz(Rectangle* rectToSplit, int left, int right, Rectangle* origRect)
{
	Rectangle* rects[3] = { NULL };
	int rectcnt = 0;

	rects[rectcnt++] = rectToSplit;

	// Split horizontally along the left.
	if (rectToSplit->left < left && left < rectToSplit->right)
	{
		Rectangle leftHalf = *rectToSplit, rightHalf = *rectToSplit;
		leftHalf.right = left;
		rightHalf.left = left;

		*rectToSplit = leftHalf;

		int spot = WmAddRectangle(&rightHalf);
		if (spot < 0)
		{
			SLogMsg("couldn't add rectangle to stack (1)...");
			return;
		}
		rects[rectcnt++] = rectToSplit = &g_WmRectangleStack[spot];
	}

	if (rectToSplit->left < right && right < rectToSplit->right)
	{
		// Split horizontally along the right.
		Rectangle leftHalf = *rectToSplit, rightHalf = *rectToSplit;
		leftHalf.right = right;
		rightHalf.left = right;

		*rectToSplit = leftHalf;
		int spot = WmAddRectangle(&rightHalf);
		if (spot < 0)
		{
			SLogMsg("couldn't add rectangle to stack (2)...");
			return;
		}
		rects[rectcnt++] = &g_WmRectangleStack[spot];
	}

	for (int i = 0; i < rectcnt; i++)
	{
		if (RectangleFullOverlap(origRect, rects[i]))
		{
			WmRemoveRectangleFromStack(rects[i]);
		}
	}
}

static void WmSplitAlong(Rectangle* rectToSplit, int top, int bottom, int left, int right, Rectangle* origRect)
{
	Rectangle* rects[3] = { NULL };
	int rectcnt = 0;

	rects[rectcnt++] = rectToSplit;
	
	if (RectangleFullOverlap(origRect, rectToSplit))
	{
		return;
	}
	
	// Split horizontally along the top.
	if (rectToSplit->top < top && top < rectToSplit->bottom)
	{
		Rectangle topHalf = *rectToSplit, bottomHalf = *rectToSplit;
		topHalf.bottom = top;
		bottomHalf.top = top;

		*rectToSplit = topHalf;

		int spot = WmAddRectangle(&bottomHalf);
		if (spot < 0)
		{
			SLogMsg("couldn't add rectangle to stack (3)...");
			return;
		}
		rects[rectcnt++] = rectToSplit = &g_WmRectangleStack[spot];
	}

	if (rectToSplit->top < bottom && bottom < rectToSplit->bottom)
	{
		// Split horizontally along the bottom.
		Rectangle topHalf = *rectToSplit, bottomHalf = *rectToSplit;
		topHalf.bottom = bottom;
		bottomHalf.top = bottom;

		*rectToSplit = topHalf;
		int spot = WmAddRectangle(&bottomHalf);
		if (spot < 0)
		{
			SLogMsg("couldn't add rectangle to stack (4)...");
			return;
		}
		rects[rectcnt++] = &g_WmRectangleStack[spot];
	}

	for (int i = 0; i < rectcnt; i++)
	{
		WmSplitAlongHoriz(rects[i], left, right, origRect);
	}
}

static void WmSplitRectStackByRect(Rectangle rect)
{
	for (int i = 0; i < g_WmRectangleStackIndex; i++)
	{
		Rectangle* rectToSplit = &g_WmRectangleStack[i];
		// if there's no overlap, then there's no point
		if (!RectangleOverlap(rectToSplit, &rect)) continue;
		// if the window rectangle entirely covers it, cover and continue
		if (RectangleFullOverlap(&rect, rectToSplit))
		{
			WmRemoveRectangleInStack(i);
			i--;
			continue;
		}

		WmSplitAlong(rectToSplit, rect.top, rect.bottom, rect.left, rect.right, & rect);
	}
}

static void WmSplitRectStackByWindow(Window* pWindow)
{
	WmSplitRectStackByRect(pWindow->m_fullRect);
}

SafeLock g_RectSplitLock;

void WmSplitRectWithWindows(Rectangle ogRect, const Window* pThisWindow, Rectangle** pStartOut, Rectangle** pEndOut, bool bIgnoreStuffBeforeExcludedWindow)
{
	LockAcquire(&g_RectSplitLock);
	
	WmClearRectangleStack();
	WmAddRectangle(&ogRect);
	
	bool bFilledIn[WINDOWS_MAX] = { 0 };
	
	bool bReachedThisWindow = false;
	if (!pThisWindow)
		bReachedThisWindow = true;
	
	// TODO: Replace this window crap with a linked list.
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		short order = g_windowDrawOrder[i];
		if (order < 0) continue;
		if (!g_windows[order].m_used) continue;
		if ( g_windows[order].m_hidden) continue;
		if (&g_windows[order] == pThisWindow)
		{
			bReachedThisWindow = true;
			continue;
		}
		
		if ((bReachedThisWindow || !bIgnoreStuffBeforeExcludedWindow) && !bFilledIn[order])
		{
			WmSplitRectStackByWindow(&g_windows[order]);
			bFilledIn[order] = true;
		}
	}
	
	*pStartOut = &g_WmRectangleStack[0];
	*pEndOut   = &g_WmRectangleStack[g_WmRectangleStackIndex];
}

void WmSplitRectWithRectList(Rectangle ogRect, const Rectangle* rectList, int nRectangles, Rectangle** pStartOut, Rectangle** pEndOut)
{
	LockAcquire(&g_RectSplitLock);
	
	WmClearRectangleStack();
	WmAddRectangle(&ogRect);
	
	for (int i = 0; i < nRectangles; i++)
	{
		WmSplitRectStackByRect(rectList[i]);
	}
	
	*pStartOut = &g_WmRectangleStack[0];
	*pEndOut   = &g_WmRectangleStack[g_WmRectangleStackIndex];
}

// call this when you are done processing the results of a successful WmSplitRect.

void WmSplitDone()
{
	LockFree(&g_RectSplitLock);
}
