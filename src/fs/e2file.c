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

uint32_t Ext2FileWrite(UNUSED FileNode* pNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
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

bool Ext2FileOpen(UNUSED FileNode* pNode)
{
	//all good
	return true;
}

void Ext2FileClose(UNUSED FileNode* pNode)
{
	//all good
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
