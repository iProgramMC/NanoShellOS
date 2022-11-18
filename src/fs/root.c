/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

       FILE SYSTEM: VFS root module
******************************************/
#include <multiboot.h>
#include <vfs.h>
#include <console.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>
#include <tar.h>
#include <debug.h>
#include <sb.h>

typedef struct RootFileNode
{
	struct RootFileNode* parent;
	struct RootFileNode* children;
	struct RootFileNode* next;
	struct RootFileNode* prev;
	
	FileNode node;
}
RootFileNode;

RootFileNode* RootFileNodeAllocate()
{
	return MmAllocate(sizeof (RootFileNode));
}

void RootFileNodeFree(RootFileNode* pNode)
{
	MmFree(pNode);
}

void RootFileNodeAddChild(RootFileNode* parent, RootFileNode* pNode)
{
	pNode->parent = parent;
	if (parent->children)
	{
		RootFileNode *pLastEnt = parent->children;
		while (pLastEnt->next)
		{
			pLastEnt = pLastEnt->next;
		}
		
		pLastEnt->next = pNode;
		pNode   ->prev = pLastEnt;
	}
	else
	{
		parent->children = pNode;
		pNode->next = pNode->prev = NULL;
	}
}

RootFileNode* RootFileNodeCreate(RootFileNode* parent)
{
	RootFileNode* pNode = RootFileNodeAllocate();
	if (!pNode)
		return NULL;
	
	memset (pNode, 0, sizeof *pNode);
	
	RootFileNodeAddChild(parent, pNode);
	
	return pNode;
}

void RootFileNodeErase(RootFileNode* pFileNode)
{
	// First of all, recursively delete children
	while (pFileNode->children)
	{
		RootFileNodeErase (pFileNode->children);
	}
	
	//If there's a previous node (which may or may not have pFileNode->parent->children
	//set to it), override that one's next value to be our next value.
	if (pFileNode->prev)
	{
		pFileNode->prev->next = pFileNode->next;
	}
	//If we don't have a previous, it's guaranteed that the parent's children value
	//points to us, so give it the next pointer on our list
	else if (pFileNode->parent)
	{
		pFileNode->parent->children = pFileNode->next;
	}
	//If there's a next element on the list, give its previous-pointer our prev-pointer.
	if (pFileNode->next)
	{
		pFileNode->next->prev = pFileNode->prev;
	}
	
	RootFileNodeFree (pFileNode);
}

RootFileNode g_rootNode;

FileNode* FsGetRootNode()
{
	return &g_rootNode.node;
}

DirEnt g_dirEnt;

// File node operations.
#if 1

static DirEnt* FsRootReadDir(FileNode* pFileNode, uint32_t n)
{
	// Get root file node
	RootFileNode* pNode = (RootFileNode*)pFileNode->m_implData;
	
	// Follow the children of this node.
	RootFileNode* pChild = pNode->children;
	
	while (pChild)
	{
		if (n == 0)
		{
			strcpy(g_dirEnt.m_name, pChild->node.m_name);
			g_dirEnt.m_inode = pChild->node.m_inode;
			g_dirEnt.m_type  = pChild->node.m_type;
			return &g_dirEnt;
		}
		else
		{
			n--;
		}
		
		pChild = pChild->next;
	}
	
	return NULL;
}

static FileNode* FsRootFindDir(FileNode* pFileNode, const char* pFileName)
{
	// Get root file node
	RootFileNode* pNode = (RootFileNode*)pFileNode->m_implData;
	
	// Follow the children of this node.
	RootFileNode* pChild = pNode->children;
	
	while (pChild)
	{
		if (strcmp (pChild->node.m_name, pFileName) == 0)
		{
			return &pChild->node;
		}
		
		pChild = pChild->next;
	}
	
	return NULL;
}

static uint32_t FsRootRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	//check lengths
	if (offset > pNode->m_length)
		return 0;
	if (offset + size > pNode->m_length)
		size = pNode->m_length - offset;
	
	//read!
	memcpy (pBuffer, (uint8_t*)pNode->m_inode + offset, size);
	return size;
}

#endif

// Setup Code
#if 1

void FsRootSetupRootNode()
{
	FileNode* pFN = FsGetRootNode();
	strcpy(pFN->m_name, "root");
	pFN->m_type  = FILE_TYPE_DIRECTORY;
	pFN->m_perms = PERM_READ | PERM_WRITE | PERM_EXEC;
	pFN->m_implData = (uint32_t)&g_rootNode;
	pFN->ReadDir  = FsRootReadDir;
	pFN->FindDir  = FsRootFindDir;
}

RootFileNode* FsRootAddFile(RootFileNode* pNode, const char* pFileName, void* pContents, size_t size)
{
	RootFileNode* pNewNode = RootFileNodeCreate(pNode);
	
	FileNode* pFN = &pNewNode->node;
	strcpy(pFN->m_name, pFileName);
	pFN->m_type   = FILE_TYPE_FILE;
	pFN->m_perms  = PERM_READ;
	pFN->m_length = size;
	pFN->m_inode  = (uint32_t)pContents;
	pFN->m_implData = (uint32_t)pNewNode;
	pFN->Read     = FsRootRead;
	
	return pNewNode;
}

RootFileNode* FsRootAddDirectory(RootFileNode* pNode, const char* pFileName)
{
	RootFileNode* pNewNode = RootFileNodeCreate(pNode);
	
	FileNode* pFN = &pNewNode->node;
	strcpy(pFN->m_name, pFileName);
	pFN->m_type   = FILE_TYPE_DIRECTORY | FILE_TYPE_FILE;
	pFN->m_perms  = PERM_READ | PERM_WRITE | PERM_EXEC;
	pFN->m_length = 0;
	pFN->m_inode  = 0;
	pFN->m_implData = (uint32_t)pNewNode;
	pFN->ReadDir  = FsRootReadDir;
	pFN->FindDir  = FsRootFindDir;
	
	return pNewNode;
}

void FsRootCreateFileAtRoot(const char* pFileName, void* pContents, size_t sz)
{
	FsRootAddFile(&g_rootNode, pFileName, pContents, sz);
}

void FsRootCreateDirAtRoot(const char* pFileName)
{
	FsRootAddDirectory(&g_rootNode, pFileName);
}

void FsRootCreateFileAt(const char* dirPath, const char* name, void* pContents, size_t sz)
{
	FileNode* pFNode = FsResolvePath(dirPath);
	
	if (!pFNode)
	{
		LogMsg("FsRootCreateFileAt(\"%s\", \"%s\") failed!", dirPath, name);
		return;
	}
	
	RootFileNode* pRFN = (RootFileNode*)pFNode->m_implData;
	FsRootAddFile(pRFN, name, pContents, sz);
}

void FsRootCreateDirAt(const char* dirPath, const char* name)
{
	FileNode* pFNode = FsResolvePath(dirPath);
	
	if (!pFNode)
	{
		LogMsg("FsRootCreateFileAt(\"%s\", \"%s\") failed!", dirPath, name);
		return;
	}
	
	RootFileNode* pRFN = (RootFileNode*)pFNode->m_implData;
	FsRootAddDirectory(pRFN, name);
}

void FsRootInit()
{
	FsRootSetupRootNode();
	
	FsRootAddFile(&g_rootNode, "test1.txt", "Hello!", 6);
	FsRootAddFile(&g_rootNode, "test2.txt", "Hello! This is a longer file.", 29);
	FsRootAddFile(&g_rootNode, "ns.ini", "[Launcher]\n\tConfigPath=/lc.txt\n\tConfigPathReserve=/lc.txt", 57);
	FsRootAddFile(&g_rootNode, "lc.txt", "version|2\nadd_item|1|1|Cab|shell:cabinet\nadd_item|96|1|Ed|shell:notepad\nadd_item|121|1|Mon|shell:sysmon", 103);
	
	FsRootAddDirectory(&g_rootNode, "Device");
	FsRootAddDirectory(&g_rootNode, "Directory");
	FsRootCreateFileAt("/Device", "Nothing.txt", "Nothing is here!", 16);
	FsRootCreateFileAt("/Directory", "Something.txt", "Something is here!", 18);
}

void FsInitializeDevicesDir()
{
	
}

#endif
