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

#define PATH_MAX (260)
#define PATH_SEP ('/')
#define PATH_THISDIR (".")
#define PATH_PARENTDIR ("..")

#define FS_DEVICE_NAME "Device"
#define FS_FSROOT_NAME "FSRoot"

#define FD_MAX 1024

enum
{
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FILE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16 //to be OR'd into the other flags
};

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

// Function pointer definitions so we can just call `file_node->Read(...);` etc.
typedef uint32_t 		(*FileReadFunc)       (struct FSNodeS*, uint32_t, uint32_t, void*);
typedef uint32_t 		(*FileWriteFunc)      (struct FSNodeS*, uint32_t, uint32_t, void*);
typedef bool     		(*FileOpenFunc)       (struct FSNodeS*, bool, bool);
typedef void     		(*FileCloseFunc)      (struct FSNodeS*);
typedef bool            (*FileOpenDirFunc)    (struct FSNodeS*);
typedef void            (*FileCloseDirFunc)   (struct FSNodeS*);
typedef struct DirEntS* (*FileReadDirFunc)    (struct FSNodeS*, uint32_t);
typedef struct FSNodeS* (*FileFindDirFunc)    (struct FSNodeS*, const char* pName);
typedef struct FSNodeS* (*FileCreateFileFunc) (struct FSNodeS* pDirectoryNode, const char* pName);
typedef void            (*FileEmptyFileFunc)  (struct FSNodeS* pFileNode);

typedef struct FSNodeS
{
	char 	           m_name[128]; //+nullterm, so 127 concrete chars
	uint32_t           m_type;
	uint32_t           m_perms;
	uint32_t           m_flags;
	uint32_t           m_inode;      //device specific
	uint32_t           m_length;     //file size
	uint32_t           m_implData;   //implementation data. TODO
	uint32_t           m_implData1;
	uint32_t           m_implData2;
	uint32_t           m_implData3;
	FileReadFunc       Read;
	FileWriteFunc      Write;
	FileOpenFunc       Open;
	FileCloseFunc      Close;
	FileOpenDirFunc    OpenDir;
	FileReadDirFunc    ReadDir;      //returns the n-th child of a directory
	FileFindDirFunc    FindDir;      //try to find a child in a directory by name
	FileCloseDirFunc   CloseDir;
	FileCreateFileFunc CreateFile;
	FileEmptyFileFunc  EmptyFile;
}
FileNode;

typedef struct DirEntS
{
	char     m_name[128]; //+nullterm, so 127 concrete chars
	uint32_t m_inode;     //device specific
}
DirEnt;

/**
 * Gets the root entry of the filesystem.
 */
FileNode* FsGetRootNode();

//Standard read/write/open/close functions.  They are prefixed with Fs to distinguish them
//from FiRead/FiWrite/FiOpen/FiClose, which deal with file handles not nodes.

//Remember the definitions above.

//These are NOT thread safe!  So don't use these.  Instead, use FiXXX functions that work on file descriptors instead.
uint32_t FsRead    (FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer);
uint32_t FsWrite   (FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer);
bool     FsOpen    (FileNode* pNode, bool read, bool write);
void     FsClose   (FileNode* pNode);
bool     FsOpenDir (FileNode* pNode);
void     FsCloseDir(FileNode* pNode);
DirEnt*  FsReadDir (FileNode* pNode, uint32_t index);
FileNode*FsFindDir (FileNode* pNode, const char* pName);


FileNode*FsResolvePath (const char* pPath);

void FiDebugDump();

//Ramdisk API:
#if 1

void FsMountRamDisk(void* pRamDisk);

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
		const char* m_openFile;
		int       m_openLine;
	}
	FileDescriptor;

	#define O_RDONLY (1)
	#define O_WRONLY (2)
	#define O_RDWR   (O_RDONLY | O_WRONLY)
	#define O_APPEND (4)
	#define O_CREAT  (8)
	#define O_EXEC   (1024)
	
	//lseek whences
	#define SEEK_SET 0
	#define SEEK_CUR 1
	#define SEEK_END 2
	
	enum
	{
		ENOTHING,
		EACCES,
		EEXIST,
		EINTR,
		EINVAL,
		EIO,
		EISDIR,
		ELOOP,
		EMFILE,
		ENAMETOOLONG,
		ENFILE,
		ENOENT,
		ENOSR,
		ENOSPC,
		ENOTDIR,
		ENXIO,
		EOVERFLOW,
		EROFS,
		EAGAIN,
		ENOMEM,
		ETXTBUSY,
		EBADF,
		ESPIPE,
	};
	
	// Opens a file and returns its descriptor.
	int FiOpenD (const char* pFileName, int oflag, const char* srcFile, int srcLine);
	#define FiOpen(pFileName, oflag)  FiOpenD(pFileName,oflag,__FILE__,__LINE__)
	
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

#endif

#endif//_VFS_H