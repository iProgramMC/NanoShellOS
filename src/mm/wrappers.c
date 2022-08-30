//  ***************************************************************
//  mm/wrappers.c - Creation date: 26/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include <memory.h>

// Namespace: Mm (Memory manager)
// These wrappers directly correspond to real NanoShell functions.
/*
inline size_t MmGetNumPages(size_t sz)
{
	return ((sz - 1) / PAGE_SIZE) + 1;
}

void* MmAllocateSinglePagePhyD(uint32_t* pPhysOut, UNUSED const char * callFile, UNUSED int callLine)
{
	if (!MuGetCurrentHeap())
	{
		cli;
		void * ptr = MhAllocateSinglePage(pPhysOut);
		sti;
		return ptr;
	}
	else
	{
		void *pAddress = NULL;
		
		if (MuMapMemory(MuGetCurrentHeap(), 1, pPhysOut, &pAddress, true, false))
			return pAddress;
		
		return NULL;
	}
}

void* MmAllocatePhyD (size_t size, UNUSED const char* callFile, UNUSED int callLine, uint32_t* physAddresses)
{
	if (!MuGetCurrentHeap())
	{
		cli;
		void * ptr = MhAllocate(size, pPhysOut);
		sti;
		return ptr;
	}
	else
	{
		void *pAddress = NULL;
		
		if (MuMapMemory(MuGetCurrentHeap(), MmGetNumPages(size), pPhysOut, &pAddress, true, false))
			return pAddress;
		
		return NULL;
	}
}

void* MmAllocateD (size_t size, UNUSED const char* callFile, UNUSED int callLine)
{
	return MmAllocatePhyD(size, callFile, callLine);
}

void* MmAllocateSinglePageD(UNUSED const char * callFile, UNUSED int callLine)
{
	return MmAllocateSinglePagePhyD(NULL, callFile, callLine);
}

void* MmAllocateKD (size_t size, UNUSED const char* callFile, UNUSED int callLine)
{
	cli;
	void * ptr = MhAllocate(size, NULL);
	sti;
	return ptr;
}

void* MmReAllocateKD (void* oldPtr, size_t size, UNUSED const char* callFile, UNUSED int callLine)
{
	cli;
	void * ptr = MhReAllocate(oldPtr, size);
	sti;
	return ptr;
}

//TODO: MmReAllocateD

void MmFreePage(void *pAddr)
{
	
}
*/


