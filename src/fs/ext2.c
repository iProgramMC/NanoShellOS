//  ***************************************************************
//  ext2.c - Creation date: 18/11/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <vfs.h>
#include <ext2.h>
#include <fat.h> // need this for the MasterBootRecord

#define C_MAX_E2_FILE_SYSTEMS (32)

// ...

typedef struct Ext2FileSystem
{
	bool     m_bMounted;
	DriveID  m_driveID;
	uint32_t m_lbaStart;
	uint32_t m_sectorCount;
	Ext2SuperBlock m_superBlock; // super block cache
}
Ext2FileSystem;

Ext2FileSystem s_ext2FileSystems[C_MAX_E2_FILE_SYSTEMS];

int Ext2GetInodeSize(Ext2FileSystem* pFS)
{
	if (pFS->m_superBlock.m_majorVersion < 1)
		return E2_DEF_INODE_STRUCTURE_SIZE;
	
	return pFS->m_superBlock.m_inodeStructureSize;
}

int Ext2GetFirstNonReservedInode(Ext2FileSystem* pFS)
{
	if (pFS->m_superBlock.m_majorVersion < 1)
		return E2_DEF_FIRST_NON_RESERVED_INODE;
	
	return pFS->m_superBlock.m_firstNonReservedInode;
}

int Ext2GetInodesPerGroup(Ext2FileSystem* pFS)
{
	return pFS->m_superBlock.m_inodesPerGroup;
}

int Ext2GetBlockSize(Ext2FileSystem* pFS)
{
	return 1 << pFS->m_superBlock.m_log2BlockSize;
}

int Ext2GetFragmentSize(Ext2FileSystem* pFS)
{
	return 1 << pFS->m_superBlock.m_log2FragmentSize;
}

void Ext2ReadSuperBlock(Ext2FileSystem* pFS)
{
	// Read the super block from an EXT2 drive.
	// It is located at 1024 bytes from the start of the volume, and is 1024 bytes in size.
	uint32_t m_superBlockSector = pFS->m_lbaStart + 2;
	
	StDeviceRead( m_superBlockSector, &pFS->m_superBlock.m_bytes, pFS->m_driveID, 2 );
	
	LogMsg("Path volume was last mounted to: %s", pFS->m_superBlock.m_pathVolumeLastMountedTo);
}

void Ext2ReadBlock(Ext2FileSystem* pFS, uint32_t blockNo, uint8_t* pOut)
{
	//TODO
}

void Ext2ReadInode(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	ASSERT(inodeNo != 0 && "The inode number may not be zero, something is definitely wrong!");
	
	// Determine which block group the inode belongs to.
	int inodesPerGroup = Ext2GetInodesPerGroup(pFS);
	
	uint32_t blockGroup = (inodeNo - 1) / inodesPerGroup;
	uint32_t index = (inodeNo - 1) % inodesPerGroup;
	
	// Determine which block contains the inode.
	int blockSize = Ext2GetBlockSize(pFS);
	uint32_t blockInodeIsIn = (index * Ext2GetInodeSize(pFS)) / blockSize;
	
	uint8_t blockData[blockSize];
	
	Ext2ReadBlock(pFS, blockInodeIsIn, blockData);
	
	//TODO: Print the data you get
}

void FsMountExt2Partition(DriveID driveID, int partitionStart, int partitionSizeSec)
{
	// Find a Fat32 structure in the list of Fat32 structures.
	int FreeArea = -1;
	for (size_t i = 0; i < ARRAY_COUNT(s_ext2FileSystems); i++)
	{
		if (!s_ext2FileSystems[i].m_bMounted)
		{
			FreeArea = i;
			break;
		}
	}
	
	if (FreeArea == -1) return;
	
	Ext2FileSystem *pFS = &s_ext2FileSystems[FreeArea];
	pFS->m_lbaStart    = partitionStart;
	pFS->m_sectorCount = partitionSizeSec;
	pFS->m_driveID     = driveID;
	pFS->m_bMounted    = true;
	Ext2ReadSuperBlock(pFS);
	
	// Try reading inode 2.
	Ext2ReadInode(pFS, 2);
}

void FsExt2Init()
{
	// probe each drive
	for (int i = 0; i < 0x100; i++)
	{
		if (!StIsDriveAvailable(i)) continue;
		
		union
		{
			uint8_t bytes[512];
			MasterBootRecord s;
		} mbr;
		
		// read one sector from block 0
		StDeviceRead( 0, mbr.bytes, i, 1 );
		
		// probe each partition
		for (int pi = 0; pi < 4; pi++)
		{
			Partition* pPart = &mbr.s.m_Partitions[pi];
			
			if (pPart->m_PartTypeDesc != 0)
			{
				// This is a valid partition.  Mount it.
				FsMountExt2Partition (i, pPart->m_StartLBA, pPart->m_PartSizeSectors);
			}
		}
		
		// well, we could try without an MBR here too, but I'm too lazy to get the size of the drive itself right now.
	}
}
