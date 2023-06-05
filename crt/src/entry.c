//  ***************************************************************
//  entry.c - Creation date: 21/04/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

__attribute__((noreturn))
void abort()
{
	*((uint32_t*)0xFFFFFFF4) = 0xFFFFFFFF;
	while (true);
}

__attribute__((noreturn)) void exit (int number);

int main(int argc, char** argv);
void MemMgrInitializeMemory();

void _I_Setup();

__attribute__((noreturn))
void _CEntry(const char* arg)
{
	MemMgrInitializeMemory();
	
	_I_Setup();
	
	char* argv[128]; 
	argv[0] = NULL;
	
	int returnValue;
	
	// No arguments? Just call with argc of 0
	if (*arg == 0)
	{
		returnValue = main(0, argv);
		exit (returnValue);
	}
	
	// Have arguments.  Copy argument string
	int slen = strlen (arg);
	char string [slen + 1];
	strcpy (string, arg);
	
	// Tokenize passed in argument string.
	int argc = 0;
	
	TokenState pState;
	memset (&pState, 0, sizeof pState);
	
	argv[argc++] = Tokenize (&pState, string, " ");
	while ((argv[argc++] = Tokenize (&pState, NULL, " ")));
	if (argv[argc - 1] == NULL)
		argc--;
	
	// Finally call the real entry point
	returnValue = main(argc, argv);
	exit(returnValue);
	
	// no return.
}

