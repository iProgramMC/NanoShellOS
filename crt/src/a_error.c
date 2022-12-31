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

// note: The error number is stored as positive.
int g_errorNum = 0;

static __attribute__((always_inline))
int absolute_value(int a)
{
	return a < 0 ? -a : a;
}

int SetErrorNumber(int err)
{
	return g_errorNum = absolute_value(err);
}

int* GetErrorNumberPointer()
{
	return &g_errorNum;
}

int GetErrorNumber()
{
	return g_errorNum;
}

const char* strerror(int errnum)
{
	return ErrNoStr(-errnum);
}

int seterrno(int en)
{
	return g_errorNum = absolute_value(en);
}

int geterrno()
{
	return g_errorNum;
}

int* geterrnoptr()
{
	return &g_errorNum;
}
