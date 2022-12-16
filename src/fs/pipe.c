//  ***************************************************************
//  pipe.c - Creation date: 09/12/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Note: Pipe objects aren't actually a part of the VFS - they're just file descriptions.

#include <vfs.h>
#include <task.h>
#include <string.h>

#define C_DEFAULT_PIPE_SIZE (4096)

// TODO: A better way to do waiting for a write.
// TODO: Allow O_NONBLOCK. This would require re-doing the read/write system calls.
extern SafeLock g_FileSystemLock;

bool FsPipeReadSingleByte(FileNode* pPipeNode, uint8_t* pOut, bool bBlock, uint32_t offset)
{
	// if the tail and the head are the same (i.e. there is nothing on the queue right now)
	if (pPipeNode->m_pipe[offset].bufferTail == pPipeNode->m_pipe[offset].bufferHead)
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
	*pOut = pPipeNode->m_pipe[offset].buffer[pPipeNode->m_pipe[offset].bufferTail];
	
	// increment the tail
	pPipeNode->m_pipe[offset].bufferTail = (pPipeNode->m_pipe[offset].bufferTail + 1) % pPipeNode->m_pipe[offset].bufferSize;
	
	KeUnsuspendTasksWaitingForPipeRead(pPipeNode);
	
	return true;
}

bool FsPipeWriteSingleByte(FileNode* pPipeNode, uint8_t data, bool bBlock, uint32_t offset)
{
	// if the tail and the head are the same (i.e. there is nothing on the queue right now)
	while ((pPipeNode->m_pipe[offset].bufferHead + 1) % pPipeNode->m_pipe[offset].bufferSize == pPipeNode->m_pipe[offset].bufferTail)
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
	
	pPipeNode->m_pipe[offset].buffer[pPipeNode->m_pipe[offset].bufferHead] = data;
	pPipeNode->m_pipe[offset].bufferHead = (pPipeNode->m_pipe[offset].bufferHead + 1) % pPipeNode->m_pipe[offset].bufferSize;
	
	KeUnsuspendTasksWaitingForPipeWrite(pPipeNode);
	
	return true;
}

uint32_t FsPipeRead(FileNode* pPipeNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer, bool block)
{
	uint8_t* pBufferBytes = (uint8_t*)pBuffer;
	
	for (uint32_t i = 0; i < size; i++)
	{
		// if we couldn't read one byte, just return
		if (!FsPipeReadSingleByte(pPipeNode, pBufferBytes + i, block, offset))
			return i;
	}
	
	return size;
}

uint32_t FsPipeWrite(FileNode* pPipeNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer, bool block)
{
	uint8_t* pBufferBytes = (uint8_t*)pBuffer;
	
	for (uint32_t i = 0; i < size; i++)
	{
		if (!FsPipeWriteSingleByte(pPipeNode, pBufferBytes[i], block, offset))
			return i;
	}
	
	return size;
}

void FsPipeOnUnreferenced(FileNode* pPipeNode)
{
	for (int i = 0; i < C_PIPE_DUPLEX_BUF_COUNT; i++)
	{
		MmFree(pPipeNode->m_pipe[i].buffer);
		pPipeNode->m_pipe[i].buffer = NULL;
	}
	MmFree(pPipeNode);
}

void FsPipeInitialize(FileNode* pPipeNode)
{
	pPipeNode->m_type            = FILE_TYPE_PIPE;
	
	for (int i = 0; i < C_PIPE_DUPLEX_BUF_COUNT; i++)
	{
		pPipeNode->m_pipe[i].buffer     = MmAllocate(C_DEFAULT_PIPE_SIZE);
		pPipeNode->m_pipe[i].bufferSize = C_DEFAULT_PIPE_SIZE;
		pPipeNode->m_pipe[i].bufferTail = pPipeNode->m_pipe[i].bufferHead = 0;
	}
	
	pPipeNode->OnUnreferenced    = FsPipeOnUnreferenced;
	pPipeNode->Read              = FsPipeRead;
	pPipeNode->Write             = FsPipeWrite;
}

FileNode* FsRootAddArbitraryFileNodeToRoot(const char* pFileName, FileNode* pFileNode);

// note: this gives the file node 1 reference. Use FsReleaseReference to release this final reference.
FileNode* FsPipeCreate(const char* pName)
{
	// TODO: add to any path
	FileNode* pfn = MmAllocate(sizeof(FileNode));
	memset(pfn, 0, sizeof *pfn);
	
	FsPipeInitialize(pfn);
	pfn->m_refCount = 1;
	pfn->m_perms = PERM_READ | PERM_WRITE;
	strcpy(pfn->m_name, pName);
	
	return pfn;
}

//note: friendly name can be NULL
int FiCreatePipe(const char* pFriendlyName, int fds[2], int oflags)
{
	if (!pFriendlyName || !*pFriendlyName)
		pFriendlyName = "pipe";
	
	FileNode* pFN = FsPipeCreate(pFriendlyName);
	
	// Open the read head.
	fds[0] = FiOpenFileNode(pFN, oflags | O_RDWR | O_DUPL0);
	
	if (fds[0] < 0)
	{
		// release the reference from FsPipeCreate. This should unload the pipe.
		FsReleaseReference(pFN);
		
		// return the error code returned by trying to open it for reading
		return fds[0];
	}
	
	// Open the write head.
	fds[1] = FiOpenFileNode(pFN, oflags | O_RDWR | O_DUPL1);
	
	if (fds[1] < 0)
	{
		// close the read head
		FiClose(fds[0]);
		fds[0] = -1;
		
		// release the reference from FsPipeCreate. This should unload the pipe.
		FsReleaseReference(pFN);
		
		// return the error code returned by trying to open it for writing.
		return fds[1];
	}
	
	// release the reference from FsPipeCreate. After this,
	// closing both endsshould unload the pipe.
	FsReleaseReference(pFN);
	
	return -ENOTHING;
}

void FsPipeTest()
{
	int fds[2];
	
	int erc = FiCreatePipe("test", fds, O_NONBLOCK);
	
	if (erc < 0)
	{
		LogMsg("pipetest: %s", GetErrNoString(erc));
		return;
	}
	
	FiWrite(fds[1], "Hello, NanoShell. Wrote to fd[1]!", 33);
	FiWrite(fds[0], "Hello, NanoShell. Wrote to fd[0]!", 33);
	
	char buffer[35];
	buffer[33] = 0;
	FiRead(fds[0], buffer, 33);
	LogMsg("The fd[0] of fortune reads... '%s'. How strange.", buffer);
	FiRead(fds[1], buffer, 33);
	LogMsg("The fd[1] of fortune reads... '%s'. How strange.", buffer);
	
	FiClose(fds[0]);
	FiClose(fds[1]);
}

