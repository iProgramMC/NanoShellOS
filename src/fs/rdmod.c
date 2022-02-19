/*****************************************
		NanoShell Operating System
		  (C) 2012 iProgramInCpp

     FILE SYSTEM: Grub module RAMDisk
******************************************/
#include <vfs.h>
#include <memory.h>
#include <string.h>
#include <print.h>

#define MAX_CONCURRENT_RAMDISKS 32

int                         g_rdsMountedCount;
FileNode*                   g_rdsMountedPointers      [MAX_CONCURRENT_RAMDISKS];
InitRdFileHeader*           g_rdsMountedFileHeaders   [MAX_CONCURRENT_RAMDISKS];
FileNode*                   g_rdsMountedRootNodes     [MAX_CONCURRENT_RAMDISKS];
int                         g_rdsMountedRootNodeCounts[MAX_CONCURRENT_RAMDISKS];

static uint32_t FsRamDiskRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	InitRdFileHeader* pHeader = &g_rdsMountedFileHeaders[pNode->m_implData2][pNode->m_inode];
	
	//check lengths
	if (offset > pHeader->m_length)
		return 0;
	if (offset + size > pHeader->m_length)
		size = pHeader->m_length - offset;
	
	//read!
	memcpy (pBuffer, (uint8_t*)pHeader->m_offset + offset, size);
	return size;
}

static DirEnt g_ramDiskDirEnt;
static DirEnt* FsRamDiskReadDir(FileNode* pNode, uint32_t index)
{
	int idx = pNode->m_implData2;
	
	FileNode* pRootNodes = g_rdsMountedRootNodes     [idx];
	int       nRootNodes = g_rdsMountedRootNodeCounts[idx];
	
	if (index >= (uint32_t)nRootNodes) return NULL;
	
	strcpy(g_ramDiskDirEnt.m_name, pRootNodes[index].m_name);
	g_ramDiskDirEnt.m_inode      = pRootNodes[index].m_inode;
	return &g_ramDiskDirEnt;
}

static FileNode* FsRamDiskFindDir(FileNode* pNode, const char* pName)
{
	int idx = pNode->m_implData2;
	
	FileNode* pRootNodes = g_rdsMountedRootNodes     [idx];
	int       nRootNodes = g_rdsMountedRootNodeCounts[idx];
	
	for (int i = 0; i < nRootNodes; i++)
	{
		if (strcmp(pName, pRootNodes[i].m_name) == 0)
			return &pRootNodes[i];
	}
	
	return NULL;
}


void FsMountRamDisk(void* pRamDisk)
{
	if (g_rdsMountedCount >= (int)ARRAY_COUNT (g_rdsMountedPointers))
	{
		LogMsg("Mounted too many ramdisks. Oh dear!");
		return;
	}
	
	//let's mount this thing :)
	g_rdsMountedCount++;
	FileNode *pRootNode = (FileNode*)MmAllocate (sizeof (FileNode));
	g_rdsMountedPointers[g_rdsMountedCount-1] = pRootNode;

	//this is a directory
	sprintf(pRootNode->m_name, "Rd%d", g_rdsMountedCount-1);
	pRootNode->m_flags = pRootNode->m_inode = pRootNode->m_length = pRootNode->m_implData = pRootNode->m_perms = 0;
	pRootNode->m_type  = FILE_TYPE_DIRECTORY;
	pRootNode->m_perms = PERM_READ;
	pRootNode->m_implData2 = g_rdsMountedCount-1;
	pRootNode->Read    = NULL;
	pRootNode->Write   = NULL;
	pRootNode->Open    = NULL;
	pRootNode->Close   = NULL;
	pRootNode->ReadDir = FsRamDiskReadDir;
	pRootNode->FindDir = FsRamDiskFindDir;
	pRootNode->OpenDir = NULL;
	pRootNode->CloseDir= NULL;
	pRootNode->CreateFile = NULL;
	pRootNode->EmptyFile  = NULL;
	
	//add files to the
	InitRdHeader* pFileHdr = (InitRdHeader*)pRamDisk;
	InitRdFileHeader* pHeaders = (InitRdFileHeader*)((uint8_t*)pRamDisk + sizeof (InitRdHeader));
	
	g_rdsMountedFileHeaders   [pRootNode->m_implData2] = pHeaders;
	g_rdsMountedRootNodeCounts[pRootNode->m_implData2] = pFileHdr->m_nFiles;
	g_rdsMountedRootNodes     [pRootNode->m_implData2] = (FileNode*)MmAllocate(sizeof(FileNode) * g_rdsMountedRootNodeCounts[pRootNode->m_implData2]);
	
	for (int i = 0; i < pFileHdr->m_nFiles; i++)
	{
		pHeaders[i].m_offset += (uint32_t)pRamDisk;
		
		FileNode* pNode = &g_rdsMountedRootNodes[pRootNode->m_implData2][i];
		strcpy (pNode->m_name, pHeaders[i].m_name);
		
		pNode->m_length = pHeaders[i].m_length;
		pNode->m_inode  = i;
		pNode->m_type   = FILE_TYPE_FILE;
		pNode->m_perms  = PERM_READ;
		pNode->m_implData2 = g_rdsMountedCount-1;
		pNode->Read    = FsRamDiskRead;
		pNode->Write   = NULL;
		pNode->Open    = NULL;
		pNode->Close   = NULL;
		pNode->ReadDir = NULL;
		pNode->FindDir = NULL;
		pNode->CreateFile = NULL;
		pNode->EmptyFile  = NULL;
		
		//if it ends with .nse...
		if (EndsWith(pNode->m_name, ".nse"))
		{
			//also make it executable
			pNode->m_perms |= PERM_EXEC;
		}
	}
}
