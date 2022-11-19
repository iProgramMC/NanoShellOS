//  ***************************************************************
//  e2inode.c - Creation date: 18/11/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <vfs.h>
#include <ext2.h>
#include <fat.h> // need this for the MasterBootRecord



void Ext2InodeToFileNode(FileNode* pFileNode, Ext2Inode* pInode, uint32_t inodeNo, const char* pName)
{
	strcpy(pFileNode->m_name, pName);
	
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
	}
	
	// NanoShell has no concept of a 'user'.. If anyone can write/read/exec, we can do that. :)
	if (pInode->m_permissions & E2_PERM_ANYONE_WRITE) pFileNode->m_perms |= PERM_WRITE;
	if (pInode->m_permissions & E2_PERM_ANYONE_READ)  pFileNode->m_perms |= PERM_READ;
	if (pInode->m_permissions & E2_PERM_ANYONE_EXEC)  pFileNode->m_perms |= PERM_EXEC;
	
	// Set the modify and create time.
	pFileNode->m_modifyTime = pInode->m_lastModTime;
	pFileNode->m_createTime = pInode->m_creationTime;
	
	// TODO: Read() and Write() calls if this is file.
	// TODO: ReadDir() and other calls if this is a directory.
}

// Adds an inode to the binary search tree.
void Ext2AddInodeToCache(Ext2FileSystem* pFS, uint32_t inodeNo, Ext2Inode* pInode, const char* pName)
{
	// Trivial case: the root is empty.
	if (!pFS->m_pInodeCacheRoot)
	{
		// Create a new inode cache unit:
		Ext2InodeCacheUnit* pUnit = MmAllocate(sizeof(Ext2InodeCacheUnit));
		memset(pUnit, 0, sizeof(Ext2InodeCacheUnit));
		
		// Set its inode number.
		pUnit->m_inodeNumber = inodeNo;
		
		// Setting the left, right and parent pointers to null is already taken care of by the memset.
		Ext2InodeToFileNode(&pUnit->m_node, pInode, inodeNo, pName);
	}
	
	
}
