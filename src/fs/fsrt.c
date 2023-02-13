/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        File System Runtime module
******************************************/

/**
 * This module is responsible for the API calls exposed
 * to the user. For now, they simply redirect to the Fr*
 * calls.
 */
#include <vfs.h>
#include <string.h>
#include <memory.h>
#include <task.h>
#include <misc.h>

extern char g_cwd[];

// Thread unsafe file system functions:
void FrCloseAllFilesFromTask(void* task);
int FrOpenInternal(const char* pFileName, FileNode* pFileNode, int oflag, const char* srcFile, int srcLine);
int FrClose(int fd);
int FrOpenDirD(const char* pFileName, const char* srcFile, int srcLine);
int FrCloseDir(int dd);
DirEnt* FrReadDir(int dd);
int FrSeekDir(int dd, int loc);
int FrTellDir(int dd);
int FrStatAt (int dd, const char *pFileName, StatResult* pOut);
int FrStat(const char *pFileName, StatResult* pOut);
size_t FrRead (int fd, void *pBuf, int nBytes);
size_t FrWrite(int fd, void *pBuf, int nBytes);
int FrIoControl(int fd, unsigned long request, void * argp);
int FrSeek (int fd, int offset, int whence);
int FrTell (int fd);
int FrTellSize(int fd);
int FrUnlinkInDir(const char* pDirName, const char* pFileName);
int FrChangeDir(const char *pfn);
int FrRename(const char* pDirOld, const char* pNameOld, const char* pDirNew, const char* pNameNew);
int FrMakeDir(const char* pDirName, const char* pFileName);
int FrRemoveDir(const char* pPath);

SafeLock g_FileSystemLock;

// Thread safe wrappers for thread unsafe functions:

static int FiOpenInternal(const char* pFileName, FileNode* pFileNode, int oflag, const char* srcFile, int srcLine)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrOpenInternal(pFileName, pFileNode, oflag, srcFile, srcLine);
	});
	return returnValue;
}

int FiClose(int fd)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrClose(fd);
	});
	return returnValue;
}

int FiOpenDirD (const char* pFileName, const char* srcFile, int srcLine)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrOpenDirD(pFileName, srcFile, srcLine);
	});
	return returnValue;
}

int FiCloseDir(int dd)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrCloseDir(dd);
	});
	return returnValue;
}

DirEnt* FiReadDir(int dd)
{
	DirEnt* returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrReadDir(dd);
	});
	return returnValue;
}

int FiSeekDir (int dd, int loc)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrSeekDir(dd, loc);
	});
	return returnValue;
}

int FiTellDir (int dd)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrTellDir(dd);
	});
	return returnValue;
}

int FiStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrStatAt(dd, pFileName, pOut);
	});
	return returnValue;
}

int FiStat(const char *pFileName, StatResult* pOut)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrStat(pFileName, pOut);
	});
	return returnValue;
}

size_t FiRead (int fd, void *pBuf, int nBytes)
{
	size_t returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrRead(fd, pBuf, nBytes);
	});
	return returnValue;
}

size_t FiWrite(int fd, void *pBuf, int nBytes)
{
	size_t returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrWrite(fd, pBuf, nBytes);
	});
	return returnValue;
}

int FiIoControl(int fd, unsigned long request, void * argp)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrIoControl(fd, request, argp);
	});
	return returnValue;
}

int FiSeek (int fd, int offset, int whence)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrSeek(fd, offset, whence);
	});
	return returnValue;
}

int FiTell (int fd)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrTell(fd);
	});
	return returnValue;
}

int FiTellSize(int fd)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrTellSize(fd);
	});
	return returnValue;
}

int FiUnlinkInDir(const char* pDirPath, const char* pFileName)
{
	int returnValue;
	USING_LOCK(&g_FileSystemLock, {
		returnValue = FrUnlinkInDir(pDirPath, pFileName);
	});
	return returnValue;
}

// Composite functions that combine API calls.

int FiOpenD(const char* pFileName, int oflag, const char* srcFile, int srcLine)
{
	// if the file path isn't absolute, make it
	if (*pFileName == '/')
	{
		return FiOpenInternal(pFileName, NULL, oflag, srcFile, srcLine);
	}
	else
	{
		char path[PATH_MAX * 2];
		strcpy(path, g_cwd);
		if (g_cwd[1] != 0) // not just '/'
			strcat(path, "/");
		strcat(path, pFileName);
		return FiOpenInternal(path, NULL, oflag, srcFile, srcLine);
	}
}

int FiOpenFileNodeD(FileNode* pFileNode, int oflag, const char* srcFile, int srcLine)
{
	return FiOpenInternal(NULL, pFileNode, oflag, srcFile, srcLine);
}

int FiRewindDir (int dd)
{
	return FiSeekDir (dd, 0);
}

// File Crash Handler
void FiReleaseResourcesFromTask(void * task)
{
	// check if we actually crashed during a file system operation...
	cli;
	if (g_FileSystemLock.m_held && g_FileSystemLock.m_task_owning_it == task)
	{
		SLogMsg("!!!!!!!!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!");
		SLogMsg("An I/O operation has failed within the task that crashed.");
		SLogMsg("This message is supposed to look pompous like this to be easily");
		SLogMsg("noticeable within the peripheral vision of the user, if they're");
		SLogMsg("running this in QEMU.");
		SLogMsg("NOTE: The lock will be unlocked, but the file system may be left");
		SLogMsg("in a state we can't really recover from!");
		
		g_FileSystemLock.m_held           = false;
		g_FileSystemLock.m_task_owning_it = NULL;
	}
	sti;
	
	LockAcquire(&g_FileSystemLock);
	FrCloseAllFilesFromTask(task);
	LockFree(&g_FileSystemLock);
}

int FiUnlinkFile (const char *pfn)
{
	char buffer[PATH_MAX];
	if (strlen (pfn) >= PATH_MAX - 1) return -ENAMETOOLONG;
	
	// is this a relative path?
	if (*pfn != '/')
	{
		// append the relative path
		if (strlen (pfn) + strlen (g_cwd) >= PATH_MAX - 2) return -EOVERFLOW;
		
		strcpy(buffer, g_cwd);
		if (strcmp(g_cwd, "/") != 0)
			strcat(buffer, "/");
		strcat(buffer, pfn);
		
		return FiUnlinkFile(buffer);
	}
	
	// copy up until the last /
	char* r = strrchr(pfn, '/');
	
	const char* ptr = pfn;
	char *head = buffer;
	
	while (ptr != r) *head++ = *ptr++;
	
	// add the null terminator
	*head = 0;
	
	if (buffer[0] == 0)
		buffer[0] = '/', buffer[1] = 0;
	
	return FiUnlinkInDir(buffer, r + 1);
}

int FiChangeDir(const char* pfn)
{
	if (*pfn == '\0') return -ENOTHING;//TODO: maybe cd into their home directory instead?
	
	int slen = strlen (pfn);
	if (slen >= PATH_MAX) return -EOVERFLOW;
	
	if (pfn[0] == '/')
	{
		// Already an absolute path.
		int rv;
		USING_LOCK(&g_FileSystemLock, {
			rv = FrChangeDir(pfn);
		});
		return rv;
	}
	
	if (strcmp (pfn, PATH_THISDIR) == 0) return -ENOTHING;
	
	char cwd_work [PATH_MAX + 2];
	memset (cwd_work, 0, sizeof cwd_work);
	strcpy (cwd_work, g_cwd);
	
	//TODO FIXME: make composite paths like "../../test/file" work -- Partially works, but only because ext2 is generous enough to give us . and .. entries
	
	if (strcmp (pfn, PATH_PARENTDIR) == 0)
	{
		for (int i = PATH_MAX - 1; i >= 0; i--)
		{
			//get rid of the last segment
			if (cwd_work[i] == PATH_SEP)
			{
				cwd_work[i + (i == 0)] = 0;
				break;
			}
		}
	}
	else
	{
		if (strlen (cwd_work) + slen + 5 >= PATH_MAX)
			return -EOVERFLOW;//path would be too large
		
		if (cwd_work[1] != 0) //i.e. not just a '/'
		{
			strcat (cwd_work, "/");
		}
		strcat (cwd_work, pfn);
	}
	
	int rv;
	USING_LOCK(&g_FileSystemLock, {
		rv = FrChangeDir(cwd_work);
	});
	
	return rv;
}

static int FiMakeDirSub(char* pPath)
{
	char* pSlashPtr = strrchr(pPath, '/');
	if (!pSlashPtr)
		return -EINVAL;
	
	*pSlashPtr = 0;
	
	char* pDirName = pPath, *pFileName = pSlashPtr + 1;
	
	int rv;
	USING_LOCK(&g_FileSystemLock, {
		rv = FrMakeDir(pDirName, pFileName);
	});
	return rv;
}

//note: This only works with absolute paths that have been checked for length.
int FiRenameSub(const char* pfnOld, const char* pfnNew)
{
	char bufferOld[PATH_MAX], bufferNew[PATH_MAX];
	
	strcpy(bufferOld, pfnOld);
	strcpy(bufferNew, pfnNew);
	
	char *pSlashOld, *pSlashNew;
	pSlashOld = strrchr(bufferOld, '/');
	pSlashNew = strrchr(bufferNew, '/');
	
	//well, these SHOULD be paths with at least one slash inside.
	ASSERT(pSlashOld && pSlashNew);
	
	*pSlashOld = *pSlashNew = 0;
	
	char* pDirOld = bufferOld, *pDirNew = bufferNew, *pNameOld = pSlashOld + 1, *pNameNew = pSlashNew + 1;
	
	int rv;
	USING_LOCK(&g_FileSystemLock, {
		rv = FrRename(pDirOld, pNameOld, pDirNew, pNameNew);
	});
	
	return rv;
}

int FiRename(const char* pfnOld, const char* pfnNew)
{
	// not a relative path
	if (pfnOld[0] != '/')
	{
		char buffer[PATH_MAX];
		if (strlen(pfnOld) + strlen(g_cwd) + 2 >= PATH_MAX) return -ENAMETOOLONG;
		
		strcpy(buffer, g_cwd);
		if (strcmp(g_cwd, "/") != 0)
			strcat(buffer, "/");
		strcat(buffer, pfnOld);
		
		return FiRename(buffer, pfnNew);
	}
	
	if (pfnNew[0] != '/')
	{
		char buffer[PATH_MAX];
		if (strlen(pfnNew) + strlen(g_cwd) + 2 >= PATH_MAX) return -ENAMETOOLONG;
		
		strcpy(buffer, g_cwd);
		if (strcmp(g_cwd, "/") != 0)
			strcat(buffer, "/");
		strcat(buffer, pfnNew);
		
		return FiRename(pfnOld, buffer);
	}
	
	if (strlen(pfnOld) >= PATH_MAX) return -ENAMETOOLONG;
	if (strlen(pfnNew) >= PATH_MAX) return -ENAMETOOLONG;
	
	// If they're the same file, just don't do anything
	if (strcmp(pfnOld, pfnNew) == 0) return -ENOTHING;
	
	return FiRenameSub(pfnOld, pfnNew);
}

int FiMakeDir(const char* pPath)
{
	if (*pPath == 0) return -ENOENT;
	
	char buf[PATH_MAX + 1];
	
	if (*pPath == '/')
	{
		if (strlen (pPath) >= PATH_MAX) return -ENAMETOOLONG;
		
		strcpy( buf, pPath );
	}
	else
	{
		if (strlen (pPath) + strlen (g_cwd) + 1 >= PATH_MAX - 2) return -ENAMETOOLONG;
		
		strcpy( buf, g_cwd );
		if (strcmp( g_cwd, "/" ))
			strcat( buf, "/" );
		strcat( buf, pPath );
	}
	
	return FiMakeDirSub(buf);
}

int FiRemoveDir(const char* pPath)
{
	if (*pPath == 0) return -EBUSY;
	
	int rv;
	USING_LOCK(&g_FileSystemLock, {
		rv = FrRemoveDir(pPath);
	});
	
	return rv;
	
}
