/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

     FILE SYSTEM: Grub module RAMDisk
******************************************/
#include <vfs.h>
#include <memory.h>
#include <string.h>
#include <print.h>
#include <tar.h>

#define MAX_CONCURRENT_RAMDISKS 32

int g_rdsMountedCount = 0;

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

static uint32_t FsRamDiskRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	InitRdFileHeader* pHeader = &((InitRdFileHeader*)pNode->m_implData)[pNode->m_inode];
	
	//check lengths
	if (offset > pHeader->m_length)
		return 0;
	if (offset + size > pHeader->m_length)
		size = pHeader->m_length - offset;
	
	//read!
	memcpy (pBuffer, (uint8_t*)pHeader->m_offset + offset, size);
	return size;
}
static uint32_t FsRamDiskTarRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer)
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

static DirEnt* FsRamDiskReadDir (FileNode *pDirNode, uint32_t * index, DirEnt* pOutputDent)
{
	FileNode *pNode = pDirNode->children;
	//TODO: Optimize this, if you have consecutive indices
	uint32_t id = 0;
	while (pNode && id < *index)
	{
		pNode = pNode->next;
		id++;
	}
	if (!pNode) return NULL;
	
	++(*index);
	
	strcpy(pOutputDent->m_name, pNode->m_name);
	pOutputDent->m_inode      = pNode->m_inode;
	pOutputDent->m_type       = pNode->m_type;
	return pOutputDent;
}

static FileNode* FsRamDiskFindDir(FileNode* pDirNode, const char* pName)
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

void FsMountTarRamDisk(void* pRamDisk)
{
	FileNode *pTarRoot = CreateFileNode (FsGetRootNode());
	
	sprintf(pTarRoot->m_name, "Tar%d", g_rdsMountedCount);
	pTarRoot->m_type  = FILE_TYPE_DIRECTORY;
	pTarRoot->m_flags = 0;
	pTarRoot->m_perms = PERM_READ;
	pTarRoot->m_length = 0;
	pTarRoot->m_implData2 = g_rdsMountedCount-1;
	
	pTarRoot->Read     = NULL;
	pTarRoot->Write    = NULL;
	pTarRoot->Open     = NULL;
	pTarRoot->Close    = NULL;
	pTarRoot->OpenDir  = NULL;
	pTarRoot->CloseDir = NULL;
	pTarRoot->ReadDir  = FsRamDiskReadDir;
	pTarRoot->FindDir  = FsRamDiskFindDir;
	
	for (Tar* pRamDiskTar = (Tar*) pRamDisk; !memcmp (pRamDiskTar->ustar, "ustar", 5); )
	{
		uint32_t fileSize = OctToBin (pRamDiskTar->size, 11);
		uint32_t pathLen  = strlen (pRamDiskTar->name);
		bool hasDotSlash = false;
		
		if (!memcmp(pRamDiskTar->name, "./", 2))
		{
			pathLen -= 2;
			hasDotSlash = true;
		}

		if (pathLen > 0)
		{
			FileNode* pNode = CreateFileNode(pTarRoot);
			strcpy (pNode->m_name, pRamDiskTar->name + (hasDotSlash ? 2 : 0));
			
			pNode->m_length = fileSize;
			//pNode->m_inode  = i;
			pNode->m_type   = FILE_TYPE_FILE;
			pNode->m_perms  = PERM_READ;
			pNode->m_implData  = fileSize;
			pNode->m_implData1 = (uint32_t) &pRamDiskTar->buffer;
			pNode->m_implData2 = g_rdsMountedCount-1;
			pNode->Read    = FsRamDiskTarRead;
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
		
		pRamDiskTar = (Tar*)((uintptr_t)pRamDiskTar + ((fileSize + 511) / 512 + 1) * 512);
	}
}

void FsMountRamDisk(void* pRamDisk)
{
	//hack
	if (*(uint16_t*)pRamDisk >= 100)
	{
		// A safety measure.  If the first "TAR" entry's ustar matches, this is a tar file.
		if (!memcmp (((uint8_t*)pRamDisk + 0x101), "ustar", 5))
		{
			FsMountTarRamDisk(pRamDisk);
			return;
		}
	}
	
	FileNode *pRdRoot = CreateFileNode (FsGetRootNode());
	
	sprintf(pRdRoot->m_name, "Rd%d", g_rdsMountedCount);
	pRdRoot->m_type  = FILE_TYPE_DIRECTORY;
	pRdRoot->m_flags = 0;
	pRdRoot->m_perms = PERM_READ;
	pRdRoot->m_length = 0;
	pRdRoot->m_implData2 = g_rdsMountedCount-1;
	
	pRdRoot->Read     = NULL;
	pRdRoot->Write    = NULL;
	pRdRoot->Open     = NULL;
	pRdRoot->Close    = NULL;
	pRdRoot->OpenDir  = NULL;
	pRdRoot->CloseDir = NULL;
	pRdRoot->ReadDir  = FsRamDiskReadDir;
	pRdRoot->FindDir  = FsRamDiskFindDir;
	
	InitRdHeader* pFileHdr = (InitRdHeader*)pRamDisk;
	InitRdFileHeader* pHeaders = (InitRdFileHeader*)((uint8_t*)pRamDisk + sizeof (InitRdHeader));
	
	for (int i = 0; i < pFileHdr->m_nFiles; i++)
	{
		pHeaders[i].m_offset += (uint32_t)pRamDisk;
		
		FileNode* pNode = CreateFileNode(pRdRoot);
		strcpy (pNode->m_name, pHeaders[i].m_name);
		
		pNode->m_length = pHeaders[i].m_length;
		pNode->m_inode  = i;
		pNode->m_type   = FILE_TYPE_FILE;
		pNode->m_perms  = PERM_READ;
		pNode->m_implData  = (int)(pHeaders + i);
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
