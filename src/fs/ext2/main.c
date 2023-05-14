//  ***************************************************************
//  fs/ext2/main.c - Creation date: 18/11/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// TODO: This implementation can't read files bigger than 64 MB. Not that we'd want to for now.
// TODO: Maybe replace these ASSERTs with proper-er error handling? :)

#include <vfs.h>
#include <ext2.h>
#include <fat.h> // need this for the MasterBootRecord
#include <debug.h>

#define CEIL_DIV_PO2(thing, divisor) ((thing + (1 << (divisor)) - 1) >> (divisor))

#define C_MAX_E2_FILE_SYSTEMS (16)

static const char s_extLetters[] = "0123456789ABCDEF";

#define ENSURE_READ_OP(op, message) ASSERT(op == DEVERR_SUCCESS && message);

void SDumpBytesAsHex(void*pData, size_t bytes, bool as_bytes);

// Forward declarations.
void Ext2FreeBlock(Ext2FileSystem *pFS, uint32_t blockNo);

Ext2FileSystem s_ext2FileSystems[C_MAX_E2_FILE_SYSTEMS];

uint32_t Ext2GetBlockGroupBlockIsIn(Ext2FileSystem* pFS, uint32_t blockNo)
{
	// We shouldn't access stuff in here
	ASSERT(blockNo != 0);
	
	return (blockNo - 1) / pFS->m_blocksPerGroup;
}

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

// Write a number of blocks from the file system. Use `uint8_t mem[pFS->m_blockSize * numBlocks]` or the equivalent MmAllocate to make space for its output.
DriveStatus Ext2WriteBlocks(Ext2FileSystem* pFS, uint32_t blockNo, uint32_t blockCount, const void* pMem)
{
	uint32_t lbaStart = pFS->m_lbaStart + blockNo * pFS->m_sectorsPerBlock, sectorCount = blockCount * pFS->m_sectorsPerBlock;
	const uint8_t* pMem1 = (const uint8_t*)pMem;
	
	while (sectorCount)
	{
		// read!
		DriveStatus ds = StDeviceWrite( lbaStart, pMem1, pFS->m_driveID, 1 );
		if (ds != DEVERR_SUCCESS)
			return ds;
		
		lbaStart++;
		sectorCount--;
		pMem1 += BLOCK_SIZE;
	}
	
	return DEVERR_SUCCESS;
}

void Ext2DumpInode(Ext2Inode* pInode, const char* pInfo)
{
	if (*pInfo)
	{
		LogMsg("Inode dump [%s]", pInfo);
	}
	else
	{
		LogMsg("Inode dump");
	}
	
	LogMsg("direct block pointers:");
	for (int i = 0; i < 12; i++)
	{
		LogMsg("%x", pInode->m_directBlockPointer[i]);
	}
	
	LogMsg("singly block pointer: %x", pInode->m_singlyIndirBlockPtr);
	LogMsg("doubly block pointer: %x", pInode->m_doublyIndirBlockPtr);
	LogMsg("triply block pointer: %x", pInode->m_triplyIndirBlockPtr);
	
	LogMsg("link count: %d", pInode->m_nLinks);
}

// Note: Before using this, make sure you've initialized pFS->m_pBlockBuffer to the super block's contents.
static void Ext2CommitSuperBlockBackup(Ext2FileSystem* pFS, int blockGroupNo)
{
	// Determine the first block of the block group.
	
	// Block 0 is not part of any block group. 
	uint32_t firstBlock = blockGroupNo * pFS->m_blocksPerGroup + 1;
	
	// Write the block.
	ASSERT(Ext2WriteBlocks(pFS, firstBlock, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
}

void Ext2FlushSuperBlock(Ext2FileSystem* pFS)
{
	// Create a block-sized buffer where we'll put the super block.
	memset(pFS->m_pBlockBuffer, 0, pFS->m_blockSize);
	
	memcpy(pFS->m_pBlockBuffer, &pFS->m_superBlock, sizeof (pFS->m_superBlock));
	
	// Flush the main super block.
	// It is located at 1024 bytes from the start of the volume, and is 1024 bytes in size.
	uint32_t m_superBlockSector = pFS->m_lbaStart + 2;
	
	StDeviceWrite( m_superBlockSector, pFS->m_pBlockBuffer, pFS->m_driveID, 2 );
	
	// If the file system has SPARSE_SUPER enabled...
	if (pFS->m_superBlock.m_readOnlyFeatures & E2_ROF_SPARSE_SBLOCKS_AND_GDTS)
	{
		// The groups chosen are 0, 1, and powers of 3, 5, and 7. We've already written zero.
		
		Ext2CommitSuperBlockBackup(pFS, 1);
		for (uint32_t blockGroupNo = 3; blockGroupNo < pFS->m_blockGroupCount; blockGroupNo *= 3) Ext2CommitSuperBlockBackup(pFS, blockGroupNo);
		for (uint32_t blockGroupNo = 5; blockGroupNo < pFS->m_blockGroupCount; blockGroupNo *= 5) Ext2CommitSuperBlockBackup(pFS, blockGroupNo);
		for (uint32_t blockGroupNo = 7; blockGroupNo < pFS->m_blockGroupCount; blockGroupNo *= 7) Ext2CommitSuperBlockBackup(pFS, blockGroupNo);
	}
	else
	{
		for (uint32_t blockGroupNo = 1; blockGroupNo < pFS->m_blockGroupCount; blockGroupNo++)
		{
			Ext2CommitSuperBlockBackup(pFS, blockGroupNo);
		}
	}
}

// TODO
// OPTIMIZE oh come on, optimize this -iProgramInCpp

void Ext2ReadFileSegment(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCacheUnit, uint32_t offset, uint32_t size, void *pMemOut)
{
	Ext2Inode* pInode = &pCacheUnit->m_inode;
	if (!pCacheUnit->m_pBlockBuffer)
	{
		pCacheUnit->m_pBlockBuffer = MmAllocate(pFS->m_blockSize);
		pCacheUnit->m_nLastBlockRead = ~0u;
	}
	
	memset(pMemOut, 0xCC, size);
	
	// will we ever hit a block boundary?
	uint32_t offsetEnd = offset + size;
	uint32_t blockStart = (offset       ) >> (pFS->m_log2BlockSize);
	uint32_t blockEnd   = (offsetEnd - 1) >> (pFS->m_log2BlockSize);
	
	uint32_t offsetWithinStartBlock = (offset    & ((1 << pFS->m_log2BlockSize) - 1)); // TODO why does this bypass our and??
	uint32_t offsetWithinEndBlock   = (offsetEnd & ((1 << pFS->m_log2BlockSize) - 1));
	
	if (offsetWithinEndBlock == 0)
		offsetWithinEndBlock = pFS->m_blockSize;
	
	// offsetable version
	uint8_t* pMem = (uint8_t*)pMemOut;
	
	//SLogMsg("blockStart: %d  blockEnd: %d  offsetWithinStartBlock: %d  offsetWithinEndBlock: %d  offset: %d  offsetEnd: %d", blockStart,blockEnd,offsetWithinStartBlock,offsetWithinEndBlock,offset,offsetEnd);
	
	for (uint32_t i = blockStart; i <= blockEnd; i++)
	{
		uint32_t blockIndex = Ext2GetInodeBlock(pInode, pFS, i);
		
		if (blockIndex)
		{
			if (pCacheUnit->m_nLastBlockRead != blockIndex)
			{
				pCacheUnit->m_nLastBlockRead = blockIndex;
				ASSERT(Ext2ReadBlocks(pFS, blockIndex, 1, pCacheUnit->m_pBlockBuffer) == DEVERR_SUCCESS);
			}
		}
		else
		{
			memset(pCacheUnit->m_pBlockBuffer, 0, pFS->m_blockSize);
		}
		
		// Are we at the start?
		if (i == blockStart)
		{
			// copy from offsetWithinStartBlock to blockSize
			
			uint32_t endMin = pFS->m_blockSize;
			if (i == blockEnd)
				endMin = offsetWithinEndBlock;
			
			memcpy(pMem, pCacheUnit->m_pBlockBuffer + offsetWithinStartBlock, endMin - offsetWithinStartBlock);
			
			pMem += (endMin - offsetWithinStartBlock);
		}
		// Are we at the end?
		else if (i == blockEnd)
		{
			// copy from 0 to offsetWithinEndBlock
			memcpy(pMem, pCacheUnit->m_pBlockBuffer, offsetWithinEndBlock);
			
			pMem += offsetWithinEndBlock;
		}
		// Nope, read the full block.
		else
		{
			memcpy(pMem, pCacheUnit->m_pBlockBuffer, pFS->m_blockSize);
			
			pMem += pFS->m_blockSize;
		}
	}
}

void Ext2WriteFileSegment(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCacheUnit, uint32_t offset, uint32_t size, const void *pMemIn)
{
	Ext2Inode* pInode = &pCacheUnit->m_inode;
	if (!pCacheUnit->m_pBlockBuffer)
	{
		pCacheUnit->m_pBlockBuffer = MmAllocate(pFS->m_blockSize);
		pCacheUnit->m_nLastBlockRead = ~0u;
	}
	
	// will we ever hit a block boundary?
	uint32_t offsetEnd = offset + size;
	uint32_t blockStart = (offset       ) >> (pFS->m_log2BlockSize);
	uint32_t blockEnd   = (offsetEnd - 1) >> (pFS->m_log2BlockSize);
	
	uint32_t offsetWithinStartBlock = (offset    & ((1 << pFS->m_log2BlockSize) - 1)); // TODO why does this bypass our and??
	uint32_t offsetWithinEndBlock   = (offsetEnd & ((1 << pFS->m_log2BlockSize) - 1));
	
	if (offsetWithinEndBlock == 0)
		offsetWithinEndBlock = pFS->m_blockSize;
	
	// offsetable version
	const uint8_t* pMem = (const uint8_t*)pMemIn;
	
	for (uint32_t i = blockStart; i <= blockEnd; i++)
	{
		uint32_t blockIndex = Ext2GetInodeBlock(pInode, pFS, i);
		
		if (blockIndex)
		{
			if (pCacheUnit->m_nLastBlockRead != blockIndex)
			{
				pCacheUnit->m_nLastBlockRead = blockIndex;
				ASSERT(Ext2ReadBlocks(pFS, blockIndex, 1, pCacheUnit->m_pBlockBuffer) == DEVERR_SUCCESS);
			}
		}
		else
		{
			continue;
		}
		
		// Are we at the start?
		if (i == blockStart)
		{
			// copy from offsetWithinStartBlock to blockSize
			
			uint32_t endMin = pFS->m_blockSize;
			if (i == blockEnd)
				endMin = offsetWithinEndBlock;
			
			memcpy(pCacheUnit->m_pBlockBuffer + offsetWithinStartBlock, pMem, endMin - offsetWithinStartBlock);
			
			pMem += (endMin - offsetWithinStartBlock);
		}
		// Are we at the end?
		else if (i == blockEnd)
		{
			// copy from 0 to offsetWithinEndBlock
			memcpy(pCacheUnit->m_pBlockBuffer, pMem, offsetWithinEndBlock);
			
			pMem += offsetWithinEndBlock;
		}
		// Nope, read the full block.
		else
		{
			memcpy(pCacheUnit->m_pBlockBuffer, pMem, pFS->m_blockSize);
			
			pMem += pFS->m_blockSize;
		}
		
		// Write this block.
		if (blockIndex)
		{
			ASSERT(Ext2WriteBlocks(pFS, blockIndex, 1, pCacheUnit->m_pBlockBuffer) == DEVERR_SUCCESS);
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
}

void Ext2LoadBlockGroupDescriptorTable(Ext2FileSystem* pFS)
{
	// if blockSize is 1024, then this will be block 2. Otherwise, it will be block 1.
	uint32_t blockGroupTableStart = (pFS->m_blockSize == 1024) ? 2 : 1;
	uint32_t blockGroupCount = (pFS->m_superBlock.m_nBlocks + pFS->m_blocksPerGroup - 1) / pFS->m_blocksPerGroup;
	
	pFS->m_blockGroupCount = blockGroupCount;
	
	SLogMsg("Block group count is %d. Blocks per group : %d", blockGroupCount, pFS->m_blocksPerGroup);
	
	uint32_t blocksToRead = (blockGroupCount * sizeof(Ext2BlockGroupDescriptor) + pFS->m_blockSize - 1) >> pFS->m_log2BlockSize;
	
	SLogMsg("Blocks to read: %d", blocksToRead);
	
	void *pMem = MmAllocate(blocksToRead << pFS->m_log2BlockSize);
	
	ENSURE_READ_OP(Ext2ReadBlocks(pFS, blockGroupTableStart, blocksToRead, pMem), "Couldn't read the whole block group descriptor array!");
	
	pFS->m_pBlockGroups = (Ext2BlockGroupDescriptor*)pMem;
	
	SLogMsg("File system version is: %s", pFS->m_superBlock.m_majorVersion ? "DYNAMIC_REVISION" : "GOOD_OLD_REVISION");
	SLogMsg("Block count: %d", pFS->m_superBlock.m_nBlocks);
}

void Ext2TestFunction()
{
	
	//#define TEST_FILE_SHRINK
#ifdef TEST_FILE_SHRINK
	// Resolve the path:
	FileNode* pFileNode = FsResolvePath("/Ext0/z2_3085.txt");
	
	// Grab the info.
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pFileNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pFileNode->m_implData1;
	Ext2Inode* pInode = &pUnit->m_inode;
	
	// Expand the inode by 16000 bytes. So its size would be 16024 bytes afterwards.
	Ext2InodeExpand(pFS, pUnit, 16000);
	
	// Shrink the inode by 3000 bytes. So its size would be 85 bytes afterwards.
	//Ext2InodeShrink(pFS, pUnit, 3000);
	Ext2DumpInode(pInode, "/Ext0/z2_3085.txt");
	
	FsReleaseReference(pFileNode);
#endif
	
#ifdef TEST_CREATE_HARD_LINK_TO
	// Get the root inode.
	FileNode* pFileNode = FsResolvePath("/Ext0/test");
	
	// Grab the info.
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pFileNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pFileNode->m_implData1;
	Ext2Inode* pInode = &pUnit->m_inode;
	
	char buf[1000];
	
	uint32_t tickCountThen = GetTickCount();
	
	for (int i = 0; i < 100; i++)
	{
		sprintf(buf, "bogus%d.txt", i);
		// Try adding a directory entry.
		Ext2AddDirectoryEntry(pFS, pUnit, buf, 20, E2_DETI_REG_FILE);
	}
	
	uint32_t tickCountNow = GetTickCount();
	
	SLogMsg("Added 100 entries in %d ms", tickCountNow - tickCountThen);
	
	FsReleaseReference(pFileNode);
#endif
}

// note: No one actually bothers to do this. (I think, from my own testing it seems like not)
// This function was mostly written to test the Ext2FlushSuperBlock function.
void Ext2UpdateLastMountedPath(Ext2FileSystem* pFS, int FreeArea)
{
	char name[10];
	strcpy(name, "/ExtX");
	
	// This replaces the X with a letter. This will be its name.
	name[4] = s_extLetters[FreeArea];
	
	strcpy(pFS->m_superBlock.m_pathVolumeLastMountedTo, name);
	
	LogMsg("Path volume last mounted to is now %s", pFS->m_superBlock.m_pathVolumeLastMountedTo);
	
	Ext2FlushSuperBlock(pFS);
}

//NOTE: This makes a copy!!
FileNode* FsRootAddArbitraryFileNodeToRoot(const char* pFileName, FileNode* pFileNode);

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
	
	// Ensure we have the correct feature set.
	uint32_t optionalFeatures, requiredFeatures, readOnlyFeatures;
	optionalFeatures = pFS->m_superBlock.m_optionalFeatures;
	requiredFeatures = pFS->m_superBlock.m_requiredFeatures;
	readOnlyFeatures = pFS->m_superBlock.m_readOnlyFeatures;
	
	// Required features:
	if (requiredFeatures & E2_REQ_UNSUPPORTED_FLAGS)
	{
		LogMsg("Warning: An Ext2 partition implements unsupported features. Good-bye! (requiredFeatures: %x)", requiredFeatures);
		return;
	}
	if (readOnlyFeatures & E2_ROF_UNSUPPORTED_FLAGS)
	{
		LogMsg("Warning: An Ext2 partition implements unsupported features. This FS is now read only (readOnlyFeatures: %x)", readOnlyFeatures);
		pFS->m_bIsReadOnly = true;
	}
	if (optionalFeatures & E2_OPT_UNSUPPORTED_FLAGS)
	{
		LogMsg("Warning: An Ext2 partition implements unsupported features. (optionalFeatures: %x)", optionalFeatures);
	}
	
	if (pFS->m_blockSize > 8192)
	{
		LogMsg("WARNING: Block size is %d > 8192, you may experience problems!", pFS->m_blockSize);
	}
	
	if (!(requiredFeatures & E2_REQ_DIR_TYPE_FIELD))
	{
		LogMsg("WARNING: This ext2 partition does not support directory type field. May want to refrain from creating any new files here.");
	}
	
	// Set up and cache basic info about the file system.
	pFS->m_inodeSize = Ext2GetInodeSize(pFS);
	pFS->m_blockSize = Ext2GetBlockSize(pFS);
	pFS->m_fragmentSize = Ext2GetFragmentSize(pFS);
	pFS->m_inodesPerGroup = Ext2GetInodesPerGroup(pFS);
	pFS->m_blocksPerGroup = Ext2GetBlocksPerGroup(pFS);
	pFS->m_log2BlockSize  = pFS->m_superBlock.m_log2BlockSize + 10;
	pFS->m_sectorsPerBlock = pFS->m_blockSize / BLOCK_SIZE;
	
	SLogMsg("Mounting Ext2 partition...  Last place where it was mounted: %s", pFS->m_superBlock.m_pathVolumeLastMountedTo);
	
	// Initialize the inode cache array.
	memset(pFS->m_inodeHashTable, 0, sizeof pFS->m_inodeHashTable);
	
	pFS->m_bMounted = true;
	pFS->m_pBlockGroups    = NULL;
	
	// TODO: We should probably do a ceil division rather than a floor one.
	uint32_t blocksToReadBlockBitmap = CEIL_DIV_PO2(pFS->m_blocksPerGroup, 3 + pFS->m_log2BlockSize);
	uint32_t blocksToReadInodeBitmap = CEIL_DIV_PO2(pFS->m_inodesPerGroup, 3 + pFS->m_log2BlockSize);
	
	pFS->m_pBlockBuffer    = MmAllocate(pFS->m_blockSize);
	pFS->m_pBlockBuffer2   = MmAllocate(pFS->m_blockSize);
	
	Ext2LoadBlockGroupDescriptorTable(pFS);
	
	// Load the bitmaps.
	pFS->m_pBlockBitmapPtr = MmAllocate(pFS->m_blockSize * blocksToReadBlockBitmap * pFS->m_blockGroupCount);
	pFS->m_pInodeBitmapPtr = MmAllocate(pFS->m_blockSize * blocksToReadInodeBitmap * pFS->m_blockGroupCount);
	
	pFS->m_blocksPerBlockBitmap = blocksToReadBlockBitmap;
	pFS->m_blocksPerInodeBitmap = blocksToReadInodeBitmap;
	
	Ext2LoadBlockBitmaps(pFS);
	Ext2LoadInodeBitmaps(pFS);
	
	char name[10];
	strcpy(name, "ExtX");
	
	// This replaces the X with a letter. This will be its name.
	name[3] = s_extLetters[FreeArea];
	
	// Read the root directory's inode, and cache it.
	Ext2InodeCacheUnit* pCacheUnit = Ext2ReadInode(pFS, 2, name, true);
	pCacheUnit->m_bPermanent = true; // Currently useless right now.
	
	pCacheUnit->m_node.m_refCount  = NODE_IS_PERMANENT;
	pCacheUnit->m_node.m_type     |= FILE_TYPE_MOUNTPOINT;
	
	// Get its filenode, and copy it. This will add the file system to the root directory.
	FsRootAddArbitraryFileNodeToRoot(name, &pCacheUnit->m_node);
	
	//Ext2UpdateLastMountedPath(pFS, FreeArea);
	
	// Try to retrieve the backups of the inodes. This is a test function.
	Ext2TestFunction();
}

#define SAFE_DELETE(thing) do { if (thing) { MmFree(thing); thing = NULL; } } while (0)

void FsExt2Cleanup(Ext2FileSystem* pFS)
{
	SAFE_DELETE(pFS->m_pBlockGroups);
	SAFE_DELETE(pFS->m_pBlockBuffer);
	SAFE_DELETE(pFS->m_pBlockBuffer2);
	SAFE_DELETE(pFS->m_pBlockBitmapPtr);
	SAFE_DELETE(pFS->m_pInodeBitmapPtr);
	
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
