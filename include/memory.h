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

#define ALLOCATE_BUT_DONT_WRITE_PHYS ((uint32_t*)-1)

#define KERNEL_BASE_ADDRESS (0xC0000000)
#define KERNEL_HEAP_BASE    (0x80000000)
#define USER_HEAP_BASE      (0x40000000)
#define PAGE_SIZE           (0x1000)

#define USER_EXEC_MAPPING_START    0x00C00000
#define USER_HEAP_DYNAMEM_START    0x40000000 // User Page Dynamic Mapper
#define KERNEL_HEAP_PBA_START      0x80000000 // Kernel Page Based Allocator
#define KERNEL_HEAP_DYNAMEM_START  0x90000000 // Kernel Page Dynamic Mapper
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

#define PAGE_BIT_SCRUB_ZERO   (0x80000000) // (1 << 31). This bit should only be treated as set if PAGE_BIT_DAI is set.

#define PAGE_BIT_ADDRESS_MASK (0xFFFFF000)

#include <main.h>
#include <multiboot.h>
#include <lock.h>

// mmap() flags
#define PROT_NONE  (0 << 0)
#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2) //not applicable here

#define MAP_FAILED ((void*) -1) //not NULL

#define MAP_FILE      (0 << 0) //retroactive, TODO
#define MAP_SHARED    (1 << 0) //means changes in the mmapped region will be written back to the file on unmap/close
#define MAP_PRIVATE   (1 << 1) //means changes won't be committed back to the source file
#define MAP_FIXED     (1 << 4) //fixed memory mapping means that we really want it at 'addr'.
#define MAP_ANONYMOUS (1 << 5) //anonymous mapping, means that there's no file backing this mapping :)
#define MAP_ANON      (1 << 5) //synonymous with "MAP_ANONYMOUS"
#define MAP_NORESERVE (0 << 0) //don't reserve swap space, irrelevent here

#define MAP_DONTREPLACE (1 << 30) //don't clobber preexisting fixed mappings there. Used with MAP_FIXED to create...
#define MAP_FIXED_NOREPLACE (MAP_DONTREPLACE | MAP_FIXED)

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

typedef struct KernelHeap
{
	PageTable* m_pPageTables[(0xC0000000 - 0x90000000) >> 22]; // The kernel's page tables, referenced by the page directory.
	uint32_t   m_nMappingHint;    // The hint to use when mapping with no hint next.
}
KernelHeap;

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
void* MmAllocateSinglePagePhy(uint32_t* pPhysOut);

/**
 * Allocates a single page (4096 bytes).
 * 
 * This returns the address of the new page, or NULL if we ran out of memory.
 * Use MmAllocateSinglePagePhy if you also want the physical address of the page.
 */
void* MmAllocateSinglePage();

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
void* MmAllocate  (size_t size);
void* MmAllocateK (size_t size);
void* MmAllocateID(size_t size);

/**
 * Does the same thing as MmAllocate, however, if pPhysAddrs is not NULL, or ALLOCATE_BUT_DONT_WRITE_PHYS,
 * writes the physical addresses of the new pages as if pPhysAddrs was an array of size ((size + 4095) / 4096)
 */
void* MmAllocatePhy(size_t size, uint32_t* pPhysAddrs);

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
void* MmReAllocate  (void* old_ptr, size_t size);
void* MmReAllocateK (void* old_ptr, size_t size);
void* MmReAllocateID(void* old_ptr, size_t size);

/**
 * Frees a memory range allocated with MmAllocate. A NULL pointer is carefully ignored.
 * 
 * Note that the last 12 bits of pAddr are ignored, but the other 20 bits MUST represent
 * the first page.
 *
 * So for example MmFree(MmAllocate(400)) behaves exactly the same as MmFree(MmAllocate(400)+500),
 * but not as MmFree(MmAllocate(400)+4100).
 */
void MmFree  (void* pAddr);
void MmFreeK (void* pAddr);
void MmFreeID(void* pAddr);

/**
 * Allocates a copy of the passed in string on the kernel heap.
 */
char * MmStringDuplicate(const char * str);

// TODO: Add helpful comments here too.
UserHeap* MuGetCurrentHeap();
UserHeap* MuCreateHeap();
UserHeap* MuCloneHeap(UserHeap* pHeapToClone);
void MuKillHeap(UserHeap* pHeap);
void MuUseHeap (UserHeap *pHeap);
void MuResetHeap();
void MmDebugDump();
int MmMapMemoryUser(void *pAddr, size_t lengthBytes, int protectionFlags, int mapFlags, int fileDes, size_t fileOffset, void **pOut);
int MmUnMapMemoryUser(void *pAddr, size_t lengthBytes);

#endif//_MEMORY_H