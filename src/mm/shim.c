//  ***************************************************************
//  mm/shim.c - Creation date: 29/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// The NanoShell compatibility shim.
#include <memory.h>
#include "memoryi.h"

/*
Define the following functions:
- MmMapPhysMemFastUnsafeRW
- MmUnmapPhysMemFastUnsafe
- MmMapPhysicalMemoryRWUnsafe
- MmAllocateSinglePagePhyD
- MmAllocateSinglePageD
- MmAllocatePhyD
- MmAllocateD
- MmAllocateKD
- MmReAllocateD
- MmReAllocateKD
- MmFreePage
- MmFree
- MmFreeK
- 
- 
*/

// Physical Memory Mapping
#if 1

void* MmMapPhysMemFastUnsafeRW(uint32_t page, bool bReadWrite)
{
	void* pMem = MhMapPhysicalMemory(page & ~(PAGE_SIZE - 1), 1, bReadWrite); 
	
	return (uint8_t*)pMem + (page & (PAGE_SIZE - 1));
}
void MmUnmapPhysMemFastUnsafe(void *pMem)
{
	MhFreePage(pMem);
}
void* MmMapPhysicalMemoryRWUnsafe(uint32_t phys_start, uint32_t phys_end, bool bReadWrite)
{
	uint32_t phys_page_start = (phys_start >> 12);
	uint32_t phys_page_end   = (phys_end   >> 12) + ((phys_end & 0xFFF) != 0);
	uint32_t num_pages_to_map = phys_page_end - phys_page_start;
	
	return MhMapPhysicalMemory(phys_start, num_pages_to_map, bReadWrite); 
}
void MmUnmapPhysicalMemoryUnsafe(void *pMem)
{
	MhFree (pMem);
}
// Safe wrappers for the above functions
void *MmMapPhysMemFastRW(uint32_t page, bool bReadWrite)
{
	KeVerifyInterruptsEnabled;
	cli;
	void* ptr = MmMapPhysMemFastUnsafeRW(page, bReadWrite);
	sti;
	return ptr;
}
void *MmMapPhysMemFast(uint32_t page)
{
	KeVerifyInterruptsEnabled;
	cli;
	void* ptr = MmMapPhysMemFastUnsafeRW(page, true);
	sti;
	return ptr;
}
void MmUnmapPhysMemFast(void* pMem)
{
	KeVerifyInterruptsEnabled;
	cli;
	MmUnmapPhysMemFastUnsafe(pMem);
	sti;
}
void* MmMapPhysicalMemoryRW(uint32_t phys_start, uint32_t phys_end, bool bReadWrite)
{
	KeVerifyInterruptsEnabled;
	cli;
	void* result = MmMapPhysicalMemoryRWUnsafe(phys_start, phys_end, bReadWrite);
	sti;
	return result;
}
void* MmMapPhysicalMemory(uint32_t phys_start, uint32_t phys_end)
{
	return MmMapPhysicalMemoryRW(phys_start, phys_end, false);
}
void MmUnmapPhysicalMemory(void *pMem)
{
	KeVerifyInterruptsEnabled;
	cli;
	MmUnmapPhysicalMemoryUnsafe(pMem);
	sti;
}
#endif

// MmAllocate*
#if 1

void* MmAllocateSinglePagePhyD (uint32_t* pOut, UNUSED const char* callFile, UNUSED int callLine)
{
	// TODO: Make this go through the user heap too.
	KeVerifyInterruptsEnabled;
	cli;
	void *pMem = MhAllocateSinglePage(pOut);
	sti;
	return pMem;
}
void* MmAllocateSinglePageD(const char* callFile, int callLine)
{
	return MmAllocateSinglePagePhyD(NULL, callFile, callLine);
}
void* MmAllocatePhyD (size_t size, UNUSED const char* callFile, UNUSED int callLine, uint32_t* physAddresses)
{
	KeVerifyInterruptsEnabled;
	
	// clear interrupts - kernel heap isn't something to race on
	cli;
	
	// allocate from the kernel heap
	void *pMem = MhAllocate(size, physAddresses);
	
	// I use this to find and track down leaks in the kernel heap
	//SLogMsg("%x <== MmAllocatePhyD %s %d", pMem, callFile, callLine);
	
	// restore interrupts, all good
	sti;
	
	return pMem;
}
void* MmAllocateD (size_t size, const char* callFile, int callLine)
{
	return MmAllocatePhyD(size, callFile, callLine, NULL);
}
void* MmAllocateKD (size_t size, const char* callFile, int callLine)
{
	return MmAllocatePhyD(size, callFile, callLine, NULL);
}
void* MmReAllocateD(void *oldPtr, size_t newSize, UNUSED const char *callFile, UNUSED int callLine)
{
	// TODO: Make this go through the user heap too.
	KeVerifyInterruptsEnabled;
	cli;
	void *pMem = MhReAllocate(oldPtr, newSize);
	sti;
	return pMem;
}
void* MmReAllocateKD (void *oldPtr, size_t newSize, const char *callFile, int callLine)
{
	return MmReAllocateD(oldPtr, newSize, callFile, callLine);
}

#endif

// MmFree*
void MmFreePage(void *pAddr)
{
	KeVerifyInterruptsEnabled;
	cli;
	MhFreePage(pAddr);
	sti;
}

void MmFree(void *pAddr)
{
	KeVerifyInterruptsEnabled;
	
	cli;
	
	MhFree(pAddr);
	
	// I use this to track down memory leaks easily
	//SLogMsg("%x => MhFree", pAddr);
	//PrintBackTrace(KeGetEBP(), KeGetEIP(), "bruh", NULL, false);
	
	sti;
}

void MmFreeK(void *pAddr)
{
	MmFree(pAddr);
}

void MmDebugDump()
{
	LogMsg("MmDebugDump TODO: Using heap %p", MuGetCurrentHeap());
}

uint32_t* MuiGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable);
bool MmIsPageMapped(uint32_t addr)
{
	cli; //avoid TOCTOU
	if (MuGetCurrentHeap())
	{
		if (MuiGetPageEntryAt(MuGetCurrentHeap(), (uintptr_t)addr, false))
		{
			sti;
			return true;
		}
	}
	
	if (MhGetPageEntry(addr))
	{
		sti;
		return true;
	}
	
	sti;
	return false;
}
