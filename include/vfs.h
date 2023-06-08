/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

   Virtual FileSystem module headerfile
******************************************/
#ifndef _VFS_H
#define _VFS_H

#include <main.h>

struct FSNodeS;
struct DirEntS;

#define PATH_SEP ('/')
#define PATH_THISDIR (".")
#define PATH_PARENTDIR ("..")

#define FS_DEVICE_NAME "Device"
#define FS_FSROOT_NAME "FSRoot"

#define C_MAX_SYMLINK_DEPTH (8)

// if the node has this many references, it's clear we never want it gone
// The root node has this property.
#define NODE_IS_PERMANENT (2000000000)

#define FD_MAX 1024

enum
{
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FILE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_PIPE,
	FILE_TYPE_SYMBOLIC_LINK,
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16, //to be OR'd into the other flags
};

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

// Function pointer definitions so we can just call `file_node->m_fileOps->Read(...);` etc.

// Reads data from a file node.
typedef int(*FileReadFunc)       (struct FSNodeS*, uint32_t, uint32_t, void*, bool bBlock);
// Writes data to a file node.
typedef int(*FileWriteFunc)      (struct FSNodeS*, uint32_t, uint32_t, const void*, bool bBlock);
// Tries to open a file node.
typedef int(*FileOpenFunc)       (struct FSNodeS*, bool, bool);
// Lets the underlying file system know that the file has been closed.
typedef int(*FileCloseFunc)      (struct FSNodeS*);
// Tries to open a directory file node.
typedef int(*FileOpenDirFunc)    (struct FSNodeS*);
// Lets the underlying file system know that the directory has been closed.
typedef int(*FileCloseDirFunc)   (struct FSNodeS*);
// Reads a single directory entry. Returns 1 if reached the end, 0 if an entry was read, or a negative number for an error.
typedef int(*FileReadDirFunc)    (struct FSNodeS*, uint32_t*, struct DirEntS*);
// Attempts to locate a file in a directory. A returned FileNode* must be released with FsReleaseReference().
typedef int(*FileFindDirFunc)    (struct FSNodeS*, const char* pName, struct FSNodeS** pOut);
// Clear a file's contents.
typedef int(*FileEmptyFileFunc)  (struct FSNodeS* pFileNode);
// Create a file in a directory.
typedef int(*FileCreateFileFunc) (struct FSNodeS* pDirectoryNode, const char* pName);
// Create a directory in a directory.
typedef int(*FileCreateDirFunc)  (struct FSNodeS* pFileNode, const char* pName);
// Removes a directory entry (hard link) from a directory.
typedef int(*FileUnlinkFileFunc) (struct FSNodeS* pDirectoryNode, const char* pName);
// Removes an empty directory from a directory.
typedef int(*FileRemoveDirFunc)  (struct FSNodeS* pDirectoryNode);
// Renames a file. pSourceDir and pDestinationDir will be part of the same file system.
typedef int(*FileRenameOpFunc)   (struct FSNodeS* pSourceDir, struct FSNodeS* pDestinationDir, const char* pSourceName, const char* pDestinationName);
// Sends an I/O control request to a device file.
typedef int(*FileIoControlFunc)  (struct FSNodeS* pFileNode, unsigned long request, void * argp);
// Lets the underlying file system know that the file node was totally unreferenced (its reference count is now 0)
typedef void(*FileOnUnreferencedFunc) (struct FSNodeS* pNode);

// If C_FILE_NODES_PER_POOL_UNIT is over 64, be sure to adjust m_bNodeFree accordingly.
#define C_FILE_NODES_PER_POOL_UNIT 64

struct tagFsPoolUnit;

typedef struct FileNodeOps
{
	FileOnUnreferencedFunc OnUnreferenced;
	FileReadFunc       Read;
	FileWriteFunc      Write;
	FileOpenFunc       Open;
	FileCloseFunc      Close;
	FileEmptyFileFunc  EmptyFile;
	FileIoControlFunc  IoControl;
	FileOpenDirFunc    OpenDir;
	FileReadDirFunc    ReadDir;
	FileFindDirFunc    FindDir;
	FileCloseDirFunc   CloseDir;
	FileCreateFileFunc CreateFile;
	FileCreateDirFunc  CreateDir;
	FileUnlinkFileFunc UnlinkFile;
	FileRemoveDirFunc  RemoveDir;
	FileRenameOpFunc   RenameOp;
}
FileNodeOps;

typedef struct FSNodeS
{
	uint32_t m_refCount;
	void*    m_pFileSystemHandle; // used to check if two FileNodes are part of the same file system
	
	// specific to the parent directory's file system and may not be modified by the child.
	// This is used for mountpoints.
	void*    m_pParentSpecific;
	
	uint8_t  m_type;
	uint8_t  m_perms;
	bool     m_bHasDirCallbacks;
	
	
	uint32_t m_inode;      //device specific
	uint32_t m_length;     //file size
	
	// implementation data
	union
	{
		struct
		{
			uint32_t m_implData;
			uint32_t m_implData1;
			uint32_t m_implData2;
			uint32_t m_implData3;
		};
		struct
		{
			uint8_t* buffer;
			uint32_t bufferSize;
			uint32_t bufferTail;
			uint32_t bufferHead;
		}
		m_pipe;
	};
	
	// timing
	uint32_t m_modifyTime;
	uint32_t m_createTime;
	uint32_t m_accessTime;
	
	const FileNodeOps* m_pFileOps;
}
FileNode;

typedef struct tagFsPoolUnit
{
	uint64_t    m_bNodeFree; // Is this node free?
	uint32_t    m_nNodeLA  [C_FILE_NODES_PER_POOL_UNIT]; // Node last accessed when?
	
	FileNode    m_nodes    [C_FILE_NODES_PER_POOL_UNIT]; // The nodes themselves
	
	struct tagFsPoolUnit* m_pNextUnit;
}
FsPoolUnit;

FileNode* MakeFileNodeFromPool();
void FreeFileNode(FileNode *pFileNode);

typedef struct DirEntS
{
	char     m_name[128]; //+nullterm, so 127 concrete chars
	uint32_t m_inode;     //device specific
	uint32_t m_type;
}
DirEnt;

// Gets the root node of the file system.
FileNode* FsGetRootNode();

//Standard read/write/open/close functions.  They are prefixed with Fs to distinguish them
//from FiRead/FiWrite/FiOpen/FiClose, which deal with file handles not nodes.

//Remember the definitions above.

//These are NOT thread safe!  So don't use these.  Instead, use FiXXX functions that work on file descriptors instead.
int FsRead      (FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, bool block);
int FsWrite     (FileNode* pNode, uint32_t offset, uint32_t size, const void* pBuffer, bool block);
int FsOpen      (FileNode* pNode, bool read, bool write);
int FsClose     (FileNode* pNode);
int FsOpenDir   (FileNode* pNode);
int FsCloseDir  (FileNode* pNode);
int FsReadDir   (FileNode* pNode, uint32_t* index, DirEnt* pOutputDent);
int FsFindDir   (FileNode* pNode, const char* pName, FileNode** pOut); //<-- Note: After using this function's output, use FsReleaseReference!!!
int FsUnlinkFile(FileNode* pNode, const char* pName);

// If the path points to a symbolic link, depending on bResolveThroughSymLinks, it does the following:
// - True:  Goes down the symbolic link chain (at a depth of 16), until the file is reached.
//          If the symbolic chain is too deep, this returns one of the symbolic links' file nodes.
// - False: Returns the symbolic link's node instead.
FileNode* FsResolvePath(const char* pPath, bool bResolveThroughSymLinks);

void FsAddReference(FileNode* pNode);
void FsReleaseReference(FileNode* pNode);

void FiDebugDump();

//Ramdisk API:
#if 1

void FsMountRamDisk(void* pRamDisk);

#endif

//Internal functions.  Should only be used by FS drivers
#if 1

void FsInit ();

#endif

//Initrd stuff:
#if 1
	typedef struct
	{
		int m_nFiles;
	}
	InitRdHeader;
	
	//! Same as in `fsmaker`
	typedef struct
	{
		uint32_t m_magic;
		char m_name[64];
		uint32_t m_offset, m_length;
	}
	InitRdFileHeader;

	/**
	* Gets the root entry of the initrd.
	*/
	FileNode* FsGetInitrdNode ();
#endif

// Basic POSIX-like API
#if 1
	//For internal use.
	typedef struct {
		bool      m_bOpen;
		FileNode *m_pNode;
		char      m_sPath[PATH_MAX+2];
		int       m_nStreamOffset;
		int       m_nFileEnd;
		bool      m_bIsFIFO; //is a char device, basically
		bool      m_bBlocking;
		
		const char* m_openFile;
		int       m_openLine;
		
		void*     m_ownerTask;
	}
	FileDescriptor;
	
	typedef struct {
		bool      m_bOpen;
		FileNode *m_pNode;
		char      m_sPath[PATH_MAX+2];
		uint32_t  m_nStreamOffset;
		DirEnt    m_sCurDirEnt;
		
		const char* m_openFile;
		int       m_openLine;
		
		void*     m_ownerTask;
	}
	DirDescriptor ;
	
	typedef struct
	{
		uint32_t m_type;
		uint32_t m_size;
		uint32_t m_blocks;
		uint32_t m_inode;
		
		uint32_t m_perms;
		uint32_t m_modifyTime;
		uint32_t m_createTime;
	}
	StatResult;
	
	// I/O control requests
	#include <ioctl.h>
	
	#define O_RDONLY   (1)
	#define O_WRONLY   (2)
	#define O_RDWR     (O_RDONLY | O_WRONLY)
	#define O_APPEND   (4)
	#define O_CREAT    (8)
	#define O_NONBLOCK (16)
	#define O_TRUNC    (32)
	#define O_EXEC     (1024)
	
	//lseek whences
	#define SEEK_SET 0
	#define SEEK_CUR 1
	#define SEEK_END 2
	
	#include <errors.h>
	
	// Opens a file and returns its descriptor.
	int FiOpenD (const char* pFileName, int oflag, const char* srcFile, int srcLine);
	#define FiOpen(pFileName, oflag)  FiOpenD(pFileName,oflag,__FILE__,__LINE__)
	
	// Opens an arbitrary file node and returns its descriptor.
	int FiOpenFileNodeD (FileNode* pFileNode, int oflag, const char* srcFile, int srcLine);
	#define FiOpenFileNode(pFileNode, oflag)  FiOpenFileNodeD(pFileNode,oflag,__FILE__,__LINE__)
	
	// Closes a file and frees its descriptor for use.
	int FiClose (int fd);
	
	// Reads from a file.
	int FiRead (int fd, void *pBuf, int nBytes);
	
	// Writes to a file.
	int FiWrite (int fd, void *pBuf, int nBytes);
	
	// Returns the current stream position of a file.
	int FiTell (int fd);
	
	// Returns the current size of a file.
	int FiTellSize (int fd);
	
	// Seeks around a file, if it's seekable.
	int FiSeek (int fd, int offset, int whence);
	
	// Opens a directory and returns a descriptor
	int FiOpenDirD (const char* pFileName, const char* srcFile, int srcLine);
	#define FiOpenDir(pFileName) FiOpenDirD(pFileName, __FILE__, __LINE__)
	
	// Closes a directory and frees its descriptor for future use.
	int FiCloseDir (int dd);
	
	// Reads a directory entry from the directory and advances the stream pointer.
	int FiReadDir(DirEnt* pDirEnt, int dd);
	DirEnt* FiReadDirLegacy(int dd);
	
	// Seeks on a directory descriptor.
	int FiSeekDir (int dd, int loc);
	
	// Rewinds the directory, or moves the directory entry pointer to the start.
	int FiRewindDir (int dd);
	
	// Get the current file directory offset.
	int FiTellDir (int dd);

	// Retrieves information about a file in the directory.
	int FiStatAt (int dd, const char *pFileName, StatResult* pOut);

	// Retrieves information about a file.
	int FiStat (const char *pFileName, StatResult* pOut);
	
	// Retrieves information about a file. If the file name points to a symbolic link, it stats the symbolic link.
	int FiLinkStat (const char *pFileName, StatResult* pOut);
	
	// Changes the current directory.
	int FiChangeDir (const char *pfn);
	
	// Removes a link to a file. If this is the last reference to a file, the file is deleted.
	int FiUnlinkFile (const char *pfn);
	
	// Moves a file from 'source' path to 'destination' path.
	int FiRename(const char* pfnOld, const char* pfnNew);
	
	// Create a new directory.
	int FiMakeDir(const char* pPath);
	
	// Delete an empty directory.
	int FiRemoveDir(const char* path);
	
	// Create a pipe set.
	int FiCreatePipe(const char* pFriendlyName, int fds[2], int oflags);
	
	// Send an I/O control request to a device on a file descriptor.
	// If the file description does not point to a device file, this returns -ENOTTY.
	int FiIoControl(int fd, unsigned long request, void * argp);
	
	// Releases all file resources owned by a task.
	void FiReleaseResourcesFromTask(void* task);
	
	// Gets the current working directory of the running process.
	const char* FiGetCwd();
	
#endif

#endif//_VFS_H