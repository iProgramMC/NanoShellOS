//  ***************************************************************
//  mm/slab.c - Creation date: 25/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// This is an efficient memory allocator for small objects created often.
#include "memoryi.h"

struct SlabContainer;

typedef struct SlabItem
{
	struct SlabItem *m_pNext, *m_pPrev;
	struct SlabContainer *m_pContainer;
	uint32_t m_bitmap[8];   // supports down to 16 byte items
	char m_data[4096 - 12 - 32];
}
SlabItem;

STATIC_ASSERT(sizeof(SlabItem) <= 4096, "This needs to fit in 1 page.");

typedef struct SlabContainer
{
	int m_itemSize;
	SlabItem *m_pFirst, *m_pLast;
	SafeLock m_lock;
}
SlabContainer;

typedef enum eSlabSize
{
	SLAB_SIZE_16,
	SLAB_SIZE_32,
	SLAB_SIZE_64,
	SLAB_SIZE_128,
	SLAB_SIZE_256,
	SLAB_SIZE_512,
	SLAB_SIZE_1024,
	SLAB_COUNT,
}
eSlabSize;

SlabContainer g_Slabs[SLAB_COUNT];

void KiInitializeSlabs()
{
	g_Slabs[SLAB_SIZE_16]  .m_itemSize = 16;
	g_Slabs[SLAB_SIZE_32]  .m_itemSize = 32;
	g_Slabs[SLAB_SIZE_64]  .m_itemSize = 64;
	g_Slabs[SLAB_SIZE_128] .m_itemSize = 128;
	g_Slabs[SLAB_SIZE_256] .m_itemSize = 256;
	g_Slabs[SLAB_SIZE_512] .m_itemSize = 512;
	g_Slabs[SLAB_SIZE_1024].m_itemSize = 1024;
}

int SlabSizeToType(int size)
{
	if (size <= 16)   return SLAB_SIZE_16;
	if (size <= 32)   return SLAB_SIZE_32;
	if (size <= 64)   return SLAB_SIZE_64;
	if (size <= 128)  return SLAB_SIZE_128;
	if (size <= 256)  return SLAB_SIZE_256;
	if (size <= 512)  return SLAB_SIZE_512;
	if (size <= 1024) return SLAB_SIZE_1024;
	return -1;
}

int SlabGetSize(void* ptr)
{
	// get the parent slab item
	SlabItem* pItem = (void*)((uintptr_t)ptr & ~(uintptr_t)0xFFF);
	
	return pItem->m_pContainer->m_itemSize;
}

void* SlabTryAllocate(SlabContainer* pCont, SlabItem* pItem)
{
	const int itemCount = sizeof( pItem->m_data ) / pCont->m_itemSize;
	const int bitmapSize = (itemCount + 31) / 32;
	const int lastBmpCount = itemCount % 32;
	
	for (int bi = 0; bi < bitmapSize; bi++)
	{
		int last = 32;
		if (bi == bitmapSize - 1 && lastBmpCount)
			last = lastBmpCount;
		
		uint32_t bmp = pItem->m_bitmap[bi];
		
		for (int i = 0; i < last; i++)
		{
			if (~bmp & (1 << i))
			{
				pItem->m_bitmap[bi] |= (1 << i);
				
				int index = (32 * bi + i) * pCont->m_itemSize;
				
				if (index + pCont->m_itemSize >= (int)sizeof pItem->m_data)
				{
					SLogMsg("SlabTryAllocate ERROR: index=%d  pCont->itemSize=%d   sizeof pItem->m_data=%d  bi=%d  i=%d", index, pCont->m_itemSize, sizeof pItem->m_data, bi, i);
				}
				
				return &pItem->m_data[index];
			}
		}
	}
	
	return NULL;
}

void* SlabAllocateByType(int type)
{
	SlabContainer* pCont = &g_Slabs[type];
	LockAcquire(&pCont->m_lock);
	
	for (SlabItem* pItem = pCont->m_pFirst; pItem; pItem = pItem->m_pNext)
	{
		void* pThing = SlabTryAllocate(pCont, pItem);
		if (pThing)
		{
			LockFree(&pCont->m_lock);
			return pThing;
		}
	}
	
	// Add a new slab item.
	SlabItem* pItem = MmAllocate(sizeof(SlabItem));
	
	ASSERT(((int)pItem & 0xFFF) == 0);
	
	memset(pItem, 0, sizeof *pItem);
	
	pItem->m_pContainer = pCont;
	
	pItem->m_pNext = pCont->m_pFirst;
	if (pCont->m_pFirst)
		pCont->m_pFirst->m_pPrev = pItem->m_pNext;
	
	pCont->m_pFirst = pItem;
	
	if (pCont->m_pLast == NULL)
		pCont->m_pLast =  pItem;
	
	void* pMem = SlabTryAllocate(pCont, pItem);
	LockFree(&pCont->m_lock);
	return pMem;
}

// Exposed.
void* SlabAllocate(int size)
{
	KeVerifyInterruptsEnabled; // so that locks can work
	
	int type = SlabSizeToType(size);
	if (type < 0)
	{
		// you should not be using SlabAllocate for things below 1024 bytes
		SLogMsg("You shouldn't be using SlabAllocate for allocations of size %d above 1024 bytes (RA: %p)", size, __builtin_return_address(0));
		return NULL;
	}
	
	return SlabAllocateByType(type);
}

void SlabFree(void* ptr)
{
	// get the parent slab item
	SlabItem* pItem = (void*)((uintptr_t)ptr & ~(uintptr_t)0xFFF);
	
	int dataOffset = (char*)ptr - pItem->m_data;
	int dataSize   = pItem->m_pContainer->m_itemSize;
	
	int dataOffsetItems = dataOffset / dataSize;
	
	// clear the bit for that bitmap. Requires a lock since some other thread might
	// come in and mess with our fetch-modify-store cycle
	LockAcquire(&pItem->m_pContainer->m_lock);
	
	pItem->m_bitmap[dataOffsetItems / 32] &= ~(1 << dataOffsetItems);
	
	LockFree(&pItem->m_pContainer->m_lock);
}
