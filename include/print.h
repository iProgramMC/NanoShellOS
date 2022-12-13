/*****************************************
		NanoShell Operating System
	   (C) 2020-2021 iProgramInCpp

   Printing and Formatting module header
******************************************/
#ifndef _PRINT_H
#define _PRINT_H

#include <main.h>
#include <stdarg.h>

size_t sprintf  (char* buffer, const char* format, ...);
size_t snprintf (char* buffer, size_t sz, const char* format, ...);
size_t vsprintf (char* buffer, const char* format, va_list list);
size_t vsnprintf(char* buffer, size_t sz, const char* format, va_list list);

void DumpBytesAsHex (void *nAddr, size_t nBytes, bool as_bytes);

#endif//_PRINT_H