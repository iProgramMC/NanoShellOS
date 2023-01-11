//  ***************************************************************
//  fatinode.c - Creation date: 06/01/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
//  Module description:
//
//      This module contains FAT32 file system code, which deals
//  with inodes (actually cluster chain + dir entry combos).
//  ***************************************************************
#include <vfs.h>
#include <fat.h>

// File operations. TODO

// *************************
//   Section : Inode Cache
// *************************

Fat32InodeCacheUnit* FatAddInodeToCache(Fat32FileSystem* pFS, uint32_t inodeNum, Fat32DirEntry* pDirEntry, uint32_t parDirIno, uint32_t parDirOfs, const char* lfnName)
{
	// Create a new inode cache unit:
	Fat32InodeCacheUnit* pUnit = MmAllocate(sizeof(Fat32InodeCacheUnit));
	memset(pUnit, 0, sizeof(Fat32InodeCacheUnit));
	
	// Set its inode number.
	pUnit->m_inodeNumber = inodeNum;
	pUnit->m_dirEntry = *pDirEntry;
	pUnit->m_parentDirInode  = parDirIno;
	pUnit->m_parentDirOffset = parDirOfs;
	
	pUnit->m_node.m_implData   = (uint32_t)pUnit; // for faster lookup
	pUnit->m_node.m_implData1 = (uint32_t)pFS;
	pUnit->m_node.m_pFileSystemHandle = pFS;
	
	//FatInodeToFileNode(&pUnit->m_node, 
	
	return pUnit;
}

Fat32InodeCacheUnit* FatReadInode(Fat32FileSystem* pFS, uint32_t inodeNum, Fat32DirEntry* pDirEntry, uint32_t parDirIno, uint32_t parDirOfs, const char* lfnName, bool bForceReload)
{
	if (!bForceReload)
	{
		Fat32InodeCacheUnit* pUnit = FatLookUpInodeCacheUnit(pFS, inodeNum);
		if (pUnit)
			return pUnit;
	}
	
	return FatAddInodeToCache(pFS, inodeNum, pDirEntry, parDirIno, parDirOfs, lfnName);
}