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

void WmClearRectangleStack()
{
	g_WmRectangleStackIndex = 0;
}

void WmRemoveRectangleInStack(int spot)
{
	g_WmRectangleStack[spot] = g_WmRectangleStack[--g_WmRectangleStackIndex];
}

//note: the rectangle MUST be from the stack.
void WmRemoveRectangleFromStack(Rectangle* rect)
{
	WmRemoveRectangleInStack((int)(rect - g_WmRectangleStack));
}

int WmAddRectangle(Rectangle* rect)
{
	if (rect->left >= rect->right || rect->top >= rect->bottom)
	{
		SLogMsg("INVALID RECT!! Left %d, Right %d, Top %d, Bottom %d", rect->left, rect->right, rect->top, rect->bottom);
		return -1;
	}
	
	if (g_WmRectangleStackIndex >= C_RECTANGLE_STACK_SIZE)
		return -1;

	g_WmRectangleStack[g_WmRectangleStackIndex] = *rect;
	return g_WmRectangleStackIndex++;
}

void WmSplitAlongHoriz(Rectangle* rectToSplit, int left, int right, Rectangle* origRect)
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

void WmSplitAlong(Rectangle* rectToSplit, int top, int bottom, int left, int right, Rectangle* origRect)
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

void WmSplitRectangleStackByWindow(Window* pWindow)
{
	Rectangle rect = pWindow->m_rect;

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

void WmSplitRectangle(Rectangle ogRect, const Window* pExcept, Rectangle** pStartOut, Rectangle** pEndOut)
{
	WmClearRectangleStack();
	WmAddRectangle(&ogRect);
	
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (!g_windows[i].m_used) continue;
		if (&g_windows[i] == pExcept) continue;
		
		WmSplitRectangleStackByWindow(&g_windows[i]);
	}
	
	*pStartOut = &g_WmRectangleStack[0];
	*pEndOut   = &g_WmRectangleStack[g_WmRectangleStackIndex];
}
