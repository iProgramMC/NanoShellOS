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
	
	// read!
	Ext2ReadFileSegment(pFS, pInode, offset, size, pBuffer);
	return size;
}

uint32_t Ext2FileWrite(UNUSED FileNode* pNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	// TODO
	
	return 0;
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

DirEnt* Ext2ReadDir(FileNode* pNode, uint32_t * index, DirEnt* pOutputDent)
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
	
	ASSERT(tempBuffer[1] < sizeof d.EntryData);
	
	if (!Ext2FileRead(pNode, *index, tempBuffer[1], d.EntryData)) return NULL;
	
	(*index) += d.dirEnt.m_entrySize;
	
	size_t nameLen = d.dirEnt.m_nameLength;
	if (nameLen > sizeof(pOutputDent->m_name) - 2)
		nameLen = sizeof(pOutputDent->m_name) - 2;
	
	memcpy(pOutputDent->m_name, d.dirEnt.m_name, nameLen);
	pOutputDent->m_name[nameLen] = '\0';
	
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

FileNode* Ext2FindDir(FileNode* pNode, const char* pName)
{
	Ext2FileSystem* pFS = (Ext2FileSystem*)pNode->m_implData1;
	DirEnt space; uint32_t index = 0;
	
	while (Ext2ReadDir(pNode, &index, &space) != NULL)
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
