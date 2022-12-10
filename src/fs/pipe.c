//  ***************************************************************
//  pipe.c - Creation date: 09/12/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include <vfs.h>
#include <task.h>
#include <string.h>

#define C_DEFAULT_PIPE_SIZE (4096)

// TODO: A better way to do waiting for a write.
// TODO: Allow O_NONBLOCK. This would require re-doing the read/write system calls.
extern SafeLock g_FileSystemLock;

bool FsPipeReadSingleByte(FileNode* pPipeNode, uint8_t* pOut, bool bBlock)
{
	// if the tail and the head are the same (i.e. there is nothing on the queue right now)
	if (pPipeNode->m_pipe.bufferTail == pPipeNode->m_pipe.bufferHead)
	{
		if (!bBlock)
			return false;
		
		// Unlock the file system lock for now.
		LockFree(&g_FileSystemLock);
		
		// Wait for a write to happen.
		WaitPipeWrite(pPipeNode);
		
		// Re-lock it.
		LockAcquire(&g_FileSystemLock);
	}
	
	// read a byte from the buffer
	*pOut = pPipeNode->m_pipe.buffer[pPipeNode->m_pipe.bufferTail];
	
	// increment the tail
	pPipeNode->m_pipe.bufferTail = (pPipeNode->m_pipe.bufferTail + 1) % pPipeNode->m_pipe.bufferSize;
	
	KeUnsuspendTasksWaitingForPipeRead(pPipeNode);
	
	return true;
}

bool FsPipeWriteSingleByte(FileNode* pPipeNode, uint8_t data, bool bBlock)
{
	// if the tail and the head are the same (i.e. there is nothing on the queue right now)
	while ((pPipeNode->m_pipe.bufferHead + 1) % pPipeNode->m_pipe.bufferSize == pPipeNode->m_pipe.bufferTail)
	{
		if (!bBlock)
			return false;
		
		// Unlock the file system lock for now.
		LockFree(&g_FileSystemLock);
		
		// Wait for a write to happen.
		WaitPipeRead(pPipeNode);
		
		// Re-lock it.
		LockAcquire(&g_FileSystemLock);
	}
	
	pPipeNode->m_pipe.buffer[pPipeNode->m_pipe.bufferHead] = data;
	pPipeNode->m_pipe.bufferHead = (pPipeNode->m_pipe.bufferHead + 1) % pPipeNode->m_pipe.bufferSize;
	
	KeUnsuspendTasksWaitingForPipeWrite(pPipeNode);
	
	return true;
}

uint32_t FsPipeRead(FileNode* pPipeNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer, bool block)
{
	uint8_t* pBufferBytes = (uint8_t*)pBuffer;
	
	for (uint32_t i = 0; i < size; i++)
	{
		// if we couldn't read one byte, just return
		if (!FsPipeReadSingleByte(pPipeNode, pBufferBytes + i, block))
			return i;
	}
	
	return size;
}

uint32_t FsPipeWrite(FileNode* pPipeNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer, bool block)
{
	uint8_t* pBufferBytes = (uint8_t*)pBuffer;
	
	for (uint32_t i = 0; i < size; i++)
	{
		if (!FsPipeWriteSingleByte(pPipeNode, pBufferBytes[i], block))
			return i;
	}
	
	return size;
}

void FsPipeOnUnreferenced(FileNode* pPipeNode)
{
	MmFree(pPipeNode->m_pipe.buffer);
	pPipeNode->m_pipe.buffer = NULL;
}

void FsPipeInitialize(FileNode* pPipeNode)
{
	pPipeNode->m_type            = FILE_TYPE_PIPE;
	pPipeNode->m_pipe.buffer     = MmAllocate(C_DEFAULT_PIPE_SIZE);
	pPipeNode->m_pipe.bufferSize = C_DEFAULT_PIPE_SIZE;
	pPipeNode->m_pipe.bufferTail = pPipeNode->m_pipe.bufferHead = 0;
	pPipeNode->OnUnreferenced    = FsPipeOnUnreferenced;
	pPipeNode->Read              = FsPipeRead;
	pPipeNode->Write             = FsPipeWrite;
}

FileNode* FsRootAddArbitraryFileNodeToRoot(const char* pFileName, FileNode* pFileNode);

void* FsPipeCreate(const char* pFileName)
{
	// TODO: add to any path
	FileNode fn;
	memset(&fn, 0, sizeof fn);
	
	FsPipeInitialize(&fn);
	fn.m_refCount = NODE_IS_PERMANENT;
	
	fn.m_perms = PERM_READ | PERM_WRITE;
	
	strcpy(fn.m_name, pFileName);
	
	return FsRootAddArbitraryFileNodeToRoot(pFileName, &fn);
}

//void* FsPipeDelete(

void FsPipeTest()
{
	FsPipeCreate("pipe1");
	
	int fd = FiOpen("/pipe1", O_WRONLY);
	
	FiWrite(fd, "Hello, NanoShell!", 17);
	
	FiClose(fd);
	
	char buffer[20];
	
	fd = FiOpen("/pipe1", O_RDONLY);
	
	FiRead(fd, buffer, 17);
	
	LogMsg("Got: '%s'", buffer);
	
	FiClose(fd);
}

