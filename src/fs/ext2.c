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

#define C_MAX_E2_FILE_SYSTEMS (16)

static const char s_extLetters[] = "0123456789ABCDEF";

#define ENSURE_READ_OP(op, message) ASSERT(op == DEVERR_SUCCESS && message);

// ...



Ext2FileSystem s_ext2FileSystems[C_MAX_E2_FILE_SYSTEMS];

// Read a number of blocks from the file system. Use `uint8_t mem[pFS->m_blockSize * numBlocks]` or the equivalent MmAllocate to make space for its output.
DriveStatus Ext2ReadBlocks(Ext2FileSystem* pFS, uint32_t blockNo, uint32_t blockCount, void* pMem)
{
	uint32_t lbaStart = pFS->m_lbaStart + blockNo * pFS->m_sectorsPerBlock, sectorCount = blockCount * pFS->m_sectorsPerBlock;
	uint8_t* pMem1 = (uint8_t*)pMem;
	
	while (sectorCount)
	{
		// read!
		DriveStatus ds = StDeviceRead( lbaStart, pMem1, pFS->m_driveID, 1 );
		if (ds != DEVERR_SUCCESS)
			return ds;
		
		lbaStart++;
		sectorCount--;
		pMem1 += BLOCK_SIZE;
	}
	
	return DEVERR_SUCCESS;
}

//note: the offset is in <block_size> units.
uint32_t Ext2GetInodeBlock(Ext2Inode* pInode, Ext2FileSystem* pFS, uint32_t offset)
{
	if (offset < 12)
	{
		return pInode->m_directBlockPointer[offset];
	}
	
	offset -= 12;
	
	// is this part of the singly-indirect block?
	uint32_t  addrsPerBlock = pFS->m_blockSize / 4;
	uint32_t* data = (uint32_t*)pFS->m_pBlockBuffer;
		
	if (offset < addrsPerBlock)
	{
		// yes, so just return from there
		ASSERT(Ext2ReadBlocks(pFS, pInode->m_singlyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
		
		return data[offset];
	}
	
	offset -= addrsPerBlock;
	
	// is this part of the doubly indirect block?
	if (offset < addrsPerBlock * addrsPerBlock)
	{
		uint32_t firstTurn  = offset / addrsPerBlock;
		uint32_t secondTurn = offset % addrsPerBlock;
		
		// read for the first turn
		ASSERT(Ext2ReadBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
		uint32_t firstTurnBlock = data[firstTurn];
		
		// read for the second turn
		ASSERT(Ext2ReadBlocks(pFS, firstTurnBlock, 1, data) == DEVERR_SUCCESS);
		return data[secondTurn];
	}
	
	// TODO: Check the triply indirect things as well soon.
	
	// well, we're trying to exceed the file's boundaries anyway, call it quits ;)
	return 0;
}

void Ext2ReadFileSegment(Ext2FileSystem* pFS, Ext2Inode* pInode, uint32_t offset, uint32_t size, void *pMemOut)
{
	// will we ever hit a block boundary?
	uint32_t offsetEnd = offset + size;
	uint32_t blockStart = (offset       ) >> (10 + pFS->m_log2BlockSize);
	uint32_t blockEnd   = (offsetEnd - 1) >> (10 + pFS->m_log2BlockSize);
	
	uint32_t offsetWithinStartBlock = (offset    & ((1024 << pFS->m_log2BlockSize) - 1));
	uint32_t offsetWithinEndBlock   = (offsetEnd & ((1024 << pFS->m_log2BlockSize) - 1));
	
	// offsetable version
	uint8_t* pMem = (uint8_t*)pMemOut;
	
	for (uint32_t i = blockStart; i <= blockEnd; i++)
	{
		uint32_t blockIndex = Ext2GetInodeBlock(pInode, pFS, i);
		
		if (blockIndex)
			ASSERT(Ext2ReadBlocks(pFS, blockIndex, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
		else
			memset(pFS->m_pBlockBuffer, 0, pFS->m_blockSize);
		
		// Are we at the start?
		if (i == blockStart)
		{
			// copy from offsetWithinStartBlock to blockSize
			
			uint32_t endMin = pFS->m_blockSize;
			if (endMin < offsetWithinEndBlock)
				endMin = offsetWithinEndBlock;
			
			memcpy(pMem, pFS->m_pBlockBuffer + offsetWithinStartBlock, pFS->m_blockSize - offsetWithinStartBlock);
			
			pMem += (pFS->m_blockSize - offsetWithinStartBlock);
		}
		// Are we at the end?
		else if (i == blockEnd)
		{
			// copy from 0 to offsetWithinEndBlock
			if (offsetWithinEndBlock == 0)
				offsetWithinEndBlock = pFS->m_blockSize;
			memcpy(pMem, pFS->m_pBlockBuffer, offsetWithinEndBlock);
		}
		// Nope, read the full block.
		else
		{
			memcpy(pMem, pFS->m_pBlockBuffer, pFS->m_blockSize);
			
			pMem += pFS->m_blockSize;
		}
		
	}
}

void SDumpBytesAsHex (void *nAddr, size_t nBytes, bool as_bytes)
{
	int ints = nBytes/4;
	if (ints > 1024) ints = 1024;
	if (ints < 4) ints = 4;
	
	uint32_t *pAddr  = (uint32_t*)nAddr;
	uint8_t  *pAddrB = (uint8_t*) nAddr;
	for (int i = 0; i < ints; i += (8 >> as_bytes))
	{
		for (int j = 0; j < (8 >> as_bytes); j++)
		{
			if (as_bytes)
			{
				SLogMsgNoCr("%b %b %b %b ", pAddrB[((i+j)<<2)+0], pAddrB[((i+j)<<2)+1], pAddrB[((i+j)<<2)+2], pAddrB[((i+j)<<2)+3]);
			}
			else
				SLogMsgNoCr("%x ", pAddr[i+j]);
		}
		for (int j = 0; j < (8 >> as_bytes); j++)
		{
			#define FIXUP(c) ((c<32||c>126)?'.':c)
			char c1 = pAddrB[((i+j)<<2)+0], c2 = pAddrB[((i+j)<<2)+1], c3 = pAddrB[((i+j)<<2)+2], c4 = pAddrB[((i+j)<<2)+3];
			SLogMsgNoCr("%c%c%c%c", FIXUP(c1), FIXUP(c2), FIXUP(c3), FIXUP(c4));
			#undef FIXUP
		}
		SLogMsg("");
	}
}

void Ext2ReadInodeMetaData(Ext2FileSystem* pFS, uint32_t inodeNo, Ext2Inode* pOutputInode)
{
	ASSERT(inodeNo != 0 && "The inode number may not be zero, something is definitely wrong!");
	
	// Determine which block group the inode belongs to.
	int inodesPerGroup = pFS->m_superBlock.m_inodesPerGroup;
	
	// Get the block group this inode is a part of.
	uint32_t blockGroup = (inodeNo - 1) / inodesPerGroup;
	
	// Get the block group's inode table address.
	uint32_t inodeTableAddr = pFS->m_pBlockGroups[blockGroup].m_startBlockAddrInodeTable;
	
	// This is the index inside that table.
	uint32_t index = (inodeNo - 1) % inodesPerGroup;
	
	// Determine which block contains the inode.
	uint32_t thing = index * pFS->m_inodeSize;
	uint32_t blockInodeIsIn = thing / pFS->m_blockSize;
	uint32_t blockInodeOffs = thing % pFS->m_blockSize;
	
	uint8_t bytes[pFS->m_blockSize];
	ASSERT(Ext2ReadBlocks(pFS, inodeTableAddr + blockInodeIsIn, 1, bytes) == DEVERR_SUCCESS);
	
	Ext2Inode* pInode = (Ext2Inode*)(bytes + blockInodeOffs);
	
	*pOutputInode = *pInode;
}

// Read an inode and add it to the inode cache. Give it a name from the system side, since
// the inodes themselves do not contain names -- that's the job of the directory entry.
// When done, return the specific cache unit.
Ext2InodeCacheUnit* Ext2ReadInode(Ext2FileSystem* pFS, uint32_t inodeNo, const char* pName, bool bForceReRead)
{
	// If the inode was already cached, and we aren't forced to re-read it, just return.
	if (!bForceReRead)
	{
		Ext2InodeCacheUnit* pUnit = Ext2LookUpInodeCacheUnit(pFS, inodeNo);
		if (pUnit)
			return pUnit;
	}
	
	Ext2Inode inode;
	Ext2ReadInodeMetaData(pFS, inodeNo, &inode);
	
	return Ext2AddInodeToCache(pFS, inodeNo, &inode, pName);
}

// Initting code

int Ext2GetInodeSize(Ext2FileSystem* pFS)
{
	if (pFS->m_superBlock.m_majorVersion == E2_GOOD_OLD_REV)
		return E2_DEF_INODE_STRUCTURE_SIZE;
	
	return pFS->m_superBlock.m_inodeStructureSize;
}

int Ext2GetFirstNonReservedInode(Ext2FileSystem* pFS)
{
	if (pFS->m_superBlock.m_majorVersion == E2_GOOD_OLD_REV)
		return E2_DEF_FIRST_NON_RESERVED_INODE;
	
	return pFS->m_superBlock.m_firstNonReservedInode;
}

int Ext2GetInodesPerGroup(Ext2FileSystem* pFS)
{
	return pFS->m_superBlock.m_inodesPerGroup;
}

int Ext2GetBlocksPerGroup(Ext2FileSystem* pFS)
{
	return pFS->m_superBlock.m_blocksPerGroup;
}

int Ext2GetBlockSize(Ext2FileSystem* pFS)
{
	return 1024 << pFS->m_superBlock.m_log2BlockSize;
}

int Ext2GetFragmentSize(Ext2FileSystem* pFS)
{
	return 1024 << pFS->m_superBlock.m_log2FragmentSize;
}

void Ext2ReadSuperBlock(Ext2FileSystem* pFS)
{
	// Read the super block from an EXT2 drive.
	// It is located at 1024 bytes from the start of the volume, and is 1024 bytes in size.
	uint32_t m_superBlockSector = pFS->m_lbaStart + 2;
	
	StDeviceRead( m_superBlockSector, &pFS->m_superBlock.m_bytes, pFS->m_driveID, 2 );
	
	LogMsg("Path volume was last mounted to: %s", pFS->m_superBlock.m_pathVolumeLastMountedTo);
}

void Ext2LoadBlockGroupDescriptorTable(Ext2FileSystem* pFS)
{
	// if blockSize is 1024, then this will be block 2. Otherwise, it will be block 1.
	uint32_t blockGroupTableStart = (pFS->m_blockSize == 1024) ? 2 : 1;
	uint32_t blockGroupCount = (pFS->m_superBlock.m_nBlocks + pFS->m_blocksPerGroup - 1) / pFS->m_blocksPerGroup;
	
	SLogMsg("Block group count is %d. Blocks per group : %d", blockGroupCount, pFS->m_blocksPerGroup);
	
	uint32_t blocksToRead = (blockGroupCount * sizeof(Ext2BlockGroupDescriptor) + pFS->m_blockSize - 1) >> pFS->m_log2BlockSize;
	
	void *pMem = MmAllocate(blocksToRead << pFS->m_log2BlockSize);
	
	ENSURE_READ_OP(Ext2ReadBlocks(pFS, blockGroupTableStart, blocksToRead, pMem), "Couldn't read the whole block group descriptor array!");
	
	pFS->m_pBlockGroups = (Ext2BlockGroupDescriptor*)pMem;
	
	SLogMsg("File system version is: %s", pFS->m_superBlock.m_majorVersion ? "DYNAMIC_REVISION" : "GOOD_OLD_REVISION");
	SLogMsg("Block count: %d", pFS->m_superBlock.m_nBlocks);
}

//NOTE: This makes a copy!!
void FsRootAddArbitraryFileNodeToRoot(const char* pFileName, FileNode* pFileNode);

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
	Ext2ReadSuperBlock(pFS);
	
	if (pFS->m_superBlock.m_nE2Signature != EXT2_SIGNATURE)
	{
		// this isn't even good! Bleh, go away.
		// We didn't do anything bad like allocate stuff or mark it as mounted. So a return is safe
		return;
	}
	
	// Set up and cache basic info about the file system.
	pFS->m_inodeSize = Ext2GetInodeSize(pFS);
	pFS->m_blockSize = Ext2GetBlockSize(pFS);
	pFS->m_fragmentSize = Ext2GetFragmentSize(pFS);
	pFS->m_inodesPerGroup = Ext2GetInodesPerGroup(pFS);
	pFS->m_blocksPerGroup = Ext2GetBlocksPerGroup(pFS);
	pFS->m_log2BlockSize  = pFS->m_superBlock.m_log2BlockSize + 10;
	pFS->m_sectorsPerBlock = pFS->m_blockSize / BLOCK_SIZE;
	
	if (pFS->m_sectorsPerBlock == 0)
	{
		LogMsg("An Ext2 partition with a block size of %d was found! It can't be less than 512 bytes, so skip.", pFS->m_blockSize);
		return;
	}
	
	SLogMsg("Mounting Ext2 partition...  Last place where it was mounted: %s", pFS->m_superBlock.m_pathVolumeLastMountedTo);
	
	pFS->m_bMounted = true;
	pFS->m_pInodeCacheRoot = NULL;
	pFS->m_pBlockGroups    = NULL;
	
	pFS->m_pBlockBuffer    = MmAllocate(pFS->m_blockSize);
	
	// load the block group descriptor table
	Ext2LoadBlockGroupDescriptorTable(pFS);
	
	char name[10];
	strcpy(name, "ExtX");
	
	// This replaces the X with a letter. This will be its name.
	name[3] = s_extLetters[FreeArea];
	
	// Read the root directory's inode, and cache it.
	Ext2InodeCacheUnit* pCacheUnit = Ext2ReadInode(pFS, 2, name, true);
	pCacheUnit->m_bPermanent = true; // Currently useless right now.
	
	// Get its filenode, and copy it. This will add the file system to the root directory.
	FsRootAddArbitraryFileNodeToRoot(name, &pCacheUnit->m_node);
}

void FsExt2Cleanup(Ext2FileSystem* pFS)
{
	if (pFS->m_pBlockGroups)
	{
		MmFree(pFS->m_pBlockGroups);
		pFS->m_pBlockGroups = NULL;
	}
	if (pFS->m_pBlockBuffer)
	{
		MmFree(pFS->m_pBlockBuffer);
		pFS->m_pBlockBuffer = NULL;
	}
	
	Ext2DeleteInodeCacheTree(pFS);
	
	pFS->m_bMounted = false;
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
