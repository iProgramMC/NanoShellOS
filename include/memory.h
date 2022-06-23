/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

    Memory Manager module header file
******************************************/
#ifndef _MEMORY_H
#define _MEMORY_H

#define BASE_ADDRESS 0xC0000000
#include <main.h>
#include <multiboot.h>

#define USER_EXEC_MAPPING_START    0x00C00000

#define USER_HEAP_DYNAMEM_START    0x40000000

#define KERNEL_HEAP_DYNAMEM_START  0x80000000

#define KERNEL_CODE_AND_DATA_START 0xC0000000

#define MMIO_AREA_0                0xD0000000

#define VBE_FRAMEBUFFER_HINT       0xE0000000

#define MMIO_AREA_1                0xF0000000
	#define MMIO_VBOX_HINT         0xFF000000

//TODO: Proper heap so we won't use 4kb per small allocation.

typedef struct {
	bool m_bPresent:1;
	bool m_bReadWrite:1;
	bool m_bUserSuper:1;
	bool m_bWriteThrough:1;
	bool m_bCacheDisabled:1;
	bool m_bAccessed:1;
	bool m_bDirty:1;
	int m_unused:5;
	uint32_t m_pAddress:20;
} 
__attribute__((packed))
PageEntry;

typedef struct {
	PageEntry m_entries[1024];
} PageTable;

typedef struct {
	// page tables (virtual address)
	PageTable *m_pTables[1024];
	
	// page tables (physical address)
	uint32_t   m_pRealTables[1024];
	
	// this table's real address
	uint32_t   m_pThisPhysical;
} PageDirectory;

#define PAGE_ENTRIES_PHYS_MAX_SIZE 128
typedef struct
{
	uint32_t *m_pageDirectory;
	PageEntry* m_pageEntries;
	int* m_memoryAllocSize;
	const char** m_memoryAllocAuthor;
	int *m_memoryAllocAuthorLine;
	
	uint32_t m_pageEntriesPhysical[PAGE_ENTRIES_PHYS_MAX_SIZE];
	
	//note: everything will be allocated dynamically about this heap, so let's add a size var too:
	int m_pageEntrySize;
	
	uint32_t m_pageDirectoryPhys;
}
Heap;


void MmFirstThingEver();


/**
 * Initializes the memory manager and heap.
 */
void MmInit();

/**
 * Maps a contiguous block of physical memory near the hint address.
 *
 * The result address will either be NULL, a kernel halt, or >= the hint address.
 *
 * MmMapPhysicalMemory by default returns a _read-only_ section, use MmMapPhysicalMemoryRW
 * to be able to write to it.
 */
uint32_t MmMapPhysicalMemoryRW(uint32_t hint, uint32_t phys_start, uint32_t phys_end, bool bReadWrite);
uint32_t MmMapPhysicalMemory  (uint32_t hint, uint32_t phys_start, uint32_t phys_end);

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
 * Maps a contiguous block of physical memory near the hint address.
 *
 * The result address will either be NULL, a kernel halt, or >= the hint address.
 *
 * MmMapPhysicalMemory by default returns a _read-only_ section, use MmMapPhysicalMemoryRW
 * to be able to write to it.
 */
uint32_t MmMapPhysicalMemoryRW(uint32_t hint, uint32_t phys_start, uint32_t phys_end, bool bReadWrite);
uint32_t MmMapPhysicalMemory  (uint32_t hint, uint32_t phys_start, uint32_t phys_end);

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

/**
 * Uses a certain page directory address (and its physical one) as the current page directory.
 * 
 * This is an _internal_ kernel function and should not be used by any non-memory code.
 */
void MmUsePageDirectory(uint32_t* curPageDir, uint32_t phys);

/**
 * Reloads CR3, invalidating the TLB.
 */
void MmTlbInvalidate();

/**
 * Reverts to the kernel's default page directory.  Useful if you're returning from an ELF executable.
 */
void MmRevertToKernelPageDir();

/**
 * Gets the kernel's default page directory's virtual address.
 */
uint32_t* MmGetKernelPageDir();

/**
 * Gets the kernel's default page directory's physical address.
 */
uint32_t MmGetKernelPageDirP();

/**
 * Dumps all information about the memory manager to the console.  Useful for debugging
 */
void MmDebugDump();

/**
 * Creates a new heap.  You can also allocate this heap on the stack, it's not a problem.
 */
bool AllocateHeapD (Heap* pHeap, int size, const char* callerFile, int callerLine);
#define AllocateHeap(pHeap, size) AllocateHeapD(pHeap, size, "[HEAP] " __FILE__, __LINE__)

/**
 * Get how many pages we can allocate on this heap.
 */
int GetHeapSize();

/**
 * Get total number of allocatable pages.
 */
int GetNumPhysPages();

/**
 * Get total number of free allocatable pages.
 */
int GetNumFreePhysPages();

/**
 * Use this heap pointer as our heap.  
 */
void UseHeap (Heap* pHeap);

/**
 * Revert back to kernel heap.  Need to do this if you want to get rid of the heap.
 */
void ResetToKernelHeap();

/**
 * Gets rid of a heap.  Does not delete the pointer, but it deinitializes the heap within.
 */
void FreeHeap(Heap* pHeap);




#endif//_MEMORY_H