//  ***************************************************************
//  a_mem.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

// If the free gap has between <size> and <size> + 32 bytes, don't split it. It's kind of a waste.
// If the free gap's size is <size> + 32 bytes and over, proceed to perform a split.
#define C_MEM_ALLOC_TOLERANCE (32)

// If a newly freed block of memory is larger than 1024 bytes, g_bAddedToLastHeaderHint gets set to
// false.. So new allocations would try that spot first.
#define C_MEM_FREE_LARGE_ENOUGH (4096)

#define MAGIC_NUMBER_1_USED (0xDDEAFB10)
#define MAGIC_NUMBER_2_USED (0x19960623)
#define MAGIC_NUMBER_1_FREE (0x00000000)
#define MAGIC_NUMBER_2_FREE (0x534F534E)

// since MAGIC_NUMBER_1_FREE is zero
#define IS_FREE(header) (!((header)->m_magicNo1))

typedef struct MemAreaHeader
{
	uint32_t m_magicNo1;
	struct MemAreaHeader* m_pNext;
	struct MemAreaHeader* m_pPrev;
	size_t m_size;
	uint32_t m_magicNo2;
}
MemAreaHeader;

// The memory area. Prefer imitating old NanoShell's way of doing things.
uint8_t*  gMemory = (uint8_t*)0x40000000;

// The size of the memory area.
const uintptr_t gMemorySize = 0x40000000; // 1 GB. Should you need more, move gMemory down in memory (but avoid going below 0x10000000!!)

// 262144 entries, so 512 KB. Not Bad. Stores the amount of memory allocations that use this page.
uint16_t  gMemoryPageReference [gMemorySize / 4096];

MemAreaHeader *gLastHeader;

// for optimization, add to the last header if we've been doing that
// Not sure that it optimizes that much actually.. only a 2ms gain at best
bool g_bAddedToLastHeaderHint = false;

bool MemMgrIsPageUsed(uintptr_t page)
{
	return gMemoryPageReference[page] != 0;
}

void MemOnOOM(int errCode, bool bUnmappingMemory);

void MemMgrRequestMemMap(uintptr_t place, size_t size)
{
	void *redundant;
	int errorCode = MemoryMap((void*)(place & ~0xFFF), size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, 0, 0, &redundant);
	
	if (errorCode != -ENOTHING)
	{
		MemOnOOM(errorCode, false);
	}
}

void MemMgrRequestMemUnMap(uintptr_t place, size_t size)
{
	__attribute__((unused)) int errorCode =
	MemoryUnmap((void*)(place & ~0xFFF), size);
}

// don't use this directly
void MemMgrAddReferenceToPage(uintptr_t page)
{
	gMemoryPageReference[page]++;
}

// this also
void MemMgrRemoveReferenceToPage(uintptr_t page)
{
	if (gMemoryPageReference[page]-- == 0)
	{
		gMemoryPageReference[page] = 0;
		LogMsg("Tried to free the same page twice? (%p)", page);
		exit(-1);
	}
}

MemAreaHeader* MemMgrSetupMemoryRegion(MemAreaHeader* pHeader, size_t nSize)
{
	pHeader->m_magicNo1 = MAGIC_NUMBER_1_USED;
	pHeader->m_magicNo2 = MAGIC_NUMBER_2_USED;
	pHeader->m_size	 = nSize;

	MemAreaHeader* pNext = (MemAreaHeader*)((uint8_t*)pHeader + sizeof *pHeader + nSize);
	pHeader->m_pNext	= pNext;
	pNext  ->m_pPrev	= pHeader;
	pNext  ->m_pNext	= NULL;

	return pNext;
}

MemAreaHeader* MemMgrGetInitialHeader()
{
	return (MemAreaHeader*)gMemory;
}

void MemMgrPerformSanityChecks(MemAreaHeader* pHeader)
{
	// Perform an address check first - make sure we aren't performing any OOB accesses.
	uint8_t* pHeaderBytes = (uint8_t*)pHeader;
	if (gMemory + gMemorySize <= pHeaderBytes || pHeaderBytes < gMemory)
	{
		LogMsg("Heap corruption detected! Block %p isn't within the memory area reserved to the program.", pHeader);
		exit(1);
	}

	// Perform a magic number check
	if ((pHeader->m_magicNo1 != MAGIC_NUMBER_1_FREE && pHeader->m_magicNo1 != MAGIC_NUMBER_1_USED) ||
		(pHeader->m_magicNo2 != MAGIC_NUMBER_2_FREE && pHeader->m_magicNo2 != MAGIC_NUMBER_2_USED))
	{
		// Uh oh! We have a heap corruption. Report to the user!
		LogMsg("Heap corruption detected! Block %p's magic numbers aren't correctly set.", pHeader);
		exit(1);
	}

	// Yep, all good
}

void MemMgrUseMemoryRegion(void* ptr, size_t sz)
{
	uintptr_t p = (uintptr_t) ptr - (uintptr_t) gMemory;
	uintptr_t pageStart = p / 0x1000, pageEnd = (p + sz) / 0x1000;
	uintptr_t mmapStreak = 0, pageStartedStreak = 0;

	for (uintptr_t page = pageStart; page <= pageEnd; page++)
	{
		//if the page isn't used, add to the current streak
		if (!MemMgrIsPageUsed(page))
		{
			if (mmapStreak++ == 0)
				pageStartedStreak = page;
		}
		//well, it's used, so commit the current changes if we have a streak
		else if (mmapStreak)
		{
			uintptr_t mmapCur = pageStartedStreak * 0x1000 + (uintptr_t) gMemory;
			MemMgrRequestMemMap(mmapCur, mmapStreak * 4096);
			mmapStreak = 0;
		}
		MemMgrAddReferenceToPage(page);
	}

	//well, in the end we might still have some streak left, so be sure to map that too.
	if (mmapStreak)
	{
		if (!pageStartedStreak)
			pageStartedStreak = pageStart;
		
		uintptr_t mmapCur = pageStartedStreak * 0x1000 + (uintptr_t) gMemory;
		MemMgrRequestMemMap(mmapCur, mmapStreak * 4096);
		mmapStreak = 0;
	}
}

void MemMgrFreeMemoryRegion(void* ptr, size_t sz)
{
	uintptr_t p = (uintptr_t) ptr - (uintptr_t) gMemory;
	uintptr_t pageStart = p / 0x1000, pageEnd = (p + sz) / 0x1000;
	uintptr_t mmapStreak = 0, pageStartedStreak = 0;

	for (uintptr_t page = pageStart; page <= pageEnd; page++)
	{
		//if the page isn't used, add to the current streak
		if (!MemMgrIsPageUsed(page))
		{
			if (mmapStreak++ == 0)
				pageStartedStreak = page;
		}
		//well, it's used, so commit the current changes if we have a streak
		else if (mmapStreak)
		{
			uintptr_t mmapCur = pageStartedStreak * 0x1000 + (uintptr_t) gMemory;
			MemMgrRequestMemUnMap(mmapCur, mmapStreak);
			mmapStreak = 0;
		}
		MemMgrRemoveReferenceToPage(page);
	}

	//well, in the end we might still have some streak left, so be sure to unmap that too.
	if (mmapStreak)
	{
		if (!pageStartedStreak)
			pageStartedStreak = pageStart;
		
		uintptr_t mmapCur = pageStartedStreak * 0x1000 + (uintptr_t) gMemory;
		MemMgrRequestMemUnMap(mmapCur, mmapStreak * 4096);
		mmapStreak = 0;
	}
}

// Initialise the first block.
void MemMgrInitializeMemory()
{
	MemAreaHeader *pHeader = MemMgrGetInitialHeader();
	
	MemMgrUseMemoryRegion(pHeader, sizeof (MemAreaHeader));
	gLastHeader = pHeader;

	pHeader->m_magicNo1 = MAGIC_NUMBER_1_FREE;
	pHeader->m_magicNo2 = MAGIC_NUMBER_2_FREE;
	pHeader->m_pNext = NULL;
	pHeader->m_pPrev = NULL;
	pHeader->m_size  = gMemorySize - sizeof(MemAreaHeader);
}

// First-fit allocation method. We could also use best-fit, however right now I'd rather not.
void* MemMgrAllocateMemory(size_t sz)
{
	if (g_bAddedToLastHeaderHint)
	{
		// try adding to the last header first. If that doesn't work, resort to
		// the method below. Switch this off if we're dangerously low on memory space
		MemAreaHeader* pHeader = gLastHeader;

		//if we don't have a last header, we've hit the boundary
		if (gLastHeader)
		{
			if (pHeader->m_size >= sz)
			{
				//TODO figure out a way to not repeat myself

				// This is good. Now, do we split this up?
				if (pHeader->m_size >= sz + sizeof (MemAreaHeader) + C_MEM_ALLOC_TOLERANCE)
				{
					// Yes. Proceed to split.
					uint8_t* pMem = (uint8_t*)&pHeader[1] + sz;

					// This will be the location of the new memory header.
					MemAreaHeader* pNewHdr = (MemAreaHeader*)pMem;

					// Mark its position as used.
					MemMgrUseMemoryRegion(pNewHdr, sizeof (MemAreaHeader));
					
					// Initialize its magic bits.
					pNewHdr->m_magicNo1 = MAGIC_NUMBER_1_FREE;
					pNewHdr->m_magicNo2 = MAGIC_NUMBER_2_FREE;
					pNewHdr->m_size	 = pHeader->m_size - sizeof(MemAreaHeader) - sz;
					pHeader->m_size	 = sz;
					// Link it up with the nodes in between
					pNewHdr->m_pNext	= pHeader->m_pNext;
					if (pHeader->m_pNext) pHeader->m_pNext->m_pPrev = pNewHdr;
					pNewHdr->m_pPrev	= pHeader;
					pHeader->m_pNext	= pNewHdr;

					gLastHeader = pNewHdr;
				}
				else
				{
					// Don't split it.
					gLastHeader = NULL;
				}

				// Update this header's magic numbers, to mark this block used.
				pHeader->m_magicNo1 = MAGIC_NUMBER_1_USED;
				pHeader->m_magicNo2 = MAGIC_NUMBER_2_USED;

				g_bAddedToLastHeaderHint = true;

				MemMgrUseMemoryRegion(pHeader, sz + sizeof (MemAreaHeader));

				// return the memory right after the header.
				return &pHeader[1];
			}
		}
	}

	// Okay, first off, navigate our linked list
	MemAreaHeader* pHeader = MemMgrGetInitialHeader(), *pLastHeader = NULL;

	while (pHeader)
	{
		// Perform a sanity check first, before doing *anything*. Has its reasons.
		MemMgrPerformSanityChecks(pHeader);

		// is this even free?
		if (IS_FREE(pHeader))
		{
			// yeah, simply check if we have a free slot here.
			if (pHeader->m_size >= sz)
			{
				// This is good. Now, do we split this up?
				if (pHeader->m_size >= sz + sizeof (MemAreaHeader) + C_MEM_ALLOC_TOLERANCE)
				{
					// Yes. Proceed to split.
					uint8_t* pMem = (uint8_t*)&pHeader[1] + sz;

					// This will be the location of the new memory header.
					MemAreaHeader* pNewHdr = (MemAreaHeader*)pMem;
					
					// Map it in memory.
					MemMgrUseMemoryRegion(pNewHdr, sizeof (MemAreaHeader));
					
					// Initialize its magic bits.
					pNewHdr->m_magicNo1 = MAGIC_NUMBER_1_FREE;
					pNewHdr->m_magicNo2 = MAGIC_NUMBER_2_FREE;
					pNewHdr->m_size	 = pHeader->m_size - sizeof(MemAreaHeader) - sz;
					pHeader->m_size	 = sz;
					// Link it up with the nodes in between
					pNewHdr->m_pNext	= pHeader->m_pNext;
					if (pHeader->m_pNext) pHeader->m_pNext->m_pPrev = pNewHdr;
					pNewHdr->m_pPrev	= pHeader;
					pHeader->m_pNext	= pNewHdr;
					
					if (pHeader == gLastHeader)
					{
						gLastHeader = pNewHdr;
						g_bAddedToLastHeaderHint = true;
					}
					else
					{
						g_bAddedToLastHeaderHint = false;
					}
				}
				else
				{
					// Don't split it.

					if (pHeader == gLastHeader)
					{
						gLastHeader = NULL;
						g_bAddedToLastHeaderHint = false;
					}
				}
				
				// Update this header's magic numbers, to mark this block used.
				pHeader->m_magicNo1 = MAGIC_NUMBER_1_USED;
				pHeader->m_magicNo2 = MAGIC_NUMBER_2_USED;
				
				MemMgrUseMemoryRegion(pHeader, sz + sizeof (MemAreaHeader));
				
				// return the memory right after the header.
				return &pHeader[1];
			}

			// Nope! Simply skip, as done below.
		}

		pLastHeader = pHeader;
		pHeader = pHeader->m_pNext;
	}

	// try to expand. If doesn't work, we have run into an out of memory situation.
	uint8_t* pMem = NULL;

	if (pLastHeader)
	{
		pMem = (uint8_t*)&pLastHeader[1] + pLastHeader->m_size;
	}
	else
	{
		LogMsg("ERROR: No pLastHeader? (line %d)", __LINE__);
		exit(-1);
	}

	if (pMem + sz >= gMemory + gMemorySize)
	{
		// yikes!
		LogMsg("Out of memory area! (trying to allocate size %d)", sz);
		return NULL;
	}

	assert(pLastHeader->m_pNext == NULL);

	// This will be the location of the new memory header.
	MemAreaHeader* pNewHdr = (MemAreaHeader*)pMem;

	// Map the relevant memory area
	MemMgrUseMemoryRegion(pNewHdr, sizeof (MemAreaHeader));

	// update the bits
	pNewHdr->m_magicNo1  = MAGIC_NUMBER_1_USED;
	pNewHdr->m_magicNo2  = MAGIC_NUMBER_2_USED;
	pNewHdr->m_pPrev	 = pLastHeader;
	pNewHdr->m_pNext	 = NULL;
	pLastHeader->m_pNext = pNewHdr;
	pNewHdr->m_size	  = sz;

	gLastHeader = pNewHdr;

	g_bAddedToLastHeaderHint = true;
	
	MemMgrUseMemoryRegion(pNewHdr, sz + sizeof (MemAreaHeader));

	return &pNewHdr[1];
}

void MemMgrFreeMemory(void *pMem)
{
	// get the header right before the memory with some cool syntax tricks
	MemAreaHeader* pHeader = & (-1)[(MemAreaHeader*)pMem];

	// Ensure the sanity of this header
	MemMgrPerformSanityChecks(pHeader);

	// already free?
	if (pHeader->m_magicNo1 == MAGIC_NUMBER_1_FREE && pHeader->m_magicNo2 == MAGIC_NUMBER_2_FREE)
	{
		LogMsg("ERROR: double free attempt at %p", pMem);
		exit(-1);
	}

	// mark it as free
	pHeader->m_magicNo1 = MAGIC_NUMBER_1_FREE;
	pHeader->m_magicNo2 = MAGIC_NUMBER_2_FREE;

	MemMgrFreeMemoryRegion(pHeader, pHeader->m_size + sizeof(MemAreaHeader));

	// if it's sufficiently large, we may want to stop allocating right at the end of our heap
	if (pHeader->m_size >= C_MEM_FREE_LARGE_ENOUGH)
	{
		g_bAddedToLastHeaderHint = false;
	}

	// try to combine with other free slots
	MemAreaHeader* pNext = pHeader->m_pNext;

	// Does it exist, and is it free?
	if (pNext && IS_FREE(pNext))
	{
		// yeah. Merge this and the next together.
		pHeader->m_size += pNext->m_size + sizeof (MemAreaHeader);
		pHeader->m_pNext = pNext->m_pNext;
		if (pNext->m_pNext) pNext->m_pNext->m_pPrev = pHeader;

		// well, pNext is no longer valid, get rid of its magic numbers.
		pNext->m_magicNo1 = 0;
		pNext->m_magicNo2 = 0;

		// mark it as unused
		MemMgrFreeMemoryRegion(pNext, 1);
	}

	MemAreaHeader* pPrev = pHeader->m_pPrev;

	// Does it exist? Is it free?
	if (pPrev && IS_FREE(pPrev))
	{
		// yeah. Merge this and the previous together.
		pPrev->m_size += pHeader->m_size + sizeof (MemAreaHeader);
		pPrev->m_pNext = pHeader->m_pNext;
		if (pHeader->m_pNext) pPrev->m_pNext->m_pPrev = pPrev;

		// well our pHeader is no longer valid, get rid of its magic numbers.
		pHeader->m_magicNo1 = 0;
		pHeader->m_magicNo2 = 0;

		// mark it as unused
		MemMgrFreeMemoryRegion(pHeader, 1);
	}
}

void MemMgrDebugDump()
{
	MemAreaHeader* pHeader = MemMgrGetInitialHeader();

	while (pHeader)
	{
		// Perform a sanity check first, before doing *anything*. Has its reasons.
		MemMgrPerformSanityChecks(pHeader);

		LogMsg("Header: %p. Next: %p, Prev: %p. Size: %zu.", pHeader, pHeader->m_pNext, pHeader->m_pPrev, pHeader->m_size);

		pHeader = pHeader->m_pNext;
	}

	LogMsg("Memory page reference: ");

	for (int i = 0; i < 50; i++)
		LogMsgNoCr("%x", gMemoryPageReference[i]);

	LogMsgNoCr("\n");
}

void MemMgrCleanup()
{
	// I think we can just do this. Pages whose page mapping isn't set are ignored.
	MemMgrRequestMemUnMap((uintptr_t)gMemory, gMemorySize);
}

// On out of memory
void MemOnOOM(int errCode, bool bUnmappingMemory)
{
	LogMsg("ERROR: ran out of memory (?). Was%s unmapping, error code: %d. Will now quit.", bUnmappingMemory ? "" : "n't", errCode);
	
	exit(-1);
}

void _I_FreeEverything()
{
	MemMgrCleanup();
}

void *malloc (size_t sz)
{
	return MemMgrAllocateMemory(sz);
}

void free (void* pMem)
{
	return MemMgrFreeMemory(pMem);
}

// Allocating kernel regions, if needed. Will cause a memory leak if not disposed of properly, so use carefully!!!
void* MmKernelAllocate(size_t sz)
{
	return _I_AllocateDebug(sz, __FILE__, __LINE__);
}

void MmKernelFree(void *pData)
{
	_I_Free(pData);
}
