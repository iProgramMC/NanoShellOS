//  ***************************************************************
//  fdtab.c - Creation date: 09/12/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

/**
 * A short description of what this module is responsible for:
 *
 * This module is responsible for translating process-local file
 * descriptor numbers to global file handle numbers.
 */
#include <process.h>
#include <vfs.h>

uint32_t* FsGetFdTable()
{
	Process* pProcess = ExGetRunningProc();
	if (pProcess)
	{
		return pProcess->pFdTable;
	}
	return NULL;
}

uint32_t* FsGetDdTable()
{
	Process* pProcess = ExGetRunningProc();
	if (pProcess)
	{
		return pProcess->pDdTable;
	}
	return NULL;
}

static int FileDescriptorToGlobalFileHandle(int i)
{
	if (i < 0 || i >= C_MAX_FDS_PER_TABLE) return -EINVAL;
	
	Process* pProc = ExGetRunningProc();
	if (!pProc) return i;
	
	uint32_t* fdTable = pProc->pFdTable;
	
	LockAcquire(&pProc->sFileTableLock);
	if (fdTable)
	{
		uint32_t r = fdTable[i];
		LockFree(&pProc->sFileTableLock);
		return r;
	}
	LockFree(&pProc->sFileTableLock);
	
	//well, you probably should be able to do this if there's no FD table, but if not, let me know -iProgramInCpp
	return i;
}

static int DirDescriptorToGlobalDirHandle(int i)
{
	if (i < 0 || i >= C_MAX_FDS_PER_TABLE) return -EINVAL;
	
	Process* pProc = ExGetRunningProc();
	if (!pProc) return i;
	
	uint32_t* ddTable = pProc->pDdTable;
	
	LockAcquire(&pProc->sFileTableLock);
	if (ddTable)
	{
		uint32_t r = ddTable[i];
		LockFree(&pProc->sFileTableLock);
		return r;
	}
	LockFree(&pProc->sFileTableLock);
	
	//well, you probably should be able to do this if there's no FD table, but if not, let me know -iProgramInCpp
	return i;
}

static int FiFindFreeFileDescriptor()
{
	uint32_t* fdTable = FsGetFdTable();
	for (int i = 0; i < C_MAX_FDS_PER_TABLE; i++)
	{
		if (fdTable[i] == 0) return i;
	}
	return -EMFILE;
}

static int FiFindFreeDirDescriptor()
{
	uint32_t* fdTable = FsGetFdTable();
	for (int i = 0; i < C_MAX_FDS_PER_TABLE; i++)
	{
		if (fdTable[i] == 0) return i;
	}
	return -EMFILE;
}

// note: This returns a global level file handle.
int FhOpenInternal(const char* pFileName, FileNode* pFileNode, int oflag, const char* srcFile, int srcLine);
int FhClose (int fd);
int FhOpenDirD (const char* pFileName, const char* srcFile, int srcLine);
int FhCloseDir (int dd);
DirEnt* FhReadDir (int dd);
int FhSeekDir (int dd, int loc);
int FhRewindDir (int dd);
int FhTellDir (int dd);
int FhStatAt (int dd, const char *pFileName, StatResult* pOut);
size_t FhRead (int fd, void *pBuf, int nBytes);
size_t FhWrite (int fd, void *pBuf, int nBytes);
int FhSeek (int fd, int offset, int whence);
int FhTell (int fd);
int FhTellSize (int fd);

int FiOpenD(const char* pFileName, int oflag, const char* srcFile, int srcLine)
{
	Process* pProc = ExGetRunningProc();
	int freeSpot = 0;
	
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		freeSpot = FiFindFreeFileDescriptor();
		if (freeSpot < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return freeSpot;
		}
	}
	
	int handle = FhOpenInternal(pFileName, NULL, oflag, srcFile, srcLine);
	if (handle < 0) return handle;
	
	if (!pProc) return handle;
	
	FsGetFdTable()[freeSpot] = handle;
	
	LockFree(&pProc->sFileTableLock);
	return freeSpot;
}

int FiOpenFileNodeD(FileNode* pFileNode, int oflag, const char* srcFile, int srcLine)
{
	Process* pProc = ExGetRunningProc();
	int freeSpot = 0;
	
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		freeSpot = FiFindFreeFileDescriptor();
		if (freeSpot < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return freeSpot;
		}
	}
	
	int handle = FhOpenInternal(NULL, pFileNode, oflag, srcFile, srcLine);
	if (handle < 0) return handle;
	
	if (!pProc) return handle;
	
	FsGetFdTable()[freeSpot] = handle;
	
	LockFree(&pProc->sFileTableLock);
	return freeSpot;
}

int FiClose(int fd)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	Process* pProc = ExGetRunningProc();
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		int status = FhClose(fd);
		if (status < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return status;
		}
		// status is zero. Remove it from the FD table
		FsGetFdTable()[fd] = 0;
		LockFree(&pProc->sFileTableLock);
		return -ENOTHING;
	}
	else
	{
		return FhClose(fd);
	}
}

int FiTell(int fd)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhTell(fd);
}

int FiTellSize(int fd)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhTellSize(fd);
}

size_t FiRead (int fd, void *pBuf, int nBytes)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhRead(fd, pBuf, nBytes);
}

size_t FiWrite (int fd, void *pBuf, int nBytes)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhWrite(fd, pBuf, nBytes);
}

int FiSeek (int fd, int offset, int whence)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhSeek(fd, offset, whence);
}

int FiOpenDirD(const char* pFileName, const char* srcFile, int srcLine)
{
	Process* pProc = ExGetRunningProc();
	int freeSpot = 0;
	
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		freeSpot = FiFindFreeDirDescriptor();
		if (freeSpot < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return freeSpot;
		}
	}
	
	int handle = FhOpenDirD(pFileName, srcFile, srcLine);
	if (handle < 0) return handle;
	
	if (!pProc) return handle;
	
	FsGetDdTable()[freeSpot] = handle;
	
	LockFree(&pProc->sFileTableLock);
	return freeSpot;
}

int FiCloseDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	Process* pProc = ExGetRunningProc();
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		int status = FhCloseDir(dd);
		if (status < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return status;
		}
		// status is zero. Remove it from the FD table
		FsGetDdTable()[dd] = 0;
		LockFree(&pProc->sFileTableLock);
		return -ENOTHING;
	}
	else
	{
		return FhCloseDir(dd);
	}
}

int FiRewindDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhRewindDir(dd);
}

int FiTellDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhTellDir(dd);
}

int FiSeekDir(int dd, int loc)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhSeekDir(dd, loc);
}

DirEnt* FiReadDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return NULL;
	return FhReadDir(dd);
}

int FiStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhStatAt(dd, pFileName, pOut);
}
