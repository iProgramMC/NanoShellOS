/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

    Resource Launch module header file
******************************************/
#ifndef _RESOURCE_H
#define _RESOURCE_H

typedef int RESOURCE_STATUS;
typedef int RESOURCE_TYPE;
typedef RESOURCE_STATUS (*RESOURCE_INVOKE)(const char* pArgument);

enum
{
	RESOURCE_NONE,
	RESOURCE_FATAL,
	RESOURCE_SHELL,
	RESOURCE_TED,
	RESOURCE_EXSCRIPT,
	RESOURCE_EXCONSOLE,
	RESOURCE_EXWINDOW,
	RESOURCE_HELP,
};

enum
{
	RESOURCE_LAUNCH_SUCCESS = 0,
	RESOURCE_LAUNCH_NOT_FOUND = 0x10000001,
	RESOURCE_LAUNCH_OUT_OF_MEMORY,
	RESOURCE_LAUNCH_NO_PROTOCOL,
	RESOURCE_LAUNCH_INVALID_PROTOCOL,
	
	//other errors...
};

RESOURCE_STATUS LaunchResource (const char* pResource);

#endif//_RESOURCE_H