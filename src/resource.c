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
	
	// After the task was created, give it a tag and start it.
	KeTaskAssignTag(pTask, pFileName);
	KeUnsuspendTask(pTask);
	
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
	
	// After the task was created, give it a tag and start it.
	KeTaskAssignTag(pTask, "Notepad");
	KeUnsuspendTask(pTask);
	
	// Consider it done.  LaunchExecutable task shall now MmFree the string allocated.
	return RESOURCE_LAUNCH_SUCCESS;
}

RESOURCE_STATUS ScribbleLaunchResource(const char* pResource)
{
	char *buffer = (char*)MmAllocate(512);
	strcpy (buffer, pResource);
	
	int errorCode = 0;
	Task* pTask = KeStartTask(PrgPaintTask, (int)buffer, &errorCode);
	if (errorCode != TASK_SUCCESS)
	{
		MmFree(buffer);
		char buffer1[1024];
        sprintf (buffer1, "Cannot create thread to execute '%s'. Out of memory?", pResource);
        MessageBox(NULL, buffer1, "Error", ICON_ERROR << 16 | MB_OK);
		
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	}
	
	// After the task was created, give it a tag and start it.
	KeTaskAssignTag(pTask, "Scribble");
	KeUnsuspendTask(pTask);
	
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
	ScribbleLaunchResource,//RESOURCE_IMAGE
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
	if (STREQ(protocol, "image"))      return RESOURCE_IMAGE;
	return RESOURCE_NONE;
}

RESOURCE_STATUS LaunchResource (const char* pResource)
{
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
	
	if (g_ResourceInvokes[resourceType])
		return g_ResourceInvokes[resourceType](pResource);
	else
		return RESOURCE_LAUNCH_INVALID_PROTOCOL;
}

// TODO: Allow loading these from a file.
FileAssociation g_FileAssociations[] =
{
	// things that match everything with a certain flag.
	{ "*", "", "Local Disk",       ICON_HARD_DRIVE,  FILE_TYPE_MOUNTPOINT    },
	{ "*", "", "File folder",      ICON_FOLDER,      FILE_TYPE_DIRECTORY     },
	{ "*", "", "Symbolic link",    ICON_CHAIN,       FILE_TYPE_SYMBOLIC_LINK },
	{ "*", "", "Character device", ICON_SERIAL,      FILE_TYPE_CHAR_DEVICE   }, // not all char devices are serial, but meh.
	
	{ "*.txt", "ted",      "Text Document",         ICON_TEXT_FILE,    0 },
	{ "*.ini", "ted",      "Configuration File",    ICON_TEXT_FILE,    0 },
	{ "*.nse", "exwindow", "Application",           ICON_APPLICATION,  0 },
	{ "*.sc",  "exscript", "NanoShell Script File", ICON_FILE_CSCRIPT, 0 },
	{ "*.c",   "ted",      "C Source File",         ICON_FILE_CSCRIPT, 0 },
	{ "*.cpp", "ted",      "C++ Source File",       ICON_FILE_CSCRIPT, 0 }, // questionable. I don't know if we'll be using C++ anytime soon in this project at all
	{ "*.h",   "ted",      "C/C++ Header File",     ICON_TEXT_FILE,    0 },
	{ "*.md",  "help",     "Help Document",         ICON_FILE_MKDOWN,  0 },
	{ "*.bmp", "image",    "BMP File",              ICON_FILE_IMAGE,   0 },
	{ "*.tga", "image",    "TGA File",              ICON_FILE_IMAGE,   0 },
	{ "*.tar", "unknown",  "Tape Archive File",     ICON_TAR_ARCHIVE,  0 },
};

FileAssociation g_DefaultAssociation = {
	"*", "unknown", "File", ICON_FILE, 0
};

FileAssociation* ResolveAssociation(const char* fileName, int fileType)
{
	// a simple loop will do since there are (right now) few entries:
	for (int i = 0; i < (int)ARRAY_COUNT(g_FileAssociations); i++)
	{
		FileAssociation* pAssoc = &g_FileAssociations[i];
		
		// In the case of pAssoc->type_mask being zero, this will return 'false' for any fileType.
		if ((fileType & pAssoc->type_mask) != pAssoc->type_mask) continue;
		
		// check if the wildcard matches.
		if (!WildcardMatches(pAssoc->match, fileName)) continue;
		
		// wildcard matches fine. So,  return
		return pAssoc;
	}
	
	return &g_DefaultAssociation;
}

RESOURCE_STATUS LaunchFileOrResource(const char* pResource)
{
	RESOURCE_STATUS state = LaunchResource(pResource);
	
	// If a protocol WAS specified, this MUST mean that we failed for a different reason, or succeeded.
	if (state != RESOURCE_LAUNCH_NO_PROTOCOL) 
		return state;
	
	// no protocol was specified:
	StatResult sr;
	int res = FiStat(pResource, &sr);
	if (res < 0)
		return RESOURCE_LAUNCH_NOT_FOUND;
	
	FileAssociation* pAssoc = ResolveAssociation(pResource, sr.m_type);
	
	RESOURCE_TYPE resourceType = ResolveProtocolString(pAssoc->protocol);
	if (resourceType == RESOURCE_NONE)
		return RESOURCE_LAUNCH_INVALID_PROTOCOL;
	
	if (!g_ResourceInvokes[resourceType])
		return RESOURCE_LAUNCH_INVALID_PROTOCOL;
	
	return g_ResourceInvokes[resourceType](pResource);
}

#endif
