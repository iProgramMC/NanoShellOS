/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

   Virtual FileSystem module headerfile
******************************************/
#ifndef _VFS_H
#define _VFS_H

#include <main.h>
#include <task.h>

struct FSNodeS;
struct DirEntS;

typedef uint64_t FileID;

#define PATH_SEP ('/')
#define PATH_THISDIR (".")
#define PATH_PARENTDIR ("..")

#define FS_DEVICE_NAME "Device"
#define FS_FSROOT_NAME "FSRoot"

#define FD_MAX 1024

#define C_MAX_FILE_SYSTEMS 128

#define NO_FILE ((FileID)0)

#define FS_MAKE_ID(fsID, fileID)  (FileID)(((uint64_t)(fsID) << 32) | fileID)

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

#define FLAG_HIDDEN (1)
#define FLAG_SYSTEM (2)

//lseek whences
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

enum
{
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FILE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16 //to be OR'd into the other flags
};

typedef struct
{
	char     name[128];
	FileID   file_id;
	int      file_length;
	uint32_t type;
}
DirectoryEntry;

// Base classes
struct File;
struct Directory;
struct FileSystem;
typedef uint32_t  (*File_Read)    (struct File* file, void *pOut, uint32_t size);
typedef uint32_t  (*File_Write)   (struct File* file, void *pIn,  uint32_t size);
typedef void      (*File_Close)   (struct File* file);
typedef bool      (*File_Seek)    (struct File* file, int position, int whence, bool bAllowExpansion);
typedef int       (*File_Tell)    (struct File* file);
typedef int       (*File_TellSize)(struct File* file);

typedef struct File
{
	DirectoryEntry entry;
	struct FileSystem *pFS;
	
	File_Read     Read;
	File_Write    Write;
	File_Close    Close;
	File_Seek     Seek;
	File_Tell     Tell;
	File_TellSize TellSize;
}
File;

typedef void   (*Directory_Close)    (struct Directory* pDir);
typedef bool   (*Directory_ReadEntry)(struct Directory* pDir, DirectoryEntry* pOut);
typedef void   (*Directory_Seek)     (struct Directory* pDir, int position);
typedef int    (*Directory_Tell)     (struct Directory* pDir);


typedef struct Directory
{
	File *file;
	struct FileSystem *pFS;
	
	Directory_Close     Close;
	Directory_ReadEntry ReadEntry;
	Directory_Seek      Seek;      // - RewindDir does basically Seek(0)
	Directory_Tell      Tell;
	
	// Work on: MakeDir, CreateFile, RemoveFile, StatFile; and possibly: Link?, Unlink?
}
Directory;

typedef Directory*(*FileSystem_OpenDir)        (struct FileSystem *pFS, uint32_t dirID);
typedef File*     (*FileSystem_OpenInt)        (struct FileSystem *pFS, DirectoryEntry* entry);
typedef bool      (*FileSystem_LocateFileInDir)(struct FileSystem *pFS, uint32_t dirID, const char *pFN, DirectoryEntry *pEntry);
typedef uint32_t  (*FileSystem_GetRootDirID)   (struct FileSystem *pFS);

typedef struct FileSystem
{
	uint32_t fsID;
	
	SafeLock lock;
	
	// to imitate the C++ mannerisms I've used prototyping this design
	FileSystem_OpenInt         OpenInt;
	FileSystem_OpenDir         OpenDir;
	FileSystem_LocateFileInDir LocateFileInDir;
	FileSystem_GetRootDirID    GetRootDirID;
}
FileSystem;

// Mounting
bool        FsMountFileSystem (FileSystem *pFS);
FileSystem* FsResolveByFsID   (uint32_t    fsID);
FileSystem* FsResolveByFileID (FileID      id);
FileID      FsGetGlobalRoot   ();
void        FsSetGlobalRoot   (FileID id);

// File operations
uint32_t FsFileRead    (File* file, void *pOut, uint32_t size);
uint32_t FsFileWrite   (File* file, void *pOut, uint32_t size);
void     FsFileClose   (File* file);
bool     FsFileSeek    (File* file, int pos, int whence, bool bAllowExpansion);
int      FsFileTell    (File* file);
int      FsFileTellSize(File* file);

// Directory operations
void FsDirectoryClose     (Directory *pDirectory);
bool FsDirectoryReadEntry (Directory *pDirectory, DirectoryEntry *pDirectoryEntryOut);
void FsDirectorySeek      (Directory *pDirectory, int position);
int  FsDirectoryTell      (Directory *pDirectory);

// File system operations
File*      FsOpenFile(DirectoryEntry* entry);
Directory* FsOpenDir(FileID fileID);
bool       FsLocateFileInDir(FileID dirID, const char *pFN, DirectoryEntry *pEntryOut);
uint32_t   FsGetRootDirID(FileSystem *pFS);

#define BASE_FI(fi)  ((File*)fi)
#define BASE_FS(fs)  ((FileSystem*)fs)

//For internal use.
typedef struct {
	bool      m_bOpen;
	File     *m_pFile;
	char      m_sPath[PATH_MAX+2];
	
	const char* m_openFile;
	int       m_openLine;
}
FileDescriptor;

typedef struct {
	bool        m_bOpen;
	Directory  *m_pDir;
	char        m_sPath[PATH_MAX+2];
	
	const char *m_openFile;
	int         m_openLine;
	
	DirectoryEntry m_sCurDirEnt;
}
DirDescriptor ;

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
	 * Initializes the initial ramdisk.
	 */
	void FsInitializeInitRd(void* pRamDisk);
#endif

// Basic POSIX-like API
#if 1
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
	
	#define O_RDONLY (1)
	#define O_WRONLY (2)
	#define O_RDWR   (O_RDONLY | O_WRONLY)
	#define O_APPEND (4)
	#define O_CREAT  (8)
	#define O_EXEC   (1024)
	
	
	#include <errors.h>
	
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
	
	
	// Opens a directory and returns a descriptor
	int FiOpenDirD (const char* pFileName, const char* srcFile, int srcLine);
	#define FiOpenDir(pFileName) FiOpenDirD(pFileName, __FILE__, __LINE__)
	
	// Closes a directory and frees its descriptor for future use.
	int FiCloseDir (int dd);
	
	// Reads a directory entry from the directory and advances the stream pointer.
	DirectoryEntry* FiReadDir (int dd);
	
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
	
	// Removes a file or an empty directory.
	int FiRemoveFile (const char *pfn);
	
#endif

#endif//_VFS_H