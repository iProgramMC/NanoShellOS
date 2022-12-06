//  ***************************************************************
//  a_math.c - Creation date: 06/12/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

int abs (int a)
{
	if (a < 0)
		return -a;
	
	return a;
}

double fabs (double x)
{
	if (x < 0)
		return -x;
	
	return x;
}
