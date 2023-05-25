//  ***************************************************************
//  mm/memoryi.h - Creation date: 29/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _MEMORY_INTERNAL_H
#define _MEMORY_INTERNAL_H

#include <main.h>
#include <multiboot.h>
#include <lock.h>
#include <memory.h>
#include <idt.h>
#include <string.h>

// Namespace guide:
// * Mu = User Heaps
// * Mh = Kernel Heap
// * Mp = Physical memory manager
// * Mr = Physical memory Reference counter manager
// * Mm = Exposed programming interface

typedef struct
{
	uint32_t m_refCounts[PAGE_SIZE / sizeof(uint32_t)];
}
RefCountTableLevel1;

typedef struct
{
	RefCountTableLevel1* m_level1[PAGE_SIZE / sizeof(uint32_t)];
}
RefCountTableLevel0;

typedef enum
{
	CLOBBER_NO,   // fail if there are any pre-existing pages
	CLOBBER_ALL,  // remove all pre-existing pages
	CLOBBER_SKIP, // skip all pre-existing pages
}
eClobberingLevel;

// Physical memory manager
uint32_t MpFindFreeFrame();
void MpSetFrame  (uint32_t frameAddr);
void MpClearFrame(uint32_t frameAddr);
int  MpGetNumFreePages();
uintptr_t MpRequestFrame(bool bIsKernelHeap);

// Physical memory reference count manager
uint32_t MrGetReferenceCount(uintptr_t page);
uint32_t MrReferencePage(uintptr_t page);
uint32_t MrUnreferencePage(uintptr_t page);

// Hardware
void MmTlbInvalidate();
void MmUsePageDirectory(uintptr_t pageDir);   //unsafe!! This is exposed just so that other memory code can use it.
void MmInvalidateSinglePage(uintptr_t add);
void MmOnPageFault(Registers* pRegs);

// Kernel page based allocator
void* MhAllocate(size_t size, uint32_t* pPhysOut);
void* MhReAllocate(void *oldPtr, size_t newSize);
void* MhAllocateSinglePage(uint32_t* pPhysOut);
void  MhFreePage(void* pPage);
void  MhFree(void* pPage);
void* MhMapPhysicalMemory(uintptr_t physMem, size_t nPages, bool bReadWrite);
void  MhUnMapPhysicalMemory(void *pAddr);
uint32_t* MhGetPageEntry(uintptr_t address);

// Kernel memory mapper
bool MkMapMemoryFixedHint(uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, int clobberingLevel, bool bIsMMIO, uint32_t nDaiFlags);
bool MkUnMap(uintptr_t mem, size_t numPages);
void MkSetUpKernelMapping();
uint32_t* MkiGetPageEntryAt(uintptr_t address);

// User heap manager
void MuUseHeap (UserHeap* pHeap);
void MuResetHeap();
UserHeap* MuCreateHeap();
UserHeap* MuGetCurrentHeap();
UserHeap* MuCloneHeap(UserHeap* pHeapToClone);
void MuKillHeap(UserHeap *pHeap);
uint32_t* MuGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable);
bool MuCreateMapping(UserHeap *pHeap, uintptr_t address, uint32_t physAddress, bool bReadWrite);
bool MuAreMappingParmsValid(uintptr_t start, size_t nPages);
bool MuIsMappingFree(UserHeap *pHeap, uintptr_t start, size_t nPages);
bool MuMapMemory(UserHeap *pHeap, size_t numPages, uint32_t* pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO);
bool MuMapMemoryNonFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO);
bool MuMapMemoryFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, int clobberingLevel, bool bIsMMIO, uint32_t nDaiFlags);
void MuCreatePageTable(UserHeap *pHeap, int pageTable);
void MuRemovePageTable(UserHeap *pHeap, int pageTable);
bool MuRemoveMapping(UserHeap *pHeap, uintptr_t address);
bool MuUnMap (UserHeap *pHeap, uintptr_t address, size_t nPages);

// Slab allocator (based on kernel heap)
void * SlabAllocate(int size);
void   SlabFree(void* ptr);
int    SlabGetSize(void* ptr);
int    SlabSizeToType(int size);

#endif
