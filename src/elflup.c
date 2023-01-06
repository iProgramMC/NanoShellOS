//  ***************************************************************
//  elflup.c - Creation date: 06/01/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
//
//        Module description:
//    This module is responsible for the lookup of kernel side
//  functions. The reason I split this up and only include the
//  bare basics is  because in the DO_CALL function,  I define
//  functions as  extern void(void).  This will  conflict with
//  any headers  that would  be included here,  which is why I
//  decided to split this into a separate file.
//
//  ***************************************************************
#include <stdint.h>
#include <elf.h> // will we ever add ElfRunProgram, ElfGetErrorMsg and ExLookUpSymbol to here? probably not.
void SLogMsg(const char*, ...);

uint32_t ElfLookUpKernelCall(const char* pCall, uint32_t* pOut)
{
	SLogMsg("ElfLookUpKernelCall('%s')", pCall);
	
	// TODO: do this in a better way
	
	#define DO_CALL(fnName) do {           \
		if (strcmp(pCall, #fnName) == 0) { \
			extern void fnName(void);      \
			*pOut = (uint32_t)&fnName;     \
			return ELF_ERROR_NONE;         \
		}                                  \
	} while (0)
	
	DO_CALL(LogString);
	DO_CALL(GetVersionNumber);
	
	return ELF_IMPORT_NOT_FOUND;
}