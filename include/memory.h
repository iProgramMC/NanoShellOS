//  ***************************************************************
//  memory.h - Creation date: 11/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _MEMORY_H
#define _MEMORY_H

#define KERNEL_BASE_ADDRESS (0xC0000000)
#define KERNEL_HEAP_BASE    (0x80000000)
#define USER_HEAP_BASE      (0x40000000)
#define PAGE_SIZE           (0x1000)

#define USER_EXEC_MAPPING_START    0x00C00000
#define USER_HEAP_DYNAMEM_START    0x40000000
#define KERNEL_HEAP_DYNAMEM_START  0x80000000
#define KERNEL_CODE_AND_DATA_START 0xC0000000
#define MMIO_AREA_0                0xD0000000
#define VBE_FRAMEBUFFER_HINT       0xE0000000
#define MMIO_AREA_1                0xF0000000
	#define MMIO_VBOX_HINT         0xFF000000

#define PAGE_BIT_PRESENT      (0x1)
#define PAGE_BIT_READWRITE    (0x2)
#define PAGE_BIT_USERSUPER    (0x4)
#define PAGE_BIT_WRITETHRU    (0x8)
#define PAGE_BIT_CACHEDISABLE (0x10)
#define PAGE_BIT_ACCESSED     (0x20)

// Page bits that the kernel uses, and are marked as 'available' by the spec
#define PAGE_BIT_MMIO         (0x800) // (1 << 11). If this is MMIO, set this bit to let the kernel know that this shouldn't be unmapped
#define PAGE_BIT_COW          (0x400) // (1 << 10). Copy On Write. If a fault occurs here, copy the specified page.
#define PAGE_BIT_DAI          (0x200) // (1 <<  9). Don't Allocate Instantly. If a fault occurs here, allocate a physical page.

#define PAGE_BIT_ADDRESS_MASK (0xFFFFF000)

#include <main.h>
#include <multiboot.h>
#include <lock.h>

typedef struct
{
	uint32_t m_pageEntries[PAGE_SIZE / 4];
}
PageTable;

// User heap structure
typedef struct UserHeap
{
	SafeLock   m_lock;             // The lock associated with this heap object
	
	int        m_nRefCount;        // The reference count. If it goes to zero, this gets killed
	
	uint32_t*  m_pPageDirectory;   // The virtual  address of the page directory
	uint32_t   m_nPageDirectory;   // The physical address of the page directory
	PageTable* m_pPageTables[512]; // The user half's page tables referenced by the page directory.
	uint32_t   m_nMappingHint;     // The hint to use when mapping with no hint next.
}
UserHeap;

// Exposed memory functions

/**
 * Initialize the physical memory manager.
 */
void MpInitialize(multiboot_info_t* pInfo);

/**
 * Initialize the kernel heap.
 */
void MhInitialize();

/**
 * Gets the number of page faults that have occurred since the system has started.
 */
int MmGetNumPageFaults();

/**
 * Gets the number of free physical memory pages, which can be used to calculate
 * the total amount of free memory available to the system.
 */
int MpGetNumFreePages();

/**
 * Gets the number of available physical memory pages.
 */
int MpGetNumAvailablePages();

/**
 * Maps a single page of physical memory to virtual memory.
 * Use MmUnmapPhysMemFastUnsafe to unmap such memory.
 *
 * The result address may be NULL.
 *
 * MmMapPhysMemFastUnsafe by default returns a _read-write_ section, use MmMapPhysMemFastUnsafeRW
 * to lock it as read-only.
 */
void *MmMapPhysMemFastRW(uint32_t page, bool bReadWrite);
void *MmMapPhysMemFast  (uint32_t page);
void  MmUnmapPhysMemFast(void*    pMem);

/**
 * Maps a contiguous block of physical memory.
 *
 * The result address will either be NULL or valid memory.
 *
 * MmMapPhysicalMemory by default returns a _read-only_ section, use MmMapPhysicalMemoryRW
 * to be able to write to it.
 */
void* MmMapPhysicalMemoryRW(uint32_t phys_start, uint32_t phys_end, bool bReadWrite);
void* MmMapPhysicalMemory  (uint32_t phys_start, uint32_t phys_end);

/**
 * Removes a physical memory mapping.
 */
void MmUnmapPhysicalMemory(void *pMem);

/**
 * Allocates a single page (4096 bytes).
 * 
 * This returns the address of the new page, or NULL if we ran out of memory.
 * pPhysOut may be NULL or not, in the case where it's not, the physical address 
 * of the page is returned.
 */
void* MmAllocateSinglePagePhyD(uint32_t* pPhysOut, const char* callFile, int callLine);
#define MmAllocateSinglePagePhy(physOut) MmAllocateSinglePagePhyD(physOut, __FILE__, __LINE__)

/**
 * Allocates a single page (4096 bytes).
 * 
 * This returns the address of the new page, or NULL if we ran out of memory.
 * Use MmAllocateSinglePagePhy if you also want the physical address of the page.
 */
void* MmAllocateSinglePageD(const char* callFile, int callLine);
#define MmAllocateSinglePage() MmAllocateSinglePageD(__FILE__, __LINE__)

/**
 * Frees a single memory page. A NULL pointer is carefully ignored.
 */
void MmFreePage(void* pAddr);

/**
 * Allocates a memory range of size `size`.  Actually allocates several pages'
 * worth of memory, so that the `size` always fits inside.
 * 
 * For example, MmAllocate(8000) behaves exactly the same as MmAllocate(4193) and MmAllocate(8192).
 * 
 * This kind of thing where we're allocating 1 byte's worth of memory but using several is not recommended,
 * because the specification can always change this later so that memory accesses beyond the size throw an error.
 *
 * This returns the starting address of the memory range we obtained.  NULL can also be returned, if we have
 * no more address space to actually allocate anything, or there are no more memory ranges available.
 * 
 * You must call MmFree or MmReAllocate with the *EXACT* address MmAllocate returned.
 *
 * See: MmFree, MmReAllocate
 */
void* MmAllocateD (size_t size, const char* callFile, int callLine);
void* MmAllocateKD(size_t size, const char* callFile, int callLine);
#define MmAllocate(size)  MmAllocateD (size, __FILE__, __LINE__)
#define MmAllocateK(size) MmAllocateKD(size, __FILE__, __LINE__)

/**
 * Resizes a MmAllocate'd memory range to the new `size`.
 * 
 * You must call MmFree or MmReAllocate (again) with the *EXACT* address
 * MmReAllocate returns.
 *
 * MmReAllocate offers no guarantee that the old pointer will still be valid.
 * It may be (and is quite optimistic about optimizing it to be as such), however,
 * if it sees no option, it will move the whole block.
 */
void* MmReAllocateD (void* old_ptr, size_t size, const char* callFile, int callLine);
void* MmReAllocateKD(void* old_ptr, size_t size, const char* callFile, int callLine);
#define MmReAllocate(old_ptr, size)  MmReAllocateD (old_ptr, size, __FILE__, __LINE__)
#define MmReAllocateK(old_ptr, size) MmReAllocateKD(old_ptr, size, __FILE__, __LINE__)

/**
 * Frees a memory range allocated with MmAllocate. A NULL pointer is carefully ignored.
 * 
 * Note that the last 12 bits of pAddr are ignored, but the other 20 bits MUST represent
 * the first page.
 *
 * So for example MmFree(MmAllocate(400)) behaves exactly the same as MmFree(MmAllocate(400)+500),
 * but not as MmFree(MmAllocate(400)+4100).
 */
void MmFree (void* pAddr);
void MmFreeK(void* pAddr);



UserHeap* MuGetCurrentHeap();
UserHeap* MuCreateHeap();
UserHeap* MuCloneHeap(UserHeap* pHeapToClone);
void MuKillHeap(UserHeap* pHeap);
void MuUseHeap (UserHeap *pHeap);
void MuResetHeap();
void MmDebugDump();

#endif//_MEMORY_H