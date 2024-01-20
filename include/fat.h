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

// Credits: https://github.com/knusbaum/kernel/blob/master/fat32.c

// should use this when generating NanoShell-specific permissions for this file
#define FAT_READONLY  (1<<0)

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

// Directory entry extension on the NT byte
#define FAT_NTBYTE_NAMELOWER (1 << 3) // 0x08
#define FAT_NTBYTE_EXTELOWER (1 << 4) // 0x10

// Arbitrary.  Specifies the maximum amount of directory entries.
#define MAX_DIR_ENTRIES 16

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

// ======= FAT FILE SYSTEM SPECIFIC DEFINITIONS =======

typedef struct
{
	char m_name[11];
	uint8_t m_attributes;
	uint8_t m_ntReserved; // reserved for use by Windows NT.. my ass
	uint8_t m_createTimeTenth;
	uint16_t m_createTime;
	uint16_t m_createDate;
	uint16_t m_lastAccessDate;
	uint16_t m_firstClusterHigh;
	uint16_t m_modifyTime;
	uint16_t m_modifyDate;
	uint16_t m_firstClusterLow;
	uint32_t m_fileSize;
}
__attribute__((packed))
FatDirectoryEntry;

STATIC_ASSERT(sizeof(FatDirectoryEntry) == 32, "Sizeof FAT dirent should be 32");


// ======= FAT32 IMPLEMENTATION SPECIFIC DEFINITIONS =======

typedef struct FatFileCacheUnit
{
	uint32_t  m_startingCluster;
	FileNode  m_node;
	
	// directory entry cache
	// these store a pointer to the directory that the file belongs to (referenced of course),
	// as well as the LFN entry's data and place within the parent directory.
	struct FatFileCacheUnit* m_pParent;
	uint32_t m_offset;
	int m_numEntries;
	FatDirectoryEntry m_entryCache[MAX_DIR_ENTRIES];
	
	// duplicate entry in case the file is deleted (m_numEntries is zero)
	FatDirectoryEntry m_sfnEntry;
}
FatFileCacheUnit;

typedef union FatClusterPage
{
	uint32_t m_fat32[4096 / 4];
	uint16_t m_fat16[4096 / 2];
	uint8_t  m_fat12[4096]; // extraction and implantation is going to be non trivial on FAT12
}
FatClusterPage;

typedef struct FatClusterList
{
	uint32_t m_sizePages;
	
	// if an entry is null, need to load the cluster page there
	FatClusterPage** m_pPages;
}
FatClusterList;

typedef struct FatFileSystem
{
	bool     m_bMounted;
	DriveID  m_driveID;
	uint32_t m_lbaStart;
	uint32_t m_sectorCount;
	bool     m_bIsReadOnly;
	BiosParameterBlock m_bpb; // BPB cache
	
	FatClusterList m_clusterList;
	
	uint8_t* m_pClusBuffer1;
	uint8_t* m_pClusBuffer2;
	uint8_t* m_pClusBuffer3;
}
FatFileSystem;

#endif//_FAT_H