//  ***************************************************************
//  a_video.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

int GetWidth (Rectangle* rect)
{
	return rect->right - rect->left;
}

int GetHeight (Rectangle* rect)
{
	return rect->bottom - rect->top;
}

void VidDrawRectangle(unsigned color, Rectangle rect)
{
	VidDrawRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidFillRectangle(unsigned color, Rectangle rect)
{
	VidFillRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

bool RectangleContains(Rectangle*r, Point*p) 
{
	return (r->left <= p->x && r->right >= p->x && r->top <= p->y && r->bottom >= p->y);
}

bool RectangleOverlap(Rectangle *r1, Rectangle *r2)
{
	return (r1->left <= r2->right && r1->right >= r2->left && r1->top <= r2->bottom && r1->bottom >= r2->top);
}

void VidSetClipRect(Rectangle* x)
{
	if (x)
	{
		VidSetClipRectP(*x);
	}
	else
	{
		Rectangle r;
		r.left   = -1;
		r.top    = -1;
		r.right  = -1;
		r.bottom = -1;
		VidSetClipRectP(r);
	}
}
