/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

    FAT File System Driver header file
******************************************/

#ifndef _FAT_H
#define _FAT_H

#include <vfs.h>
#include <console.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>
#include <storabs.h>

#define FSDEBUG

#ifdef FSDEBUGX
#define DebugLogMsg LogMsg
#elif defined(FSDEBUG)
#define DebugLogMsg SLogMsg
#else
#define DebugLogMsg(...)
#endif

#ifdef MSVC

#define CLUSTER_SIZE_DECL(pFS) \
	65536

#else

#define CLUSTER_SIZE_DECL(pFS) \
	(pFS->bpb.m_nSectorsPerCluster * SECTOR_SIZE)

#endif

// Credits: https://github.com/knusbaum/kernel/blob/master/fat32.c

#define ADD_TIMESTAMP_TO_FILES


// should use this when generating NanoShell-specific permissions for this file
#define FAT_READONLY (1<<0)

#define FAT_HIDDEN    (1<<1)
#define FAT_SYSTEM    (1<<2)

// In the root of a FAT file system, there's a special file whose name matches the volume ID.
// NanoShell should and does ignore any files with this bit set.
#define FAT_VOLUME_ID (1<<3)

// If this is a directory.  Pretty simple, really.
#define FAT_DIRECTORY (1<<4)

// Archive bit.  Usually it's used to determine whether a file has been backed up or not (this bit
// will be reset when you write to the file later on).  Not used by NanoShell.
#define FAT_ARCHIVE   (1<<5)

// Long file name bits.  Useful when we need LFN support.
#define FAT_LFN       (FAT_READONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID)

// FAT End of chain special magic number cluster.  Marks the end of a chain.
// This is just like the NULL pointer in most singly linked list implementations.
#define FAT_END_OF_CHAIN 0x0FFFFFF8

// Extensions that Microsoft uses
#define FAT_MSFLAGS_NAMELOWER (1 << 3) // 0x08
#define FAT_MSFLAGS_EXTELOWER (1 << 4) // 0x10

typedef struct
{
	uint16_t m_nBytesPerSector;     // usually 512
	uint8_t  m_nSectorsPerCluster;  // must be 1 or over.
	uint16_t m_nReservedSectors;    // before we reach the FAT
	uint8_t  m_nFAT;                // number of FATs (NanoShell will only use the first one)
	uint16_t m_nDirEntries;
	uint16_t m_nTotalSectors;
	uint8_t  m_mediaDescType;
	uint16_t m_nSectorsPerFat12or16;
	uint16_t m_nSectorsPerTrack;
	uint16_t m_nHeadsOrSizesOnMedia;//?
	uint32_t m_nHiddenSectors;
	uint32_t m_nLargeSectorsOnMedia;
	
	// Extended Boot Record
	uint32_t m_nSectorsPerFat32;
	uint16_t m_nFlags;
	uint16_t m_nFatVersion;
	uint32_t m_nRootDirStartCluster;// where our root logic should start
	uint16_t m_sectorNumberFsInfo;
	uint16_t m_sectorNmbrBckpBtSctr;
	uint8_t  m_driveNumber;
	uint8_t  m_WindowsFlags;        //used only by Windows
	uint8_t  m_Signature;
	uint32_t m_nVolumeID;           //something like AABB-CCDD
	char     m_sVolumeLabel[12];    //! NOT null terminated! Instead you have to `memcpy` it into a null termed string.
	char     m_sSystemID   [9];     //Same as above.
}
__attribute__((packed))
BiosParameterBlock;

typedef struct
{
	uint8_t  m_BootIndicator;
	uint8_t  m_StartCHS[3]; //not used
	uint8_t  m_PartTypeDesc;
	uint8_t  m_EndCHS[3]; //not used
	uint32_t m_StartLBA; //used
	uint32_t m_PartSizeSectors;
}
__attribute__((packed))
Partition;

typedef struct
{
	uint8_t   m_BootLoaderCode[446];
	Partition m_Partitions[4];
	uint16_t  m_BootSignature; //most of the time this is 0x55AA
}
__attribute__((packed))
MasterBootRecord;


typedef struct FatDirectoryEntry
{
#pragma pack (push, 1)
	char sfn[8];
	char ext[3];
	char attrib;
	uint32_t unk0;
	uint32_t unk1;
	uint16_t clusNumHigh;
	uint16_t lastModTime;
	uint16_t lastModDate;
	uint16_t clusNumLow;
	uint32_t fileSize;
#pragma pack (pop)
}
__attribute__((packed))
FatDirectoryEntry;
STATIC_ASSERT(sizeof(FatDirectoryEntry) == 32, "FatDirectoryEntry must be 32 bytes!");

struct Fat32FileSystem;
struct Fat32ClusterChain;

typedef struct Fat32ClusterChain // File
{
	// Inherited from File:
	DirectoryEntry entry;
	struct Fat32FileSystem *pFS; // I can do this. Don't judge.
	
	File_Read  Read;
	File_Close Close;
	File_Seek  Seek;
	
	// Our own variables for the cluster chain
	bool taken;
	uint32_t currentCluster;//to allow for forwards seek
	uint32_t firstCluster;	//to allow for backwards seek
	uint32_t offsetBytes;
	bool    clusterCacheLoaded;
	uint8_t clusterCache[65536];
}
Fat32ClusterChain;

typedef struct Fat32Directory
{
	Fat32ClusterChain *file;
	struct Fat32FileSystem *pFS;
	
	Directory_Close     Close;
	Directory_ReadEntry ReadEntry;
	Directory_Seek      Seek;
	Directory_Tell      Tell;
	
	// Our own variables for the directory
	bool taken;
	int  told;
}
Fat32Directory;

#define SECTOR_SIZE 512

#define C_MAX_CLUSCHAIN_HANDLES 64
#define C_MAX_DIRECTORY_HANDLES 64
typedef struct Fat32FileSystem
{
	// The base FileSystem's variables.
	uint32_t fsID;
	
	SafeLock lock;
	
	// to imitate the C++ mannerisms I've used prototyping this design
	FileSystem_OpenInt         OpenInt;
	FileSystem_OpenDir         OpenDir;
	FileSystem_LocateFileInDir LocateFileInDir;
	FileSystem_GetRootDirID    GetRootDirID;
	
	BiosParameterBlock bpb;
	uint8_t  diskID; // as would be in storabs.c

	uint32_t partStart, partSize;

	uint32_t fatBeginSector;
	uint32_t clusBeginSector;
	uint32_t clusSize, clusSizeLog2;
	uint32_t clusAllocHint;

	uint32_t* pFat;
	uint8_t*  pFatModified;
	uint8_t*  pFatLoaded;

	Fat32ClusterChain fileHandles[C_MAX_CLUSCHAIN_HANDLES];
	Fat32Directory    dirHandles [C_MAX_DIRECTORY_HANDLES];
}
Fat32FileSystem;


Fat32FileSystem *FsConstructFat32FileSystem    (uint32_t fsID, uint8_t diskID, uint32_t PartitionStart, uint32_t PartitionSize);
FileSystem      *FsConstructFat32FileSystemBase(uint32_t fsID, uint8_t diskID, uint32_t PartitionStart, uint32_t PartitionSize);













// Old crap below...

















typedef struct
{
	char     m_pName[128];
	uint8_t  m_dirFlags;
	uint32_t m_firstCluster,
	         m_fileSize;
}
FatDirEntry;

typedef struct
{
	uint32_t     m_startCluster;
	FatDirEntry* m_pEntries;
	uint32_t     m_nEntries;
}
FatDirectory;

typedef struct
{
	bool         m_bMounted;
	uint32_t*    m_pFat;
	bool*        m_pbFatModified;
	bool         m_bFatLock;
	bool         m_bReportErrors;
	int          m_nReportedErrors;
	//FileNode*    m_pRoot;
	BiosParameterBlock m_bpb;
	uint32_t     m_fatBeginSector,
	             m_clusBeginSector,
				 m_clusSize,
				 m_clusAllocHint, //! to avoid fragmentation
				 m_partStartSec,
				 m_partSizeSec,
				 m_driveID;
}
FatFileSystem;

extern FatFileSystem s_Fat32Structures[32];

enum
{
	MOUNT_SUCCESS,
	MOUNT_ERR_TOO_MANY_PARTS_MOUNTED = 0x7000,
	MOUNT_ERR_NOT_FAT32,
};

#ifdef EXPERIMENTAL
void CheckDiskFatMain(int fat_number);
#else
#define CheckDiskFatMain(c) LogMsg("Check disk not implemented in non-experimental mode.")
#endif

#endif//_FAT_H