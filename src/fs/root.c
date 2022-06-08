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

/*
// Misc device files
#if 1

static uint32_t FsNullRead(UNUSED FileNode* pNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer)
{
	memset (pBuffer, 0, size);
	return size;
}

static uint32_t FsNullWrite(UNUSED FileNode* pNode, UNUSED uint32_t offset, uint32_t size, UNUSED void* pBuffer)
{
	return size;
}

static uint32_t FsRandomRead(UNUSED FileNode* pNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer)
{
	char* pText = (char*)pBuffer;
	for (uint32_t i = 0; i < size; i++)
	{
		char out = (char)GetRandom() ^ 0x4E;
		*(pText++) = out;
	}
	return size;
}

static uint32_t FsRandomWrite(UNUSED FileNode* pNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	return 0;
}

static Console *s_InodeToConsole[] = {
	NULL,
	&g_debugConsole,
	&g_debugSerialConsole,
};

enum {
	DEVICE_NULL,
	DEVICE_DEBUG_CONSOLE,
	DEVICE_DEBUG_E9_CONSOLE,
};

static uint32_t FsTeletypeRead(UNUSED FileNode* pNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	return 0; // Can't read anything!
}

static uint32_t FsTeletypeWrite(FileNode* pNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer)
{
	const char* pText = (const char*)pBuffer;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!s_InodeToConsole[pNode->m_inode]) return 0;
		CoPrintChar (s_InodeToConsole[pNode->m_inode], *(pText++));
	}
	return size;
}*/


//#endif

//#if 1
/*static DirEnt g_DirEnt; 

static uint32_t FsRootFsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	//check lengths
	if (offset > pNode->m_implData)
		return 0;
	if (offset + size > pNode->m_implData)
		size = pNode->m_implData - offset;
	
	//read!
	memcpy (pBuffer, (uint8_t*)pNode->m_implData1 + offset, size);
	return size;
}
static DirEnt* FsRootFsReadDir (FileNode *pDirNode, uint32_t index)
{
	FileNode *pNode = pDirNode->children;
	//TODO: Optimize this, if you have consecutive indices
	uint32_t id = 0;
	while (pNode && id < index)
	{
		pNode = pNode->next;
		id++;
	}
	if (!pNode) return NULL;
	strcpy(g_DirEnt.m_name, pNode->m_name);
	g_DirEnt.m_inode      = pNode->m_inode;
	g_DirEnt.m_type       = pNode->m_type;
	return &g_DirEnt;
}
static FileNode* FsRootFsFindDir(FileNode* pDirNode, const char* pName)
{
	FileNode *pNode = pDirNode->children;
	while (pNode)
	{
		if (strcmp (pNode->m_name, pName) == 0)
			return pNode;
		pNode = pNode->next;
	}
	return NULL;
}
static DirEnt* FsDevFsReadDir (FileNode *pDirNode, uint32_t index)
{
	FileNode *pNode = pDirNode->children;
	//TODO: Optimize this, if you have consecutive indices
	uint32_t id = 0;
	while (pNode && id < index)
	{
		pNode = pNode->next;
		id++;
	}
	if (!pNode) return NULL;
	strcpy(g_DirEnt.m_name, pNode->m_name);
	g_DirEnt.m_inode      = pNode->m_inode;
	g_DirEnt.m_type       = pNode->m_type;
	return &g_DirEnt;
}
static FileNode* FsDevFsFindDir(FileNode* pDirNode, const char* pName)
{
	FileNode *pNode = pDirNode->children;
	while (pNode)
	{
		if (strcmp (pNode->m_name, pName) == 0)
			return pNode;
		pNode = pNode->next;
	}
	return NULL;
}*/

void FsInitializeDevicesDir()
{
	/*
	// Add device directory
	FileNode *pDevDir = CreateFileNode(FsGetRootNode());
	pDevDir->m_length = 0;
	pDevDir->m_inode  = 0;
	pDevDir->m_type   = FILE_TYPE_DIRECTORY | FILE_TYPE_FILE;
	pDevDir->m_perms  = PERM_READ | PERM_WRITE;
	pDevDir->ReadDir  = FsDevFsReadDir;
	pDevDir->FindDir  = FsDevFsFindDir;
	strcpy (pDevDir->m_name, "Device");
	
#define DEFINE_DEVICE(name, read, write, inode) do {\
	FileNode *pNode = CreateFileNode (pDevDir);\
	pNode->m_length = 0;\
	pNode->m_inode  = inode;\
	pNode->m_type   = FILE_TYPE_CHAR_DEVICE;\
	pNode->m_perms  = PERM_READ|PERM_WRITE;\
	strcpy (pNode->m_name, name);\
	pNode->Read  = read;\
	pNode->Write = write;\
	pNode->Open  = NULL;\
	pNode->Close = NULL;\
	pNode->ReadDir = NULL;\
	pNode->FindDir = NULL;\
	pNode->OpenDir = NULL;\
	pNode->CloseDir= NULL;\
	pNode->CreateFile = NULL;\
	pNode->EmptyFile  = NULL;\
} while (0)
	
	#include "vfs_dev_defs.h"
	
#undef DEFINE_DEVICE
*/
}
/*
FileNode* FsResolvePathCreateDirs (const char* pPath)
{
	char path_copy[PATH_MAX]; //max path
	if (strlen (pPath) >= PATH_MAX-1) return NULL;
	strcpy (path_copy, pPath);
	
	TokenState state;
	state.m_bInitted = 0;
	char* initial_filename = Tokenize (&state, path_copy, "/");
	if (!initial_filename) return NULL;
	
	//is it just an empty string? if yes, we're using
	//an absolute path.  Otherwise we gotta append the CWD 
	//and run this function again.
	if (*initial_filename == 0)
	{
		FileNode *pNode = FsGetRootNode ();//TODO
		while (true)
		{
			char* path = Tokenize (&state, NULL, "/");
			
			//are we done?
			if (path && *path)
			{
				//nope, resolve pNode again.
				FileNode *pOldNode = pNode;
				pNode = FsFindDir (pOldNode, path);
				if (!pNode)
				{
					// Try creating it.
					pNode = CreateFileNode (pOldNode);
					
					if (!pNode) return NULL;
					
					// Initialize stuff
					
					strcpy (pNode->m_name, path);
					pNode->m_inode = 0;
					pNode->m_type  = FILE_TYPE_DIRECTORY | FILE_TYPE_FILE;
					pNode->m_implData  = 0;
					pNode->m_implData1 = 0;
					pNode->ReadDir = FsRootFsReadDir;
					pNode->FindDir = FsRootFsFindDir;
					pNode->m_perms = PERM_READ | PERM_EXEC;
					
				}
			}
			else
			{
				return pNode;
			}
		}
	}
	else
	{
		//TODO
		//LogMsg("Not an absolute path");
		return NULL;
	}
}
*/
void FsInitializeInitRd(void* pRamDisk)
{	
	// Add files to the ramdisk.
	
	int i = 0;
	
	/*FileNode *pRoot = FsGetRootNode();
	
	pRoot->ReadDir = FsRootFsReadDir;
	pRoot->FindDir = FsRootFsFindDir;
	*/
	SLogMsg("Adding Files...");
	for (Tar *ramDiskTar = (Tar *) pRamDisk; !memcmp(ramDiskTar->ustar, "ustar", 5);)
	{
		SLogMsg("Adding file %s...", ramDiskTar->name);
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
			char directory_name[256], file_name[256], full_file_name[512];
			
			char *pname = ramDiskTar->name + hasDotSlash;
			strcpy (full_file_name, pname);
			pname = full_file_name;
			
			// While we have slashes, proceed
			char *pname2 = pname; bool set = false;
			do
			{
				char *ptr = strchr (pname2 + set, '/');
				if (ptr)
				{
					pname2 = ptr; set = true;
				}
				else break;
			}
			while (true);
			
			if (set)
				*(pname2++) = 0;
			strcpy (directory_name, full_file_name);
			strcpy (file_name,      pname2);
			
			/*
			FileNode *pNewNode = NULL;
			
			if (directory_name[0] == 0)
			{
				if (file_name[0] != 0) // Non-empty file?
				{
					pNewNode = CreateFileNode (pRoot);
				}
			}
			else
			{
				FileNode *pDir = FsResolvePathCreateDirs (directory_name);
				if (!pDir)
				{
					LogMsg("Cannot find directory %s", directory_name);
					KeBugCheck (BC_EX_FILE_SYSTEM, NULL);
				}
				
				if (file_name[0] != 0) // Non-empty file?
				{
					pNewNode = CreateFileNode (pDir);
				}
			}
			
			if (file_name[0] != 0) // Non-empty file?
			{
				if (!pNewNode)
				{
					LogMsg("Unable to create file entry %s", file_name);
					KeBugCheck (BC_EX_FILE_SYSTEM, NULL);
				}
			
				strcpy(pNewNode->m_name, file_name);
	
				pNewNode->m_length    = fileSize;
				pNewNode->m_inode     = i;
				pNewNode->m_type      = FILE_TYPE_FILE;
				pNewNode->m_implData  = fileSize;
				pNewNode->m_implData1 = (uint32_t) &ramDiskTar->buffer;
				pNewNode->Read        = FsRootFsRead;
				pNewNode->Write       = NULL;
				pNewNode->Open        = NULL;
				pNewNode->Close       = NULL;
				pNewNode->ReadDir     = NULL;
				pNewNode->FindDir     = NULL;
				pNewNode->OpenDir     = NULL;
				pNewNode->CloseDir    = NULL;
				pNewNode->CreateFile  = NULL;
				pNewNode->EmptyFile   = NULL;
				pNewNode->m_perms     = PERM_READ;
				
				//if it ends with .nse also make it executable
				if (EndsWith(pNewNode->m_name, ".nse"))
				{
					pNewNode->m_perms |= PERM_EXEC;
				}
				i++;
			}*/
		}

		// Advance to the next entry
		ramDiskTar = (Tar *) ((uintptr_t) ramDiskTar + ((fileSize + 511) / 512 + 1) * 512);
	}
	SLogMsg("Adding files done");
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
	uint32_t pInitrdAddress = 0xc0000000 + initRdModule->mod_start;
	
	// We should no longer have the problem of it hitting our frame bitset.
	
	SLogMsg("Init Ramdisk module Start address: %x, End address: %x", initRdModule->mod_start, initRdModule->mod_end);
	//If the end address went beyond 1 MB:
	if (initRdModule->mod_end >= 0x100000)
	{
		//Actually go to the effort of mapping the initrd to be used.
		pInitrdAddress = MmMapPhysicalMemory (0x30000000, initRdModule->mod_start, initRdModule->mod_end);
	}
	
	SLogMsg("Physical address that we should load from: %x", pInitrdAddress);
	
	// Load the initrd last so its entries show up last when we type 'ls'.
	FsInitializeInitRd((void*)pInitrdAddress);
}

//#endif
