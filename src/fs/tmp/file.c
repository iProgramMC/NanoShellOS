//  ***************************************************************
//  fs/tmp/file.c - Creation date: 14/05/2023
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

int FsTempFileChangeMode(FileNode* pFileNode, int mode)
{
	mode &= PERM_READ | PERM_WRITE | PERM_EXEC;
	
	pFileNode->m_perms = mode;
	
	return ERR_SUCCESS;
}

int FsTempFileChangeTime(FileNode* pFileNode, int atime, int mtime)
{
	if (atime != -1)
		pFileNode->m_accessTime = (uint32_t)atime;
	
	if (mtime != -1)
		pFileNode->m_modifyTime = (uint32_t)mtime;
	
	return ERR_SUCCESS;
}

int FsTempFileRead(FileNode* pFileNode, uint32_t offset, uint32_t size, void* pBuffer, UNUSED bool bBlock)
{
	if (offset >= pFileNode->m_length)
		return 0;
	if (offset + size >= pFileNode->m_length)
		size = (pFileNode->m_length - offset);
	if (!size)
		return 0;
	
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	memcpy(pBuffer, pTFNode->m_pFileData + offset, size);
	
	pFileNode->m_accessTime = GetEpochTime();
	
	return size;
}

void FsTempFileShrink(FileNode* pFileNode, uint32_t newSize)
{
	if (pFileNode->m_length == newSize)
		return;
	
	ASSERT(newSize < pFileNode->m_length && "Don't use FsTempFileShrink to expand a file!");
	
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	if (!pTFNode->m_bMutable)
		return;
	
	uint32_t sNewCapacityPages = (newSize + PAGE_SIZE - 1) / PAGE_SIZE;
	uint32_t sNewCapacity      = sNewCapacityPages * PAGE_SIZE;
	
	if (sNewCapacityPages != pTFNode->m_nFileSizePages)
	{
		uint8_t* pData = MmReAllocate(pTFNode->m_pFileData, sNewCapacity);
		if (!pData)
		{
			// hmm, this shouldn't happen.
			SLogMsg("FsTempFileShrink couldn't reallocate to a smaller size.");
			return;
		}
		
		pTFNode->m_pFileData      = pData;
		pTFNode->m_nFileSizePages = sNewCapacityPages;
	}
	
	pFileNode->m_length = newSize;
}

int FsTempFileWrite(FileNode* pFileNode, uint32_t offset, uint32_t size, const void* pBuffer, UNUSED bool bBlock)
{
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	if (!pTFNode->m_bMutable)
		return 0;
	
	uint32_t sCapacity = PAGE_SIZE * pTFNode->m_nFileSizePages;
	
	uint32_t startOffs = offset, endOffs = offset + size;
	if (startOffs >= endOffs)
		return 0;
	
	if (endOffs > sCapacity)
	{
		uint32_t sNewCapacityPages = (endOffs + PAGE_SIZE - 1) / PAGE_SIZE;
		uint32_t sNewCapacity      = sNewCapacityPages * PAGE_SIZE;
		
		uint8_t* pData = MmReAllocate(pTFNode->m_pFileData, sNewCapacity);
		if (!pData)
		{
			// running out of memory is very much a possibility, so truncate the write to our current capacity
			endOffs = sCapacity;
			
			if (startOffs >= endOffs)
				return 0;
			
			size = endOffs - startOffs;
		}
		else
		{
			pTFNode->m_pFileData      = pData;
			pTFNode->m_nFileSizePages = sNewCapacityPages;
			
			// set the newly expanded stuff to 0.
			memset(pData + sCapacity, 0, sNewCapacity - sCapacity);
			
			sCapacity = sNewCapacity;
		}
	}
	
	// do the actual write
	memcpy(pTFNode->m_pFileData + offset, pBuffer, size);
	
	if (pFileNode->m_length < endOffs)
		pFileNode->m_length = endOffs;
	
	pFileNode->m_accessTime = pFileNode->m_modifyTime = GetEpochTime();
	
	return size;
}

int FsTempFileOpen(FileNode* pFileNode, UNUSED bool read, bool write)
{
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	if (write && !pTFNode->m_bMutable)
		return ERR_ACCESS_DENIED; // can't write!
	
	return ERR_SUCCESS;
}

int FsTempFileClose(UNUSED FileNode* pFileNode)
{
	return ERR_SUCCESS;
}

int FsTempFileEmpty(FileNode* pFileNode)
{
	// empty the file entirely.
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	if (!pTFNode->m_bMutable)
		return ERR_NOT_SUPPORTED;
	
	MmFree(pTFNode->m_pFileData);
	pTFNode->m_pFileData      = NULL;
	pTFNode->m_nFileSizePages = 0;
	
	return ERR_SUCCESS;
}
