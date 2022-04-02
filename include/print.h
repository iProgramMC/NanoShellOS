/*****************************************
		NanoShell Operating System
	   (C) 2020-2021 iProgramInCpp

   Printing and Formatting module header
******************************************/
#ifndef _PRINT_H
#define _PRINT_H

#include <main.h>
#include <stdarg.h>

void sprintf(char*a, const char*c, ...);
void vsprintf(char* memory, const char* format, va_list list);

void DumpBytesAsHex (void *nAddr, size_t nBytes, bool as_bytes);

#endif//_PRINT_H