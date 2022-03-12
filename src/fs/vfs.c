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
 
#define MULTITASKED_WINDOW_MANAGER//to make ACQUIRE_LOCK and FREE_LOCK to not build to nothing
#include <vfs.h>
#include <string.h>
#include <memory.h>
#include <misc.h>

uint32_t FsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	if (pNode)
	{
		if (pNode->Read)
			return pNode->Read(pNode, offset, size, pBuffer);
		else return 0;
	}
	else return 0;
}
uint32_t FsWrite(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	if (pNode)
	{
		if (pNode->Write)
		{
			return pNode->Write(pNode, offset, size, pBuffer);
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
DirEnt* FsReadDir(FileNode* pNode, uint32_t index)
{
	if (pNode)
	{
		if (pNode->ReadDir && (pNode->m_type & FILE_TYPE_DIRECTORY))
			return pNode->ReadDir(pNode, index);
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
		if (pNode->OpenDir && (pNode->m_type & FILE_TYPE_DIRECTORY))
			pNode->OpenDir(pNode);
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
FileNode* FsCreateEmptyFile(FileNode* pDirNode, const char* pFileName)
{
	if (pDirNode)
	{
		if (pDirNode->CreateFile && (pDirNode->m_type & FILE_TYPE_DIRECTORY))
			return pDirNode->CreateFile(pDirNode, pFileName);
		return NULL;
	}
	return NULL;
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
		//LogMsg("Resolving absolute path '%s'", pPath);
		FileNode *pNode = FsGetRootNode ();//TODO
		while (true)
		{
			char* path = Tokenize (&state, NULL, "/");
			
			//are we done?
			if (path && *path)
			{
				//LogMsg("Finding directory: '%s'", path);
				//nope, resolve pNode again.
				pNode = FsFindDir (pNode, path);
				if (!pNode)
				{
					//LogMsg("File not found inside %s", path);
					return NULL;
				}
			}
			else
			{
				//LogMsg("Done!  Returning to caller...");
				return pNode;
			}
		}
	}
	else
	{
		//TODO
		//LogMsg("Not an absolute path");
		return NULL;
	}
}

// Default
#if 1
FileNode g_root;

FileNode* FsGetRootNode ()
{
	return &g_root;
}
// Basic functions


FileNode* CreateFileNode (FileNode* pParent)
{
	//Create the file node
	FileNode* pNode = MmAllocateK (sizeof (FileNode));
	if (!pNode)
		return pNode;
	
	memset (pNode, 0, sizeof *pNode);
	
	//Add it to the parent
	pNode->parent = pParent;
	if (pParent->children)
	{
		FileNode *pLastEnt = pParent->children;
		while (pLastEnt->next)
		{
			pLastEnt = pLastEnt->next;
		}
		
		pLastEnt->next = pNode;
		pNode   ->prev = pLastEnt;
	}
	else
	{
		pParent->children = pNode;
		pNode->next = pNode->prev = NULL;
	}
	return pNode;
}
// Also removes the underlying tree from the VFS, so you can easily do EraseFileNode(&g_root).  Don't do this.
void EraseFileNode (FileNode* pFileNode)
{
	// First of all, recursively delete children
	while (pFileNode->children)
	{
		EraseFileNode (pFileNode->children);
	}
	
	//If there's a previous node (which may or may not have pFileNode->parent->children
	//set to it), override that one's next value to be our next value.
	if (pFileNode->prev)
	{
		pFileNode->prev->next = pFileNode->next;
	}
	//If we don't have a previous, it's guaranteed that the parent's children value
	//points to us, so give it the next pointer on our list
	else if (pFileNode->parent)
	{
		pFileNode->parent->children = pFileNode->next;
	}
	//If there's a next element on the list, give its previous-pointer our prev-pointer.
	if (pFileNode->next)
	{
		pFileNode->next->prev = pFileNode->prev;
	}
	
	MmFreeK (pFileNode);
}

void FsInitializeDevicesDir();
//First time setup of the file manager
void FsSetup ()
{
	memset(&g_root, 0, sizeof g_root);
	
	strcpy(g_root.m_name, "root");
	g_root.m_perms = PERM_READ | PERM_WRITE | PERM_EXEC;
	g_root.m_type  = FILE_TYPE_DIRECTORY | FILE_TYPE_FILE;
	
	// Starts off empty, gets more files as it gets up and running.
	
	FsInitializeDevicesDir();
}
#endif

// File Descriptor handlers:
#if 1

FileDescriptor g_FileNodeToDescriptor[FD_MAX];

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

static int FiFindFreeFileDescriptor(const char* reqPath)
{
	for (int i = 0; i < FD_MAX; i++)
	{
		if (g_FileNodeToDescriptor[i].m_bOpen)
			if (strcmp (g_FileNodeToDescriptor[i].m_sPath, reqPath) == 0)
				return -EAGAIN;
	}
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!g_FileNodeToDescriptor[i].m_bOpen)
			return i;
	}
	return -ENFILE;
}

//TODO: improve MT
bool g_fileSystemLock = false;

int FiOpenD (const char* pFileName, int oflag, const char* srcFile, int srcLine)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	// find a free fd to open:
	int fd = FiFindFreeFileDescriptor(pFileName);
	if (fd < 0)
	{
		FREE_LOCK (g_fileSystemLock);
		return fd;
	}
	
	//find the node:
	bool hasClearedAlready = false;
	FileNode* pFile = FsResolvePath(pFileName);
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
				FREE_LOCK (g_fileSystemLock);
				return -EEXIST;
			}
			
			// Try creating a file
			pFile = FsCreateEmptyFile (pDir, fileNameSimple);
			hasClearedAlready = true;
			SLogMsg("Has cleared file already");
			
			if (!pFile)
			{
				FREE_LOCK (g_fileSystemLock);
				return -EEXIST;
			}
		}
		else
		{
			//Can't append to/read from a missing file!
			FREE_LOCK (g_fileSystemLock);
			return -EEXIST;
		}
	}
	
	//if we are trying to read, but we can't:
	if ((oflag & O_RDONLY) && !(pFile->m_perms & PERM_READ))
	{
		FREE_LOCK (g_fileSystemLock);
		return -EACCES;
	}
	//if we are trying to write, but we can't:
	if ((oflag & O_WRONLY) && !(pFile->m_perms & PERM_WRITE))
	{
		FREE_LOCK (g_fileSystemLock);
		return -EACCES;
	}
	//if we are trying to execute, but we can't:
	if ((oflag & O_EXEC) && !(pFile->m_perms & PERM_EXEC))
	{
		FREE_LOCK (g_fileSystemLock);
		return -EACCES;
	}
	
	if (pFile->m_type & FILE_TYPE_DIRECTORY)
	{
		FREE_LOCK (g_fileSystemLock);
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
		FREE_LOCK (g_fileSystemLock);
		return -EIO;
	}
	
	//we have all the perms, let's write the filenode there:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	pDesc->m_bOpen 			= true;
	strcpy(pDesc->m_sPath, pFileName);
	pDesc->m_pNode 			= pFile;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_nFileEnd		= pFile->m_length;
	pDesc->m_bIsFIFO		= pFile->m_type == FILE_TYPE_CHAR_DEVICE;
	
	FREE_LOCK (g_fileSystemLock);
	
	if ((oflag & O_APPEND) && (oflag & O_WRONLY))
	{
		// Automatically seek to the end
		FiSeek(fd, SEEK_END, 0);
	}
	
	return fd;
}
bool FiIsValidDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	if (!g_FileNodeToDescriptor[fd].m_bOpen) return false;
	return true;
}

int FiClose (int fd)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		FREE_LOCK(g_fileSystemLock);
		return -EBADF;
	}
	
	//closes the file:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsClose (pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	
	FREE_LOCK(g_fileSystemLock);
	return -ENOTHING;
}

size_t FiRead (int fd, void *pBuf, int nBytes)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		FREE_LOCK(g_fileSystemLock);
		return -EBADF;
	}
	
	if (nBytes < 0)
	{
		FREE_LOCK(g_fileSystemLock);
		return -EINVAL;
	}
	
	int rv = FsRead (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf);
	if (rv < 0) 
	{
		FREE_LOCK(g_fileSystemLock);
		return rv;
	}
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	FREE_LOCK(g_fileSystemLock);
	return rv;
}

//TODO
size_t FiWrite (int fd, void *pBuf, int nBytes)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		FREE_LOCK(g_fileSystemLock);
		return -EBADF;
	}
	
	if (nBytes < 0)
	{
		FREE_LOCK(g_fileSystemLock);
		return -EINVAL;
	}
	
	int rv = FsWrite (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf);
	if (rv < 0) 
	{
		FREE_LOCK(g_fileSystemLock);
		return rv;
	}
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	FREE_LOCK(g_fileSystemLock);
	return rv;
}

int FiSeek (int fd, int offset, int whence)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		FREE_LOCK(g_fileSystemLock);
		return -EBADF;
	}
	
	if (g_FileNodeToDescriptor[fd].m_bIsFIFO)
	{
		FREE_LOCK(g_fileSystemLock);
		return -ESPIPE;
	}
	
	if (whence < 0 || whence > SEEK_END)
	{
		FREE_LOCK(g_fileSystemLock);
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
		FREE_LOCK(g_fileSystemLock);
		return -EOVERFLOW;
	}
	
	g_FileNodeToDescriptor[fd].m_nStreamOffset = realOffset;
	FREE_LOCK(g_fileSystemLock);
	return -ENOTHING;
}

int FiTell (int fd)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		FREE_LOCK(g_fileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nStreamOffset;
	FREE_LOCK(g_fileSystemLock);
	return rv;
}

int FiTellSize (int fd)
{
	ACQUIRE_LOCK (g_fileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		FREE_LOCK(g_fileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nFileEnd;
	FREE_LOCK(g_fileSystemLock);
	return rv;
}

#endif

