//  ***************************************************************
//  a_error.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

int g_errorNum = 0;

int SetErrorNumber(int err)
{
	return g_errorNum = err;
}

int* GetErrorNumberPointer()
{
	return &g_errorNum;
}

int GetErrorNumber()
{
	return errno;
}

const char* strerror(int errnum)
{
	return ErrNoStr(errnum);
}
