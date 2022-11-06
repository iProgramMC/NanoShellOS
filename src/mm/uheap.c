//  ***************************************************************
//  mm/uheap.c - Creation date: 11/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mui (Memory manager, User heap, Internal)
// Namespace: Mu (Memory manager, User heap, Exposed)

#include <string.h>
#include <memory.h>
#include <errors.h>
#include "memoryi.h"

// THREADING: These functions are currently thread-unsafe. Please use the wrapper functions.
UserHeap* g_pCurrentUserHeap;

UserHeap* MuiGetCurrentHeap()
{
	return g_pCurrentUserHeap;
}

extern uint32_t g_KernelPageDirectory[];

// Creates a new (empty) heap structure.
UserHeap* MuCreateHeap()
{
	UserHeap *pHeap;
	if (KeCheckInterruptsDisabled())
	{
		pHeap = MhAllocate(sizeof(UserHeap), NULL);
	}
	else
	{
		pHeap = MmAllocateK(sizeof(UserHeap));
	}
	
	pHeap->m_nRefCount       = 1; // One reference. When this heap gets cloned, its reference count is increased.
	pHeap->m_nPageDirectory  = 0;
	pHeap->m_nMappingHint    = USER_HEAP_BASE;
	pHeap->m_lock.m_held     = false;
	pHeap->m_lock.m_task_owning_it = NULL;
	
	if (KeCheckInterruptsDisabled())
	{
		pHeap->m_pPageDirectory  = MhAllocateSinglePage(&pHeap->m_nPageDirectory);
	}
	else
	{
		pHeap->m_pPageDirectory  = MmAllocateSinglePagePhy(&pHeap->m_nPageDirectory);
	}
	
	memset(pHeap->m_pPageDirectory, 0, PAGE_SIZE);
	
	// Copy the latter half from the kernel heap.
	for (int i = 0x200; i < 0x400; i++)
		pHeap->m_pPageDirectory[i] = g_KernelPageDirectory[i];
	
	memset(pHeap->m_pPageTables, 0, sizeof(pHeap->m_pPageTables));
	
	return pHeap;
}

void MuiCreatePageTable(UserHeap *pHeap, int pageTable)
{
	// Reset the page table there.
	pHeap->m_pPageDirectory[pageTable] = 0;
	
	// Create a new page table page on the kernel heap.
	pHeap->m_pPageTables[pageTable] = MmAllocateSinglePagePhy(&pHeap->m_pPageDirectory[pageTable]);
	memset(pHeap->m_pPageTables[pageTable], 0, PAGE_SIZE);
	
	// Assign its bits, too.
	pHeap->m_pPageDirectory[pageTable] |= PAGE_BIT_PRESENT | PAGE_BIT_READWRITE;
}

void MuiKillPageEntry(uint32_t* pPageEntry, uintptr_t address)
{
	if (*pPageEntry & PAGE_BIT_PRESENT)
	{
		uint32_t memFrame = *pPageEntry & PAGE_BIT_ADDRESS_MASK;
		
		if (!(*pPageEntry & PAGE_BIT_MMIO))
		{
			MpClearFrame(memFrame);
			MrUnreferencePage(memFrame);
		}
		
		// Remove it!!!
		*pPageEntry = 0;
		
		if (address)
			MmInvalidateSinglePage(address);
	}
}

void MuiKillPageTablesEntries(PageTable* pPageTable)
{
	// Free each of the pages, if there are any
	for (int i = 0; i < 0x400; i++)
	{
		if (pPageTable->m_pageEntries[i] & PAGE_BIT_PRESENT)
		{
			MuiKillPageEntry(&pPageTable->m_pageEntries[i], 0);
		}
	}
}

void MuiRemovePageTable(UserHeap *pHeap, int pageTable)
{
	// If we have a page table there...
	if (pHeap->m_pPageTables[pageTable])
	{
		MuiKillPageTablesEntries(pHeap->m_pPageTables[pageTable]);
		
		// reset it to zero
		pHeap->m_pPageDirectory[pageTable] = 0;
		
		// Get rid of the page we've allocated for it.
		if (KeCheckInterruptsDisabled())
		{
			MhFree(pHeap->m_pPageTables[pageTable]);
		}
		else
		{
			MmFree(pHeap->m_pPageTables[pageTable]);
		}
		pHeap->m_pPageTables[pageTable] = NULL;
	}
}

uint32_t* MuiGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable)
{
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
	
	// Is there a page table there?
	if (addressSplit.pageTable >= 0x200) // is in the kernel's half
	{
		return NULL;
	}
	
	if (!pHeap->m_pPageTables[addressSplit.pageTable])
	{
		// No. Create one if needed.
		if (bGeneratePageTable)
			MuiCreatePageTable(pHeap, addressSplit.pageTable);
		else
			return NULL;
	}
	
	if (!pHeap->m_pPageTables[addressSplit.pageTable])
	{
		// Still not? Guess something failed then.
		return NULL;
	}
	
	// Alright, grab a reference to the page entry, and return it.
	uint32_t* pPageEntry = &pHeap->m_pPageTables[addressSplit.pageTable]->m_pageEntries[addressSplit.pageEntry];
	return pPageEntry;
}

// Creates a new structure and copies all of the pages of the previous one. Uses COW to achieve this.
// (Unfortunately, this won't work on the i386 itself, but works on i486 and up, and I'm not sure we
//  can get this running on an i386 without some hacks to get around the lack of a boot loader)
// TODO: Allow a compiler switch to disable CoW in case something weird was found that makes it impossible
UserHeap* MuiCloneHeap(UserHeap* pHeapToClone)
{
	UserHeap *pHeap = MuCreateHeap();
	
	// Clone each page table
	for (int i = 0x000; i < 0x200; i++)
	{
		if (pHeapToClone->m_pPageTables[i])
		{
			MuCreatePageTable(pHeap, i);
			
			for (int entry = 0x000; entry < 0x400; entry++)
			{
				uintptr_t vAddr = i << 22 | entry << 12;
				uint32_t* pEntryDst = MuiGetPageEntryAt(pHeap,        vAddr, true);
				uint32_t* pEntrySrc = MuiGetPageEntryAt(pHeapToClone, vAddr, true);
				
				if (*pEntrySrc & PAGE_BIT_DAI)
				{
					// Make the second page be demand-paged too. Because it doesn't have a
					// physical page frame assigned to it this should be fine.
					//SLogMsg("Page %p is demand paged", vAddr);
					*pEntryDst = *pEntrySrc;
				}
				else if (*pEntrySrc & PAGE_BIT_PRESENT)
				{
					//We don't have a way to track the reference count of individual pages.
					//So when the parent gets killed, the children will be working with a
					//freed page, which could potentially be overwritten, and uh oh.
					
					//This is fixed by keeping the parent alive until all the children have been killed too
					
					//ensure that the bit is clear
					
					// MMIO is inilligible for CoW
					if (*pEntrySrc & PAGE_BIT_MMIO)
					{
						SLogMsg("Page %p is MMIO", vAddr);
						// Make the second page also point to the MMIO stuff
						// MMIO is not subject to reference counting
						*pEntryDst = *pEntrySrc;
					}
					else
					{
						//use the same physical page as the source
						*pEntryDst = (*pEntrySrc & PAGE_BIT_ADDRESS_MASK) | PAGE_BIT_PRESENT;
						
						//increase the reference count, because another heap will point here too!
						MrReferencePage(*pEntrySrc & PAGE_BIT_ADDRESS_MASK);
						
						//if it's read-write, set the COW bit
						if (*pEntrySrc & PAGE_BIT_READWRITE)
						{
							*pEntryDst |= PAGE_BIT_COW;
							
							// also set it to the source, because otherwise, the source could modify
							// the page's contents for the destination
							*pEntrySrc |= PAGE_BIT_COW;
							*pEntrySrc &= ~PAGE_BIT_READWRITE;
							
							// If we make changes to the TLB, invalidate the page that was modified, because otherwise,
							// the heap that was cloned may sneak in a write after the clone, which will affect the clone
							MmInvalidateSinglePage((uintptr_t)vAddr);
						}
					}
				}
			}
		}
	}
	
	return pHeap;
}

// Returns whether the thread has been destroyed or not
bool MuiKillHeap(UserHeap* pHeap)
{
	if (g_pCurrentUserHeap == pHeap)
	{
		LogMsg("ERROR: Not allowed to delete currently used heap!");
		SLogMsg("ERROR: Not allowed to delete currently used heap!");
		KeStopSystem();
	}
	
	pHeap->m_nRefCount--;
	if (pHeap->m_nRefCount < 0)
	{
		// Uh oh
		LogMsg("Heap %p has reference count %d, uh oh", pHeap, pHeap->m_nRefCount);
		KeStopSystem();
	}
	
	if (pHeap->m_nRefCount == 0)
	{
		SLogMsg("Heap %p will be killed off now.", pHeap);
		// To kill the heap, first we need to empty all the page tables
		for (int i = 0; i < 0x200; i++)
		{
			if (pHeap->m_pPageTables[i])
			{
				MuiRemovePageTable(pHeap, i);
			}
		}
		
		if (KeCheckInterruptsDisabled())
		{
			MhFree(pHeap->m_pPageDirectory);
			MhFree(pHeap);
		}
		else
		{
			MmFree(pHeap->m_pPageDirectory);
			MmFree(pHeap);
		}
		
		return true;
	}
	else
	{
		SLogMsg("Heap %p won't die yet, because it's reference count is %d!", pHeap, pHeap->m_nRefCount);
		
		return false;
	}
}

// Create a mapping at `address` to point to `physAddress` in the user heap `pHeap`.
bool MuiCreateMapping(UserHeap *pHeap, uintptr_t address, uint32_t physAddress, bool bReadWrite)
{
	uint32_t* pPageEntry = MuiGetPageEntryAt(pHeap, address, true);
	
	if (!pPageEntry)
	{
		// If the page entry couldn't be created, return false because we could not map. :^(
		return false;
	}
	
	// Is there a page entry already?
	if (*pPageEntry & (PAGE_BIT_PRESENT | PAGE_BIT_DAI))
	{
		// Yeah... For obvious reasons we can't map here
		return false;
	}
	
	if (physAddress)
	{
		*pPageEntry = physAddress & PAGE_BIT_ADDRESS_MASK;
		*pPageEntry |= PAGE_BIT_PRESENT;
		if (bReadWrite)
			*pPageEntry |= PAGE_BIT_READWRITE;
		
		SLogMsg("Adding reference");
		
		MrReferencePage(physAddress);
	}
	else
	{
		*pPageEntry = PAGE_BIT_DAI;
	}
	
	// WORK: This might not actually be needed if TLB doesn't actually cache non-present pages. Better to be on the safe side, though
	MmInvalidateSinglePage(address);
	
	return true;
}

bool MuiRemoveMapping(UserHeap *pHeap, uintptr_t address)
{
	uint32_t* pPageEntry = MuiGetPageEntryAt(pHeap, address, false);
	
	if (!pPageEntry)
	{
		// If the page entry doesn't exist, return false to tell the caller
		return false;
	}
	
	// A page entry was allocated here, and it has been copied-on-write
	MuiKillPageEntry (pPageEntry, address);
	
	return true;
}

// Checks if a specified set of mapping parameters is valid.
bool MuAreMappingParmsValid(uintptr_t start, size_t nPages)
{
	// If the size is bigger than 2 GB, there's no way we're able to map this.
	if (nPages >= 0x80000000)
		return false;
	
	// If it goes beyond the user heap's 2 GB of memory space, that means that we shouldn't map there.
	if (start + nPages * PAGE_SIZE >= KERNEL_HEAP_BASE)
		return false;
	
	return true;
}

// Checks if a continuous chain of mappings is free.
bool MuiIsMappingFree(UserHeap *pHeap, uintptr_t start, size_t nPages)
{
	if (!MuAreMappingParmsValid (start, nPages))
		return false;
	
	for (size_t i = 0; i < nPages; i++)
	{
		uint32_t* pPageEntry = MuiGetPageEntryAt(pHeap, start + i * PAGE_SIZE, false);
		
		// assume that it's free if that's null
		if (pPageEntry)
		{
			if (*pPageEntry & PAGE_BIT_PRESENT)
				return false;
		}
	}
	return true;
}

// Maps a chunk of memory, and forces a hint. If pPhysicalAddresses is null, allocate NEW pages.
// Otherwise, pPhysicalAddresses is treated as an ARRAY of physical page addresses of size `numPages`.
// If `bAllowClobbering` is on, all the previous mappings will be discarded. Dangerous!
bool MuiMapMemoryFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, bool bAllowClobbering, bool bIsMMIO)
{
	if (bIsMMIO && !pPhysicalAddresses)
	{
		//oh come on, you can't ask me to allocate pages that will never be freed!  :(
		bIsMMIO = false;
	}
	
	if (bAllowClobbering)
	{
		if (!MuAreMappingParmsValid(hint, numPages))
			// can't map here!
			return false;
	}
	else
	{
		if (!MuiIsMappingFree(pHeap, hint, numPages))
			// can't map here!
			return false;
	}
	
	// map here.
	size_t numPagesMappedSoFar = 0;
	for (numPagesMappedSoFar = 0; numPagesMappedSoFar < numPages; numPagesMappedSoFar++)
	{
		uint32_t* pPageEntry = MuiGetPageEntryAt(pHeap, hint + numPagesMappedSoFar * PAGE_SIZE, true);
		if (!pPageEntry)
			goto _rollback;
		
		// If this is going to get clobbered..
		if (*pPageEntry & (PAGE_BIT_PRESENT | PAGE_BIT_DAI))
		{
			if (!bAllowClobbering)
				goto _rollback;
			
			// Reset it
			MuiRemoveMapping(pHeap, hint + numPagesMappedSoFar * PAGE_SIZE); 
		}
		
		if (pPhysicalAddresses)
		{
			*pPageEntry = (pPhysicalAddresses[numPagesMappedSoFar]) & PAGE_BIT_ADDRESS_MASK;
			*pPageEntry |= PAGE_BIT_PRESENT;
		}
		else
		{
			*pPageEntry |= PAGE_BIT_DAI;
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
		MuiRemoveMapping(pHeap, hint + i * PAGE_SIZE);
	}
	
	return false;
}

uintptr_t MuiFindPlaceAroundHint(UserHeap *pHeap, uintptr_t hint, size_t numPages)
{
	//OPTIMIZE oh come on, optimize this - iProgramInCpp
	
	//start from the `hint` address
	uintptr_t end = KERNEL_HEAP_BASE - numPages;
	for (uintptr_t searchHead = hint; searchHead < end; searchHead++)
	{
		if (MuiIsMappingFree(pHeap, searchHead, numPages))
			return searchHead;
	}
	
	//start from the user heap base address
	for (uintptr_t searchHead = USER_HEAP_BASE; searchHead < hint; searchHead++)
	{
		if (MuiIsMappingFree(pHeap, searchHead, numPages))
			return searchHead;
	}
	
	//No space left? hrm.
	return 0;
}

// Maps a chunk of memory with a hint address, but not fixed, so it can diverge a little bit.
bool MuiMapMemoryNonFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO)
{
	// find a place to map, based on the hint
	uintptr_t newHint = MuiFindPlaceAroundHint(pHeap, hint, numPages);
	
	if (newHint == 0) return false;
	
	*pAddressOut = (void*)newHint;
	
	return MuiMapMemoryFixedHint(pHeap, newHint, numPages, pPhysicalAddresses, bReadWrite, false, bIsMMIO);
}

// Maps a chunk a memory without a hint address.
bool MuiMapMemory(UserHeap *pHeap, size_t numPages, uint32_t* pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO)
{
	void* address;
	
	bool bResult = MuiMapMemoryNonFixedHint(pHeap, pHeap->m_nMappingHint, numPages, pPhysicalAddresses, &address, bReadWrite, bIsMMIO);
	if (!bResult)
		return false;
	
	// update the hint
	pHeap->m_nMappingHint = (uintptr_t)address + PAGE_SIZE * numPages;
	
	if (pHeap->m_nMappingHint >= KERNEL_HEAP_BASE)
		pHeap->m_nMappingHint  = USER_HEAP_BASE;
	
	*pAddressOut = address;
	
	return true;
}

// Unmaps a chunk of memory that has been mapped.
bool MuiUnMap (UserHeap *pHeap, uintptr_t mem, size_t numPages)
{
	ASSERT((mem & (PAGE_SIZE - 1)) == 0 && "The memory address passed in must be aligned with the page size.");
	
	// You can't unmap kernel space addresses from user heap functions..
	if (mem >= KERNEL_BASE_ADDRESS || (mem + numPages) >= KERNEL_BASE_ADDRESS)
	{
		return false;
	}
	
	// It is not an error if the indicated range does not contain any mapped pages.
	
	for (uintptr_t page = mem, i = numPages; i; page += PAGE_SIZE, i--)
	{
		MuiRemoveMapping(pHeap, page);
	}
	
	return true;
}

// Stop using the user heap, switch to the basic kernel heap.
void MuiResetHeap()
{
	g_pCurrentUserHeap = NULL;
	
	MmUsePageDirectory((uintptr_t)g_KernelPageDirectory - KERNEL_BASE_ADDRESS);
}

// Allows usage of the user heap. Changes CR3 to the user heap's page directory.
void MuiUseHeap (UserHeap* pHeap)
{
	if (g_pCurrentUserHeap)
		MuiResetHeap();
	
	g_pCurrentUserHeap = pHeap;
	
	if (!pHeap) return;
	
	MmUsePageDirectory((uintptr_t)pHeap->m_nPageDirectory);
}

void MuUseHeap (UserHeap *pHeap)
{
	cli;
	MuiUseHeap(pHeap);
	sti;
}

void MuResetHeap()
{
	cli;
	MuiResetHeap();
	sti;
}

// THREAD-SAFE WRAPPERS
UserHeap* MuGetCurrentHeap()
{
	return MuiGetCurrentHeap();
}

UserHeap* MuCloneHeap(UserHeap* pHeapToClone)
{
	// Lock the heap to clone's lock
	LockAcquire (&pHeapToClone->m_lock);
	
	UserHeap* pClonedHeap = MuiCloneHeap (pHeapToClone);
	
	LockFree (&pHeapToClone->m_lock);
	return pClonedHeap;
}

void MuKillHeap(UserHeap *pHeap)
{
	LockAcquire (&pHeap->m_lock);
	if (!MuiKillHeap(pHeap))
		LockFree(&pHeap->m_lock);
}

uint32_t* MuGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable)
{
	LockAcquire (&pHeap->m_lock);
	uint32_t* ptr = MuiGetPageEntryAt(pHeap, address, bGeneratePageTable);
	LockFree (&pHeap->m_lock);
	return ptr;
}

bool MuCreateMapping(UserHeap *pHeap, uintptr_t address, uint32_t physAddress, bool bReadWrite)
{
	LockAcquire (&pHeap->m_lock);
	bool res = MuiCreateMapping(pHeap, address, physAddress, bReadWrite);
	LockFree (&pHeap->m_lock);
	return res;
}

bool MuIsMappingFree(UserHeap *pHeap, uintptr_t start, size_t nPages)
{
	LockAcquire (&pHeap->m_lock);
	bool res = MuiIsMappingFree(pHeap, start, nPages);
	LockFree (&pHeap->m_lock);
	return res;
}

bool MuMapMemory(UserHeap *pHeap, size_t numPages, uint32_t* pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO)
{
	LockAcquire (&pHeap->m_lock);
	bool res = MuiMapMemory(pHeap, numPages, pPhysicalAddresses, pAddressOut, bReadWrite, bIsMMIO);
	LockFree (&pHeap->m_lock);
	return res;
}

bool MuMapMemoryNonFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO)
{
	LockAcquire (&pHeap->m_lock);
	bool res = MuiMapMemoryNonFixedHint(pHeap, hint, numPages, pPhysicalAddresses, pAddressOut, bReadWrite, bIsMMIO);
	LockFree (&pHeap->m_lock);
	return res;
}

bool MuMapMemoryFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, bool bAllowClobbering, bool bIsMMIO)
{
	LockAcquire (&pHeap->m_lock);
	bool res = MuiMapMemoryFixedHint(pHeap, hint, numPages, pPhysicalAddresses, bReadWrite, bAllowClobbering, bIsMMIO);
	LockFree (&pHeap->m_lock);
	return res;
}

void MuCreatePageTable(UserHeap *pHeap, int pageTable)
{
	LockAcquire (&pHeap->m_lock);
	MuiCreatePageTable(pHeap, pageTable);
	LockFree (&pHeap->m_lock);
}

void MuRemovePageTable(UserHeap *pHeap, int pageTable)
{
	LockAcquire (&pHeap->m_lock);
	MuiRemovePageTable(pHeap, pageTable);
	LockFree (&pHeap->m_lock);
}

bool MuRemoveMapping(UserHeap *pHeap, uintptr_t address)
{
	LockAcquire (&pHeap->m_lock);
	bool res = MuiRemoveMapping(pHeap, address);
	LockFree (&pHeap->m_lock);
	return res;
}

bool MuUnMap(UserHeap *pHeap, uintptr_t address, size_t numPages)
{
	LockAcquire (&pHeap->m_lock);
	bool b = MuiUnMap (pHeap, address, numPages);
	LockFree (&pHeap->m_lock);
	return b;
}

// User exposed functions

int MmMapMemoryUser(void *pAddr, size_t lengthBytes, int protectionFlags, int mapFlags, UNUSED int fileDes, UNUSED size_t fileOffset, void **pOut)
{
	if (!(mapFlags & MAP_ANON))
	{
		SLogMsg("MmMapMemoryUser tried to map file-backed memory, but that isn't supported!");
		*pOut = MAP_FAILED;
		
		return ERR_INVALID_PARM; //for now
	}
	
	// check page alignment
	if (((uintptr_t)pAddr & (PAGE_SIZE - 1)) != 0)
	{
		*pOut = MAP_FAILED;
		
		return ERR_INVALID_PARM;
	}
	
	UserHeap *pHeap = MuGetCurrentHeap();
	if (!pHeap)
	{
		SLogMsg("There's no heap available here?");
		*pOut = MAP_FAILED;
		
		return ERR_INVALID_PARM;
	}
	
	// Get the number of pages required
	size_t numPages = ((lengthBytes - 1) / PAGE_SIZE) + 1;
	bool bWrite = protectionFlags & PROT_WRITE;
	
	if (mapFlags & MAP_FIXED)
	{
		bool bAllowClobbering = !(mapFlags & MAP_DONTREPLACE);
		
		bool bResult = MuMapMemoryFixedHint(pHeap, (uintptr_t)pAddr, numPages, NULL, bWrite, bAllowClobbering, false);
		
		if (!bResult)
		{
			*pOut = MAP_FAILED;
			
			if (bAllowClobbering)
				return ERR_NO_MEMORY;
			else
				return ERR_FILE_EXISTS; // EEXIST
		}
		else
		{
			return ERR_NOTHING; // Success!
		}
	}
	// Take the pAddr parm as a hint
	else if (pAddr)
	{
		bool b = MuMapMemoryNonFixedHint(pHeap, (uintptr_t)pAddr, numPages, NULL, pOut, bWrite, false);
		if (!b)
		{
			*pOut = MAP_FAILED;
			return ERR_NO_MEMORY;
		}
		else
		{
			return ERR_NOTHING; // Success!
		}
	}
	else
	{
		bool b = MuMapMemory(pHeap, numPages, NULL, pOut, bWrite, false);
		if (!b)
		{
			*pOut = MAP_FAILED;
			return ERR_NO_MEMORY;
		}
		else
		{
			return ERR_NOTHING; // Success!
		}
	}
}

int MmUnMapMemoryUser(void *pAddr, size_t lengthBytes)
{
	// Get the number of pages required
	size_t numPages = ((lengthBytes - 1) / PAGE_SIZE) + 1;
	
	UserHeap *pHeap = MuGetCurrentHeap();
	if (!pHeap)
	{
		SLogMsg("There's no heap available here?");
		return ERR_INVALID_PARM;
	}
	
	bool bResult = MuUnMap (pHeap, (uintptr_t)pAddr, numPages);
	
	if (bResult)
		return ERR_INVALID_PARM;
	
	return ERR_NOTHING; // Success!
}


