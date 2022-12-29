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
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16 //to be OR'd into the other flags
};

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

// Function pointer definitions so we can just call `file_node->Read(...);` etc.
typedef uint32_t 		(*FileReadFunc)       (struct FSNodeS*, uint32_t, uint32_t, void*, bool bBlock);
typedef uint32_t 		(*FileWriteFunc)      (struct FSNodeS*, uint32_t, uint32_t, void*, bool bBlock);
typedef bool     		(*FileOpenFunc)       (struct FSNodeS*, bool, bool);
typedef void     		(*FileCloseFunc)      (struct FSNodeS*);
typedef bool            (*FileOpenDirFunc)    (struct FSNodeS*);
typedef void            (*FileCloseDirFunc)   (struct FSNodeS*);
typedef struct DirEntS* (*FileReadDirFunc)    (struct FSNodeS*, uint32_t*, struct DirEntS*);
typedef struct FSNodeS* (*FileFindDirFunc)    (struct FSNodeS*, const char* pName);
typedef void            (*FileEmptyFileFunc)  (struct FSNodeS* pFileNode);
typedef int             (*FileCreateFileFunc) (struct FSNodeS* pDirectoryNode, const char* pName);
typedef int             (*FileCreateDirFunc)  (struct FSNodeS* pFileNode, const char* pName);
typedef int             (*FileUnlinkFileFunc) (struct FSNodeS* pDirectoryNode, const char* pName);
typedef int             (*FileRemoveDirFunc)  (struct FSNodeS* pDirectoryNode);
typedef int             (*FileRenameOpFunc)   (struct FSNodeS* pSourceDir, struct FSNodeS* pDestinationDir, const char* pSourceName, const char* pDestinationName);
typedef int             (*FileIoControlFunc)  (struct FSNodeS* pFileNode, unsigned long request, void * argp);
typedef void            (*FileOnUnreferencedFunc) (struct FSNodeS* pNode);

// If C_FILE_NODES_PER_POOL_UNIT is over 64, be sure to adjust m_bNodeFree accordingly.
#define C_FILE_NODES_PER_POOL_UNIT 64

struct tagFsPoolUnit;

typedef struct FSNodeS
{
	uint32_t           m_refCount;
	void*              m_pFileSystemHandle; // used to check if two FileNodes are part of the same file system
	
	//note: try not to use the m_name field. On ext2, it's going to always be the first thing that was found with that inode.
	char 	           m_name[128]; //+nullterm, so actually 127 chars
	uint32_t           m_type;
	uint32_t           m_perms;
	uint32_t           m_flags;
	uint32_t           m_inode;      //device specific
	uint32_t           m_length;     //file size
	
	// implementation data
	union
	{
		struct
		{
			uint32_t           m_implData;
			uint32_t           m_implData1;
			uint32_t           m_implData2;
			uint32_t           m_implData3;
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
	uint32_t           m_modifyTime;
	uint32_t           m_createTime;
	uint32_t           m_accessTime;
	
	// File callbacks
	FileReadFunc       Read;
	FileWriteFunc      Write;
	FileOpenFunc       Open;
	FileCloseFunc      Close;
	// Opens a directory, and loads it into the file system structure.
	FileOpenDirFunc    OpenDir;
	// Returns the N-th child of a directory.
	FileReadDirFunc    ReadDir;
	// Tries to find a child with a name in a directory.
	FileFindDirFunc    FindDir;
	// Closes a directory.
	FileCloseDirFunc   CloseDir;
	// Creates an empty file and sets it up inside the file system.
	FileCreateFileFunc CreateFile;
	// Removes all the contents of a file.
	FileEmptyFileFunc  EmptyFile;
	// Creates an empty directory and sets up all the necessary stuff.
	FileCreateDirFunc  CreateDir;
	// Removes a specific link. On FAT32, this will always delete the file, since cluster chains don't have reference counts.
	// On Ext2, this removes one of the links to the file's inode. If there are no more links, the inode itself is deleted.
	FileUnlinkFileFunc UnlinkFile;
	// Removes an empty directory.
	FileRemoveDirFunc  RemoveDir;
	// This operation is a bit strange, in that it takes two file nodes (neither must necessarily be this one).
	FileRenameOpFunc   RenameOp;
	// Sends a request to the underlying device.
	FileIoControlFunc  IoControl;
	// This function is called everytime the reference count of a file reaches zero.
	FileOnUnreferencedFunc OnUnreferenced;
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
uint32_t FsRead      (FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, bool block);
uint32_t FsWrite     (FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, bool block);
bool     FsOpen      (FileNode* pNode, bool read, bool write);
void     FsClose     (FileNode* pNode);
bool     FsOpenDir   (FileNode* pNode);
void     FsCloseDir  (FileNode* pNode);
DirEnt*  FsReadDir   (FileNode* pNode, uint32_t* index, DirEnt* pOutputDent);
FileNode*FsFindDir   (FileNode* pNode, const char* pName); //<-- Note: After using this function's output, use FsReleaseReference!!!
int      FsUnlinkFile(FileNode* pNode, const char* pName);


FileNode*FsResolvePath (const char* pPath);

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
	
	/**
	 * Initializes the initial ramdisk.
	 */
	void FsInitializeInitRd(void* pRamDisk);
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
	size_t FiRead (int fd, void *pBuf, int nBytes);
	
	// Writes to a file.
	size_t FiWrite (int fd, void *pBuf, int nBytes);
	
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
	DirEnt* FiReadDir (int dd);
	
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
	
#endif

#endif//_VFS_H