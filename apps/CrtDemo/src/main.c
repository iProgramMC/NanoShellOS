/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
        C Runtime Demo application

             Main source file
******************************************/
#include <nsstandard.h>

int main(int argc, char** argv)
{
	LogMsg("Hello, world! Argument count: '%d'", argc);
	
	for (int i = 0; i < argc; i++)
	{
		LogMsg("ARG: \"%s\"", argv[i]);
	}
	
	sleep(3000);
	
	LogMsg("Allocating something");
	// Allocate an amount of memory
	void* memory = malloc(8192);
	LogMsg("Got Memory: %p", memory);
	
	memset (memory, 0, 8192);
	
	// Sleep for a small amount of time
	sleep(5000);
	
	LogMsg("Quitting!");
	// Exit, without removing memory
	// This should demonstrate the CRT's capability of self -cleanup.
	exit (1);
	
	LogMsg("???");
	
	return 0;
}
