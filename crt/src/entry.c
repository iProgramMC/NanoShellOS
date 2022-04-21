//
// log.c
//
// Copyright (C) 2022 iProgramInCpp.
//
// The standard NanoShell library internal implementation.
// DO NOT MODIFY!  Changing these files may affect the stability of programs.
// -- Entry Point.
//

#include "crtlib.h"
#include "crtinternal.h"

__attribute__((noreturn)) void exit (int number);

int NsMain (int argc, char** argv);

__attribute__((noreturn))
void _CEntry(const char* arg)
{
	char* argv[128]; 
	argv[0] = NULL;
	
	int returnValue;
	
	// No arguments? Just call with argc of 0
	if (*arg == 0)
	{
		returnValue = NsMain (0, argv);
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
	returnValue = NsMain (argc, argv);
	exit(returnValue);
	
	// no return.
}

