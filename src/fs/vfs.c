/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        Virtual File System module
******************************************/

/**
 * A short description of what this module is responsible for:
 *
 * This module is responsible for handling the file system.
 * Its root is "/".  Storage drives will be later on mounted
 * here, but they will use storabs.c's functionality.
 */
#include <vfs.h>
#include <string.h>
#include <memory.h>
#include <task.h>
#include <misc.h>

char g_cwd[PATH_MAX+2];

void FsAddReference(FileNode* pNode)
{
	//SLogMsg("FsAddReference(%s) -> %d", pNode->m_name, pNode->m_refCount + 1);
	if (pNode->m_refCount == NODE_IS_PERMANENT) return;
	
	pNode->m_refCount++;
}
void FsReleaseReference(FileNode* pNode)
{
	//SLogMsg("FsReleaseReference(%s) -> %d", pNode->m_name, pNode->m_refCount - 1);
	
	// if it's permanent, return
	if (pNode->m_refCount == NODE_IS_PERMANENT) return;
	
	ASSERT(pNode->m_refCount > 0);
	pNode->m_refCount--;
	
	if (pNode->m_refCount == 0)
	{
		if (pNode->OnUnreferenced)
			pNode->OnUnreferenced(pNode);
	}
}

uint32_t FsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, UNUSED bool block)
{
	if (pNode)
	{
		if (pNode->Read)
			return pNode->Read(pNode, offset, size, pBuffer, block);
		else return 0;
	}
	else return 0;
}

uint32_t FsWrite(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, UNUSED bool block)
{
	if (pNode)
	{
		if (pNode->Write)
		{
			return pNode->Write(pNode, offset, size, pBuffer, block);
		}
		else return 0;
	}
	else return 0;
}

bool FsOpen(FileNode* pNode, bool read, bool write)
{
	if (pNode)
	{
		if (pNode->Open)
			return pNode->Open(pNode, read, write);
		else return true;
	}
	//FIXME: This just assumes the file is prepared for opening.
	else return true;
}

void FsClose(FileNode* pNode)
{
	if (pNode)
	{
		if (pNode->Close)
			pNode->Close(pNode);
	}
}

DirEnt* FsReadDir(FileNode* pNode, uint32_t* index, DirEnt* pOutputDent)
{
	if (pNode)
	{
		if (pNode->ReadDir && (pNode->m_type & FILE_TYPE_DIRECTORY))
			return pNode->ReadDir(pNode, index, pOutputDent);
		else return NULL;
	}
	else return NULL;
}

FileNode* FsFindDir(FileNode* pNode, const char* pName)
{
	if (pNode)
	{
		if (pNode->FindDir && (pNode->m_type & FILE_TYPE_DIRECTORY))
			return pNode->FindDir(pNode, pName);
		else return NULL;
	}
	else return NULL;
}

bool FsOpenDir(FileNode* pNode)
{
	if (pNode)
	{
		if (pNode->OpenDir && (pNode->m_type & FILE_TYPE_DIRECTORY))
			return pNode->OpenDir(pNode);
		else return true;//Assume it is opened.
	}
	else return false;
}

void FsCloseDir(FileNode* pNode)
{
	if (pNode)
	{
		if (pNode->CloseDir && (pNode->m_type & FILE_TYPE_DIRECTORY))
			pNode->CloseDir(pNode);
	}
}

void FsClearFile(FileNode* pNode)
{
	if (pNode)
	{
		if (pNode->EmptyFile && !(pNode->m_type & FILE_TYPE_DIRECTORY))
			pNode->EmptyFile(pNode);
	}
}

int FsUnlinkFile(FileNode* pNode, const char* pName)
{
	if (!pNode) return -ENOENT;
	
	// if the directory we're trying to remove from isn't actually a directory
	if (!(pNode->m_type & FILE_TYPE_DIRECTORY)) return -ENOTDIR;
	
	// if there's no way to unlink a file
	if (!pNode->UnlinkFile) return -ENOTSUP;
	
	return pNode->UnlinkFile(pNode, pName);
}

int FsCreateEmptyFile(FileNode* pDirNode, const char* pFileName)
{
	if (!pDirNode) return -EIO;
	
	if (!pDirNode->CreateFile) return -ENOTSUP;
	if (!(pDirNode->m_type & FILE_TYPE_DIRECTORY)) return -ENOTDIR;
	
	return pDirNode->CreateFile(pDirNode, pFileName);
}

int FsCreateDir(FileNode* pDirNode, const char *pFileName)
{
	if (!pDirNode) return -EIO;
	if (!pDirNode->CreateDir) return -ENOTSUP;
	if (!(pDirNode->m_type & FILE_TYPE_DIRECTORY)) return -ENOTDIR;
	
	FileNode* pChildIfExists = FsFindDir(pDirNode, pFileName);
	if (pChildIfExists)
	{
		FsReleaseReference(pChildIfExists);
		return -EEXIST;
	}
	
	return pDirNode->CreateDir(pDirNode, pFileName);
}

FileNode* FsResolvePath (const char* pPath)
{
	char path_copy[PATH_MAX]; //max path
	if (strlen (pPath) >= PATH_MAX-1) return NULL;
	strcpy (path_copy, pPath);
	
	TokenState state;
	state.m_bInitted = 0;
	char* initial_filename = Tokenize (&state, path_copy, "/");
	if (!initial_filename) return NULL;
	
	//is it just an empty string? if yes, we're using
	//an absolute path.  Otherwise we gotta append the CWD 
	//and run this function again.
	if (*initial_filename == 0)
	{
		FileNode *pNode = FsGetRootNode();
		while (true)
		{
			char* path = Tokenize (&state, NULL, "/");
			
			//are we done?
			if (path && *path)
			{
				//nope, resolve pNode again.
				FileNode* pOldNode = pNode;
				pNode = FsFindDir (pNode, path);
				
				//release the old one
				FsReleaseReference(pOldNode);
				
				//if we don't actually have it, return NULL
				if (!pNode) return NULL;
			}
			else
			{
				return pNode;
			}
		}
	}
	else
	{
		// Not an absolute path. Append the CD
		char path_copy[PATH_MAX * 2 + 5]; //max path
		path_copy[0] = 0;
		strcpy (path_copy, g_cwd);
		if (g_cwd[1] != 0) // there's not just /
			strcat (path_copy, "/");
		strcat (path_copy, pPath);
		
		if (strlen (path_copy) >= PATH_MAX - 5)
			// Errno: ENAMETOOLONG
			return NULL;
		
		// try to resolve this as a standard path
		return FsResolvePath (path_copy);
	}
}

// Default
#if 1
// Basic functions

void FsRootInit();

void FsInitializeDevicesDir();

//First time setup of the file manager
void FsInit ()
{
	strcpy(g_cwd, "/");
	FsRootInit();
	FsInitializeDevicesDir();
}

#endif

// File Descriptor handlers:
#if 1

FileDescriptor g_FileNodeToDescriptor[FD_MAX];
DirDescriptor  g_DirNodeToDescriptor [FD_MAX];

void FiPoolDebugDump(void);

void FiDebugDump()
{
	LogMsg("Listing opened files.");
	for (int i = 0; i < FD_MAX; i++)
	{
		FileDescriptor* p = &g_FileNodeToDescriptor[i];
		if (p->m_bOpen)
		{
			LogMsg("FD:%d\tFL:%d\tFS:%d\tP:%d FN:%x FC:%s:%d\tFN:%s",i,p->m_nFileEnd,p->m_nStreamOffset,
					p->m_bIsFIFO, p->m_pNode, p->m_openFile, p->m_openLine, p->m_sPath);
		}
	}
	LogMsg("Done");
}

static int FhFindFreeFileDescriptor(const char* reqPath)
{
	if (reqPath && *reqPath)
	{
		for (int i = 0; i < FD_MAX; i++)
		{
			if (g_FileNodeToDescriptor[i].m_bOpen)
				if (strcmp (g_FileNodeToDescriptor[i].m_sPath, reqPath) == 0)
					return -EAGAIN;
		}
	}
	
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!g_FileNodeToDescriptor[i].m_bOpen)
			return i;
	}
	
	return -ENFILE;
}

static int FhFindFreeDirDescriptor(const char* reqPath)
{
	for (int i = 0; i < FD_MAX; i++)
	{
		if (g_DirNodeToDescriptor[i].m_bOpen)
			if (strcmp (g_DirNodeToDescriptor[i].m_sPath, reqPath) == 0)
				return -EAGAIN;
	}
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!g_DirNodeToDescriptor[i].m_bOpen)
			return i;
	}
	return -ENFILE;
}

//TODO: improve MT
SafeLock g_FileSystemLock;

bool FhIsValidDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_FileNodeToDescriptor[fd].m_bOpen;
}

bool FhIsValidDirDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_DirNodeToDescriptor[fd].m_bOpen;
}

int FhSeek (int fd, int offset, int whence);

int FhOpenInternal(const char* pFileName, FileNode* pFileNode, int oflag, const char* srcFile, int srcLine)
{
	LockAcquire (&g_FileSystemLock);
	// find a free fd to open:
	int fd = FhFindFreeFileDescriptor(pFileName);
	if (fd < 0)
	{
		LockFree (&g_FileSystemLock);
		return fd;
	}
	
	//find the node:
	bool hasClearedAlready = false;
	bool bPassedFileNodeDirectly = false;
	
	FileNode* pFile;
	
	if (pFileNode)
	{
		pFile = pFileNode;
		bPassedFileNodeDirectly = true;
	}
	else
	{
		pFile = FsResolvePath(pFileName);
		if (!pFile)
		{
			// Allow creation, if O_CREAT was specified and we need to write data
			if ((oflag & O_CREAT) && (oflag & O_WRONLY))
			{
				// Resolve the directory's name
				char fileName[strlen(pFileName) + 1];
				strcpy (fileName, pFileName);
				
				char* fileNameSimple = NULL;
				
				for (int i = strlen (pFileName); i >= 0; i--)
				{
					if (fileName[i] == '/')
					{
						fileName[i] = 0;
						fileNameSimple = fileName + i + 1;
						break;
					}
				}
				
				FileNode* pDir = FsResolvePath(fileName);
				if (!pDir)
				{
					//couldn't even find parent dir
					LockFree (&g_FileSystemLock);
					return -ENOENT;
				}
				
				// Try creating a file
				if (FsCreateEmptyFile (pDir, fileNameSimple) < 0)
				{
					LockFree (&g_FileSystemLock);
					return -ENOSPC;
				}
				
				pFile = FsFindDir(pDir, fileNameSimple);
				
				FsReleaseReference(pDir);
				
				hasClearedAlready = true;
				
				if (!pFile)
				{
					LockFree (&g_FileSystemLock);
					return -ENOSPC;
				}
			}
			else
			{
				//Can't append to/read from a missing file!
				LockFree (&g_FileSystemLock);
				return -ENOENT;
			}
		}
	}
	
	//if we are trying to read, but we can't:
	if ((oflag & O_RDONLY) && !(pFile->m_perms & PERM_READ))
	{
		LockFree (&g_FileSystemLock);
		return -EACCES;
	}
	//if we are trying to write, but we can't:
	if ((oflag & O_WRONLY) && !(pFile->m_perms & PERM_WRITE))
	{
		LockFree (&g_FileSystemLock);
		return -EACCES;
	}
	//if we are trying to execute, but we can't:
	if ((oflag & O_EXEC) && !(pFile->m_perms & PERM_EXEC))
	{
		LockFree (&g_FileSystemLock);
		return -EACCES;
	}
	
	if (pFile->m_type & FILE_TYPE_DIRECTORY)
	{
		LockFree (&g_FileSystemLock);
		return -EISDIR;
	}
	
	//If we have O_CREAT and O_WRONLY:
	if ((oflag & O_CREAT) && (oflag & O_WRONLY))
	{
		//If the filenode we opened isn't empty, empty it ourself
		if (!hasClearedAlready)
		{
			FsClearFile(pFile);
		}
	}
	
	//open it:
	if (!FsOpen(pFile, (oflag & O_RDONLY) != 0, (oflag & O_WRONLY) != 0))
	{
		LockFree (&g_FileSystemLock);
		return -EIO;
	}
	
	//we have all the perms, let's write the filenode there:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	
	if (pFileName)
		strcpy(pDesc->m_sPath, pFileName);
	else
		strcpy(pDesc->m_sPath, "");
	
	pDesc->m_bOpen 			= true;
	pDesc->m_pNode 			= pFile;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_nFileEnd		= pFile->m_length;
	pDesc->m_bIsFIFO		= pFile->m_type == FILE_TYPE_CHAR_DEVICE || pFile->m_type == FILE_TYPE_PIPE;
	pDesc->m_bBlocking      = !(oflag & O_NONBLOCK);
	
	// if we passed the reference already, add 1 to the reference counter of this node.
	// otherwise, FsResolvePath or FsCreateFile have already done that.
	if (bPassedFileNodeDirectly)
	{
		FsAddReference(pFile);
	}
	
	LockFree (&g_FileSystemLock);
	
	if ((oflag & O_APPEND) && (oflag & O_WRONLY))
	{
		// Automatically seek to the end
		FhSeek(fd, SEEK_END, 0);
	}
	
	return fd;
}

int FhClose (int fd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	//closes the file:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsClose (pDesc->m_pNode);
	
	FsReleaseReference(pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

int FhOpenDirD (const char* pFileName, const char* srcFile, int srcLine)
{
	LockAcquire (&g_FileSystemLock);
	// find a free fd to open:
	int dd = FhFindFreeDirDescriptor(pFileName);
	if (dd < 0)
	{
		LockFree (&g_FileSystemLock);
		return dd;
	}
	FileNode* pDir = FsResolvePath(pFileName);
	if (!pDir)
	{
		// No File
		LockFree (&g_FileSystemLock);
		return -ENOENT;
	}
	
	if (!(pDir->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pDir);
		
		// Not a Directory
		LockFree (&g_FileSystemLock);
		return -ENOTDIR;
	}
	
	// Try to open the Directory
	bool result = FsOpenDir (pDir);
	if (!result)
	{
		LockFree (&g_FileSystemLock);
		return -EIO; // Cannot open the directory
	}
	
	//we have all the perms, let's write the filenode there:
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_bOpen 			= true;
	strcpy(pDesc->m_sPath, pFileName);
	pDesc->m_pNode 			= pDir;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	
	LockFree (&g_FileSystemLock);
	
	return dd;
}

int FhCloseDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	//closes the file:
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsCloseDir (pDesc->m_pNode);
	
	FsReleaseReference(pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

DirEnt* FhReadDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return NULL;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	DirEnt* pDirEnt = FsReadDir (pDesc->m_pNode, &pDesc->m_nStreamOffset, &pDesc->m_sCurDirEnt);
	if (!pDirEnt)
	{
		LockFree (&g_FileSystemLock);
		return NULL;
	}
	
	LockFree (&g_FileSystemLock);
	return &pDesc->m_sCurDirEnt;
}

int FhSeekDir (int dd, int loc)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	if (loc < 0)
	{
		LockFree (&g_FileSystemLock);
		return -EOVERFLOW;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_nStreamOffset = loc;
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

int FhRewindDir (int dd)
{
	return FhSeekDir (dd, 0);
}

int FhTellDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	LockFree (&g_FileSystemLock);
	return pDesc->m_nStreamOffset;
}

int FhStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	FileNode *pNode = FsFindDir(pDesc->m_pNode, pFileName);
	if (!pNode)
	{
		LockFree (&g_FileSystemLock);
		return -ENOENT;
	}
	
	pOut->m_type       = pNode->m_type;
	pOut->m_size       = pNode->m_length;
	pOut->m_inode      = pNode->m_inode;
	pOut->m_perms      = pNode->m_perms;
	pOut->m_modifyTime = pNode->m_modifyTime;
	pOut->m_createTime = pNode->m_createTime;
	pOut->m_blocks     = (pNode->m_length / 512) + ((pNode->m_length % 512) != 0);
	
	FsReleaseReference(pNode);
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

int FiStat (const char *pFileName, StatResult* pOut)
{
	LockAcquire (&g_FileSystemLock);
	
	FileNode *pNode = FsResolvePath(pFileName);
	if (!pNode)
	{
		LockFree (&g_FileSystemLock);
		return -ENOENT;
	}
	
	pOut->m_type       = pNode->m_type;
	pOut->m_size       = pNode->m_length;
	pOut->m_inode      = pNode->m_inode;
	pOut->m_perms      = pNode->m_perms;
	pOut->m_modifyTime = pNode->m_modifyTime;
	pOut->m_createTime = pNode->m_createTime;
	pOut->m_blocks     = (pNode->m_length / 512) + ((pNode->m_length % 512) != 0);
	
	FsReleaseReference(pNode);
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

size_t FhRead (int fd, void *pBuf, int nBytes)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	if (nBytes < 0)
	{
		LockFree (&g_FileSystemLock);
		return -EINVAL;
	}
	
	int rv = FsRead (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf, g_FileNodeToDescriptor[fd].m_bBlocking);
	if (rv < 0) 
	{
		LockFree (&g_FileSystemLock);
		return rv;
	}
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	LockFree (&g_FileSystemLock);
	return rv;
}

size_t FhWrite (int fd, void *pBuf, int nBytes)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	if (nBytes < 0)
	{
		LockFree (&g_FileSystemLock);
		return -EINVAL;
	}
	
	int rv = FsWrite (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf, g_FileNodeToDescriptor[fd].m_bBlocking);
	if (rv < 0) 
	{
		LockFree (&g_FileSystemLock);
		return rv;
	}
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	LockFree (&g_FileSystemLock);
	return rv;
}

int FhSeek (int fd, int offset, int whence)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	if (g_FileNodeToDescriptor[fd].m_bIsFIFO)
	{
		LockFree (&g_FileSystemLock);
		return -ESPIPE;
	}
	
	if (whence < 0 || whence > SEEK_END)
	{
		LockFree (&g_FileSystemLock);
		return -EINVAL;
	}
	
	int realOffset = offset;
	switch (whence)
	{
		case SEEK_CUR:
			realOffset += g_FileNodeToDescriptor[fd].m_nStreamOffset;
			break;
		case SEEK_END:
			realOffset += g_FileNodeToDescriptor[fd].m_nFileEnd;
			break;
	}
	if (realOffset > g_FileNodeToDescriptor[fd].m_nFileEnd)
	{
		LockFree (&g_FileSystemLock);
		return -EOVERFLOW;
	}
	
	g_FileNodeToDescriptor[fd].m_nStreamOffset = realOffset;
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

int FhTell (int fd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nStreamOffset;
	LockFree (&g_FileSystemLock);
	return rv;
}

int FhTellSize (int fd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FhIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nFileEnd;
	LockFree (&g_FileSystemLock);
	return rv;
}

extern char g_cwd[PATH_MAX+2];

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
	
	LockAcquire(&g_FileSystemLock);
	
	FileNode *pDir = FsResolvePath (buffer);
	if (!pDir)
	{
		LockFree(&g_FileSystemLock);
		return -ENOENT;
	}
	
	// note: The file node is as valid as there is a reference to it.
	// FsResolvePath adds a reference to the node, so it will only be invalid
	// when we release the reference.
	int errorCode = FsUnlinkFile (pDir, r + 1);
	
	FsReleaseReference(pDir);
	LockFree(&g_FileSystemLock);
	
	return errorCode;
}

int FiChangeDir (const char *pfn)
{
	if (*pfn == '\0') return -ENOTHING;//TODO: maybe cd into their home directory instead?
	
	int slen = strlen (pfn);
	if (slen >= PATH_MAX) return -EOVERFLOW;
	
	if (pfn[0] == '/')
	{
		LockAcquire(&g_FileSystemLock);
		
		// Absolute Path
		FileNode *pNode = FsResolvePath (pfn);
		if (!pNode) return -EEXIST;
		
		if (!(pNode->m_type & FILE_TYPE_DIRECTORY))
		{
			FsReleaseReference(pNode);
			LockFree(&g_FileSystemLock);
			return -ENOTDIR;
		}
		
		FsReleaseReference(pNode);
		
		//this should work!
		strcpy (g_cwd, pfn);
		
		LockFree(&g_FileSystemLock);
		
		return -ENOTHING;
	}
	
	if (strcmp (pfn, PATH_THISDIR) == 0) return -ENOTHING;
	
	char cwd_work [sizeof (g_cwd)];
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
	
	LockAcquire(&g_FileSystemLock);
	
	//resolve the path
	FileNode *pNode = FsResolvePath (cwd_work);
	
	if (!pNode)
	{
		LockFree(&g_FileSystemLock);
		
		return -ENOENT; //does not exist
	}
	
	if (!(pNode->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pNode);
		LockFree(&g_FileSystemLock);
		return -ENOTDIR; //not a directory
	}
	
	FsReleaseReference(pNode);
	
	//this should work!
	strcpy (g_cwd, cwd_work);
	
	LockFree(&g_FileSystemLock);
	
	return -ENOTHING;
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
	
	LockAcquire(&g_FileSystemLock);
	
	// resolve the directories
	FileNode* pDirNodeOld = FsResolvePath(pDirOld);
	
	if (!pDirNodeOld)
	{
		LockFree(&g_FileSystemLock);
		return -ENOENT;
	}
	
	FileNode* pDirNodeNew = FsResolvePath(pDirNew);
	
	if (!pDirNodeNew)
	{
		FsReleaseReference(pDirNodeOld);
		LockFree(&g_FileSystemLock);
		return -ENOENT;
	}
	
	if (pDirNodeOld->m_pFileSystemHandle != pDirNodeNew->m_pFileSystemHandle)
	{
		// No cross file system action allowed. Must use manual copy / delete combo.
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		LockFree(&g_FileSystemLock);
		return -EXDEV;
	}
	
	if (!(pDirNodeOld->m_type & FILE_TYPE_DIRECTORY) || !(pDirNodeNew->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		LockFree(&g_FileSystemLock);
		return -ENOTDIR;
	}
	
	if (!pDirNodeOld->RenameOp)
	{
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		LockFree(&g_FileSystemLock);
		return -ENOTSUP;
	}
	
	// If the file already exists we must overwrite it. If we can't do that, simply bail.
	FileNode* pNodeWeWillOverwrite = pDirNodeNew->FindDir(pDirNodeNew, pNameNew);
	if (pNodeWeWillOverwrite)
	{
		int result = -ENOTSUP; // can't overwrite the entry
		
		// If we have an unlink function in the new dir node:
		if (pDirNodeNew->UnlinkFile)
		{
			// Try to unlink this.
			result = pDirNodeNew->UnlinkFile(pDirNodeNew, pNameNew);
		}
		
		// if we couldn't
		if (result != -ENOTHING)
		{
			FsReleaseReference(pDirNodeOld);
			FsReleaseReference(pDirNodeNew);
			LockFree(&g_FileSystemLock);
			return result;
		}
		
		FsReleaseReference(pNodeWeWillOverwrite);
	}
	
	// okay, now, we should be able to just perform the rename operation
	int result = pDirNodeOld->RenameOp(pDirNodeOld, pDirNodeNew, pNameOld, pNameNew);
	
	FsReleaseReference(pDirNodeOld);
	FsReleaseReference(pDirNodeNew);
	LockFree(&g_FileSystemLock);
	
	return result;
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

static int FiMakeDirSub(char* pPath)
{
	char* pSlashPtr = strrchr(pPath, '/');
	ASSERT(pSlashPtr);
	
	*pSlashPtr = 0;
	
	char* pDirName = pPath, *pFileName = pSlashPtr + 1;
	
	FileNode *pNode = FsResolvePath(pDirName);
	if (!pNode) return -ENOENT;
	
	int status = FsCreateDir(pNode, pFileName);
	
	FsReleaseReference(pNode);
	
	return status;
}

int FiMakeDir(const char* pPath)
{
	if (*pPath == 0) return -ENOENT;
	
	char buf[PATH_MAX];
	
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
	
	LockAcquire(&g_FileSystemLock);
	
	int status = FiMakeDirSub(buf);
	
	LockFree(&g_FileSystemLock);
	
	return status;
}

int FiRemoveDir(const char* pPath)
{
	if (*pPath == 0) return -EBUSY;
	
	LockAcquire(&g_FileSystemLock);
	
	FileNode* pNode = FsResolvePath(pPath);
	
	if (!pNode->RemoveDir)
	{
		LockFree(&g_FileSystemLock);
		return -ENOTSUP;
	}
	
	int status = pNode->RemoveDir(pNode);
	
	FsReleaseReference(pNode);
	LockFree(&g_FileSystemLock);
	
	return status;
}

#endif

static const char* ErrorStrings[] = {
	"Success",
	"Permission denied",
	"File exists",
	"Interrupted system call",
	"Invalid argument",
	"Input/output error",
	"Is a directory",
	"Too many symbolic links",
	"Too many open files",
	"File or path name too long",
	"Too many open files in system",
	"No such file or directory",
	"Out of stream resources",
	"No space left on device",
	"Not a directory",
	"No such device or address",
	"Value is too large for defined data type",
	"Read only file system",
	"Already open",
	"Not enough memory",
	"Text file busy",
	"Bad file descriptor",
	"Illegal seek (is FIFO)",
	"Computer bought the farm",
	"Operation not supported",
	"Cross device operation not supported",
	"Resource is busy",
	"Directory is not empty",
};

STATIC_ASSERT(ARRAY_COUNT(ErrorStrings) == ECOUNT, "Change this if adding error codes.");

const char *GetErrNoString(int errno)
{
	errno = -errno;
	if (errno < 0)
		return "Unknown Error";
	if (errno >= ECOUNT)
		return "Unknown Error";
	
	return ErrorStrings[errno];
}
