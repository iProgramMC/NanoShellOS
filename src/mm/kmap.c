//  ***************************************************************
//  mm/kmap.c - Creation date: 18/03/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mk (Memory manager, Kernel page mapping)
#include "memoryi.h"

KernelHeap g_KernelHeap;
extern uint32_t g_KernelPageDirectory[];

void MkSetUpKernelMapping()
{
	// Allocate our page tables
	uint32_t start  = 0x90000000 >> 22;
	uint32_t finish = 0xc0000000 >> 22;
	
	// make sure the kernel mapper's page tables are included in the page directory
	for (uint32_t i = start; i < finish; i++)
	{
		uint32_t phys = 0;
		g_KernelHeap.m_pPageTables[i - start] = MhAllocateSinglePage(&phys);
		
		g_KernelPageDirectory[i] = phys | PAGE_BIT_PRESENT | PAGE_BIT_READWRITE;
	}
}

uint32_t* MkiGetPageEntryAt(uintptr_t address)
{
	if (address < KERNEL_HEAP_DYNAMEM_START || address >= KERNEL_CODE_AND_DATA_START)
		return NULL;
	
	// Split the address up into chunks
	union
	{
		struct
		{
			uint32_t pageOffset: 12; //won't use this, it's there just to offset by 12 bytes
			uint32_t pageEntry : 10;
			uint32_t pageTable : 10;
		};
		uintptr_t address;
	}
	addressSplit;
	
	addressSplit.address = address;
	
	uint32_t startPageTable = KERNEL_HEAP_DYNAMEM_START >> 22;
	
	// Alright, grab a reference to the page entry, and return it.
	uint32_t* pPageEntry = &g_KernelHeap.m_pPageTables[addressSplit.pageTable - startPageTable]->m_pageEntries[addressSplit.pageEntry];
	return pPageEntry;
}

static bool MkiAreMappingParmsValid(uintptr_t start, size_t nPages)
{
	if (nPages >= KERNEL_CODE_AND_DATA_START - KERNEL_HEAP_DYNAMEM_START)
		return false;
	
	if (start < KERNEL_HEAP_DYNAMEM_START)
		return false;
	
	if (start + nPages * PAGE_SIZE >= KERNEL_CODE_AND_DATA_START)
		return false;
	
	return true;
}

static bool MkiIsMappingFree(uintptr_t start, size_t nPages)
{
	if (!MkiAreMappingParmsValid(start, nPages))
		return false;
	
	for (size_t i = 0; i < nPages; i++)
	{
		uint32_t* pPageEntry = MkiGetPageEntryAt(start + i * PAGE_SIZE);
		
		// assume that it's free if it's null
		if (pPageEntry)
		{
			if (*pPageEntry)
				return false;
		}
	}
	
	return true;
}

void MuiKillPageEntry(uint32_t* pPageEntry, uintptr_t address);

static bool MkiRemoveMapping(uintptr_t address)
{
	uint32_t * pPageEntry = MkiGetPageEntryAt(address);
	
	if (!pPageEntry)
		// if the page entry doesn't exist, return false to tell the caller
		return false;
	
	MuiKillPageEntry(pPageEntry, address);
	
	return true;
}

// Maps a chunk of memory, and forces a hint. If pPhysicalAddresses is null, allocate NEW pages.
// Otherwise, pPhysicalAddresses is treated as an ARRAY of physical page addresses of size `numPages`.
// If `bAllowClobbering` is on, all the previous mappings will be discarded. Dangerous!
// The `nDaiFlags` passed in only apply if the page is to be mapped as DAI (don't allocate instantly)
bool MkiMapMemoryFixedHint(uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, int clobberingLevel, bool bIsMMIO, uint32_t nDaiFlags)
{
	if (bIsMMIO && !pPhysicalAddresses)
	{
		//oh come on.
		bIsMMIO = false;
	}
	
	if (!MkiAreMappingParmsValid(hint, numPages))
		// can't map here!
		return false;
	
	if (clobberingLevel == CLOBBER_NO)
	{
		if (!MkiIsMappingFree(hint, numPages))
			// can't map here!
			return false;
	}
	
	// map here.
	size_t numPagesMappedSoFar = 0;
	for (numPagesMappedSoFar = 0; numPagesMappedSoFar < numPages; numPagesMappedSoFar++)
	{
		uint32_t* pPageEntry = MkiGetPageEntryAt(hint + numPagesMappedSoFar * PAGE_SIZE);
		if (!pPageEntry)
			goto _rollback;
		
		// If this is going to get clobbered..
		if (*pPageEntry & (PAGE_BIT_PRESENT | PAGE_BIT_DAI))
		{
			if (clobberingLevel == CLOBBER_ALL)
				// Reset it
				MkiRemoveMapping(hint + numPagesMappedSoFar * PAGE_SIZE); 
			else if (clobberingLevel == CLOBBER_SKIP)
				// Skip it
				continue;
			else
				// Fail
				goto _rollback;
		}
		
		if (pPhysicalAddresses)
		{
			*pPageEntry = (pPhysicalAddresses[numPagesMappedSoFar]) & PAGE_BIT_ADDRESS_MASK;
			*pPageEntry |= PAGE_BIT_PRESENT;
		}
		else
		{
			*pPageEntry |= PAGE_BIT_DAI | nDaiFlags;
		}
		
		if (bReadWrite)
			*pPageEntry |= PAGE_BIT_READWRITE;
		
		if (bIsMMIO)
			*pPageEntry |= PAGE_BIT_MMIO;
		
		MmInvalidateSinglePage(hint + numPagesMappedSoFar * PAGE_SIZE);
	}
	
	// Seems to have succeeded.
	return true;
	
_rollback:
	// Roll back our changes in case something went wrong during the mapping process
	for (size_t i = 0; i < numPagesMappedSoFar; i++)
	{
		MkiRemoveMapping(hint + i * PAGE_SIZE);
	}
	
	return false;
}

// Unmaps a chunk of memory that has been mapped.
bool MkiUnMap(uintptr_t mem, size_t numPages)
{
	ASSERT((mem & (PAGE_SIZE - 1)) == 0 && "The memory address passed in must be aligned with the page size.");
	
	// You can't unmap kernel space addresses from user heap functions..
	if (mem < KERNEL_BASE_ADDRESS)
	{
		return false;
	}
	
	// It is not an error if the indicated range does not contain any mapped pages.
	
	for (uintptr_t page = mem, i = numPages; i; page += PAGE_SIZE, i--)
	{
		MkiRemoveMapping(page);
	}
	
	return true;
}

bool MkMapMemoryFixedHint(uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, int clobberingLevel, bool bIsMMIO, uint32_t nDaiFlags)
{
	cli;
	bool result = MkiMapMemoryFixedHint(hint, numPages, pPhysicalAddresses, bReadWrite, clobberingLevel, bIsMMIO, nDaiFlags);
	sti;
	return result;
}

bool MkUnMap(uintptr_t mem, size_t numPages)
{
	cli;
	bool result = MkiUnMap(mem, numPages);
	sti;
	return result;
}

