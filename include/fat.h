/*****************************************
		NanoShell Operating System
		(C)2022-2023 iProgramInCpp

    FAT File System Driver header file
******************************************/

#ifndef _FAT_H
#define _FAT_H

#include <vfs.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>
#include <storabs.h>

#define C_FAT32_HASH_TABLE_BUCKET_COUNT (256)

typedef struct
{
	uint8_t  m_padding0[0xB];
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
	uint8_t  m_reserved0[12];
	uint8_t  m_driveNumber;
	uint8_t  m_reserved1;
	uint8_t  m_extBootSignature;
	uint32_t m_nVolumeID;
	char     m_sVolumeLabel[11];    //! NOT null terminated! Instead you have to `memcpy` it into a null termed string.
	char     m_sSystemID   [8];     //Same as above.
}
__attribute__((packed))
BiosParameterBlock;

typedef struct
{
	// FileNode has a 'name' entry, use that one instead.
	uint8_t m_fatAttributes;
}
Fat32DirEntry;

// Hash table list unit.
typedef struct Fat32ChainCacheUnit
{
	uint32_t m_clusNumber; // The starting cluster number
	struct Fat32ChainCacheUnit *pNext, *pPrev;
	bool m_bPermanent; // if false, this may be deleted if its reference count is zero
	
	FileNode      m_node;
	Fat32DirEntry m_dirEntry;
	
	void*    m_pBlockBuffer;
	uint32_t m_nLastBlockRead;
	
	uint32_t m_nClusAllocHint;
	
	bool     m_bAboutToBeDeleted;
}
Fat32ChainCacheUnit;

// This stores 8 sectors' worth of FAT.
typedef struct
{
	uint32_t m_entries[(BLOCK_SIZE / sizeof(uint32_t)) * 8];
}
FaTableSectorGroup;

typedef struct
{
	FaTableSectorGroup** m_pSecGrps;
	uint32_t m_nSecGrps;
}
FaTable;

typedef struct
{
	bool     m_bMounted;
	DriveID  m_driveID;
	uint32_t m_lbaStart;
	uint32_t m_sectorCount;
	BiosParameterBlock m_bpb;
	bool     m_bIsReadOnly;
	uint32_t m_sectorsPerCluster;
	
	// The file allocation table itself.
	FaTable  m_fat;
	
	struct
	{
		Fat32ChainCacheUnit *pFirst, *pLast;
	}
	m_clusterChainHashTable[C_FAT32_HASH_TABLE_BUCKET_COUNT];
	
	uint8_t* m_pBlockBuffer;
	uint8_t* m_pBlockBuffer2;
}
Fat32FileSystem;

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

#endif//_FAT_H