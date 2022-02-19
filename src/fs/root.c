/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

       FILE SYSTEM: VFS root module
******************************************/
#include <vfs.h>
#include <console.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>

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
}


#endif

// RAM disk/initrd
#if 1
InitRdHeader* g_pInitRdHeader;
InitRdFileHeader* g_pInitRdFileHeaders;
FileNode* g_pInitRdRoot;
FileNode* g_pInitRdDev;  //add a dirnode for /dev to mount stuff later
FileNode* g_pRootNodes; //list of root nodes.
uint32_t  g_nRootNodes; //number of root nodes.
FileNode* g_pDevNodes; //list of dev nodes.
uint32_t  g_nDevNodes; //number of dev nodes.

static DirEnt g_DirEnt; // directory /dev

static uint32_t FsRootFsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	InitRdFileHeader* pHeader = &g_pInitRdFileHeaders[pNode->m_inode];
	
	//check lengths
	if (offset > pHeader->m_length)
		return 0;
	if (offset + size > pHeader->m_length)
		size = pHeader->m_length - offset;
	
	//read!
	memcpy (pBuffer, (uint8_t*)pHeader->m_offset + offset, size);
	return size;
}
//TODO: open, close etc
//TODO: make a different separate way to do this stuff?
extern int       g_fatsMountedCount;
extern FileNode* g_fatsMountedPointers[32];
extern int       g_rdsMountedCount;
extern FileNode* g_rdsMountedPointers[32];
static DirEnt* FsRootFsReadDir(FileNode* pNode, uint32_t index)
{
	//uint32_t filesBeforeInitrd = 1 + g_fatsMountedCount + g_rdsMountedCount;
	if (pNode == g_pInitRdRoot)
	{
		if (index == 0)
		{
			strcpy (g_DirEnt.m_name, FS_DEVICE_NAME);
			g_DirEnt.m_inode = 0;
			return &g_DirEnt;
		}
		index--;
		if (index < (uint32_t)g_rdsMountedCount)
		{
			strcpy (g_DirEnt.m_name, g_rdsMountedPointers[index]->m_name);
			g_DirEnt.m_inode = g_rdsMountedPointers[index]->m_inode;
			return &g_DirEnt;
		}
		index -= g_rdsMountedCount;
		if (index < (uint32_t)g_fatsMountedCount)
		{
			strcpy (g_DirEnt.m_name, g_fatsMountedPointers[index]->m_name);
			g_DirEnt.m_inode = g_fatsMountedPointers[index]->m_inode;
			return &g_DirEnt;
		}
		index -= g_fatsMountedCount;
	}
	if (index >= g_nRootNodes)
		return NULL;
	
	strcpy(g_DirEnt.m_name, g_pRootNodes[index].m_name);
	g_DirEnt.m_inode      = g_pRootNodes[index].m_inode;
	return &g_DirEnt;
}

static FileNode* FsRootFsFindDir(FileNode* pNode, const char* pName)
{
	if (pNode == g_pInitRdRoot)
	{
		if (strcmp(pName, FS_DEVICE_NAME) == 0)
			return g_pInitRdDev;
		else
		{
			for (int i = 0; i < g_rdsMountedCount; i++)
			{
				if (strcmp (pName, g_rdsMountedPointers[i]->m_name) == 0)
				{
					return g_rdsMountedPointers[i];
				}
			}
			for (int i = 0; i < g_fatsMountedCount; i++)
			{
				if (strcmp (pName, g_fatsMountedPointers[i]->m_name) == 0)
				{
					return g_fatsMountedPointers[i];
				}
			}
		}
	}
	
	for (uint32_t i = 0; i < g_nRootNodes; i++)
	{
		if (strcmp(pName, g_pRootNodes[i].m_name) == 0)
			return &g_pRootNodes[i];
	}
	
	return NULL;
}

static DirEnt* FsDevReadDir(UNUSED FileNode* pNode, uint32_t index)
{
	if (index >= g_nDevNodes)
		return NULL;
	
	strcpy(g_DirEnt.m_name, g_pDevNodes[index].m_name);
	g_DirEnt.m_inode = g_pDevNodes[index].m_inode;
	return &g_DirEnt;
}

static FileNode* FsDevFindDir(UNUSED FileNode* pNode, const char* pName)
{
	for (uint32_t i = 0; i < g_nDevNodes; i++)
	{
		if (strcmp(pName, g_pDevNodes[i].m_name) == 0)
			return &g_pDevNodes[i];
	}
	
	return NULL;
}

FileNode* FsGetRootNode ()
{
	//TODO: initialize it?
	return g_pInitRdRoot;
}

void FsInitializeDevicesDir ()
{
	int devCount = 0, index = 0;
	
#define DEFINE_DEVICE(_1, _2, _3, _4) do\
	devCount++;\
while (0)
	#include "vfs_dev_defs.h"
	
	g_pDevNodes = (FileNode*)MmAllocate(sizeof(FileNode) * devCount);
	g_nDevNodes = devCount;
	
#undef DEFINE_DEVICE
	
#define DEFINE_DEVICE(name, read, write, inode) do {\
	g_pDevNodes[index].m_length = 0;\
	g_pDevNodes[index].m_inode  = inode;\
	g_pDevNodes[index].m_type   = FILE_TYPE_CHAR_DEVICE;\
	g_pDevNodes[index].m_perms  = PERM_READ|PERM_WRITE;\
	strcpy (g_pDevNodes[index].m_name, name);\
	\
	g_pDevNodes[index].Read  = read;\
	g_pDevNodes[index].Write = write;\
	g_pDevNodes[index].Open  = NULL;\
	g_pDevNodes[index].Close = NULL;\
	g_pDevNodes[index].ReadDir = NULL;\
	g_pDevNodes[index].FindDir = NULL;\
	g_pDevNodes[index].OpenDir = NULL;\
	g_pDevNodes[index].CloseDir= NULL;\
	g_pDevNodes[index].CreateFile = NULL;\
	g_pDevNodes[index].EmptyFile  = NULL;\
	index++;\
} while (0)
	
	#include "vfs_dev_defs.h"
	
#undef DEFINE_DEVICE
}

void FsInitializeInitRd(void* pRamDisk)
{
	uint32_t location = (uint32_t)pRamDisk;
	//initialize the file headers and stuff.
	g_pInitRdHeader = (InitRdHeader*)pRamDisk;
	g_pInitRdFileHeaders  = (InitRdFileHeader*)(pRamDisk + sizeof(InitRdHeader));
	
	//initialize the root directory
	g_pInitRdRoot = (FileNode*)MmAllocate(sizeof(FileNode));
	strcpy(g_pInitRdRoot->m_name, FS_FSROOT_NAME);
	g_pInitRdRoot->m_flags = g_pInitRdRoot->m_inode = g_pInitRdRoot->m_length = g_pInitRdRoot->m_implData = g_pInitRdRoot->m_perms = 0;
	g_pInitRdRoot->m_type = FILE_TYPE_DIRECTORY;
	g_pInitRdRoot->m_perms = PERM_READ;
	g_pInitRdRoot->Read    = NULL;
	g_pInitRdRoot->Write   = NULL;
	g_pInitRdRoot->Open    = NULL;
	g_pInitRdRoot->Close   = NULL;
	g_pInitRdRoot->ReadDir = FsRootFsReadDir;
	g_pInitRdRoot->FindDir = FsRootFsFindDir;
	g_pInitRdRoot->OpenDir = NULL;
	g_pInitRdRoot->CloseDir= NULL;
	g_pInitRdRoot->CreateFile = NULL;
	g_pInitRdRoot->EmptyFile  = NULL;
	
	// Initialize the /dev dir.
	g_pInitRdDev = (FileNode*)MmAllocate(sizeof(FileNode));
	strcpy(g_pInitRdDev->m_name, FS_DEVICE_NAME);
	g_pInitRdDev->m_flags = g_pInitRdDev->m_inode = g_pInitRdDev->m_length = g_pInitRdDev->m_implData = g_pInitRdDev->m_perms = 0;
	g_pInitRdDev->m_type = FILE_TYPE_DIRECTORY;
	g_pInitRdDev->m_perms = PERM_READ | PERM_WRITE;
	g_pInitRdDev->Read    = NULL;
	g_pInitRdDev->Write   = NULL;
	g_pInitRdDev->Open    = NULL;
	g_pInitRdDev->Close   = NULL;
	g_pInitRdDev->ReadDir = FsDevReadDir;
	g_pInitRdDev->FindDir = FsDevFindDir;
	g_pInitRdDev->OpenDir = NULL;
	g_pInitRdDev->CloseDir= NULL;
	g_pInitRdDev->CreateFile = NULL;
	g_pInitRdDev->EmptyFile  = NULL;
	
	// Initialize devices
	FsInitializeDevicesDir();
	
	// Add files to the ramdisk.
	g_pRootNodes = (FileNode*)MmAllocate(sizeof(FileNode) * g_pInitRdHeader->m_nFiles);
	g_nRootNodes = g_pInitRdHeader->m_nFiles;
	
	// for every file
	for (int i = 0; i < g_pInitRdHeader->m_nFiles; i++)
	{
		g_pInitRdFileHeaders[i].m_offset += location;
		//create a new filenode
		strcpy(g_pRootNodes[i].m_name, g_pInitRdFileHeaders[i].m_name);
		g_pRootNodes[i].m_length = g_pInitRdFileHeaders[i].m_length;
		g_pRootNodes[i].m_inode = i;
		g_pRootNodes[i].m_type = FILE_TYPE_FILE;
		g_pRootNodes[i].Read    = FsRootFsRead;
		g_pRootNodes[i].Write   = NULL;
		g_pRootNodes[i].Open    = NULL;
		g_pRootNodes[i].Close   = NULL;
		g_pRootNodes[i].ReadDir = NULL;
		g_pRootNodes[i].FindDir = NULL;
		g_pRootNodes[i].OpenDir = NULL;
		g_pRootNodes[i].CloseDir= NULL;
		g_pRootNodes[i].CreateFile = NULL;
		g_pRootNodes[i].EmptyFile  = NULL;
		g_pRootNodes[i].m_perms = PERM_READ;
		
		//if it ends with .nse...
		if (EndsWith(g_pRootNodes[i].m_name, ".nse"))
		{
			//also make it executable
			g_pRootNodes[i].m_perms |= PERM_EXEC;
		}
	}
}
#endif
