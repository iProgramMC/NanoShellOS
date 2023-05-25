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

TempFSNode* FsTempCreateNode(FileNode* pParentDir, bool bDirectory)
{
	TempFSNode* pTFNode = MmAllocate(sizeof(TempFSNode));
	if (!pTFNode) return NULL;
	
	memset(pTFNode, 0, sizeof *pTFNode);
	pTFNode->m_bMutable = true;
	
	FileNode* pFNode = &pTFNode->m_node;
	pFNode->m_refCount = 0;
	
	// we don't store it in one centralized file system instance. All TMPFS instances are part of one giant hivemind.
	// However, we will store a handle to our own TempFSNode
	pFNode->m_pFileSystemHandle = NULL;
	pFNode->m_pParentSpecific   = pTFNode;
	
	pFNode->m_perms  = PERM_READ | PERM_WRITE | PERM_EXEC;
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
		pFNode->m_bHasDirCallbacks = true;
		
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
		pFNode->m_bHasDirCallbacks = false;
		
		pFNode->Open      = FsTempFileOpen;
		pFNode->Close     = FsTempFileClose;
		pFNode->Read      = FsTempFileRead;
		pFNode->Write     = FsTempFileWrite;
		pFNode->EmptyFile = FsTempFileEmpty;
	}
	
	return pTFNode;
}

int FsTempDirAddEntry(FileNode* pFileNode, FileNode* pChildNode, const char* pName);

void FsTempCreatePermanentFile(FileNode* pNode, const char* fileName, uint8_t* buffer, uint32_t fileSize)
{
	TempFSNode* pTFNode = FsTempCreateNode(pNode, false);
	pTFNode->m_bMutable = false;
	pTFNode->m_node.m_refCount = NODE_IS_PERMANENT;
	pTFNode->m_node.m_length = fileSize;
	pTFNode->m_pFileData = buffer;
	pTFNode->m_nFileSizePages = (fileSize + 4095) / 4096;
	
	int res = FsTempDirAddEntry(pNode, &pTFNode->m_node, fileName);
	if (res < 0)
	{
		SLogMsg("Couldn't create file '%s'", fileName);
		pTFNode->m_node.m_refCount = 1;
		FsReleaseReference(&pTFNode->m_node);
		return;
	}
}

void FsTempFreeNode(TempFSNode* pNode)
{
	SLogMsg("FsTempFreeNode(%p)", pNode);
	
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
	TempFSNode* pTFNode = FsTempCreateNode(NULL, true);
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

