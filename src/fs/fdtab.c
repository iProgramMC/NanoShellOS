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

#define FDLogMsg(...)
//#define FDLogMsg SLogMsg

#define FD_INVALID (~0U)

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
		if (fdTable[i] == FD_INVALID) return i;
	}
	return -EMFILE;
}

static int FiFindFreeDirDescriptor()
{
	uint32_t* fdTable = FsGetFdTable();
	for (int i = 0; i < C_MAX_FDS_PER_TABLE; i++)
	{
		if (fdTable[i] == FD_INVALID) return i;
	}
	return -EMFILE;
}

/*
int FiDuplicateHandle(int fd)
{
	Process* pProc = ExGetRunningProc();
	if (!pProc) return -ENOTSUP;
	
	LockAcquire(&pProc->sFileTableLock);
	int freeSpot = FiFindFreeFileDescriptor();
	if (freeSpot < 0)
	{
		LockFree(&pProc->sFileTableLock);
		return freeSpot;
	}
	
	uint32_t* fdTable = FsGetFdTable();
	fdTable[freeSpot] = fdTable[fd];
	
	LockFree(&pProc->sFileTableLock);
	return freeSpot;
}
*/

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
	if (handle < 0)
	{
		LockFree(&pProc->sFileTableLock);
		return handle;
	}
	
	if (!pProc)
	{
		FDLogMsg("FiOpenD() => %d (kernel)", handle);
		return handle;
	}
	
	FsGetFdTable()[freeSpot] = handle;
	
	LockFree(&pProc->sFileTableLock);
	FDLogMsg("FiOpenD() => %d [%d]", freeSpot, handle);
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
	if (handle < 0)
	{
		LockFree(&pProc->sFileTableLock);
		return handle;
	}
	
	if (!pProc)
	{
		FDLogMsg("FiOpenDirD() => %d (kernel)", handle);
		return handle;
	}
	
	FsGetFdTable()[freeSpot] = handle;
	
	LockFree(&pProc->sFileTableLock);
	return freeSpot;
}

int FiClose(int fd)
{
	FDLogMsg("FiClose(%d)", fd);
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	Process* pProc = ExGetRunningProc();
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		int status = FhClose(handle);
		if (status < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return status;
		}
		// status is zero. Remove it from the FD table
		FsGetFdTable()[fd] = FD_INVALID;
		LockFree(&pProc->sFileTableLock);
		return -ENOTHING;
	}
	else
	{
		return FhClose(handle);
	}
}

int FiTell(int fd)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhTell(handle);
}

int FiTellSize(int fd)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhTellSize(handle);
}

size_t FiRead (int fd, void *pBuf, int nBytes)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhRead(handle, pBuf, nBytes);
}

size_t FiWrite (int fd, void *pBuf, int nBytes)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhWrite(handle, pBuf, nBytes);
}

int FiSeek (int fd, int offset, int whence)
{
	int handle = FileDescriptorToGlobalFileHandle(fd);
	if (handle < 0) return handle;
	return FhSeek(handle, offset, whence);
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
	if (handle < 0)
	{
		LockFree(&pProc->sFileTableLock);
		return handle;
	}
	
	if (!pProc) return handle;
	
	FsGetDdTable()[freeSpot] = handle;
	
	LockFree(&pProc->sFileTableLock);
	FDLogMsg("FiOpenDirD() => %d [%d]", freeSpot, handle);
	return freeSpot;
}

int FiCloseDir(int dd)
{
	FDLogMsg("FiCloseDir(%d)", dd);
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	Process* pProc = ExGetRunningProc();
	if (pProc)
	{
		LockAcquire(&pProc->sFileTableLock);
		int status = FhCloseDir(handle);
		if (status < 0)
		{
			LockFree(&pProc->sFileTableLock);
			return status;
		}
		// status is zero. Remove it from the FD table
		FsGetDdTable()[dd] = FD_INVALID;
		LockFree(&pProc->sFileTableLock);
		return -ENOTHING;
	}
	else
	{
		return FhCloseDir(handle);
	}
}

int FiRewindDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhRewindDir(handle);
}

int FiTellDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhTellDir(handle);
}

int FiSeekDir(int dd, int loc)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhSeekDir(handle, loc);
}

DirEnt* FiReadDir(int dd)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return NULL;
	return FhReadDir(handle);
}

int FiStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	int handle = DirDescriptorToGlobalDirHandle(dd);
	if (handle < 0) return handle;
	return FhStatAt(handle, pFileName, pOut);
}
