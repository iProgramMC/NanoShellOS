//  ***************************************************************
//  crtinternal.h - Creation date: 21/04/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _CRTINTERNAL_H
#define _CRTINTERNAL_H

#define CALL(funcName, funcIndex, retType, ...) retType funcName (__VA_ARGS__);
#define CALLI(funcName, funcIndex, retType, ...) retType _I_ ## funcName (__VA_ARGS__);
#define RARGS(...)
#define SARGS(...)
#define CALL_END

#include "calldefs.h"

#undef CALL
#undef CALLI
#undef RARGS
#undef SARGS
#undef CALL_END

// currently, this does nothing. However, it's safe to say we will need it when we
// move to a multithreaded version of this libc
#define THREAD_LOCAL 

#endif//_CRTINTERNAL_H