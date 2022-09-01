//  ***************************************************************
//  calls.c - Creation date: 21/04/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"

#define WINDOW_CALL_BASE 0xc0007c00

// Type Definitions.
#define CALL(funcName, funcIndex, retType, ...)\
	typedef retType(* P_I_ ## funcName) (__VA_ARGS__);
#define CALLI(funcName, funcIndex, retType, ...)\
	typedef retType(* P_I_ ## funcName) (__VA_ARGS__);
#define RARGS(...)
#define SARGS(...)
#define CALL_END

#include "calldefs.h"

#undef CALL
#undef CALLI
#undef RARGS
#undef SARGS
#undef CALL_END

// Wrapper function definitions.
// CALL  = Function that's available for use.
// CALLI = Internal function that shouldn't be used.
#define CALL(funcName, funcIndex, retType, ...)\
	retType funcName (__VA_ARGS__)\
	{\
		*((uint32_t*)(WINDOW_CALL_BASE + 0xFC)) = funcIndex;\
		P_I_ ## funcName p_func = (P_I_ ## funcName)WINDOW_CALL_BASE;\
		
#define CALLI(funcName, funcIndex, retType, ...)\
	retType _I_ ## funcName (__VA_ARGS__)\
	{\
		*((uint32_t*)(WINDOW_CALL_BASE + 0xFC)) = funcIndex;\
		P_I_ ## funcName p_func = (P_I_ ## funcName)WINDOW_CALL_BASE;\
		
#define RARGS(...)\
		return p_func (__VA_ARGS__);\
	}	
#define SARGS(...)\
		p_func (__VA_ARGS__);\
	}
#define CALL_END //empty, use if needed

#include "calldefs.h"

#undef CALL
#undef RARGS
#undef SARGS
#undef CALL_END


