//  ***************************************************************
//  fs/tmp/main.c - Creation date: 14/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <tmpfs.h>
#include <string.h>
#include <memory.h>
#include <time.h>

TempFSNode* FsTempCreateNode(const char* pName, FileNode* pParentDir, bool bDirectory)
{
	TempFSNode* pTFNode = MmAllocate(sizeof(TempFSNode));
	if (!pTFNode) return NULL;
	
	memset(pTFNode, 0, sizeof *pTFNode);
	
	FileNode* pFNode = &pTFNode->m_node;
	pFNode->m_refCount = 0;
	
	// we don't store it in one centralized file system instance. All TMPFS instances are part of one giant hivemind.
	// However, we will store a handle to our own TempFSNode
	pFNode->m_pFileSystemHandle = NULL;
	pFNode->m_pParentSpecific   = pTFNode;
	
	strncpy(pFNode->m_name, pName, sizeof pFNode->m_name);
	pFNode->m_name[sizeof pFNode->m_name - 1] = 0;
	
	pFNode->m_perms  = PERM_READ | PERM_WRITE | PERM_EXEC;
	pFNode->m_flags  = 0;
	pFNode->m_inode  = (int)pFNode;
	pFNode->m_length = 0;
	pFNode->m_implData   = (int)pTFNode;
	pFNode->m_implData1  = 0;
	pFNode->m_implData2  = 0;
	pFNode->m_implData3  = 0;
	pFNode->m_modifyTime = GetEpochTime();
	pFNode->m_createTime = pFNode->m_modifyTime;
	pFNode->m_accessTime = pFNode->m_modifyTime;
	
	pFNode->OnUnreferenced = FsTempFileOnUnreferenced;
	
	if (bDirectory)
	{
		pFNode->m_type = FILE_TYPE_DIRECTORY;
		
		pFNode->OpenDir        = FsTempDirOpen;
		pFNode->CloseDir       = FsTempDirClose;
		pFNode->ReadDir        = FsTempDirRead;
		pFNode->FindDir        = FsTempDirLookup;
		pFNode->CreateFile     = FsTempDirCreate;
		pFNode->CreateDir      = FsTempDirCreateDir;
		pFNode->UnlinkFile     = FsTempDirUnlink;
		pFNode->RemoveDir      = FsTempDirRemoveDir;
		pFNode->RenameOp       = FsTempDirRenameOp;
		
		FsTempDirSetup(pFNode, pParentDir);
	}
	else
	{
		pFNode->m_type = FILE_TYPE_FILE;
		
		pFNode->Open      = FsTempFileOpen;
		pFNode->Close     = FsTempFileClose;
		pFNode->Read      = FsTempFileRead;
		pFNode->Write     = FsTempFileWrite;
		pFNode->EmptyFile = FsTempFileEmpty;
	}
	
	return pTFNode;
}

void FsTempFreeNode(TempFSNode* pNode)
{
	SLogMsg("FsTempFreeNode(%p aka '%s')", pNode, pNode->m_node.m_name);
	
	// if it has any data, free that too.
	// note that we do not allow deleting directories with files in them,
	// so we'll assume the directory is empty
	if (pNode->m_pFileData)
	{
		MmFree(pNode->m_pFileData);
		pNode->m_pFileData      = NULL;
		pNode->m_nFileSizePages = 0;
	}
	
	MmFree(pNode);
}

void FsMountRamDisk(UNUSED void* unused)
{
}

//NOTE: This makes a copy!!
FileNode* FsRootAddArbitraryFileNodeToRoot(const char* pFileName, FileNode* pFileNode);

const char g_TempNodeContents[] = "Hello, world!";

void FsTempFileOnUnreferenced(FileNode* pFileNode)
{
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	FsTempFreeNode(pTFNode);
}

FileNode* g_pRootNode;

FileNode* FsGetRootNode()
{
	return g_pRootNode;
}

void FsTempInit()
{
	TempFSNode* pTFNode = FsTempCreateNode("root", NULL, true);
	FileNode* pFNode = &pTFNode->m_node;
	
	// this will be referenced in the root.
	pFNode->m_refCount = NODE_IS_PERMANENT;
	
	// set it to the root node:
	g_pRootNode = pFNode;
}

void FsInitializeDevicesDir()
{
	g_pRootNode->CreateDir(g_pRootNode, "Device");
}
