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

char g_cwdStr[PATH_MAX+2];
FileNode* g_pCwdNode = NULL;

FileNode* FsGetCwdNode()
{
	return g_pCwdNode;
}

void FrGetCwdSub(FileNode* pNode, char* cwdPtr, size_t cwdPtrSize, int depthLvl)
{
	if (depthLvl == 0)
	{
		strncpy(cwdPtr, "...", cwdPtrSize);
		cwdPtr[cwdPtrSize - 1] = 0;
		return;
	}
	
	// if we are the root node, we are done!
	if (FsGetRootNode() == pNode)
		return;
	
	char our_name[PATH_MAX];
	strcpy(our_name, "???");
	bool bHaveName = false;
	
	FileNode* pParent = pNode->m_pParent;
	
	if (pParent)
	{
		// we have a parent!
		FsReleaseReference(pParent);
		
		uint32_t index = pNode->m_parentDirIndex;
		
		// Browse the parent for our name
		DirEnt de;
		int result = FsReadDir(pParent, &index, &de);
		
		if (result >= 0)
		{
			our_name[0] = '/';
			strncpy(our_name + 1, de.m_name, sizeof our_name - 1);
			our_name[sizeof our_name - 1] = 0;
			bHaveName = true;
		}
		
		if (!bHaveName)
		{
			SLogMsg("FrGetCwdSub I/O error %d while trying to find ourselves within the parent!!", result);
		}
		else
		{
			// Call this function recursively on the parent to get its path:
			FrGetCwdSub(pParent, cwdPtr, cwdPtrSize, depthLvl - 1);
		}
		
		FsReleaseReference(pParent);
	}
	
	strlcat(cwdPtr, our_name, cwdPtrSize);
}

const char* FrGetCwd()
{
	g_cwdStr[0] = 0;
	
	FrGetCwdSub(g_pCwdNode, g_cwdStr, sizeof g_cwdStr, 32);
	
	// special case at the root directory
	if (g_cwdStr[0] == 0)
		strcpy(g_cwdStr, "/");
	
	return g_cwdStr;
}

void FsAddReference(FileNode* pNode)
{
	if (pNode->m_refCount == NODE_IS_PERMANENT) return;
	//SLogMsg("+ Reference(%p) [%p] => %d", pNode, __builtin_return_address(0), pNode->m_refCount + 1);
	
	pNode->m_refCount++;
}

void FsReleaseReference(FileNode* pNode)
{
	// if it's permanent, return
	if (pNode->m_refCount == NODE_IS_PERMANENT) return;
	//SLogMsg("- Reference(%p) [%p] => %d", pNode, __builtin_return_address(0), pNode->m_refCount - 1);
	
	ASSERT(pNode->m_refCount > 0);
	pNode->m_refCount--;
	
	if (pNode->m_refCount == 0)
	{
		if (pNode->m_bHasDirCallbacks)
		{
			if (pNode->m_pParent)
			{
				FsReleaseReference(pNode->m_pParent);
				pNode->m_pParent = NULL;
			}
		}
		
		if (pNode->m_pFileOps->OnUnreferenced)
			pNode->m_pFileOps->OnUnreferenced(pNode);
	}
}

int FsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, bool block)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	if (!pNode->m_pFileOps->Read)
		return ERR_NOT_SUPPORTED;
	
	return pNode->m_pFileOps->Read(pNode, offset, size, pBuffer, block);
}

int FsWrite(FileNode* pNode, uint32_t offset, uint32_t size, const void* pBuffer, bool block)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	if (!pNode->m_pFileOps->Write)
		return ERR_NOT_SUPPORTED;
	
	return pNode->m_pFileOps->Write(pNode, offset, size, pBuffer, block);
}

int FsIoControl(FileNode* pNode, unsigned long request, void * argp)
{
	if (!pNode)
		return -EINVAL;
	
	if (pNode->m_bHasDirCallbacks)
		return -EISDIR;
	
	if (!pNode->m_pFileOps->IoControl)
		return -ENOTTY;
	
	return pNode->m_pFileOps->IoControl(pNode, request, argp);
}

int FsOpen(FileNode* pNode, bool read, bool write)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	// Assume we could open it.
	if (!pNode->m_pFileOps->Open)
		return ERR_SUCCESS;
	
	return pNode->m_pFileOps->Open(pNode, read, write);
}

int FsClose(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	// Assume we could close it.
	if (!pNode->m_pFileOps->Close)
		return ERR_SUCCESS;
	
	return pNode->m_pFileOps->Close(pNode);
}

int FsReadDir(FileNode* pNode, uint32_t* index, DirEnt* pOutputDent)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	if (!pNode->m_pFileOps->ReadDir)
		return ERR_NOT_SUPPORTED;
	
	return pNode->m_pFileOps->ReadDir(pNode, index, pOutputDent);
}

int FsFindDir(FileNode* pNode, const char* pName, FileNode** pFN)
{
	*pFN = NULL;
	
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	if (!pNode->m_pFileOps->FindDir)
		return ERR_NOT_SUPPORTED;
	
	return pNode->m_pFileOps->FindDir(pNode, pName, pFN);
}

int FsOpenDir(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	// Assume we could open it.
	if (!pNode->m_pFileOps->OpenDir)
		return ERR_SUCCESS;
	
	return pNode->m_pFileOps->OpenDir(pNode);
}

int FsCloseDir(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	// Assume we could close it.
	if (!pNode->m_pFileOps->CloseDir)
		return ERR_SUCCESS;
	
	return pNode->m_pFileOps->CloseDir(pNode);
}

int FsClearFile(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	if (!pNode->m_pFileOps->EmptyFile)
		return ERR_NOT_SUPPORTED;
	
	return pNode->m_pFileOps->EmptyFile(pNode);
}

int FsUnlinkFile(FileNode* pNode, const char* pName)
{
	if (!pNode) return -ENOENT;
	
	// if the directory we're trying to remove from isn't actually a directory
	if (!pNode->m_bHasDirCallbacks) return -ENOTDIR;
	
	// if there's no way to unlink a file
	if (!pNode->m_pFileOps->UnlinkFile) return -ENOTSUP;
	
	return pNode->m_pFileOps->UnlinkFile(pNode, pName);
}

int FsCreateEmptyFile(FileNode* pDirNode, const char* pFileName)
{
	if (!pDirNode) return -EIO;
	
	if (!pDirNode->m_bHasDirCallbacks) return -ENOTDIR;
	
	if (!pDirNode->m_pFileOps->CreateFile) return -ENOTSUP;
	
	return pDirNode->m_pFileOps->CreateFile(pDirNode, pFileName);
}

int FsCreateDir(FileNode* pDirNode, const char *pFileName)
{
	if (!pDirNode) return -EIO;
	if (!pDirNode->m_pFileOps->CreateDir) return -ENOTSUP;
	if (!pDirNode->m_bHasDirCallbacks) return -ENOTDIR;
	
	FileNode* pChildIfExists = NULL;
	
	int result = FsFindDir(pDirNode, pFileName, &pChildIfExists);
	if (FAILED(result) && result != -ENOENT)
	{
		return result;
	}
	
	if (pChildIfExists)
	{
		FsReleaseReference(pChildIfExists);
		return -EEXIST;
	}
	
	return pDirNode->m_pFileOps->CreateDir(pDirNode, pFileName);
}

// pBuf needs to be PATH_MAX or bigger in size.
int FsReadSymbolicLink(FileNode* pNode, char* pBuf)
{
	if (pNode->m_length >= PATH_MAX)
		return -ENAMETOOLONG;
	
	if (!pNode->m_pFileOps->Read)
		return ERR_NOT_SUPPORTED;
	
	for (int i = 0; i < (int)pNode->m_length; i++)
		pBuf[i] = 'A';
	
	int read = pNode->m_pFileOps->Read(pNode, 0, pNode->m_length, pBuf, true);
	
	if (read < 0)
		return read;
	
	if (FAILED(read))
		return ERR_IO_ERROR;
	
	pBuf[pNode->m_length] = 0;
	return 0;
}

// note: this does NO checks on pPath being longer than PATH_MAX, we assume it's already been done!
FileNode* FsResolvePathInternal(FileNode* pStartNode, const char* pPath, bool bResolveSymLinks, int nSymLinkDepth, int * errNo)
{
	*errNo = 0;
	
	if (!*pPath)
		return pStartNode;
	
	if (nSymLinkDepth >= C_MAX_SYMLINK_DEPTH)
	{
		*errNo = -ELOOP;
		return NULL;
	}
	
	char sCurrentFileName[PATH_MAX];
	const char *pPath2 = pPath;
	int index = 0;
	
	// since we will go through AT LEAST one iteration of the while loop (since the path isn't "nothing")
	// we need to add another reference to cancel out the one we would remove in the loop.
	FsAddReference(pStartNode);
	
	FileNode* pFileNode = pStartNode;
	
	while (true)
	{
		index = 0;
		while (*pPath2 != '/' && *pPath2 != '\0')
		{
			sCurrentFileName[index++] = *pPath2;
			pPath2++;
		}
		sCurrentFileName[index] = 0;
		
		bool bAtEnd = (*pPath2 == '\0'); //that, or it's at a '/'
		
		if (!bAtEnd)
		{
			pPath2++;
		}
		
		FileNode* pChild = NULL;
		
		if (strcmp(sCurrentFileName, ".") == 0)
		{
			pChild = pFileNode;
			FsAddReference(pChild); // to imitate the effect that FsFindDir adds
		}
		else if (strcmp(sCurrentFileName, "..") == 0)
		{
			pChild = pFileNode->m_pParent;
			FsAddReference(pChild); // to imitate the effect that FsFindDir adds
		}
		else
		{
			int result = FsFindDir(pFileNode, sCurrentFileName, &pChild);
			if (FAILED(result))
			{
				FsReleaseReference(pFileNode);
				return NULL;
			}
		}
		
		if (!pChild)
		{
			FsReleaseReference(pFileNode);
			return NULL;
		}
		
		// note. Currently, we keep the parent referenced, since we need to maybe resolve a symlink.
		// if we're a symbolic link, read it
		if (pChild->m_type == FILE_TYPE_SYMBOLIC_LINK && (!bAtEnd || bResolveSymLinks))
		{
			char slink[PATH_MAX];
			
			*errNo = FsReadSymbolicLink(pChild, slink);
			if (*errNo < 0)
			{
				// couldn't read. Unreference everything we know, and return.
				FsReleaseReference(pFileNode);
				FsReleaseReference(pChild);
				return NULL;
			}
			
			if (slink[0] == '/') // absolute symbolic link:
			{
				// start over from scratch.
				FsReleaseReference(pFileNode);
				FsReleaseReference(pChild);
				pFileNode = FsResolvePathInternal(FsGetRootNode(), slink, bResolveSymLinks, nSymLinkDepth + 1, errNo);
			}
			else // relative sym link
			{
				// release the child (the symlink), and start the lookup from our parent.
				// This is why kept it referenced.
				FsReleaseReference(pChild);
				FileNode* pNewNode = FsResolvePathInternal(pFileNode, slink, bResolveSymLinks, nSymLinkDepth + 1, errNo);
				FsReleaseReference(pFileNode);
				pFileNode = pNewNode;
			}
		}
		else
		{
			// dereference the parent, and update the pointer to the child.
			FsReleaseReference(pFileNode);
			pFileNode = pChild;
		}
		
		if (bAtEnd)
			return pFileNode;
	}
}

FileNode* FsResolvePath (const char* pPath, bool bResolveSymLinks)
{
	// ensure the path is long enough
	size_t pathLen = strlen(pPath);
	if (pathLen >= PATH_MAX)
		return NULL;
	
	FileNode* pStartNode = FsGetRootNode();
	
	char path[PATH_MAX];
	if (*pPath == '/')
	{
		strcpy(path, pPath + 1);
	}
	else
	{
		pStartNode = FsGetCwdNode();
		strcpy(path, pPath);
	}
	
	UNUSED int errNo = 0;
	FsAddReference(pStartNode);
	
	FileNode* pResult = FsResolvePathInternal(pStartNode, path, bResolveSymLinks, 0, &errNo);
	
	FsReleaseReference(pStartNode);
	
	return pResult;
}

// Default
#if 1
// Basic functions

void FsTempInit();

void FsInitializeDevicesDir();

//First time setup of the file manager
void FsInit ()
{
	FsTempInit();
	g_pCwdNode = FsGetRootNode();
	FsInitializeDevicesDir();
}

#endif

// File Descriptor handlers:
#if 1

FileDescriptor g_FileNodeToDescriptor[FD_MAX];

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

static int FrFindFreeFileDescriptor()
{
	/*
	if (reqPath && *reqPath)
	{
		for (int i = 0; i < FD_MAX; i++)
		{
			if (g_FileNodeToDescriptor[i].m_bOpen)
				if (strcmp (g_FileNodeToDescriptor[i].m_sPath, reqPath) == 0)
					return -EAGAIN;
		}
	}
	*/
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!g_FileNodeToDescriptor[i].m_bOpen)
			return i;
	}
	
	return -ENFILE;
}

bool FrIsValidDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_FileNodeToDescriptor[fd].m_bOpen;
}

int FrSeek (int fd, int offset, int whence);

int FrOpenInternal(const char* pFileName, FileNode* pFileNode, int oflag, const char* srcFile, int srcLine)
{
	// find a free fd to open:
	int fd = FrFindFreeFileDescriptor();
	if (fd < 0)
		return fd;
	
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
		pFile = FsResolvePath(pFileName, true);
		if (!pFile)
		{
			// Allow creation, if O_CREAT was specified and we need to write data
			if ((oflag & O_CREAT) && (oflag & O_WRONLY))
			{
				// Resolve the directory's name
				char fileName[PATH_MAX + 2];
				strcpy (fileName, pFileName);
				
				const char* fileNameSimple = NULL;
				
				for (int i = strlen (pFileName); i >= 0; i--)
				{
					if (fileName[i] == '/')
					{
						fileName[i] = 0;
						fileNameSimple = fileName + i + 1;
						break;
					}
				}
				
				// if no slash could be removed, this is the simple file name.
				if (fileNameSimple == NULL)
				{
					fileNameSimple = pFileName;
					strcpy(fileName, ".");
				}
				
				FileNode* pDir = FsResolvePath(fileName, false);
				if (!pDir)
				{
					SLogMsg("Warning: Couldn't even find parent dir '%s' ('%s')", fileName, pFileName);
					//couldn't even find parent dir
					return -ENOENT;
				}
				
				// this SHOULD be true...
				ASSERT(pDir->m_type & FILE_TYPE_DIRECTORY);
				
				// Try creating a file
				if (FsCreateEmptyFile (pDir, fileNameSimple) < 0)
					return -ENOSPC;
				
				int err = FsFindDir(pDir, fileNameSimple, &pFile);
				if (FAILED(err))
				{
					FsReleaseReference(pDir);
					return err;
				}
				
				FsReleaseReference(pDir);
				
				hasClearedAlready = true;
				
				if (!pFile)
					return -ENOSPC;
			}
			else
			{
				//Can't append to/read from a missing file!
				return -ENOENT;
			}
		}
		
		// if the file itself is a symlink (we asked it to resolve through sym links), simply return -ELOOP
		// to let the user know the symlink chain is too deep or some other issue happened
		if (pFile->m_type == FILE_TYPE_SYMBOLIC_LINK)
		{
			FsReleaseReference(pFile);
			
			return -ELOOP;
		}
	}
	
	//if we are trying to read, but we can't:
	if ((oflag & O_RDONLY) && !(pFile->m_perms & PERM_READ))
		return -EACCES;
	
	//if we are trying to write, but we can't:
	if ((oflag & O_WRONLY) && !(pFile->m_perms & PERM_WRITE))
		return -EACCES;
	
	//if we are trying to execute, but we can't:
	if ((oflag & O_EXEC) && !(pFile->m_perms & PERM_EXEC))
		return -EACCES;
	
	if (pFile->m_type & FILE_TYPE_DIRECTORY)
		return -EISDIR;
	
	//If we have should truncate the file:
	if ((oflag & O_WRONLY) && (oflag & O_TRUNC))
	{
		//If the filenode we opened isn't empty, empty it ourself
		if (!hasClearedAlready)
		{
			int result = FsClearFile(pFile);
			if (FAILED(result))
				return result;
		}
	}
	
	//open it:
	int res = FsOpen(pFile, (oflag & O_RDONLY) != 0, (oflag & O_WRONLY) != 0);
	if (res < 0)
		return res;
	
	//we have all the perms, let's write the filenode there:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	memset(pDesc, 0, sizeof *pDesc);
	
	if (pFileName)
		strcpy(pDesc->m_sPath, pFileName);
	else
		strcpy(pDesc->m_sPath, "");
	
	pDesc->m_bOpen 			= true;
	pDesc->m_bIsDirectory   = false;
	pDesc->m_pNode 			= pFile;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_nFileEnd		= pFile->m_length;
	pDesc->m_bIsFIFO		= pFile->m_type == FILE_TYPE_CHAR_DEVICE || pFile->m_type == FILE_TYPE_PIPE;
	pDesc->m_bBlocking      = !(oflag & O_NONBLOCK);
	pDesc->m_ownerTask      = KeGetRunningTask();
	
	// if we passed the reference already, add 1 to the reference counter of this node.
	// otherwise, FsResolvePath or FsCreateFile have already done that.
	if (bPassedFileNodeDirectly)
	{
		FsAddReference(pFile);
	}
	
	if ((oflag & O_APPEND) && (oflag & O_WRONLY))
	{
		// Automatically seek to the end
		FrSeek(fd, SEEK_END, 0);
	}
	
	return fd;
}

int FrClose (int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	//closes the file:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsClose(pDesc->m_pNode);
	
	FsReleaseReference(pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	pDesc->m_ownerTask = NULL;
	
	return -ENOTHING;
}

int FrOpenDirD (const char* pFileName, const char* srcFile, int srcLine)
{
	// find a free fd to open:
	int dd = FrFindFreeFileDescriptor();
	if (dd < 0)
		return dd;
	
	FileNode* pDir = FsResolvePath(pFileName, true);
	if (!pDir)
		// No File
		return -ENOENT;
	
	if (pDir->m_type == FILE_TYPE_SYMBOLIC_LINK)
	{
		FsReleaseReference(pDir);
		return -ELOOP;
	}
	
	if (!(pDir->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pDir);
		
		// Not a Directory
		return -ENOTDIR;
	}
	
	// Try to open the Directory
	int result = FsOpenDir(pDir);
	if (result < 0)
		return result; // Cannot open the directory
	
	//we have all the perms, let's write the filenode there:
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[dd];
	memset(pDesc, 0, sizeof *pDesc);
	
	pDesc->m_bOpen 			= true;
	pDesc->m_bIsDirectory   = true;
	strcpy(pDesc->m_sPath, pFileName);
	pDesc->m_pNode 			= pDir;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_ownerTask      = KeGetRunningTask();
	
	return dd;
}

int FrCloseDir (int dd)
{
	if (!FrIsValidDescriptor(dd))
		return -EBADF;
	
	//closes the file:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[dd];
	if (!pDesc->m_bIsDirectory)
		return -ENOTDIR;
	
	pDesc->m_bOpen = false;
	
	strcpy(pDesc->m_sPath, "");
	
	FsCloseDir(pDesc->m_pNode);
	
	FsReleaseReference(pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	pDesc->m_ownerTask     = NULL;
	
	return -ENOTHING;
}

DirEnt* FrReadDirLegacy(int dd)
{
	if (!FrIsValidDescriptor(dd))
		return NULL;
	
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[dd];
	if (!pDesc->m_bIsDirectory)
		return NULL;
	
	int errCode = FsReadDir (pDesc->m_pNode, &pDesc->m_nStreamOffset, &pDesc->m_sCurDirEnt);
	if (errCode != 0) // if we've reached a failure OR the end
	{
		// issue with this API is that there's no way to know what exactly failed.
		return NULL;
	}
	
	return &pDesc->m_sCurDirEnt;
}

int FrReadDir(DirEnt* pDirEnt, int dd)
{
	if (!FrIsValidDescriptor(dd))
		return ERR_BAD_FILE_DES;
	
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[dd];
	if (!pDesc->m_bIsDirectory)
		return -ENOTDIR;
	
	return FsReadDir(pDesc->m_pNode, &pDesc->m_nStreamOffset, pDirEnt);
}

int FrSeekDir (int dd, int loc)
{
	if (!FrIsValidDescriptor(dd))
		return -EBADF;
	
	if (loc < 0)
		return -EOVERFLOW;
	
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[dd];
	if (!pDesc->m_bIsDirectory)
		return -ENOTDIR;
	
	pDesc->m_nStreamOffset = loc;
	
	return -ENOTHING;
}

int FrTellDir (int dd)
{
	if (!FrIsValidDescriptor(dd))
		return -EBADF;
	
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[dd];
	if (!pDesc->m_bIsDirectory)
		return -ENOTDIR;
	
	return pDesc->m_nStreamOffset;
}

void FrStatFileNode(FileNode* pNode, StatResult* pOut)
{
	pOut->m_type       = pNode->m_type;
	pOut->m_size       = pNode->m_length;
	pOut->m_inode      = pNode->m_inode;
	pOut->m_perms      = pNode->m_perms;
	pOut->m_modifyTime = pNode->m_modifyTime;
	pOut->m_createTime = pNode->m_createTime;
	pOut->m_blocks     = (pNode->m_length / 512) + ((pNode->m_length % 512) != 0);
}

int FrStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	if (!FrIsValidDescriptor(dd))
		return -EBADF;
	
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[dd];
	if (!pDesc->m_bIsDirectory)
		return -ENOTDIR;
	
	FileNode *pNode = NULL;
	
	int err = FsFindDir(pDesc->m_pNode, pFileName, &pNode);
	if (FAILED(err))
		return err;
	
	if (!pNode)
		return -ENOENT;
	
	FrStatFileNode(pNode, pOut);
	
	FsReleaseReference(pNode);
	return -ENOTHING;
}

int FrStat (const char *pFileName, StatResult* pOut)
{
	FileNode *pNode = FsResolvePath(pFileName, true);
	if (!pNode)
		return -ENOENT;
	
	FrStatFileNode(pNode, pOut);
	
	FsReleaseReference(pNode);
	return -ENOTHING;
}

int FrLinkStat (const char *pFileName, StatResult* pOut)
{
	FileNode *pNode = FsResolvePath(pFileName, false);
	if (!pNode)
		return -ENOENT;
	
	FrStatFileNode(pNode, pOut);
	
	FsReleaseReference(pNode);
	return -ENOTHING;
}

int FrFileDesStat(int fd, StatResult* pOut)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	// this can stat both file descriptors and directory descriptors alike. So no need to check
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	
	FrStatFileNode(pDesc->m_pNode, pOut);
	
	return -ENOTHING;
}

size_t FrRead (int fd, void *pBuf, int nBytes)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	if (nBytes < 0)
		return -EINVAL;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	int rv = FsRead (pDesc->m_pNode, pDesc->m_nStreamOffset, (uint32_t)nBytes, pBuf, pDesc->m_bBlocking);
	if (rv < 0) 
		return rv;
	
	pDesc->m_nStreamOffset += rv;
	return rv;
}

size_t FrWrite (int fd, void *pBuf, int nBytes)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	if (nBytes < 0)
		return -EINVAL;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	int rv = FsWrite (pDesc->m_pNode, pDesc->m_nStreamOffset, (uint32_t)nBytes, pBuf, pDesc->m_bBlocking);
	if (rv < 0)
		return rv;
	
	pDesc->m_nStreamOffset += rv;
	return rv;
}

int FrIoControl(int fd, unsigned long request, void * argp)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	return FsIoControl(pDesc->m_pNode, request, argp);
}

int FrSeek (int fd, int offset, int whence)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	if (pDesc->m_bIsFIFO)
		return -ESPIPE;
	
	if (whence < 0 || whence > SEEK_END)
		return -EINVAL;
	
	int realOffset = offset;
	
	switch (whence)
	{
		case SEEK_CUR:
			realOffset += pDesc->m_nStreamOffset;
			break;
		case SEEK_END:
			realOffset += pDesc->m_nFileEnd;
			break;
	}
	
	if (realOffset > pDesc->m_nFileEnd)
		return -EOVERFLOW;
	
	pDesc->m_nStreamOffset = realOffset;
	return -ENOTHING;
}

int FrTell (int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	return pDesc->m_nStreamOffset;
}

int FrTellSize (int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	if (pDesc->m_bIsDirectory)
		return -EISDIR;
	
	return pDesc->m_nFileEnd;
}

extern char g_cwd[PATH_MAX+2];

int FrUnlinkInDir(const char* pDirName, const char* pFileName)
{
	FileNode *pDir = FsResolvePath (pDirName, false);
	if (!pDir)
		return -ENOENT;
	
	// note: The file node is as valid as there is a reference to it.
	// FsResolvePath adds a reference to the node, so it will only be invalid
	// when we release the reference.
	int errorCode = FsUnlinkFile (pDir, pFileName);
	FsReleaseReference(pDir);
	return errorCode;
}

int FrChangeDir(const char *pfn)
{
	FileNode *pNode = FsResolvePath(pfn, true);
	if (!pNode) return -ENOENT;
	
	if (pNode->m_type == FILE_TYPE_SYMBOLIC_LINK)
	{
		FsReleaseReference(pNode);
		return -ELOOP;
	}
	
	if (!(pNode->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pNode);
		return -ENOTDIR;
	}
	
	FileNode* pOldCWD = g_pCwdNode;
	
	// set the new CWD node:
	g_pCwdNode = pNode;
	
	// release the old one:
	FsReleaseReference(pOldCWD);
	
	// here, we didn't need to FsAddReference and FsReleaseReference, since they'd cancel each other out, and
	// FsResolvePath() adds 1 to the reference counter anyways
	
	return -ENOTHING;
}

int FrFileDesChangeDir(int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	FileDescriptor* pDesc = &g_FileNodeToDescriptor[fd];
	
	FileNode *pNode = pDesc->m_pNode;
	
	if (pNode->m_type == FILE_TYPE_SYMBOLIC_LINK)
	{
		FsReleaseReference(pNode);
		return -ELOOP;
	}
	
	if (!(pNode->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pNode);
		return -ENOTDIR;
	}
	
	FileNode* pOldCWD = g_pCwdNode;
	
	// add a reference to it:
	FsAddReference(pNode);
	
	// set the new CWD node:
	g_pCwdNode = pNode;
	
	// release the old one:
	FsReleaseReference(pOldCWD);
	
	return -ENOTHING;
}

//note: This only works with absolute paths that have been checked for length.
int FrRename(const char* pDirOld, const char* pNameOld, const char* pDirNew, const char* pNameNew)
{
	// resolve the directories
	FileNode* pDirNodeOld = FsResolvePath(pDirOld, true);
	
	if (!pDirNodeOld)
		return -ENOENT;
	
	FileNode* pDirNodeNew = FsResolvePath(pDirNew, true);
	
	if (!pDirNodeNew)
	{
		FsReleaseReference(pDirNodeOld);
		return -ENOENT;
	}
	
	if (pDirNodeOld->m_pFileSystemHandle  != pDirNodeNew->m_pFileSystemHandle ||
		pDirNodeOld->m_pFileOps           != pDirNodeNew->m_pFileOps          ||
		pDirNodeOld->m_pFileOps->RenameOp != pDirNodeNew->m_pFileOps->RenameOp)
	{
		// No cross file system action allowed. Must use manual copy / delete combo.
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return -EXDEV;
	}
	
	if (!pDirNodeOld->m_bHasDirCallbacks || !pDirNodeNew->m_bHasDirCallbacks)
	{
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return -ENOTDIR;
	}
	
	if (!pDirNodeOld->m_pFileOps->RenameOp)
	{
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return -ENOTSUP;
	}
	
	// If the file already exists we must overwrite it. If we can't do that, simply bail.
	FileNode* pNodeWeWillOverwrite = NULL;
	
	int result = FsFindDir(pDirNodeNew, pNameNew, &pNodeWeWillOverwrite);
	
	// if we failed, and it wasn't because of a 'file not found' error:
	if (FAILED(result) && result != ERR_NO_FILE)
	{
		// bail right away
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return result;
	}
	
	if (pNodeWeWillOverwrite)
	{
		int result = -ENOTSUP; // can't overwrite the entry
		
		// If we have an unlink function in the new dir node:
		if (pDirNodeNew->m_pFileOps->UnlinkFile)
		{
			// Try to unlink this.
			result = pDirNodeNew->m_pFileOps->UnlinkFile(pDirNodeNew, pNameNew);
		}
		
		// if we couldn't
		if (result != -ENOTHING)
		{
			FsReleaseReference(pDirNodeOld);
			FsReleaseReference(pDirNodeNew);
			return result;
		}
		
		FsReleaseReference(pNodeWeWillOverwrite);
	}
	
	// okay, now, we should be able to just perform the rename operation
	result = pDirNodeOld->m_pFileOps->RenameOp(pDirNodeOld, pDirNodeNew, pNameOld, pNameNew);
	
	FsReleaseReference(pDirNodeOld);
	FsReleaseReference(pDirNodeNew);
	return result;
}

int FrMakeDir(const char* pDirName, const char* pFileName)
{
	if (!*pDirName)
		pDirName = ".";
	
	FileNode *pNode = FsResolvePath(pDirName, false);
	if (!pNode) return -ENOENT;
	
	int status = FsCreateDir(pNode, pFileName);
	
	FsReleaseReference(pNode);
	return status;
}

int FrRemoveDir(const char* pPath)
{
	FileNode* pNode = FsResolvePath(pPath, false);
	if (!pNode)
		return -ENOENT;
	
	if (!pNode->m_pFileOps->RemoveDir)
		return -ENOTSUP;
	
	int status = pNode->m_pFileOps->RemoveDir(pNode);
	FsReleaseReference(pNode);
	return status;
}

void FrCloseAllFilesFromTask(void* task)
{
	// ok, the lock is now free, we should start closing files.
	for (int i = 0; i < (int)ARRAY_COUNT(g_FileNodeToDescriptor); i++)
	{
		if (g_FileNodeToDescriptor[i].m_ownerTask == task)
			FrClose(i);
	}
	for (int i = 0; i < (int)ARRAY_COUNT(g_FileNodeToDescriptor); i++)
	{
		if (g_FileNodeToDescriptor[i].m_ownerTask == task)
			FrCloseDir(i);
	}
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
	"Symbolic link chain too deep",
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
	"Invalid input/output control request",
	"Range error",
	"Domain error",
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

void FsUtilAddArbitraryFileNode(const char* pDirPath, const char* pFileName, FileNode* pSrcNode)
{
	FileNode* pFN = FsResolvePath(pDirPath, false);
	int failure = 0;
	if (!pFN)
	{
	_fail:
		SLogMsg("Couldn't FsUtilAddArbitraryFileNode('%s', '%s')! (failure ID: %d)", pDirPath, pFileName, failure);
		
		if (pFN)
			FsReleaseReference(pFN);
		
		return;
	}
	
	if (pSrcNode->m_bHasDirCallbacks)
	{
		failure = pFN->m_pFileOps->CreateDir(pFN, pFileName);
		if (failure < 0)
			goto _fail;
	}
	else
	{
		failure = pFN->m_pFileOps->CreateFile(pFN, pFileName);
		if (failure < 0)
			goto _fail;
	}
	
	// it worked, look it up:
	FileNode* pNewNode = NULL;
	FsFindDir(pFN, pFileName, &pNewNode);
	
	if (!pNewNode)
	{
		failure = 1;
		goto _fail;
	}
	
	void* oldParentSpecific  = pNewNode->m_pParentSpecific;
	void* oldParent          = pNewNode->m_pParent;
	uint32_t oldParentDirIdx = pNewNode->m_parentDirIndex;
	
	// copy the new node's contents over the old one:
	*pNewNode = *pSrcNode;
	
	// make sure to pin it
	pNewNode->m_refCount = NODE_IS_PERMANENT;
	
	// restore the old "parent specific" stuff...
	pNewNode->m_pParentSpecific = oldParentSpecific;
	pNewNode->m_pParent         = oldParent;
	pNewNode->m_parentDirIndex  = oldParentDirIdx;
	
	// unreference the parent.
	FsReleaseReference(pFN);
}
