//
// crt.c
//
// Copyright (C) 2022 iProgramInCpp.
//
// Internal function definitions
//

#ifndef _CRTINTERNAL_H
#define _CRTINTERNAL_H

#define CALL(funcName, funcIndex, retType, ...) retType _I_ ## funcName (__VA_ARGS__);
#define RARGS(...)
#define SARGS(...)
#define CALL_END

#include "wincalls.h"

#undef CALL
#undef RARGS
#undef SARGS
#undef CALL_END

#endif//_CRTINTERNAL_H