/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

          Resource Launch module
******************************************/
#include <window.h>
#include <wbuiltin.h>
#include <shell.h>
#include <elf.h>
#include <wterm.h>
#include <vfs.h>
#include <resource.h>
#include <print.h>
#include <string.h>

#define STREQ(str1,str2) (!strcmp(str1,str2))

static const char* g_ErrorMsgs [] = {
	"Unknown Resource Launch Error",
	"The specified resource '%s' does not exist.",
	"Insufficient memory to open the resource '%s'. Quit one or more NanoShell applications and then try again.",
	"The resource protocol for the resource '%s' has not been specified.",
	"The resource protocol for the resource '%s' is invalid."
};

const char* GetResourceErrorText (RESOURCE_STATUS state)
{
	if (state < RESOURCE_LAUNCH_NOT_FOUND)
		return g_ErrorMsgs[0];
	state -= RESOURCE_LAUNCH_NOT_FOUND - 1;
	if (state > (int)ARRAY_COUNT (g_ErrorMsgs))
		return g_ErrorMsgs[0];
	return g_ErrorMsgs[state];
}

// Resource invoke definitions:
#if 1

void LaunchExecutable (int fd);
RESOURCE_STATUS CabinetExecute(const char* filename)
{
	// Execute a cabinet file
	int ec1 = 0, ec2 = 0;
	
	ec1 = ElfRunProgram (filename, NULL, true, true, 0, &ec2);
	
	return ec1 == ELF_ERROR_NONE ? RESOURCE_LAUNCH_SUCCESS : RESOURCE_LAUNCH_OUT_OF_MEMORY;
}

RESOURCE_STATUS CabinetExecuteScript(const char* pFileName)
{
	char *buffer = (char*)MmAllocate(512);
	strcpy (buffer, "ec ");
	strcat (buffer, pFileName);
	strcat (buffer, "\n");
	
	// Create the Launch Executable thread with the file descriptor as its parameter.
	int errorCode = 0;
    Task* pTask = KeStartTask (TerminalHostTask, (int)buffer, &errorCode);
    if (errorCode != TASK_SUCCESS)
    {
		MmFree(buffer);
		char buffer1[1024];
        sprintf (buffer1, "Cannot create thread to execute '%s'. Out of memory?", pFileName);
        MessageBox(NULL, buffer1, "Error", ICON_ERROR << 16 | MB_OK);
		
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
    }
	
	// After the task was created, give it a tag.
	KeVerifyInterruptsEnabled;
	cli;
	KeTaskAssignTag(pTask, pFileName);
	sti;
	
	// Consider it done.  LaunchExecutable task shall now MmFree the string allocated.
	return RESOURCE_LAUNCH_SUCCESS;
}

RESOURCE_STATUS NotepadLaunchResource(const char* pResource)
{
	char *buffer = (char*)MmAllocate(512);
	strcpy (buffer, pResource);
	
	int errorCode = 0;
	Task* pTask = KeStartTask(BigTextEntry, (int)buffer, &errorCode);
	if (errorCode != TASK_SUCCESS)
	{
		MmFree(buffer);
		char buffer1[1024];
        sprintf (buffer1, "Cannot create thread to execute '%s'. Out of memory?", pResource);
        MessageBox(NULL, buffer1, "Error", ICON_ERROR << 16 | MB_OK);
		
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	}
	
	// After the task was created, give it a tag.
	KeVerifyInterruptsEnabled;
	cli;
	KeTaskAssignTag(pTask, "Notepad");
	sti;
	
	// Consider it done.  LaunchExecutable task shall now MmFree the string allocated.
	return RESOURCE_LAUNCH_SUCCESS;
}

RESOURCE_STATUS LaunchResourceLauncher(const char* pResourceID);
RESOURCE_STATUS HelpOpenResource(const char* pResourceID);

#endif

// Main interface
#if 1

const RESOURCE_INVOKE g_ResourceInvokes[] = {
	NULL,//RESOURCE_NONE
	NULL,//RESOURCE_FATAL
	LaunchResourceLauncher,//RESOURCE_SHELL
	NotepadLaunchResource,//RESOURCE_TED
	CabinetExecuteScript,//RESOURCE_EXSCRIPT
	NULL,//RESOURCE_EXCONSOLE
	CabinetExecute,//RESOURCE_EXWINDOW
	HelpOpenResource,//RESOURCE_HELP
};

RESOURCE_TYPE ResolveProtocolString(const char* protocol)
{
	if (STREQ(protocol, "fatal"))      return RESOURCE_FATAL;
	if (STREQ(protocol, "shell"))      return RESOURCE_SHELL;
	if (STREQ(protocol, "ted"))        return RESOURCE_TED;
	if (STREQ(protocol, "exscript"))   return RESOURCE_EXSCRIPT;
	if (STREQ(protocol, "exconsole"))  return RESOURCE_EXCONSOLE;
	if (STREQ(protocol, "exwindow"))   return RESOURCE_EXWINDOW;
	if (STREQ(protocol, "help"))       return RESOURCE_HELP;
	return RESOURCE_NONE;
}

RESOURCE_STATUS LaunchResource (const char* pResource)
{
	SLogMsg("LaunchResource: MainArg:\"%s\"", pResource);
	char protocolString [21];
	const char* colonPtr = strchr ((char*)pResource, ':');
	if (colonPtr == NULL)
		return RESOURCE_LAUNCH_NO_PROTOCOL;
	
	int length = colonPtr - pResource;
	if (length > 20)
		return RESOURCE_LAUNCH_INVALID_PROTOCOL;
	memcpy (protocolString, pResource, length);
	protocolString[length] = 0;
	
	pResource = colonPtr + 1;
	
	RESOURCE_TYPE resourceType = ResolveProtocolString(protocolString);
	if (resourceType == RESOURCE_NONE)
		return RESOURCE_LAUNCH_INVALID_PROTOCOL;
	
	SLogMsg("LaunchResource: protocolType:%d, arg:%s", resourceType, pResource);
	
	if (g_ResourceInvokes[resourceType])
		return g_ResourceInvokes[resourceType](pResource);
	else
		return RESOURCE_LAUNCH_INVALID_PROTOCOL;
}

#endif
