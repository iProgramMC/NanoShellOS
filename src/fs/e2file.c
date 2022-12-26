//  ***************************************************************
//  e2file.c - Creation date: 20/11/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <vfs.h>
#include <ext2.h>
#include <misc.h>

static int FileTypeToExt2TypeHint(int fileType)
{
	if (fileType & FILE_TYPE_DIRECTORY)
		return E2_DETI_DIRECTORY;
	
	switch (fileType)
	{
		case FILE_TYPE_FILE:         return E2_DETI_REG_FILE;
		case FILE_TYPE_CHAR_DEVICE:  return E2_DETI_CHAR_DEV;
		case FILE_TYPE_BLOCK_DEVICE: return E2_DETI_BLOCK_DEV;
	}
	
	return E2_DETI_UNKNOWN;
}

uint32_t Ext2FileRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, UNUSED bool block)
{
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	Ext2Inode* pInode = &pUnit->m_inode;
	
	ASSERT(pUnit->m_inodeNumber == pNode->m_inode);
	
	// Make sure to cap the offset.
	if (offset > pInode->m_size) 
		return 0;
	
	if (offset + size > pInode->m_size)
		size = pInode->m_size - offset;
	
	if (size == 0) return 0;
	
	// read!
	Ext2ReadFileSegment(pFS, pUnit, offset, size, pBuffer);
	return size;
}

uint32_t Ext2FileWrite(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, UNUSED bool block)
{
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	Ext2Inode* pInode = &pUnit->m_inode;
	
	ASSERT(pUnit->m_inodeNumber == pNode->m_inode);
	
	// Make sure to cap the offset.
	if (offset > pInode->m_size) 
		return 0;
	
	if (offset + size > pInode->m_size)
	{
		Ext2InodeExpand(pFS, pUnit, size - (pInode->m_size - offset));
	}
	
	if (size == 0) return 0;
	
	// write!
	Ext2WriteFileSegment(pFS, pUnit, offset, size, pBuffer);
	return size;
}

void Ext2FileEmpty(FileNode* pNode)
{
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	Ext2Inode* pInode = &pUnit->m_inode;
	
	Ext2InodeShrink(pFS, pUnit, pInode->m_size);
}

bool Ext2FileOpen(UNUSED FileNode* pNode)
{
	//all good
	return true;
}

void Ext2FileClose(UNUSED FileNode* pNode)
{
	//all good
}

static uint32_t Ext2CalculateDirEntSize(Ext2DirEnt* pDirEnt)
{
	uint32_t sz = pDirEnt->m_nameLength + 8;
	
	sz = 4 * ((3 + sz) / 4);
	
	return sz;
}

// TODO: Don't duplicate the same code. Ext2AddDirectoryEntry, Ext2RemoveDirectoryEntry and Ext2ChangeDirEntryTypeIndicator share similar skeleton.

void Ext2AddDirectoryEntry(Ext2FileSystem *pFS, Ext2InodeCacheUnit* pUnit, const char* pName, uint32_t inodeNo, uint8_t typeIndicator)
{
	//note: I don't think we want to allocate something in the heap right now.
	uint8_t buffer[pFS->m_blockSize];
	
	size_t nameLen = strlen(pName);
	if (nameLen >= 255)
	{
		nameLen  = 255;
	}
	
	union
	{
		uint8_t* pEntryData;
		Ext2DirEnt* pDirEnt;
	}
	newDEntry;
	
	newDEntry.pEntryData = buffer;
	
	newDEntry.pDirEnt->m_inode         = inodeNo;
	newDEntry.pDirEnt->m_typeIndicator = typeIndicator;
	newDEntry.pDirEnt->m_nameLength    = nameLen;
	
	memcpy(newDEntry.pDirEnt->m_name, pName, nameLen);
	
	// ceil it to a multiple of 4
	newDEntry.pDirEnt->m_entrySize = Ext2CalculateDirEntSize(newDEntry.pDirEnt);
	
	// For each block in the directory inode.
	uint32_t blockNo = 0, offset = 0;
	do
	{
	TRY_AGAIN:
		blockNo = Ext2GetInodeBlock(&pUnit->m_inode, pFS, offset++);
		
		if (blockNo == 0) break;
		
		// look through all the dentries
		ASSERT(Ext2ReadBlocks(pFS, blockNo, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
		
		Ext2DirEnt* pDirEnt = (Ext2DirEnt*)pFS->m_pBlockBuffer;
		Ext2DirEnt* pEndPoint = (Ext2DirEnt*)(pFS->m_pBlockBuffer + pFS->m_blockSize);
		
		while (pDirEnt < pEndPoint)
		{
			// check if the entry's size can fit this dentry too
			uint32_t actualEntrySize = Ext2CalculateDirEntSize(pDirEnt);
			
			if (pDirEnt->m_inode == 0)
				actualEntrySize = 0;
			
			if (pDirEnt->m_entrySize >= newDEntry.pDirEnt->m_entrySize + actualEntrySize)
			{
				// calculate the point where the new entry will go.
				Ext2DirEnt* pDestEnt = (Ext2DirEnt*)((uint8_t*)pDirEnt + actualEntrySize);
				
				uint32_t newActualEntrySize = newDEntry.pDirEnt->m_entrySize;
				
				// set the new entry's size to pad the space that was occupied by the old one
				newDEntry.pDirEnt->m_entrySize = pDirEnt->m_entrySize - actualEntrySize;
				
				// update the old one so it only uses the space it should
				pDirEnt->m_entrySize = actualEntrySize;
				
				// clear it. Just in case
				memset(pDestEnt, 0, newDEntry.pDirEnt->m_entrySize);
				
				// copy the new one
				memcpy(pDestEnt, newDEntry.pDirEnt, newActualEntrySize);
				
				// write!
				ASSERT(Ext2WriteBlocks(pFS, blockNo, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
				
				// force a refresh
				pUnit->m_nLastBlockRead = ~0u;
				
				// Now, get the entry itself. This will be a hard link
				Ext2InodeCacheUnit* pDestUnit = Ext2ReadInode(pFS, inodeNo, pName, false);
				
				// Increase the destination Inode's reference count
				pDestUnit->m_inode.m_nLinks++;
				
				Ext2FlushInode(pFS, pDestUnit);
				
				return;
			}
			
			pDirEnt = (Ext2DirEnt*)((uint8_t*)pDirEnt + pDirEnt->m_entrySize);
		}
	}
	while (true);
	
	// note: This should be fine, if the directory isn't <multiple of block size> sized then I don't think it's good
	memset(buffer, 0, pFS->m_blockSize);
	
	// Note: We create a new entry here in the hope that it gets used later on.
	Ext2DirEnt* pNewDirEnt = (Ext2DirEnt*)(buffer);
	pNewDirEnt->m_entrySize = pFS->m_blockSize;
	
	// Write it
	ASSERT(Ext2FileWrite(&pUnit->m_node, pUnit->m_inode.m_size, pFS->m_blockSize, buffer, true));
	
	if (offset < 1)
		offset = 0;
	else
		offset -= 1;
	
	// Try again.
	goto TRY_AGAIN;
}

// note: Don't use this behemoth directly. Instead, use one of the functions that use this behind the scenes.
static int Ext2RemoveDirectoryEntry(Ext2FileSystem *pFS, Ext2InodeCacheUnit* pUnit, const char* pName, bool bForceDirsToo, bool bDontDeleteInode, bool bByInode, uint32_t nInodeIn, uint32_t* pInodeOut, uint8_t* pTypeIndicatorOut)
{
	//note: I don't think we want to allocate something in the heap right now.
	char name[256];
	
	// For each block in the directory inode.
	uint32_t blockNo = 0, offset = 0;
	do
	{
		blockNo = Ext2GetInodeBlock(&pUnit->m_inode, pFS, offset++);
		
		if (blockNo == 0) break;
		
		// look through all the dentries
		ASSERT(Ext2ReadBlocks(pFS, blockNo, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
		
		Ext2DirEnt* pDirEnt = (Ext2DirEnt*)pFS->m_pBlockBuffer, *pPrevDirEnt = pDirEnt;
		Ext2DirEnt* pEndPoint = (Ext2DirEnt*)(pFS->m_pBlockBuffer + pFS->m_blockSize);
		
		while (pDirEnt < pEndPoint)
		{
			if (!bByInode)
			{
				memcpy(name, pDirEnt->m_name, pDirEnt->m_nameLength);
				name[pDirEnt->m_nameLength] = 0;
			}
			
			if ((bByInode && pDirEnt->m_inode == nInodeIn) || (!bByInode && strcmp(name, pName) == 0))
			{
				//we found the entry!
				uint32_t oldEntrySize  = pDirEnt->m_entrySize;
				uint32_t oldEntryInode = pDirEnt->m_inode;
				
				if (pInodeOut)         *pInodeOut         = pDirEnt->m_inode;
				if (pTypeIndicatorOut) *pTypeIndicatorOut = pDirEnt->m_typeIndicator;
				
				// get the inode itself. This inode's reference count will be decreased.
				Ext2InodeCacheUnit* pDestUnit = Ext2ReadInode(pFS, oldEntryInode, pName, false);
				
				// Add a reference to this while we delete it.
				FsAddReference(&pDestUnit->m_node);
				
				if ((pDestUnit->m_node.m_type & FILE_TYPE_DIRECTORY) && !bForceDirsToo)
				{
					FsReleaseReference(&pDestUnit->m_node);
					
					// can't delete directories right now!
					return -EISDIR;
				}
				
				// If this is the first directory entry, move the next one backwards over this one
				// note: this case is unlikely, since this is usually the '.' entry, UNLESS this is
				// the root.
				if (pPrevDirEnt == pDirEnt)
				{
					Ext2DirEnt* pNextDirEnt = (Ext2DirEnt*)((uint8_t*)pDirEnt + pDirEnt->m_entrySize);
					
					memmove(pDirEnt, pNextDirEnt, pNextDirEnt->m_entrySize);
					
					pDirEnt->m_entrySize += oldEntrySize;
				}
				// well, it's not, so just expand the previous over this one
				else
				{
					pPrevDirEnt->m_entrySize += oldEntrySize;
				}
				
				// write!
				ASSERT(Ext2WriteBlocks(pFS, blockNo, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
				
				// force a refresh
				pUnit->m_nLastBlockRead = ~0u;
				// decrease the inode's reference count
				ASSERT(pDestUnit->m_inode.m_nLinks > 0);
				pDestUnit->m_inode.m_nLinks--;
				
				// note: This boolean flag is only used when renaming something.
				if (pDestUnit->m_inode.m_nLinks == 0 && !bDontDeleteInode)
				{
					pDestUnit->m_bAboutToBeDeleted = true;
				}
				
				// flush its data.
				Ext2FlushInode(pFS, pDestUnit);
				
				// Remove the reference - we're done with it.
				FsReleaseReference(&pDestUnit->m_node);
				
				return ENOTHING;
			}
			
			pPrevDirEnt = pDirEnt;
			pDirEnt = (Ext2DirEnt*)((uint8_t*)pDirEnt + pDirEnt->m_entrySize);
		}
	}
	while (true);
	
	return -ENOENT;
}

int Ext2ChangeDirEntryTypeIndicator(Ext2FileSystem *pFS, Ext2InodeCacheUnit* pUnit, const char* pName, uint8_t typeIndicator)
{
	//note: I don't think we want to allocate something in the heap right now.
	char name[256];
	
	// For each block in the directory inode.
	uint32_t blockNo = 0, offset = 0;
	do
	{
		blockNo = Ext2GetInodeBlock(&pUnit->m_inode, pFS, offset++);
		
		if (blockNo == 0) break;
		
		// look through all the dentries
		ASSERT(Ext2ReadBlocks(pFS, blockNo, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
		
		Ext2DirEnt* pDirEnt = (Ext2DirEnt*)pFS->m_pBlockBuffer;
		Ext2DirEnt* pEndPoint = (Ext2DirEnt*)(pFS->m_pBlockBuffer + pFS->m_blockSize);
		
		while (pDirEnt < pEndPoint)
		{
			memcpy(name, pDirEnt->m_name, pDirEnt->m_nameLength);
			name[pDirEnt->m_nameLength] = 0;
			
			if (strcmp(name, pName) == 0)
			{
				//we found the entry! Replace its type indicator.
				pDirEnt->m_typeIndicator = typeIndicator;
				
				ASSERT(Ext2WriteBlocks(pFS, blockNo, 1, pFS->m_pBlockBuffer) == DEVERR_SUCCESS);
				pUnit->m_nLastBlockRead = ~0u;
				
				return ENOTHING;
			}
			
			pDirEnt = (Ext2DirEnt*)((uint8_t*)pDirEnt + pDirEnt->m_entrySize);
		}
	}
	while (true);
	
	return -ENOENT;
}

int Ext2RenameDirectoryEntry(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pOldUnit, Ext2InodeCacheUnit* pNewUnit, const char* pOldName, const char* pNewName)
{
	uint32_t inodeNo       = 0;
	uint8_t  typeIndicator = 0;
	
	// remove the old entry. Make sure to not actually delete the inode.
	int result = Ext2RemoveDirectoryEntry(pFS, pOldUnit, pOldName, true, true, false, 0, &inodeNo, &typeIndicator);
	if (result < 0) return result;
	
	Ext2AddDirectoryEntry(pFS, pNewUnit, pNewName, inodeNo, typeIndicator);
	
	return -ENOTHING;
}

int Ext2RenameOp(FileNode* pSrcNode, FileNode* pDstNode, const char* pSrcName, const char* pDstName)
{
	if (pSrcNode->m_implData1 != pDstNode->m_implData1) return -EXDEV;
	
	// since they're a same we can choose whichever side we want
	Ext2FileSystem* pFS = (Ext2FileSystem*)pSrcNode->m_implData1;
	
	Ext2InodeCacheUnit* pSrcIcu = (Ext2InodeCacheUnit*)pSrcNode->m_implData;
	Ext2InodeCacheUnit* pDstIcu = (Ext2InodeCacheUnit*)pDstNode->m_implData;
	
	return Ext2RenameDirectoryEntry(pFS, pSrcIcu, pDstIcu, pSrcName, pDstName);
}
void SDumpBytesAsHex (void *nAddr, size_t nBytes, bool as_bytes);

DirEnt* Ext2ReadDirInternal(FileNode* pNode, uint32_t * index, DirEnt* pOutputDent, bool bSkipDotAndDotDot)
{
	// Read the directory entry, starting at (*index).
	
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	
	//note: I don't think we want to allocate something in the heap right now.
	uint8_t buffer[pFS->m_blockSize];
	uint32_t tempBuffer[2] = { 0 };
	
try_again:;
	
	if (!Ext2FileRead(pNode, *index, sizeof(uint32_t) + sizeof(uint16_t), tempBuffer, true)) return NULL;
	
	// The entry length is in tempBuffer[1].
	tempBuffer[1] &= 0xFFFF;
	
	//if (tempBuffer[1] == 0) return NULL;
	
	union
	{
		uint8_t* EntryData;
		Ext2DirEnt* dirEnt;
	} d;
	
	d.EntryData = buffer;
	
	// if it's bigger, chances are it's a Padding entry.
	if (tempBuffer[1] < pFS->m_blockSize)
	{
		if (!Ext2FileRead(pNode, *index, tempBuffer[1], d.EntryData, true)) return NULL;
	}
	
	(*index) += d.dirEnt->m_entrySize;
	
	// if the inode is zero
	if (d.dirEnt->m_inode == 0)
	{
		// try again?!
		goto try_again;
	}
	
	if (tempBuffer[1] >= pFS->m_blockSize)
	{
		return NULL;
	}
	
	d.dirEnt->m_name[d.dirEnt->m_nameLength] = 0;
	
	size_t nameLen = d.dirEnt->m_nameLength;
	if (nameLen > sizeof(pOutputDent->m_name) - 2)
		nameLen = sizeof(pOutputDent->m_name) - 2;
	
	memcpy(pOutputDent->m_name, d.dirEnt->m_name, nameLen);
	pOutputDent->m_name[nameLen] = '\0';
	
	if (bSkipDotAndDotDot)
	{
		// If it's a '.' or '..' entry, skip it. We provide our own implementation anyway.
		if (strcmp(pOutputDent->m_name, ".") == 0  ||  strcmp(pOutputDent->m_name, "..") == 0)
			return Ext2ReadDirInternal(pNode, index, pOutputDent, bSkipDotAndDotDot);
	}
	
	pOutputDent->m_inode = d.dirEnt->m_inode;
	
	// convert the type indicator
	switch (d.dirEnt->m_typeIndicator)
	{
		case E2_DETI_REG_FILE:  pOutputDent->m_type = FILE_TYPE_FILE;                        break;
		case E2_DETI_CHAR_DEV:  pOutputDent->m_type = FILE_TYPE_CHAR_DEVICE;                 break;
		case E2_DETI_BLOCK_DEV: pOutputDent->m_type = FILE_TYPE_BLOCK_DEVICE;                break;
		case E2_DETI_DIRECTORY: pOutputDent->m_type = FILE_TYPE_DIRECTORY | FILE_TYPE_FILE;  break;
		default:                pOutputDent->m_type = FILE_TYPE_NONE;                        break;
	}
	
	return pOutputDent;
}

DirEnt* Ext2ReadDir(FileNode* pNode, uint32_t * index, DirEnt* pOutputDent)
{
	return Ext2ReadDirInternal(pNode, index, pOutputDent, true);
}

FileNode* Ext2FindDir(FileNode* pNode, const char* pName)
{
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	DirEnt space; uint32_t index = 0;
	
	while (Ext2ReadDirInternal(pNode, &index, &space, false) != NULL)
	{
		if (strcmp(space.m_name, pName) == 0)
		{
			// Load the inode
			Ext2InodeCacheUnit* pCU = Ext2ReadInode(pFS, space.m_inode, space.m_name, false);
			
			if (!pCU) return NULL;
			
			FsAddReference(&pCU->m_node);
			
			return &pCU->m_node;
		}
	}
	
	return NULL;
}

// Create a hard link. Will cause an I/O error on FAT32, because it doesn't actually support hard links.
void Ext2CreateHardLinkTo(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCurrentDir, const char* pName, Ext2InodeCacheUnit* pDestinationInode)
{
	Ext2AddDirectoryEntry(pFS, pCurrentDir, pName, pDestinationInode->m_inodeNumber, FileTypeToExt2TypeHint(pDestinationInode->m_node.m_type));
}

// Creates a new empty inode.
int Ext2CreateFileAndInode(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCurrentDir, const char* pName, uint8_t typeIndicator, uint32_t* pInodeOut)
{
	uint32_t inodeNo = Ext2AllocateInode(pFS);
	if (inodeNo == ~0u) return -ENOSPC;
	
	// Clear the inode. It will have a reference count of zero.
	Ext2InodeCacheUnit fakeUnit;
	memset(&fakeUnit, 0, sizeof fakeUnit);
	
	fakeUnit.m_inodeNumber = inodeNo;
	
	if (pInodeOut)
		*pInodeOut = inodeNo;
	
	// set some default permissions for now
	fakeUnit.m_inode.m_permissions |= E2_PERM_ANYONE_WRITE | E2_PERM_ANYONE_READ;
	
	switch (typeIndicator)
	{
		case E2_DETI_REG_FILE:  fakeUnit.m_inode.m_permissions |= E2_INO_FILE;        break;
		case E2_DETI_DIRECTORY: fakeUnit.m_inode.m_permissions |= E2_INO_DIRECTORY;   break;
		case E2_DETI_CHAR_DEV:  fakeUnit.m_inode.m_permissions |= E2_INO_CHAR_DEV;    break;
		case E2_DETI_BLOCK_DEV: fakeUnit.m_inode.m_permissions |= E2_INO_BLOCK_DEV;   break;
		case E2_DETI_SYMLINK:   fakeUnit.m_inode.m_permissions |= E2_INO_SYM_LINK;    break;
		case E2_DETI_SOCKET:    fakeUnit.m_inode.m_permissions |= E2_INO_UNIX_SOCKET; break;
		case E2_DETI_FIFO:      fakeUnit.m_inode.m_permissions |= E2_INO_FIFO;        break;
		default: typeIndicator = 0;
	}
	
	fakeUnit.m_inode.m_lastAccessTime =
	fakeUnit.m_inode.m_creationTime   =
	fakeUnit.m_inode.m_lastModTime    = GetEpochTime();
	
	Ext2FlushInode(pFS, &fakeUnit);
	
	// Add a directory entry to it.
	Ext2AddDirectoryEntry(pFS, pCurrentDir, pName, inodeNo, typeIndicator);
	
	return -ENOTHING;
}

int Ext2CreateFile(FileNode* pNode, const char* pName)
{
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	
	ASSERT(pUnit->m_inodeNumber == pNode->m_inode);
	
	return Ext2CreateFileAndInode(pFS, pUnit, pName, E2_DETI_REG_FILE, NULL);
}

int Ext2CreateDir(FileNode* pFileNode, const char* pName)
{
	Ext2FileSystem* pFS = (Ext2FileSystem*)pFileNode->m_implData1;
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pFileNode->m_implData;
	
	// Create a file whose type indicator is E2_DETI_DIRECTORY
	
	uint32_t newDirInode = 0;
	int status = Ext2CreateFileAndInode(pFS, pUnit, pName, E2_DETI_DIRECTORY, &newDirInode);
	
	if (status < 0)   return status;
	if (!newDirInode) return -EIO;
	
	// Setup the block buffer. This will be the initial state of the directory -- just two entries, '.' and '..'
	uint8_t buffer [pFS->m_blockSize];
	memset(buffer, 0, pFS->m_blockSize);
	
	// Setup the '.' entry.
	Ext2DirEnt* pDotEntry = (Ext2DirEnt*)buffer;
	
	pDotEntry->m_inode      = newDirInode;
	pDotEntry->m_entrySize  = 12; // 8 bytes header, 4 bytes name (since entries must be aligned to 4 bytes)
	pDotEntry->m_nameLength = 1;  // Just a dot
	pDotEntry->m_typeIndicator = E2_DETI_DIRECTORY;
	pDotEntry->m_name[0] = '.';
	
	// Setup the '..' entry
	Ext2DirEnt* pDotDotEntry = (Ext2DirEnt*)(&buffer[pDotEntry->m_entrySize]);
	
	pDotDotEntry->m_inode      = pFileNode->m_inode;
	pDotDotEntry->m_entrySize  = pFS->m_blockSize - pDotEntry->m_entrySize;
	pDotDotEntry->m_nameLength = 2; // Two dots
	pDotDotEntry->m_typeIndicator = E2_DETI_DIRECTORY;
	memset(pDotDotEntry->m_name, '.', 2);
	
	// Look up the file.
	FileNode* pNewFile = Ext2FindDir(pFileNode, pName);
	if (!pNewFile) return -EIO;
	
	// Write it to the file.
	Ext2FileWrite(pNewFile, 0, pFS->m_blockSize, buffer, true);
	
	// Increase the number of directories in this inode's block group by 1.
	uint32_t bgdIndex = (pFileNode->m_inode - 1) / pFS->m_inodesPerGroup;
	Ext2BlockGroupDescriptor* pDescriptor = &pFS->m_pBlockGroups[bgdIndex];
	pDescriptor->m_nDirs++;
	Ext2FlushBlockGroupDescriptor(pFS, bgdIndex);
	
	// Increase the link count of this directory. It starts out with a link count of 1, because we
	// created it with Ext2CreateFileAndInode, and we need a second one to accomodate the '.' link.
	
	Ext2InodeCacheUnit* pNewUnit = (Ext2InodeCacheUnit*)pNewFile->m_implData;
	pNewUnit->m_inode.m_nLinks++;
	Ext2FlushInode(pFS, pNewUnit);
	
	// Increase the link count of the parent directory, since the '..' entry is just a hard link.
	
	Ext2InodeCacheUnit* pParentUnit = (Ext2InodeCacheUnit*)pFileNode->m_implData;
	pParentUnit->m_inode.m_nLinks++;
	Ext2FlushInode(pFS, pParentUnit);
	
	// Okay, seems like everything went along properly, just return success.
	return -ENOTHING;
}

int Ext2UnlinkFile(FileNode* pNode, const char* pName)
{
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	
	ASSERT(pUnit->m_inodeNumber == pNode->m_inode);
	
	return Ext2RemoveDirectoryEntry(pFS, pUnit, pName, false, false, false, 0, NULL, NULL);
}

void Ext2FileOnUnreferenced(FileNode* pNode)
{
	//SLogMsg("File %s unreferenced", pNode->m_name);
	
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	
	ASSERT(pUnit->m_inodeNumber == pNode->m_inode);
	
	// TODO
	
	// Is this inode about to be deleted?
	if (!pUnit->m_bAboutToBeDeleted) return;
	
	// Yes. Assert that the inode's link count is zero.
	ASSERT(pUnit->m_inode.m_nLinks == 0);
	
	// This will delete the inode.
	pUnit->m_inode.m_deletionTime = GetEpochTime();
	
	// Free all the blocks.
	for (uint32_t i = 0; i < 12; i++)
	{
		if (pUnit->m_inode.m_directBlockPointer[i] == 0) continue;
		
		Ext2FreeBlock(pFS, pUnit->m_inode.m_directBlockPointer[i]);
	}
	
	Ext2FreeIndirectList(pFS, pUnit->m_inode.m_singlyIndirBlockPtr, 0);
	Ext2FreeIndirectList(pFS, pUnit->m_inode.m_doublyIndirBlockPtr, 1);
	Ext2FreeIndirectList(pFS, pUnit->m_inode.m_triplyIndirBlockPtr, 2);
	
	bool bIsDirectory = pUnit->m_node.m_type & FILE_TYPE_DIRECTORY;
	
	Ext2FlushInode(pFS, pUnit);
	
	Ext2FreeInode(pFS, pUnit->m_inodeNumber);
	
	// if it's a directory, also reduce the nDirs count in the local block group
	if (bIsDirectory)
	{
		uint32_t bgd = (pUnit->m_inodeNumber - 1) / pFS->m_inodesPerGroup;
		
		pFS->m_pBlockGroups[bgd].m_nDirs--;
		
		Ext2FlushBlockGroupDescriptor(pFS, bgd);
	}
	
	// delete the inode from the cache unit
	Ext2RemoveInodeCacheUnit(pFS, pUnit->m_inodeNumber);
}

int Ext2RemoveDir(FileNode* pNode)
{
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	Ext2InodeCacheUnit* pUnit = (Ext2InodeCacheUnit*)pNode->m_implData;
	Ext2Inode* pInode = &pUnit->m_inode;
	
	// check if we are empty
	DirEnt de; uint32_t index = 0;
	while (Ext2ReadDir(pNode, &index, &de) != NULL)
	{
		// if it's not '.' or '..'
		if (strcmp(de.m_name, ".") != 0 && strcmp(de.m_name, "..") != 0)
		{
			//well, this directory isn't actually empty
			return -ENOTEMPTY;
		}
	}
	
	// if there is a '..' entry, we can remove ourselves.
	FileNode* pDotDotEntry = Ext2FindDir(pNode, "..");
	if (!pDotDotEntry) return -EBUSY;
	
	// if there's a link across file systems for some reason (e.g. if something's mounted INSIDE of this file system)
	if (pDotDotEntry->m_pFileSystemHandle != pNode->m_pFileSystemHandle)
	{
		FsReleaseReference(pDotDotEntry);
		return -EBUSY;
	}
	
	Ext2InodeCacheUnit* pParentUnit = (Ext2InodeCacheUnit*)pDotDotEntry->m_implData;
	
	// remove the '..' link from ourselves. The '..' link points to the parent.
	pParentUnit->m_inode.m_nLinks--;
	
	// remove the '.' link from ourselves. This simply involves decreasing our link count by one.
	pInode->m_nLinks--;
	
	// remove this entry from the parent. This will decrease our link count by one, if successful.
	int status = Ext2RemoveDirectoryEntry(pFS, pParentUnit, NULL, true, false, true, pNode->m_inode, NULL, NULL);
	if (status < 0)
	{
		// revert the changes
		pParentUnit->m_inode.m_nLinks++;
		pInode->m_nLinks++;
		
		// bail
		FsReleaseReference(pDotDotEntry);
		return status;
	}
	
	// we could remove this entry from the parent. Ideally, our link count should be zero now.
	ASSERT(pInode->m_nLinks == 0);
	
	// flush our changes to the parent unit
	Ext2FlushInode(pFS, pParentUnit);
	
	FsReleaseReference(pDotDotEntry);
	
	// wait until someone else removes our reference.
	return -ENOTHING;
}
