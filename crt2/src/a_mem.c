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

void SLogMsg(const char * c, ...);
void SLogMsgNoCr(const char* fmt, ...);

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

#define C_MEMORY_SIZE (0x40000000)  // 1 GB. Should you need more, move gMemory down in memory (but avoid going below 0x10000000!!)

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

#define SIZE_AND_LOCATION_PADDING (1 << 2) // 4

// The memory area. Prefer imitating old NanoShell's way of doing things.
uint8_t*  gMemory = (uint8_t*)0x40000000;

// The size of the memory area.
const uintptr_t gMemorySize = C_MEMORY_SIZE;

// 262144 entries, so 512 KB. Not Bad. Stores the amount of memory allocations that use this page.
uint16_t  gMemoryPageReference [C_MEMORY_SIZE / 4096];

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

void MemMgrAbort()
{
	abort();
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
		abort();
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
		MemMgrAbort();
	}

	// Perform a magic number check
	if ((pHeader->m_magicNo1 != MAGIC_NUMBER_1_FREE && pHeader->m_magicNo1 != MAGIC_NUMBER_1_USED) ||
		(pHeader->m_magicNo2 != MAGIC_NUMBER_2_FREE && pHeader->m_magicNo2 != MAGIC_NUMBER_2_USED))
	{
		// Uh oh! We have a heap corruption. Report to the user!
		LogMsg("Heap corruption detected! Block %p's magic numbers aren't correctly set.", pHeader);
		MemMgrAbort();
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
	if (sz == 0)
		return NULL;
	
	// Pad the size to four bytes, or one double word.
	sz = (sz + SIZE_AND_LOCATION_PADDING - 1) & ~(SIZE_AND_LOCATION_PADDING - 1);
	
	// TODO OPTIMIZE - Store a pointer to the biggest block that's free right now, or something else like that.

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
		MemMgrAbort();
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
	if (pMem == NULL)
		return;
	
	// check the padding first
	if (((uintptr_t)pMem & (SIZE_AND_LOCATION_PADDING - 1)) != 0)
	{
		LogMsg("Error: Address passed in is NOT padded to %d!", SIZE_AND_LOCATION_PADDING);
		abort();
	}
	
	// get the header right before the memory with some cool syntax tricks
	MemAreaHeader* pHeader = & (-1)[(MemAreaHeader*)pMem];
	
	if (gLastHeader == pHeader)
	{
		gLastHeader = NULL;
		g_bAddedToLastHeaderHint = false;
	}

	// Ensure the sanity of this header
	MemMgrPerformSanityChecks(pHeader);

	// already free?
	if (pHeader->m_magicNo1 == MAGIC_NUMBER_1_FREE && pHeader->m_magicNo2 == MAGIC_NUMBER_2_FREE)
	{
		LogMsg("ERROR: double free attempt at %p", pMem);
		MemMgrAbort();
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

/*
void* MemMgrReAllocateMemory(void* pMem, size_t size)
{
	if (pMem == NULL)
		pMem = NULL;
	
	if (!pMem)
		return MemMgrAllocateMemory(size);
	
	// get the header right before the memory with some cool syntax tricks
	MemAreaHeader* pHeader = & (-1)[(MemAreaHeader*)pMem];

	// Ensure the sanity of this header
	MemMgrPerformSanityChecks(pHeader);
	
	void* pNewMem = MemMgrAllocateMemory(size);
	if (!pNewMem)
		return NULL;
	
	size_t minSize = pHeader->m_size;
	if (minSize > size)
		minSize = size;
	
	memcpy(pNewMem, pMem, minSize);
	
	MemMgrFreeMemory(pMem);
	
	return pNewMem;
}
*/

void* MemMgrReAllocateMemory(void* pMem, size_t size)
{
	// get the header right before the memory with some cool syntax tricks
	MemAreaHeader* pHeader = & (-1)[(MemAreaHeader*)pMem];

	// Ensure the sanity of this header
	MemMgrPerformSanityChecks(pHeader);
	
	if (gLastHeader == pHeader)
	{
		gLastHeader = NULL;
		g_bAddedToLastHeaderHint = false;
	}
	
	// Actually, is the size the same?
	if (size == pHeader->m_size)
	{
		return pMem;
	}
	
	// Are we trying to shrink this memory region? This case is trivial.
	if (size < pHeader->m_size)
	{
		// Temporarily add a separate reference to the memory of this header.
		// This will ensure that no memory gets removed when we remove the
		// other references later.
		MemMgrUseMemoryRegion(pHeader, size + sizeof(MemAreaHeader));
		
		// Get rid of the references from the old header.
		MemMgrFreeMemoryRegion(pHeader, pHeader->m_size + sizeof(MemAreaHeader));
		
		// Update the size of this header.
		pHeader->m_size = size;
		
		return pMem;
	}
	
	// TODO: fix this code
	/*
	
	// How much space do we have between this and the next header?
	MemAreaHeader* pNext = pHeader->m_pNext;
	
	if ((uintptr_t)(pNext) >= (uintptr_t)pHeader + sizeof(MemAreaHeader) + size)
	{
		// Yes. Expand this area using the same method as above for shrinking.
		MemMgrUseMemoryRegion(pHeader, size + sizeof(MemAreaHeader));
		
		// Get rid of the references from the old header.
		MemMgrFreeMemoryRegion(pHeader, pHeader->m_size + sizeof(MemAreaHeader));
		
		// Update the size of this header.
		pHeader->m_size = size;
		
		return pMem;
	}
	
	*/
	
	// We can't! Means we need to relocate.
	void * pNewMem = MemMgrAllocateMemory(size);
	if (!pNewMem) return NULL;
	
	memcpy(pNewMem, pMem, pHeader->m_size);
	
	MemMgrFreeMemory(pMem);
	
	return pNewMem;
}

void MemMgrDebugDump()
{
	MemAreaHeader* pHeader = MemMgrGetInitialHeader();

	while (pHeader)
	{
		// Perform a sanity check first, before doing *anything*. Has its reasons.
		MemMgrPerformSanityChecks(pHeader);

		SLogMsg("Header: %p. Next: %p, Prev: %p. Size: %u.", pHeader, pHeader->m_pNext, pHeader->m_pPrev, pHeader->m_size);

		pHeader = pHeader->m_pNext;
	}

	SLogMsg("Memory page reference: ");

	for (int i = 0; i < 50; i++)
		SLogMsgNoCr("%x", gMemoryPageReference[i]);

	SLogMsgNoCr("\n");
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
	
	abort();
}

void _I_FreeEverything()
{
	MemMgrCleanup();
}

void* realloc (void * ptr, size_t size)
{
	if (!ptr) return malloc(size);
	
	return MemMgrReAllocateMemory(ptr, size);
}

void *malloc (size_t sz)
{
	void* ptr = MemMgrAllocateMemory(sz);
	
	// if we have obtained some memory, scrub it
	if (ptr) memset(ptr, 0x69, sz);
	
	return ptr;
}

void *calloc (size_t nmemb, size_t size)
{
	void *ptr = MemMgrAllocateMemory(nmemb * size);
	
	if (ptr == NULL) return ptr;
	
	memset(ptr, 0, nmemb * size);
	return ptr;
}

void free (void* pMem)
{
	if (!pMem) return;
	
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

void * MmKernelReAllocate(void* ptr, size_t sz)
{
	return _I_ReAllocateDebug(ptr, sz, __FILE__, __LINE__);
}
