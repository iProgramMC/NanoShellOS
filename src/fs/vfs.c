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

// Shorthands for common FS operations
#if 1

uint32_t FsFileRead(File* file, void *pOut, uint32_t size)
{
	if (!file->Read)
		return 0;
	
	return file->Read(file, pOut, size);
}
uint32_t FsFileWrite(File* file, void *pIn, uint32_t size)
{
	if (!file->Write)
		return 0;
	
	return file->Write(file, pIn, size);
}
void FsFileClose(File* file)
{
	if (!file->Close) return;
	
	return file->Close(file);
}
bool FsFileSeek(File* file, int pos, int whence, bool bAllowExpansion)
{
	if (!file->Seek)
		return true;
	
	return file->Seek(file, pos, whence, bAllowExpansion);
}
int FsFileTell (File *file)
{
	if (!file->Tell)
		return 0;
	
	return file->Tell(file);
}

void FsDirectoryClose (Directory *pDirectory)
{
	if (!pDirectory->Close)
		return;
	
	pDirectory->Close(pDirectory);
}
bool FsDirectoryReadEntry (Directory *pDirectory, DirectoryEntry *pDirectoryEntryOut)
{
	if (!pDirectory->ReadEntry)
		return false; //! what would be the purpose of a directory if you can't read from it?
	
	return pDirectory->ReadEntry(pDirectory, pDirectoryEntryOut);
}
void FsDirectorySeek (Directory *pDirectory, int position)
{
	if (!pDirectory->Seek)
		return;
	
	pDirectory->Seek(pDirectory, position);
}
int FsDirectoryTell (Directory *pDirectory)
{
	if (!pDirectory->Tell)
		return 0;
	
	return pDirectory->Tell(pDirectory);
}

File* FsOpenFile(DirectoryEntry* entry)
{
	FileSystem *pFS = FsResolveByFileID(entry->file_id);
	if (!pFS) return NULL;
	
	return pFS->OpenInt(pFS, entry);
}
Directory* FsOpenDir(FileID fileID)
{
	FileSystem *pFS = FsResolveByFileID(fileID);
	if (!pFS) return NULL;
	
	return pFS->OpenDir(pFS, (uint32_t)fileID);
}
bool FsLocateFileInDir(FileID dirID, const char *pFN, DirectoryEntry *pEntryOut)
{
	FileSystem *pFS = FsResolveByFileID (dirID);
	if (!pFS) return NULL;
	
	return pFS->LocateFileInDir (pFS, (uint32_t)dirID, pFN, pEntryOut);
}
uint32_t FsGetRootDirID(FileSystem *pFS)
{
	return pFS->GetRootDirID(pFS);
}

#endif

// Mount Manger
#if 1

FileSystem *g_pMountedFileSystems[C_MAX_FILE_SYSTEMS];
FileID g_file_root = 0;

bool FsMountFileSystem(FileSystem *pFS)
{
	int freeArea = -1;
	for (int i = 0; i < C_MAX_FILE_SYSTEMS; i++)
	{
		if (!g_pMountedFileSystems[i])
		{
			freeArea = i;
			break;
		}
	}
	if (freeArea < 0) return false;
	
	g_pMountedFileSystems [freeArea] = pFS;
	
	return true;
}

FileSystem* FsResolveByFsID(uint32_t fsID)
{
	for (int i = 0; i < C_MAX_FILE_SYSTEMS; i++)
	{
		if (g_pMountedFileSystems[i]  &&  g_pMountedFileSystems[i]->fsID == fsID)
			return g_pMountedFileSystems[i];
	}
	return NULL;
}
FileSystem* FsResolveByFileID(FileID id)
{
	return FsResolveByFsID((uint32_t)((uint64_t)id >> 32));
}

FileID FsGetGlobalRoot()
{
	return g_file_root;
}

void FsSetGlobalRoot(FileID id)
{
	g_file_root = id;
}

FileID FsResolveFilePath (const char *pPath, DirectoryEntry *pEntryOut)
{
	TokenState state;
	
	state.m_bInitted = false;
	
	char path[PATH_MAX + 5];
	strcpy(path, pPath);
	
	char *path1 = Tokenize (&state, path, "/");
	if (*path1 != 0)
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
			return NO_FILE;
		
		// try to resolve this as a standard path
		return FsResolveFilePath (path_copy, pEntryOut);
	}
	
	FileID currentFile = FsGetGlobalRoot();
	bool   bIsThisADir = true;
	
	strcpy(pEntryOut->name, "root");
	pEntryOut->type  = FILE_TYPE_FILE | FILE_TYPE_DIRECTORY;
	pEntryOut->file_length  = -1;
	pEntryOut->file_id      = currentFile;
	
	while (true)
	{
		path1 = Tokenize(&state, NULL, "/");
		if (!path1) break;
		if (!*path1) break;
		
		if (!bIsThisADir)
		{
			SLogMsg("Path '%s' is malformed - trying to access files inside a file that isn't a directory", pPath);
			return NO_FILE;
		}
		
		FileSystem *pFS = FsResolveByFileID(currentFile);
		bool b = pFS->LocateFileInDir(pFS, (uint32_t)currentFile, path1, pEntryOut);
		if (!b)
		{
			SLogMsg("Path '%s' couldn't be located (%s)", pPath, path1);
			return NO_FILE;
		}
		
		bIsThisADir = !!(pEntryOut->type & FILE_TYPE_DIRECTORY);
		currentFile = pEntryOut->file_id;
	}
	
	return currentFile;
}

void FsInit()
{
}
#endif

// File Descriptor handlers:
#if 1

FileDescriptor g_FileNodeToDescriptor[FD_MAX];
DirDescriptor  g_DirNodeToDescriptor [FD_MAX];

void FiDebugDump()
{
	/*LogMsg("Listing opened files.");
	for (int i = 0; i < FD_MAX; i++)
	{
		FileDescriptor* p = &g_FileNodeToDescriptor[i];
		if (p->m_bOpen)
		{
			LogMsg("FD:%d\tFL:%d\tFS:%d\tP:%d FN:%x FC:%s:%d\tFN:%s",i,p->m_nFileEnd,p->m_nStreamOffset,
					p->m_bIsFIFO, p->m_pNode, p->m_openFile, p->m_openLine, p->m_sPath);
		}
	}
	LogMsg("Done");*/
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
		{
			if (strcmp (g_DirNodeToDescriptor[i].m_sPath, reqPath) == 0)
			{
				SLogMsg("Found same file open");
				return -EAGAIN;
			}
		}
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
	/*
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
	*/
	
	return -EIO;
}

int FiClose (int fd)
{
	/*
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
	*/
	return -EIO;
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
	DirectoryEntry entry;
	
	FileID fileID = FsResolveFilePath(pFileName, &entry);
	if (!fileID)
	{
		// No File
		LockFree (&g_FileSystemLock);
		return -ENOENT;
	}
	
	if (!(entry.type & FILE_TYPE_DIRECTORY))
	{
		// Not a Directory
		LockFree (&g_FileSystemLock);
		return -ENOTDIR;
	}
	
	// Try to open the Directory
	Directory* directory = FsOpenDir(fileID);
	if (!directory)
	{
		LockFree (&g_FileSystemLock);
		return -EIO; // Cannot open the directory
	}
	
	//we have all the perms, let's write the filenode there:
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_bOpen 			= true;
	strcpy(pDesc->m_sPath, pFileName);
	pDesc->m_pDir  			= directory;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	
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
	
	pDesc->m_pDir->Close(pDesc->m_pDir);
	
	pDesc->m_pDir = NULL;
	
	LockFree (&g_FileSystemLock);
	return -ENOTHING;
}

DirectoryEntry* FiReadDir (int dd)
{
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return NULL;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	bool result = pDesc->m_pDir->ReadEntry(pDesc->m_pDir, &pDesc->m_sCurDirEnt);
	
	if (!result)
	{
		LockFree (&g_FileSystemLock);
		return NULL;
	}
	
	LockFree (&g_FileSystemLock);
	return &pDesc->m_sCurDirEnt;
}

int FiSeekDir (int dd, int loc)
{
	return -EIO;
	/*
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
	*/
}

int FiRewindDir (int dd)
{
	return FiSeekDir (dd, 0);
}

int FiTellDir (int dd)
{
	return -EIO;
	/*
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDirDescriptor(dd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	LockFree (&g_FileSystemLock);
	return pDesc->m_nStreamOffset;
	*/
}

int FiStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	return -EIO;
	/*
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
	*/
}

int FiStat (const char *pFileName, StatResult* pOut)
{
	return -EIO;
	/*
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
	*/
}

size_t FiRead (int fd, void *pBuf, int nBytes)
{
	return -EIO;
	/*
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
	*/
}

//TODO
size_t FiWrite (int fd, void *pBuf, int nBytes)
{
	return -EIO;
	/*
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
	*/
}

int FiSeek (int fd, int offset, int whence)
{
	return -EIO;
	/*
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
	*/
}

int FiTell (int fd)
{
	return -EIO;
	/*
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nStreamOffset;
	LockFree (&g_FileSystemLock);
	return rv;
	*/
}

int FiTellSize (int fd)
{
	return -EIO;
	/*
	LockAcquire (&g_FileSystemLock);
	if (!FiIsValidDescriptor(fd))
	{
		LockFree (&g_FileSystemLock);
		return -EBADF;
	}
	int rv = g_FileNodeToDescriptor[fd].m_nFileEnd;
	LockFree (&g_FileSystemLock);
	return rv;
	*/
}


extern char g_cwd[PATH_MAX+2];
int FiRemoveFile (const char *pfn)
{
	return -EIO;
	/*
	FileNode *p = FsResolvePath (pfn);
	if (!p) return -ENOENT;
	
	return FsRemoveFile (p); // Node no longer valid : )
	*/
}
int FiChangeDir (const char *pfn)
{
	if (*pfn == '\0') return -ENOTHING;//TODO: maybe cd into their home directory instead?
	
	int slen = strlen (pfn);
	if (slen >= PATH_MAX) return -EOVERFLOW;
	
	DirectoryEntry entry;
	
	if (pfn[0] == '/')
	{
		// Absolute Path
		FileID fileID = FsResolveFilePath (pfn, &entry);
		
		if (!fileID) return -EEXIST;
		if (!(entry.type & FILE_TYPE_DIRECTORY)) return -ENOTDIR;
		
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
	FileID fileID = FsResolveFilePath (pfn, &entry);
	
	if (!fileID)
		return -ENOENT; //does not exist
	if (!(entry.type & FILE_TYPE_DIRECTORY))
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
	"Computer bought the farm",
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



























