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
int FsRemoveFile(FileNode* pNode)
{
	if (!pNode) return -ENOENT;
	
	if (!pNode->RemoveFile) return -ENOMEM;
	if (pNode->m_type & FILE_TYPE_DIRECTORY) return -ENOMEM;
	
	int e;
	if (!(e = pNode->RemoveFile(pNode)))
	{
		EraseFileNode (pNode);
		return e;
	}
	return e;
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
		FileNode *pNode = FsGetRootNode ();//TODO
		while (true)
		{
			char* path = Tokenize (&state, NULL, "/");
			
			//are we done?
			if (path && *path)
			{
				//nope, resolve pNode again.
				pNode = FsFindDir (pNode, path);
				if (!pNode)
				{
					return NULL;
				}
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
DirDescriptor  g_DirNodeToDescriptor [FD_MAX];

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

static int FiFindFreeDirDescriptor(const char* reqPath)
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

bool FiIsValidDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_FileNodeToDescriptor[fd].m_bOpen;
}

bool FiIsValidDirDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_DirNodeToDescriptor[fd].m_bOpen;
}

int FiOpenD (const char* pFileName, int oflag, const char* srcFile, int srcLine)
{
	LockAcquire (&g_FileSystemLock);
	// find a free fd to open:
	int fd = FiFindFreeFileDescriptor(pFileName);
	if (fd < 0)
	{
		LockFree (&g_FileSystemLock);
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
				LockFree (&g_FileSystemLock);
				return -ENOENT;
			}
			
			// Try creating a file
			pFile = FsCreateEmptyFile (pDir, fileNameSimple);
			hasClearedAlready = true;
			SLogMsg("Has cleared file already");
			
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
	pDesc->m_bOpen 			= true;
	strcpy(pDesc->m_sPath, pFileName);
	pDesc->m_pNode 			= pFile;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_nFileEnd		= pFile->m_length;
	pDesc->m_bIsFIFO		= pFile->m_type == FILE_TYPE_CHAR_DEVICE;
	
	LockFree (&g_FileSystemLock);
	
	if ((oflag & O_APPEND) && (oflag & O_WRONLY))
	{
		// Automatically seek to the end
		FiSeek(fd, SEEK_END, 0);
	}
	
	return fd;
}

int FiClose (int fd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	//closes the file:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsClose (pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

int FiOpenDirD (const char* pFileName, const char* srcFile, int srcLine)
{
	LockAcquire (&g_FileSystemLock);
	// find a free fd to open:
	int dd = FiFindFreeDirDescriptor(pFileName);
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

int FiCloseDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	//closes the file:
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsCloseDir (pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

DirEnt* FiReadDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return NULL;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	DirEnt* pDirEnt = FsReadDir (pDesc->m_pNode, pDesc->m_nStreamOffset);
	if (!pDirEnt)
	{
		LockFree (&g_FileSystemLock);
		return NULL;
	}
	
	pDesc->m_sCurDirEnt = *pDirEnt;
	
	pDesc->m_nStreamOffset++;
	
	LockFree (&g_FileSystemLock);
	return &pDesc->m_sCurDirEnt;
}

int FiSeekDir (int dd, int loc)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
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

int FiRewindDir (int dd)
{
	return FiSeekDir (dd, 0);
}

int FiTellDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	LockFree (&g_FileSystemLock);
	return pDesc->m_nStreamOffset;
}

int FiStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
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
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

size_t FiRead (int fd, void *pBuf, int nBytes)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	if (nBytes < 0)
	{
		LockFree (&g_FileSystemLock);
		return -EINVAL;
	}
	
	int rv = FsRead (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf);
	if (rv < 0) 
	{
		LockFree (&g_FileSystemLock);
		return rv;
	}
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	LockFree (&g_FileSystemLock);
	return rv;
}

//TODO
size_t FiWrite (int fd, void *pBuf, int nBytes)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	if (nBytes < 0)
	{
		LockFree (&g_FileSystemLock);
		return -EINVAL;
	}
	
	int rv = FsWrite (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf);
	if (rv < 0) 
	{
		LockFree (&g_FileSystemLock);
		return rv;
	}
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	LockFree (&g_FileSystemLock);
	return rv;
}

int FiSeek (int fd, int offset, int whence)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
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

int FiTell (int fd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nStreamOffset;
	LockFree (&g_FileSystemLock);
	return rv;
}

int FiTellSize (int fd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nFileEnd;
	LockFree (&g_FileSystemLock);
	return rv;
}


extern char g_cwd[PATH_MAX+2];
int FiRemoveFile (const char *pfn)
{
	FileNode *p = FsResolvePath (pfn);
	if (!p) return -ENOENT;
	
	return FsRemoveFile (p); // Node no longer valid : )
}
int FiChangeDir (const char *pfn)
{
	if (*pfn == '\0') return -ENOTHING;//TODO: maybe cd into their home directory instead?
	
	int slen = strlen (pfn);
	if (slen >= PATH_MAX) return -EOVERFLOW;
	
	
	if (pfn[0] == '/')
	{
		// Absolute Path
		FileNode *pNode = FsResolvePath (pfn);
		if (!pNode) return -EEXIST;
		if (!(pNode->m_type & FILE_TYPE_DIRECTORY)) return -ENOTDIR;
		
		//this should work!
		strcpy (g_cwd, pfn);
		return -ENOTHING;
	}
	
	if (strcmp (pfn, PATH_THISDIR) == 0) return -ENOTHING;
	
	
	char cwd_work [sizeof (g_cwd)];
	memset (cwd_work, 0, sizeof cwd_work);
	strcpy (cwd_work, g_cwd);
	
	//TODO FIXME: make composite paths like "../../test/file" work
	
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
	
	//resolve the path
	FileNode *pNode = FsResolvePath (cwd_work);
	if (!pNode)
		return -ENOENT; //does not exist
	if (!(pNode->m_type & FILE_TYPE_DIRECTORY))
		return -ENOTDIR; //not a directory
	
	//this should work!
	strcpy (g_cwd, cwd_work);
	
	return -ENOTHING;
}

#endif

static const char* ErrorStrings[] = {
	"Success",
	"Permission denied",
	"File exists",
	"Interrupted system call",
	"Invalid argument",
	"I/O error",
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
