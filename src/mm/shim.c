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

void* MmAllocateSinglePagePhy(uint32_t* pOut)
{
	// TODO: Make this go through the user heap too.
	KeVerifyInterruptsEnabled;
	cli;
	void *pMem = MhAllocateSinglePage(pOut);
	sti;
	return pMem;
}

void* MmAllocateSinglePage()
{
	return MmAllocateSinglePagePhy(NULL);
}

// Since slab items page aligned addresses are reserved for something else,
// the check for whether a memory chunk is part of a slab is as easy as checking
// if the pointer is page aligned or not.
static bool MmIsPartOfSlab(void* ptr)
{
	return ((uintptr_t)ptr & 0xFFF) != 0;
}

void* MmAllocateInternal(size_t size, uint32_t* physAddresses, bool bInterruptsEnabled)
{
	if (bInterruptsEnabled)
	{
		KeVerifyInterruptsEnabled;
		cli;
	}
	
	void *pMem = NULL;
	if (!physAddresses && size <= 1024)
	{
		// rely on the slab allocator:
		pMem = SlabAllocate(size);
	}
	else
	{
		pMem = MhAllocate(size, physAddresses);
	}
	
	if (bInterruptsEnabled)
		sti;
	
	return pMem;
}

void* MmAllocatePhy (size_t size, uint32_t* physAddresses)
{
	return MmAllocateInternal(size, physAddresses, true);
}

void* MmAllocate(size_t size)
{
	return MmAllocateInternal(size, NULL, true);
}

void* MmAllocateK(size_t size)
{
	return MmAllocateInternal(size, NULL, true);
}

void* MmAllocateID(size_t size)
{
	return MmAllocateInternal(size, NULL, false);
}

void* MmReAllocateID(void *oldPtr, size_t newSize)
{
	if (MmIsPartOfSlab(oldPtr))
	{
		int slabSize = SlabGetSize(oldPtr);
		
		// if we don't actually need to resize:
		if (SlabSizeToType((int)newSize) == SlabSizeToType(slabSize))
			return oldPtr;
		
		// I think it's fine if we just copy:
		void* pNewMem = MmAllocateInternal(newSize, NULL, false);
		
		memcpy(pNewMem, oldPtr, slabSize);
		
		SlabFree(oldPtr);
		
		return pNewMem;
	}
	else
	{
		return MhReAllocate(oldPtr, newSize);
	}
}

void* MmReAllocate(void *oldPtr, size_t newSize)
{
	KeVerifyInterruptsEnabled;
	
	cli;
	void* pMem = MmReAllocateID(oldPtr, newSize);
	sti;
	
	return pMem;
}

void* MmReAllocateK (void *oldPtr, size_t newSize)
{
	return MmReAllocate(oldPtr, newSize);
}

#endif

// MmFree*
void MmFreePage(void *pAddr)
{
	KeVerifyInterruptsEnabled;
	
	ASSERT(!MmIsPartOfSlab(pAddr));
	
	cli;
	MhFreePage(pAddr);
	sti;
}

void MmFreeID(void *pAddr)
{
	if (MmIsPartOfSlab(pAddr))
		SlabFree(pAddr);
	else
		MhFree(pAddr);
}

void MmFree(void *pAddr)
{
	KeVerifyInterruptsEnabled;
	
	// I use this to track down memory leaks easily
	//SLogMsg("%x => MhFree", pAddr);
	//PrintBackTrace(KeGetEBP(), KeGetEIP(), "bruh", NULL, false);
	
	cli;
	MmFreeID(pAddr);
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
