//  ***************************************************************
//  a_assert.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

void OnAssertionFail(const char *cond_msg, const char *file, int line)
{
	LogMsg("ASSERTION FAILED!");
	LogMsg("The assertion \"%s\" failed at %s:%d.", cond_msg, file, line);
	LogMsg("The program will now exit.");
	exit(1);
}
