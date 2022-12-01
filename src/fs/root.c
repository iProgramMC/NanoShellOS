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

// note: here, we don't actually use FsReleaseReference, because everything is permanent anyway

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

static DirEnt* FsRootReadDir(FileNode* pFileNode, uint32_t * nIndex, DirEnt* pOutputDent)
{
	// Get root file node
	RootFileNode* pNode = (RootFileNode*)pFileNode->m_implData;
	
	// Follow the children of this node.
	RootFileNode* pChild = pNode->children;
	
	int n = *nIndex;
	
	while (pChild)
	{
		if (n == 0)
		{
			strcpy(pOutputDent->m_name, pChild->node.m_name);
			pOutputDent->m_inode = pChild->node.m_inode;
			pOutputDent->m_type  = pChild->node.m_type;
			
			++(*nIndex);
			
			return pOutputDent;
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
	pFN->m_refCount = NODE_IS_PERMANENT;
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
	pFN->m_refCount = NODE_IS_PERMANENT;
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
	pFN->m_refCount = NODE_IS_PERMANENT;
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

//NOTE: This makes a copy!!
void FsRootAddArbitraryFileNodeToRoot(const char* pFileName, FileNode* pFileNode)
{
	RootFileNode* pFN = FsRootAddFile(&g_rootNode, pFileName, NULL, 0);
	
	pFN->node = *pFileNode;
}

void FsRootCreateDirAtRoot(const char* pFileName)
{
	FsRootAddDirectory(&g_rootNode, pFileName);
}

void FsRootCreateFileAt(const char* path, void* pContents, size_t sz)
{
	char buffer[PATH_MAX+1];
	
	if (strlen(path) >= PATH_MAX) return;
	strcpy(buffer, path);
	
	char* name = strrchr(buffer, '/');
	if (name == NULL)
	{
		SLogMsg("FsRootCreateFileAt(\"%s\"): strrchr(buffer, '/') returned NULL!", path);
		return;
	}
	*name = 0;
	name++;
	
	FileNode* pFNode = FsResolvePath(buffer);
	
	if (!pFNode)
	{
		LogMsg("FsRootCreateFileAt(\"%s\") failed!", path);
		return;
	}
	
	RootFileNode* pRFN = (RootFileNode*)pFNode->m_implData;
	FsRootAddFile(pRFN, name, pContents, sz);
}

void FsRootCreateDirAt(const char* path)
{
	if (FsResolvePath(path))
		return;
	
	char buffer[PATH_MAX+1];
	
	if (strlen(path) >= PATH_MAX) return;
	strcpy(buffer, path);
	
	char* name = strrchr(buffer, '/');
	if (name == NULL)
	{
		SLogMsg("FsRootCreateDirAt(\"%s\"): strrchr(buffer, '/') returned NULL!", path);
		return;
	}
	*name = 0;
	name++;
	
	FileNode* pFNode = FsResolvePath(buffer);
	
	if (!pFNode)
	{
		LogMsg("FsRootCreateFileAt(\"%s\") failed!", path);
		return;
	}
	
	RootFileNode* pRFN = (RootFileNode*)pFNode->m_implData;
	FsRootAddDirectory(pRFN, name);
}

void FsRootInit()
{
	FsRootSetupRootNode();
	
	FsRootCreateDirAt("/Device");
	FsRootCreateFileAt("/Device/help.txt", "This directory contains all the device 'files' that NanoShell has loaded.", 73);
}

static uint32_t OctToBin(char *data, uint32_t size)
{
	uint32_t value = 0;
	while (size > 0)
	{
		size--;
		value *= 8;
		value += *data++ - '0';
	}
	return value;
}

void FsInitializeInitRd(void* pRamDisk)
{	
	// Add files to the root FS.
	for (Tar *ramDiskTar = (Tar *) pRamDisk; !memcmp(ramDiskTar->ustar, "ustar", 5);)
	{
		uint32_t fileSize = OctToBin(ramDiskTar->size, 11);
		uint32_t pathLength = strlen(ramDiskTar->name);
		bool hasDotSlash = false;

		if (ramDiskTar->name[0] == '.')
		{
			pathLength -= 1;
			hasDotSlash = true;
		}

		if (pathLength > 0)
		{
			char *pname = ramDiskTar->name + hasDotSlash;
			char buffer[PATH_MAX];
			
			if (strlen(pname) >= sizeof buffer) goto _skip;
			
			strcpy(buffer, pname);
			
			// For each slash character in the buffer:
			int lastRemovedSlashIndex = -1;
			for (int i = 0; buffer[i] != 0; i++)
			{
				if (buffer[i] == '/')
				{
					// try removing it...
					buffer[i] = 0;
					
					lastRemovedSlashIndex = i;
					
					// add the directory, if there is one
					if (buffer[0] != 0)
					{
						FsRootCreateDirAt(buffer);
					}
					
					// restore it
					buffer[i] = '/';
				}
			}
			
			if (buffer[0] != 0)
			{
				//note: this should be safe anyway even if lastRemovedSlashIndex == -1, as it
				//just gets neutralized to zero.
				if (buffer[lastRemovedSlashIndex + 1] != 0)
				{
					FsRootCreateFileAt(buffer, &ramDiskTar->buffer, fileSize);
				}
			}
		}
	_skip:

		// Advance to the next entry
		ramDiskTar = (Tar *) ((uintptr_t) ramDiskTar + ((fileSize + 511) / 512 + 1) * 512);
	}
	SLogMsg("Init ramdisk is fully setup!");
}

void FsInitRdInit()
{
	// Initialize the ramdisk
	multiboot_info_t* pInfo = KiGetMultibootInfo();
	if (pInfo->mods_count != 1)
		KeBugCheck(BC_EX_INITRD_MISSING, NULL);

	//Usually the mods table is below 1m.
	if (pInfo->mods_addr >= 0x100000)
	{
		LogMsg("Module table starts at %x.  OS state not supported", pInfo->mods_addr);
		KeStopSystem();
	}
	//The initrd module is here.
	multiboot_module_t *initRdModule = (void*) (pInfo->mods_addr + 0xc0000000);

	//Precalculate an address we can use
	void* pInitrdAddress = (void*)(0xc0000000 + initRdModule->mod_start);
	
	// We should no longer have the problem of it hitting our frame bitset.
	
	SLogMsg("Init Ramdisk module Start address: %x, End address: %x", initRdModule->mod_start, initRdModule->mod_end);
	//If the end address went beyond 1 MB:
	if (initRdModule->mod_end >= 0x100000)
	{
		//Actually go to the effort of mapping the initrd to be used.
		pInitrdAddress = MmMapPhysicalMemory (initRdModule->mod_start, initRdModule->mod_end);
	}
	
	SLogMsg("Physical address that we should load from: %x", pInitrdAddress);
	
	// Load the initrd last so its entries show up last when we type 'ls'.
	FsInitializeInitRd(pInitrdAddress);
}

void FsInitializeDevicesDir()
{
	
}

#endif
