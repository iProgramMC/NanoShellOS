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

void SetMousePos (UNUSED unsigned pX, UNUSED unsigned pY)
{
	//TODO
}

void VidDrawRectangle(unsigned color, Rectangle rect)
{
	VidDrawRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidFillRectangle(unsigned color, Rectangle rect)
{
	VidFillRect(color, rect.left, rect.top, rect.right, rect.bottom);
}
