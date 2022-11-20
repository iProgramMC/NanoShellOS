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

// File operations.
uint32_t Ext2FileRead (FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer);
uint32_t Ext2FileWrite(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer);

DirEnt   *Ext2ReadDir(FileNode* pNode, uint32_t * index, DirEnt* pOutputDent);
FileNode *Ext2FindDir(FileNode* pNode, const char* pName);

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
	
	pFileNode->m_length     = pInode->m_size;
	
	// TODO: Read() and Write() calls if this is file.
	// TODO: ReadDir() and other calls if this is a directory.
	if (pFileNode->m_type == FILE_TYPE_DIRECTORY)
	{
		pFileNode->ReadDir = Ext2ReadDir;
		pFileNode->FindDir = Ext2FindDir;
	}
	else
	{
		pFileNode->Read  = Ext2FileRead;
		pFileNode->Write = Ext2FileWrite;
	}
}

void Ext2AddInodeToCacheRecursive(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pCurrUnit, Ext2InodeCacheUnit* pUnitNew)
{
	if (pCurrUnit->m_inodeNumber == pUnitNew->m_inodeNumber)
	{
		SLogMsg("NOTE: Inode %d is already cached.", pUnitNew->m_inodeNumber);
		
		// copy the data anyways
		pCurrUnit->m_inode = pUnitNew->m_inode;
		pCurrUnit->m_node  = pUnitNew->m_node;
		
		// free the new unit.
		MmFree(pUnitNew);
		
		return;
	}
	
	if (pCurrUnit->m_inodeNumber < pUnitNew->m_inodeNumber)
	{
		// Add to the right. If there's nothing, put it right there.
		if (!pCurrUnit->pRight)
		{
			pCurrUnit->pRight = pUnitNew;
			pUnitNew->pParent = pCurrUnit;
		}
		else
		{
			Ext2AddInodeToCacheRecursive(pFS, pCurrUnit->pRight, pUnitNew);
		}
	}
	else
	{
		// Add to the left. If there's nothing, put it right there.
		if (!pCurrUnit->pLeft)
		{
			pCurrUnit->pLeft = pUnitNew;
			pUnitNew->pParent = pCurrUnit;
		}
		else
		{
			Ext2AddInodeToCacheRecursive(pFS, pCurrUnit->pLeft, pUnitNew);
		}
	}
}

// Adds an inode to the binary search tree.
Ext2InodeCacheUnit* Ext2AddInodeToCache(Ext2FileSystem* pFS, uint32_t inodeNo, Ext2Inode* pInode, const char* pName)
{
	// Create a new inode cache unit:
	Ext2InodeCacheUnit* pUnit = MmAllocate(sizeof(Ext2InodeCacheUnit));
	memset(pUnit, 0, sizeof(Ext2InodeCacheUnit));
	
	// Set its inode number.
	pUnit->m_inodeNumber = inodeNo;
	pUnit->m_inode = *pInode; // copy the inode's data.
	
	pUnit->m_node.m_implData  = (uint32_t)pUnit; // for faster lookup
	pUnit->m_node.m_implData1 = (uint32_t)pFS;
	
	Ext2InodeToFileNode(&pUnit->m_node, pInode, inodeNo, pName);
	
	// Trivial case: the root is empty.
	if (!pFS->m_pInodeCacheRoot)
	{
		// Setting the left, right and parent pointers to null is already taken care of by the memset.
		// Update the root.
		pFS->m_pInodeCacheRoot = pUnit;
	}
	else
	{
		// Binary search a place where to put it.
		Ext2AddInodeToCacheRecursive(pFS, pFS->m_pInodeCacheRoot, pUnit);
	}
	
	return pUnit;
}

Ext2InodeCacheUnit* Ext2LookUpInodeCacheUnitRecursive(Ext2InodeCacheUnit* pCurrentUnit, uint32_t inodeNo)
{
	if (!pCurrentUnit) return NULL;
	
	if (pCurrentUnit->m_inodeNumber == inodeNo) return pCurrentUnit;
	
	// Look at the children.
	if (pCurrentUnit->m_inodeNumber < inodeNo)
	{
		// Right.
		return Ext2LookUpInodeCacheUnitRecursive(pCurrentUnit->pRight, inodeNo);
	}
	else
	{
		// Left.
		return Ext2LookUpInodeCacheUnitRecursive(pCurrentUnit->pLeft, inodeNo);
	}
}

Ext2InodeCacheUnit* Ext2LookUpInodeCacheUnit(Ext2FileSystem* pFS, uint32_t inodeNo)
{
	return Ext2LookUpInodeCacheUnitRecursive(pFS->m_pInodeCacheRoot, inodeNo);
}

void Ext2DumpInodeCacheTreeRec(Ext2InodeCacheUnit* pNode, int numDashes)
{
	if (!pNode) return;
	
	for (int i = 0; i < numDashes; i++) SLogMsgNoCr("-");
	SLogMsg("%d", pNode->m_inodeNumber);
	
	Ext2DumpInodeCacheTreeRec(pNode->pLeft,  1);
	Ext2DumpInodeCacheTreeRec(pNode->pRight, 1);
}

void Ext2DumpInodeCacheTree(Ext2FileSystem* pFS)
{
	Ext2DumpInodeCacheTreeRec(pFS->m_pInodeCacheRoot, 0);
}

/*
void Ext2InodeCacheUnitShiftNodes(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pUnit)
{
	
}
*/

void Ext2DeleteInodeCacheUnit(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pUnit)
{
	/*if (pUnit->pLeft == NULL)
	{
		Ext2InodeCacheUnitShiftNodes(pFS, pUnit, pUnit->pRight);
		return;
	}
	if (pUnit->pRight == NULL)
	{
		Ext2InodeCacheUnitShiftNodes(pFS, pUnit, pUnit->pLeft);
		return;
	}*/
	
	// TODO: Won't bother for now.
}

void Ext2DeleteInodeCacheLeaf(Ext2InodeCacheUnit* pUnit)
{
	if (!pUnit) return;
	
	// delete the left
	Ext2DeleteInodeCacheLeaf(pUnit->pLeft);
	
	// delete the right
	Ext2DeleteInodeCacheLeaf(pUnit->pRight);
	
	// delete ourselves
	MmFree(pUnit);
}

void Ext2DeleteInodeCacheTree(Ext2FileSystem* pFS)
{
	Ext2DeleteInodeCacheLeaf(pFS->m_pInodeCacheRoot);
	pFS->m_pInodeCacheRoot = NULL;
}
