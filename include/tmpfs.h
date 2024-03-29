//  ***************************************************************
//  tmpfs.h - Creation date: 14/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#ifndef _TMPFS_H
#define _TMPFS_H

#include <vfs.h>

#define C_MAX_TMPFS_FILENAME (128 - 4) // 124, to fit the entire dent structure into 128 bytes.

struct TempFSNode; // forward declare for the DirectoryEntry

typedef struct
{
	// the pointer to the inode
	struct TempFSNode* m_pNode;
	
	// the file name
	char m_filename[C_MAX_TMPFS_FILENAME];
}
TempFSDirEntry;

typedef struct TempFSNode
{
	// The vnode representing the file.
	// Here, the reference count serves a dual purpose.
	// 1. Lets us know how many directory entries reference the file.
	// 2. 
	FileNode m_node;
	
	// The data in question. The actual size of the file is stored in the vnode.
	union
	{
		uint8_t        *m_pFileData;
		TempFSDirEntry *m_pDirEntries;
	};
	
	// The file's size, in pages.
	size_t   m_nFileSizePages;
	
	// If this file is mutable.
	bool     m_bMutable;
}
TempFSNode;

typedef struct
{
	TempFSNode* m_pRootNode;
}
TempFSInstance;

void FsTempInit();

TempFSNode* FsTempCreateNode(FileNode* pParentDir, bool bDirectory);
void FsTempFreeNode(TempFSNode* pNode);
void FsTempFileOnUnreferenced(FileNode* pFileNode);

// File Ops
void FsTempFileShrink(FileNode* pFileNode, uint32_t newSize);
int FsTempFileRead(FileNode* pFileNode, uint32_t offset, uint32_t size, void* pBuffer, bool bBlock);
int FsTempFileWrite(FileNode* pFileNode, uint32_t offset, uint32_t size, const void* pBuffer, bool bBlock);
int FsTempFileOpen(FileNode* pFileNode, bool read, bool write);
int FsTempFileClose(FileNode* pFileNode);
int FsTempFileEmpty(FileNode* pFileNode);
int FsTempFileChangeMode(FileNode* pFileNode, int mode);
int FsTempFileChangeTime(FileNode* pFileNode, int atime, int mtime);

// Directory Ops
int FsTempDirOpen   (FileNode* pFileNode);
int FsTempDirClose  (FileNode* pFileNode);
int FsTempDirRead(FileNode* pFileNode, uint32_t* pOffset, DirEnt* pDirEnt);
int FsTempDirLookup(FileNode* pFileNode, const char* pName, FileNode** pFileNdoe);
int FsTempDirCreate   (FileNode* pFileNode, const char* pName);
int FsTempDirCreateDir(FileNode* pFileNode, const char* pName);
int FsTempDirUnlink   (FileNode* pFileNode, const char* pName);
int FsTempDirRemoveDir(FileNode* pFileNode);
int FsTempDirRenameOp (FileNode* pFileNodeSrc, FileNode* pFileNodeDst, const char* pFileNameSrc, const char* pFileNameDst);
void FsTempDirSetup   (FileNode* pFileNode, FileNode* pParentNode);

#endif//_TMPFS_H