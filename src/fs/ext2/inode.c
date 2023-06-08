//  ***************************************************************
//  fs/ext2/inode.c - Creation date: 18/11/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <vfs.h>
#include <ext2.h>

const FileNodeOps g_Ext2FileOps =
{
	.OnUnreferenced = Ext2FileOnUnreferenced,
	.Read      = Ext2FileRead,
	.Write     = Ext2FileWrite,
	.EmptyFile = Ext2FileEmpty,
};

const FileNodeOps g_Ext2DirOps =
{
	.OnUnreferenced = Ext2FileOnUnreferenced,
	.ReadDir    = Ext2ReadDir,
	.FindDir    = Ext2FindDir,
	.CreateFile = Ext2CreateFile,
	.UnlinkFile = Ext2UnlinkFile,
	.CreateDir  = Ext2CreateDir,
	.RemoveDir  = Ext2RemoveDir,
	.RenameOp   = Ext2RenameOp,
};

// *************************
//   Section : Inode Cache
// *************************

void Ext2InodeToFileNode(FileNode* pFileNode, Ext2Inode* pInode, uint32_t inodeNo)
{
	// Set the inode number
	pFileNode->m_inode = inodeNo;
	
	// Get the file type.
	switch (pInode->m_permissions & E2_INO_TYPE_MASK)
	{
		//don't currently support:
		//FIFO
		//SYMLINK
		//UNIX SOCKET
		default:
			pFileNode->m_type = FILE_TYPE_NONE;
			break;
		case E2_INO_CHAR_DEV:
			pFileNode->m_type = FILE_TYPE_CHAR_DEVICE;
			break;
		case E2_INO_BLOCK_DEV:
			pFileNode->m_type = FILE_TYPE_BLOCK_DEVICE;
			break;
		case E2_INO_FILE:
			pFileNode->m_type = FILE_TYPE_FILE;
			break;
		case E2_INO_DIRECTORY:
			pFileNode->m_type = FILE_TYPE_DIRECTORY;
			break;
		case E2_INO_SYM_LINK:
			pFileNode->m_type = FILE_TYPE_SYMBOLIC_LINK;
			break;
	}
	
	// NanoShell has no concept of a 'user'.. If anyone can write/read/exec, we can do that. :)
	if (pInode->m_permissions & E2_PERM_ANYONE_WRITE) pFileNode->m_perms |= PERM_WRITE;
	if (pInode->m_permissions & E2_PERM_ANYONE_READ)  pFileNode->m_perms |= PERM_READ;
	//if (pInode->m_permissions & E2_PERM_ANYONE_EXEC)
	pFileNode->m_perms |= PERM_EXEC;
	
	// If the file is too big we probably shouldn't try to modify it.
	if (pInode->m_size > 64*1024*1024 || pInode->m_upperSize > 0)
	{
		pFileNode->m_perms &= ~PERM_WRITE;
	}
	
	// Set the modify and create time.
	pFileNode->m_modifyTime = pInode->m_lastModTime;
	pFileNode->m_createTime = pInode->m_creationTime;
	
	pFileNode->m_length     = pInode->m_size;
	
	// TODO: Read() and Write() calls if this is file.
	// TODO: ReadDir() and other calls if this is a directory.
	if (pFileNode->m_type == FILE_TYPE_DIRECTORY)
	{
		pFileNode->m_bHasDirCallbacks = true;
		pFileNode->m_pFileOps = &g_Ext2DirOps;
	}
	else
	{
		pFileNode->m_bHasDirCallbacks = false;
		pFileNode->m_pFileOps = &g_Ext2FileOps;
	}
}

void Ext2FreeInodeCacheUnit(Ext2InodeCacheUnit* pUnit)
{
	if (pUnit->m_pBlockBuffer)
	{
		MmFree(pUnit->m_pBlockBuffer);
		pUnit->m_pBlockBuffer = NULL;
	}
	
	MmFree(pUnit);
}

void FsExt2OnErase(UNUSED const void* key, void* data)
{
	Ext2InodeCacheUnit* pToFree = data;
	
	ASSERT((uint32_t)key == pToFree->m_inodeNumber);
	
	Ext2FreeInodeCacheUnit(pToFree);
}

Ext2InodeCacheUnit* Ext2LookUpInodeCacheUnit(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	Ext2InodeCacheUnit* pUnit = HtLookUp(pFS->m_pInodeHashTable, (void*)inodeNo);
	
	return pUnit;
}

void Ext2AddInodeCacheUnitToCache(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pUnit)
{
	HtSet(pFS->m_pInodeHashTable, (void*)pUnit->m_inodeNumber, pUnit);
}

void Ext2RemoveInodeCacheUnit(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	HtErase(pFS->m_pInodeHashTable, (void*)inodeNo);
}

// Adds an inode to the binary search tree.
Ext2InodeCacheUnit* Ext2AddInodeToCache(Ext2FileSystem* pFS, uint32_t inodeNo, Ext2Inode* pInode)
{
	//SLogMsg("sizeof = %d", sizeof(Ext2InodeCacheUnit));
	// Create a new inode cache unit:
	Ext2InodeCacheUnit* pUnit = MmAllocate(sizeof(Ext2InodeCacheUnit));
	memset(pUnit, 0, sizeof(Ext2InodeCacheUnit));
	
	// Set its inode number.
	pUnit->m_inodeNumber = inodeNo;
	pUnit->m_inode = *pInode; // copy the inode's data.
	
	pUnit->m_node.m_implData  = (uint32_t)pUnit; // for faster lookup
	pUnit->m_node.m_implData1 = (uint32_t)pFS;
	pUnit->m_node.m_pFileSystemHandle = pFS;
	
	Ext2InodeToFileNode(&pUnit->m_node, pInode, inodeNo);
	
	if (pFS->m_bIsReadOnly)
	{
		pUnit->m_node.m_perms &= ~PERM_WRITE;
	}
	
	Ext2AddInodeCacheUnitToCache(pFS, pUnit);
	
	// determine a rough hint. This is just the maximum block address.
	pUnit->m_nBlockAllocHint = 0;
	
	for (int i = 0; i < 12; i++)
	{
		if (pUnit->m_nBlockAllocHint < pUnit->m_inode.m_directBlockPointer[i])
			pUnit->m_nBlockAllocHint = pUnit->m_inode.m_directBlockPointer[i];
	}
	
	if (pUnit->m_nBlockAllocHint < pUnit->m_inode.m_singlyIndirBlockPtr) pUnit->m_nBlockAllocHint = pUnit->m_inode.m_singlyIndirBlockPtr;
	if (pUnit->m_nBlockAllocHint < pUnit->m_inode.m_doublyIndirBlockPtr) pUnit->m_nBlockAllocHint = pUnit->m_inode.m_doublyIndirBlockPtr;
	if (pUnit->m_nBlockAllocHint < pUnit->m_inode.m_triplyIndirBlockPtr) pUnit->m_nBlockAllocHint = pUnit->m_inode.m_triplyIndirBlockPtr;
	
	return pUnit;
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

void Ext2FlushInode(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pUnit)
{
	Ext2Inode* pInputInode = &pUnit->m_inode;
	uint32_t inodeNo = pUnit->m_inodeNumber;
	
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
	
	*pInode = *pInputInode;
	
	ASSERT(Ext2WriteBlocks(pFS, inodeTableAddr + blockInodeIsIn, 1, bytes) == DEVERR_SUCCESS);
}

// Read an inode and add it to the inode cache. Give it a name from the system side, since
// the inodes themselves do not contain names -- that's the job of the directory entry.
// When done, return the specific cache unit.
Ext2InodeCacheUnit* Ext2ReadInode(Ext2FileSystem* pFS, uint32_t inodeNo, bool bForceReRead)
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
	
	return Ext2AddInodeToCache(pFS, inodeNo, &inode);
}

// **************************************
//   Section : Inode Block Manipulation
// **************************************

static int Ext2CalculateIBlocks(int nBlocks, int nBlockSize)
{
	return nBlocks * (nBlockSize / 512);
}

enum
{
	EXT2_INODE_USE_NOTHING,
	EXT2_INODE_USE_DIRECT,
	EXT2_INODE_USE_SINGLY,
	EXT2_INODE_USE_DOUBLY,
	EXT2_INODE_USE_TRIPLY,
};

// How this works in a nutshell (cases)
// EXT2_INODE_USE_NOTHING: blockAddrs is unused.
// EXT2_INODE_USE_DIRECT: blockIndices is a pointer to 1 int referring to the index used in pInode->m_directBlockPointer.
// EXT2_INODE_USE_SINGLY: blockIndices is a pointer to 1 int  referring to the indices used in DA(pInode->m_singlyIndirBlockPtr)[*0].
// EXT2_INODE_USE_DOUBLY: blockIndices is a pointer to 2 ints referring to the indices used in DA(DA(pInode->m_doublyIndirBlockPtr)[*0])[*1].
// EXT2_INODE_USE_TRIPLY: blockIndices is a pointer to 3 ints referring to the indices used in DA(DA(DA(pInode->m_triplyIndirBlockPtr)[*0])[*1])[*2].
// (DA = Data at a block address, *X = The Xth element of blockIndices.)
void Ext2GetInodeBlockLocation(uint32_t offset, uint32_t* useWhat, uint32_t* blockIndices, uint32_t addrsPerBlock)
{
	useWhat[0] = EXT2_INODE_USE_NOTHING;
	
	if (offset < 12)
	{
		useWhat[0] = EXT2_INODE_USE_DIRECT;
		blockIndices[0] = offset;
		return;
	}
	
	offset -= 12;
	
	// is this part of the singly-indirect block?
		
	if (offset < addrsPerBlock)
	{
		useWhat[0] = EXT2_INODE_USE_SINGLY;
		blockIndices[0] = offset;
		return;
	}
	
	offset -= addrsPerBlock;
	
	// is this part of the doubly indirect block?
	if (offset < addrsPerBlock * addrsPerBlock)
	{
		uint32_t firstTurn  = offset / addrsPerBlock;
		uint32_t secondTurn = offset % addrsPerBlock;
		
		useWhat[0] = EXT2_INODE_USE_DOUBLY;
		blockIndices[0] = firstTurn;
		blockIndices[1] = secondTurn;
		return;
	}
	
	SLogMsg("ERROR: Don't handle triply indirect things yet. Offset: %d", offset);
	
	// TODO: Check the triply indirect things as well soon.
	
	// well, we're trying to exceed the file's boundaries anyway, call it quits ;)
	return;
}

//note: the offset is in <block_size> units.
uint32_t Ext2ReadWriteInodeBlock(Ext2Inode* pInode, Ext2FileSystem* pFS, uint32_t offset, bool bWrite, uint32_t blockNo)
{
	uint32_t addrsPerBlock = pFS->m_blockSize / 4;
	
	uint32_t useWhat = EXT2_INODE_USE_NOTHING;
	uint32_t blockIndices[3];
	
	Ext2GetInodeBlockLocation(offset, &useWhat, blockIndices, addrsPerBlock);
	
	uint32_t* data = (uint32_t*)pFS->m_pBlockBuffer2;
	
	switch (useWhat)
	{
		case EXT2_INODE_USE_DIRECT:
		{
			if (!bWrite)
			{
				return pInode->m_directBlockPointer[blockIndices[0]];
			}
			
			pInode->m_directBlockPointer[blockIndices[0]] = blockNo;
			
			//TODO: Somehow mark this inode as dirty.
			
			return blockNo;
		}
		case EXT2_INODE_USE_SINGLY:
		{
			if (!bWrite)
			{
				if (pInode->m_singlyIndirBlockPtr == 0) return 0;
				
				ASSERT(Ext2ReadBlocks(pFS, pInode->m_singlyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
				return data[blockIndices[0]];
			}
			
			if (pInode->m_singlyIndirBlockPtr == 0)
			{
				// Don't allocate unless blockNo is not zero.
				if (blockNo == 0) return blockNo;
				
				// Allocate a new block.
				uint32_t blk = Ext2AllocateBlock(pFS, blockNo);
				if (blk == ~0u)
				{
					// Huh? We ran out of blocks. Wellp.
					return 0;
				}
				else
				{
					pInode->m_singlyIndirBlockPtr = blk;
					//TODO: Somehow mark this inode as dirty.
					memset(data, 0, pFS->m_blockSize);
				}
			}
			else
			{
				ASSERT(Ext2ReadBlocks(pFS, pInode->m_singlyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
			}
			
			data[blockIndices[0]] = blockNo;
			ASSERT(Ext2WriteBlocks(pFS, pInode->m_singlyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
			
			// If the singly indirect list is full of zeroes...
			bool bIsFullOfZeroes = true;
			for (size_t i = 0; i < pFS->m_blockSize / sizeof(uint32_t); i++)
			{
				if (data[i] != 0)
				{
					bIsFullOfZeroes = false;
					break;
				}
			}
			
			if (bIsFullOfZeroes)
			{
				Ext2FreeBlock(pFS, pInode->m_singlyIndirBlockPtr);
				pInode->m_singlyIndirBlockPtr = 0;
			}
			
			return blockNo;
		}
		case EXT2_INODE_USE_DOUBLY:
		{
			if (!bWrite)
			{
				if (pInode->m_doublyIndirBlockPtr == 0) return 0;
				
				ASSERT(Ext2ReadBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
				
				uint32_t thing = data[blockIndices[0]];
				if (thing == 0) return 0;
				
				ASSERT(Ext2ReadBlocks(pFS, thing, 1, data) == DEVERR_SUCCESS);
				
				return data[blockIndices[1]];
			}
			
			// If doubly indirect is zero...
			if (pInode->m_doublyIndirBlockPtr == 0)
			{
				// Don't allocate unless blockNo is not zero.
				if (blockNo == 0) return blockNo;
				
				// Allocate it. The goal is to overwrite the entry inside the inode
				uint32_t blk = Ext2AllocateBlock(pFS, blockNo);
				if (blk == ~0u)
				{
					// Huh? We ran out of blocks. Wellp.
					return 0;
				}
				else
				{
					pInode->m_doublyIndirBlockPtr = blk;
					
					// TODO: Somehow mark this inode as dirty.
					memset(data, 0, pFS->m_blockSize);
					ASSERT(Ext2WriteBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
				}
			}
			else
			{
				ASSERT(Ext2ReadBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
			}
			
			// If the stuff inside the doubly direct is zero.
			uint32_t firstTurn = data[blockIndices[0]];
			if (firstTurn == 0)
			{
				// Don't allocate unless blockNo is not zero.
				if (blockNo == 0) return blockNo;
				
				// Allocate it. The goal is to overwrite the entry inside the inode
				uint32_t blk = Ext2AllocateBlock(pFS, blockNo);
				if (blk == ~0u)
				{
					// Huh? We ran out of blocks. Wellp.
					return 0;
				}
				else
				{
					// Read the doubly indirect table into memory. The 'data' pointer currently has its contents.
					ASSERT(Ext2ReadBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
					
					// Overwrite the entry in the doubly indirect table.
					data[blockIndices[0]] = blk;
					
					// Write the doubly indirect table into memory. The 'data' pointer currently has its contents.
					ASSERT(Ext2WriteBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
					
					// Reset 'data' to null.
					memset(data, 0, pFS->m_blockSize);
					
					// Write it to the disk.
					ASSERT(Ext2WriteBlocks(pFS, firstTurn, 1, data) == DEVERR_SUCCESS);
				}
			}
			else
			{
				ASSERT(Ext2ReadBlocks(pFS, firstTurn, 1, data) == DEVERR_SUCCESS);
			}
			
			// All good.
			data[blockIndices[1]] = blockNo;
			ASSERT(Ext2WriteBlocks(pFS, firstTurn, 1, data) == DEVERR_SUCCESS);
			
			// If the first turn is full of zeroes...
			bool bIsFullOfZeroes = true;
			for (size_t i = 0; i < pFS->m_blockSize / sizeof(uint32_t); i++)
			{
				if (data[i] != 0)
				{
					bIsFullOfZeroes = false;
					break;
				}
			}
			
			if (bIsFullOfZeroes)
			{
				//well, we can free this and remove it from the doubly indirect list
				Ext2FreeBlock(pFS, firstTurn);
				
				ASSERT(Ext2ReadBlocks(pFS, pInode->m_doublyIndirBlockPtr, 1, data) == DEVERR_SUCCESS);
				ASSERT(data[blockIndices[0]] == firstTurn);
				
				data[blockIndices[0]] = 0;
				
				// Check if this is full of zeroes too.
				bIsFullOfZeroes = true;
				for (size_t i = 0; i < pFS->m_blockSize / sizeof(uint32_t); i++)
				{
					if (data[i] != 0)
					{
						bIsFullOfZeroes = false;
						break;
					}
				}
				
				if (bIsFullOfZeroes)
				{
					Ext2FreeBlock(pFS, pInode->m_doublyIndirBlockPtr);
					pInode->m_doublyIndirBlockPtr = 0;
				}
			}
			
			return blockNo;
		}
		// TODO: handle triply indirect blocks
	}
	
	return 0;
}

uint32_t Ext2GetInodeBlock(Ext2Inode* pInode, Ext2FileSystem* pFS, uint32_t offset)
{
	return Ext2ReadWriteInodeBlock(pInode, pFS, offset, false, 0);
}

void Ext2SetInodeBlock(Ext2Inode* pInode, Ext2FileSystem* pFS, uint32_t offset, uint32_t blockNo)
{
	ASSERT(Ext2ReadWriteInodeBlock(pInode, pFS, offset, true, blockNo) == blockNo);
}

// Expands an inode by 'byHowMuch' bytes.
int Ext2InodeExpand(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCacheUnit, uint32_t byHowMuch)
{
	uint32_t inodeNo = pCacheUnit->m_inodeNumber;
	
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
	uint64_t thing = index * pFS->m_inodeSize;
	uint32_t blockInodeIsIn = (uint32_t)(thing / pFS->m_blockSize);
	uint32_t blockInodeOffs = (uint32_t)(thing % pFS->m_blockSize);
	
	uint8_t bytes[pFS->m_blockSize];
	if (Ext2ReadBlocks(pFS, inodeTableAddr + blockInodeIsIn, 1, bytes) != DEVERR_SUCCESS)
		return ERR_IO_ERROR;
	
	Ext2Inode* pInodePlaceOnDisk = (Ext2Inode*)(bytes + blockInodeOffs);
	
	uint32_t byteSizeOld = pCacheUnit->m_inode.m_size, byteSizeNew = byteSizeOld + byHowMuch;
	
	uint32_t blockSizeOld = (byteSizeOld + pFS->m_blockSize - 1) >> pFS->m_log2BlockSize;
	uint32_t blockSizeNew = (byteSizeNew + pFS->m_blockSize - 1) >> pFS->m_log2BlockSize;
	
	bool bSuccessfulResize = true;
	
	// Allocate the missing blocks.
	for (uint32_t i = blockSizeOld; i < blockSizeNew; i++)
	{
		uint32_t newBlock = Ext2AllocateBlock(pFS, pCacheUnit->m_nBlockAllocHint);
		
		// If a block couldn't be allocated, we have run out of space and we need to stop resizing immediately.
		if (newBlock == EXT2_INVALID_INODE)
		{
			bSuccessfulResize = false;
			
			for (uint32_t bi = blockSizeOld; bi < i; bi++)
			{
				uint32_t blk = Ext2GetInodeBlock(pInodePlaceOnDisk, pFS, bi);
				Ext2FreeBlock(pFS, blk);
				Ext2SetInodeBlock(pInodePlaceOnDisk, pFS, bi, 0);
			}
			
			break;
		}
		
		// Set the block in the correct place.
		Ext2SetInodeBlock(pInodePlaceOnDisk, pFS, i, newBlock);
		
		memcpy(&pCacheUnit->m_inode.m_directBlockPointer[0], &pInodePlaceOnDisk->m_directBlockPointer[0], sizeof pInodePlaceOnDisk->m_directBlockPointer);
		pCacheUnit->m_inode.m_singlyIndirBlockPtr = pInodePlaceOnDisk->m_singlyIndirBlockPtr;
		pCacheUnit->m_inode.m_doublyIndirBlockPtr = pInodePlaceOnDisk->m_doublyIndirBlockPtr;
		pCacheUnit->m_inode.m_triplyIndirBlockPtr = pInodePlaceOnDisk->m_triplyIndirBlockPtr;
		
		pCacheUnit->m_nBlockAllocHint = newBlock;
	}
	
	if (bSuccessfulResize)
	{		
		pInodePlaceOnDisk->m_size += byHowMuch;
		pCacheUnit->m_inode.m_size += byHowMuch;
		pCacheUnit->m_node.m_length += byHowMuch;
		
		pInodePlaceOnDisk->m_nBlocks = pCacheUnit->m_inode.m_nBlocks = Ext2CalculateIBlocks(blockSizeNew, pFS->m_blockSize);
		
		// Write the block containing the inode back to disk.
		if (Ext2WriteBlocks(pFS, inodeTableAddr + blockInodeIsIn, 1, bytes) != DEVERR_SUCCESS)
			return ERR_IO_ERROR;
	}
	
	return bSuccessfulResize ? ERR_SUCCESS : ERR_IO_ERROR;
}

int Ext2InodeShrink(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCacheUnit, uint32_t byHowMuch)
{
	uint32_t inodeNo = pCacheUnit->m_inodeNumber;
	
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
	uint64_t thing = index * pFS->m_inodeSize;
	uint32_t blockInodeIsIn = (uint32_t)(thing / pFS->m_blockSize);
	uint32_t blockInodeOffs = (uint32_t)(thing % pFS->m_blockSize);
	
	uint8_t bytes[pFS->m_blockSize];
	if (Ext2ReadBlocks(pFS, inodeTableAddr + blockInodeIsIn, 1, bytes) != DEVERR_SUCCESS)
		return ERR_IO_ERROR;
	
	Ext2Inode* pInodePlaceOnDisk = (Ext2Inode*)(bytes + blockInodeOffs);
	
	// don't try to shrink the file beyond its size.
	if (byHowMuch >= pInodePlaceOnDisk->m_size)
		byHowMuch  = pInodePlaceOnDisk->m_size;
	
	uint32_t byteSizeOld = pCacheUnit->m_inode.m_size, byteSizeNew = byteSizeOld - byHowMuch;
	
	pInodePlaceOnDisk->m_size -= byHowMuch;
	pCacheUnit->m_inode.m_size -= byHowMuch;
	pCacheUnit->m_node.m_length -= byHowMuch;
	
	uint32_t blockSizeOld = (byteSizeOld + pFS->m_blockSize - 1) >> pFS->m_log2BlockSize;
	uint32_t blockSizeNew = (byteSizeNew + pFS->m_blockSize - 1) >> pFS->m_log2BlockSize;
	
	// Free the extra blocks.
	for (uint32_t i = blockSizeNew; i < blockSizeOld; i++)
	{
		uint32_t blockToFree = Ext2GetInodeBlock(pInodePlaceOnDisk, pFS, i);
		
		Ext2FreeBlock(pFS, blockToFree);
		
		Ext2SetInodeBlock(pInodePlaceOnDisk, pFS, i, 0);
		
		memcpy(&pCacheUnit->m_inode.m_directBlockPointer[0], &pInodePlaceOnDisk->m_directBlockPointer[0], sizeof pInodePlaceOnDisk->m_directBlockPointer);
		pCacheUnit->m_inode.m_singlyIndirBlockPtr = pInodePlaceOnDisk->m_singlyIndirBlockPtr;
		pCacheUnit->m_inode.m_doublyIndirBlockPtr = pInodePlaceOnDisk->m_doublyIndirBlockPtr;
		pCacheUnit->m_inode.m_triplyIndirBlockPtr = pInodePlaceOnDisk->m_triplyIndirBlockPtr;
	}
	
	pInodePlaceOnDisk->m_nBlocks = pCacheUnit->m_inode.m_nBlocks = Ext2CalculateIBlocks(blockSizeNew, pFS->m_blockSize);
	
	// Write the block containing the inode back to disk.
	if (Ext2WriteBlocks(pFS, inodeTableAddr + blockInodeIsIn, 1, bytes) != DEVERR_SUCCESS)
		return ERR_IO_ERROR;
	
	return ERR_SUCCESS;
}


// ******************************
//   Section : Inode Allocation
// ******************************

void Ext2LoadInodeBitmaps(Ext2FileSystem *pFS)
{
	// For each block group...
	for (uint32_t bg = 0; bg < pFS->m_blockGroupCount; bg++)
	{
		Ext2BlockGroupDescriptor* pBG = &pFS->m_pBlockGroups[bg];
		ASSERT(Ext2ReadBlocks(pFS, pBG->m_blockAddrInodeUsageBmp, pFS->m_blocksPerInodeBitmap, &pFS->m_pInodeBitmapPtr[(bg * pFS->m_blocksPerInodeBitmap) << pFS->m_log2BlockSize]) == DEVERR_SUCCESS);
	}
}

void Ext2FlushInodeBitmap(Ext2FileSystem *pFS, UNUSED uint32_t bgdIndex)
{
	uint32_t* pData = (uint32_t*)&pFS->m_pInodeBitmapPtr[(bgdIndex * pFS->m_blocksPerInodeBitmap) << pFS->m_log2BlockSize];
	
	ASSERT(Ext2WriteBlocks(pFS, pFS->m_pBlockGroups[bgdIndex].m_blockAddrInodeUsageBmp, pFS->m_blocksPerInodeBitmap, pData) == DEVERR_SUCCESS);
}

uint32_t Ext2AllocateInode(Ext2FileSystem* pFS)
{
	uint32_t freeBGD = ~0u;
	
	for (uint32_t i = 0; i < pFS->m_blockGroupCount; i++)
	{
		Ext2BlockGroupDescriptor *pBG = &pFS->m_pBlockGroups[i];
		
		if (pBG->m_nFreeInodes > 0)
		{
			freeBGD = i;
			break;
		}
	}
	
	if (freeBGD == ~0u)
	{
		//well, we did not really have space for an inode.
		return ~0u;
	}
	
	Ext2BlockGroupDescriptor *pBG = &pFS->m_pBlockGroups[freeBGD];
	
	// Look for a free block inside the block group.
	uint32_t entriesPerBlock = pFS->m_inodesPerGroup / 32;
	uint32_t* pData = (uint32_t*)&pFS->m_pInodeBitmapPtr[(freeBGD * pFS->m_blocksPerInodeBitmap) << pFS->m_log2BlockSize];
	
	for (uint32_t k = 0; k < entriesPerBlock; k++)
	{
		// if all the blocks here are allocated....
		if (pData[k] == ~0u)
			continue;
		
		for (uint32_t l = 0; l < 32; l++)
		{
			if (pData[k] & (1 << l)) continue;
			
			// Set the bit in the bitmap and return.
			pData[k] |= (1 << l);
			
			// Flush the bitmap.
			Ext2FlushInodeBitmap(pFS, freeBGD);
			
			// Update the free inodes.
			pBG->m_nFreeInodes--;
			Ext2FlushBlockGroupDescriptor(pFS, freeBGD);
			
			// Update the superblock.
			pFS->m_superBlock.m_nFreeInodes--;
			Ext2FlushSuperBlock(pFS);
			
			return 1 + freeBGD * pFS->m_inodesPerGroup + k * 32 + l;
		}
	}
	
	// Maybe this entry was faulty. well, that's the problem of the driver that wrote this...
	// TODO: Don't just bail out if we have such a faulty thing.
	LogMsg("ERROR: Block group descriptor %d, whose m_nFreeInodes is %d actually lied and there are no blocks inside! An ``e2fsck'' MUST be performed.", freeBGD, pBG->m_nFreeInodes);
	return ~0u;
}

// If bWrite is set, takes the value from pResultInOut and sets the 'used' bit of that entry.
// If bWrite is clear, takes the 'used' bit of the entry and sets pResultInOut to that.
void Ext2InodeBitmapCheck(Ext2FileSystem* pFS, uint32_t inodeNo, bool bWrite, bool* pResultInOut)
{
	inodeNo--; //since inode numbers start at 1
	
	uint32_t bgd = inodeNo / pFS->m_inodesPerGroup, idxInsideBgd = inodeNo % pFS->m_inodesPerGroup;
	
	uint32_t* pData = (uint32_t*)&pFS->m_pInodeBitmapPtr[(bgd * pFS->m_blocksPerInodeBitmap) << pFS->m_log2BlockSize];
	
	uint32_t bitOffset = idxInsideBgd / 32, bitIndex = idxInsideBgd % 32;
	
	if (bWrite)
	{
		if (*pResultInOut)
			pData[bitOffset] |=  (1 << bitIndex);
		else
			pData[bitOffset] &= ~(1 << bitIndex);
		
		Ext2FlushInodeBitmap(pFS, bgd);
		
		Ext2BlockGroupDescriptor *pBG = &pFS->m_pBlockGroups[bgd];
		pBG->m_nFreeInodes++;
		Ext2FlushBlockGroupDescriptor(pFS, bgd);
		
		pFS->m_superBlock.m_nFreeInodes++;
		Ext2FlushSuperBlock(pFS);
	}
	else
	{
		*pResultInOut = (pData[bitOffset] & (1 << bitIndex)) != 0;
	}
}

bool Ext2CheckInodeFree(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	bool result = false;
	Ext2InodeBitmapCheck(pFS, inodeNo, false, &result);
	return !result;
}

void Ext2CheckInodeMarkFree(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	bool result = false;
	Ext2InodeBitmapCheck(pFS, inodeNo, true, &result);
}

void Ext2CheckInodeMarkUsed(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	bool result = true;
	Ext2InodeBitmapCheck(pFS, inodeNo, true, &result);
}

void Ext2FreeInode(Ext2FileSystem *pFS, uint32_t inodeNo)
{
	Ext2CheckInodeMarkFree(pFS, inodeNo);
}
