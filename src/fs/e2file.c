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

uint32_t Ext2FileRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
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

uint32_t Ext2FileWrite(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
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
	
	SLogMsg("Gotta expand!");
	
	// note: This should be fine, if the directory isn't <multiple of block size> sized then I don't think it's good
	memset(buffer, 0, pFS->m_blockSize);
	
	// Note: We create a new entry here in the hope that it gets used later on.
	Ext2DirEnt* pNewDirEnt = (Ext2DirEnt*)(buffer);
	pNewDirEnt->m_entrySize = pFS->m_blockSize;
	
	// Write it
	ASSERT(Ext2FileWrite(&pUnit->m_node, pUnit->m_inode.m_size, pFS->m_blockSize, buffer));
	
	if (offset < 1)
		offset = 0;
	else
		offset -= 1;
	
	// Try again.
	goto TRY_AGAIN;
}

DirEnt* Ext2ReadDirInternal(FileNode* pNode, uint32_t * index, DirEnt* pOutputDent, bool bSkipDotAndDotDot)
{
	// Read the directory entry, starting at (*index).
	
	uint32_t tempBuffer[2] = { 0 };
	
	if (!Ext2FileRead(pNode, *index, sizeof(uint32_t) + sizeof(uint16_t), tempBuffer)) return NULL;
	
	// The entry length is in tempBuffer[1].
	tempBuffer[1] &= 0xFFFF;
	
	union
	{
		uint8_t EntryData[1024];
		Ext2DirEnt dirEnt;
	} d;
	
	// if it's bigger, chances are it's a Padding entry.
	if (tempBuffer[1] < sizeof d.EntryData)
	{
		if (!Ext2FileRead(pNode, *index, tempBuffer[1], d.EntryData)) return NULL;
	}
	
	(*index) += d.dirEnt.m_entrySize;
	
	if (tempBuffer[1] >= sizeof d.EntryData)
	{
		return NULL;
	}
	
	d.dirEnt.m_name[d.dirEnt.m_nameLength] = 0;
	
	size_t nameLen = d.dirEnt.m_nameLength;
	if (nameLen > sizeof(pOutputDent->m_name) - 2)
		nameLen = sizeof(pOutputDent->m_name) - 2;
	
	memcpy(pOutputDent->m_name, d.dirEnt.m_name, nameLen);
	pOutputDent->m_name[nameLen] = '\0';
	
	if (bSkipDotAndDotDot)
	{
		// If it's a '.' or '..' entry, skip it. We provide our own implementation anyway.
		if (strcmp(pOutputDent->m_name, ".") == 0  ||  strcmp(pOutputDent->m_name, "..") == 0)
			return Ext2ReadDirInternal(pNode, index, pOutputDent, bSkipDotAndDotDot);
	}
	
	pOutputDent->m_inode = d.dirEnt.m_inode;
	
	// convert the type indicator
	switch (d.dirEnt.m_typeIndicator)
	{
		case E2_DETI_REG_FILE:  pOutputDent->m_type = FILE_TYPE_FILE;
		case E2_DETI_CHAR_DEV:  pOutputDent->m_type = FILE_TYPE_CHAR_DEVICE;
		case E2_DETI_BLOCK_DEV: pOutputDent->m_type = FILE_TYPE_BLOCK_DEVICE;
		case E2_DETI_DIRECTORY: pOutputDent->m_type = FILE_TYPE_DIRECTORY;
		default:                pOutputDent->m_type = FILE_TYPE_NONE;
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
