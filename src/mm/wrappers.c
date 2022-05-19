/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

   Thread safe wrappers for MM functions
******************************************/

#include <memory.h>

extern Heap* g_pHeap;

// Heap wrappers
#if 1

void ResetToKernelHeapUnsafe();
void UseHeapUnsafe (Heap* pHeap);
void FreeHeapUnsafe (Heap* pHeap);

void ResetToKernelHeap()
{
	cli;
	ResetToKernelHeapUnsafe();
	sti;
}

void UseHeap (Heap* pHeap)
{
	cli;
	UseHeapUnsafe (pHeap);
	sti;
}
void FreeHeap (Heap* pHeap)
{
	cli;
	FreeHeapUnsafe(pHeap);
	sti;
}

#endif

// MmAllocate wrappers
#if 1

void *MmAllocateSinglePageUnsafePhyD (uint32_t* pOut, const char* callFile, int callLine);
void *MmAllocateSinglePageUnsafeD    (const char* callFile, int callLine);
void *MmAllocateUnsafePhyD           (size_t size, const char* callFile, int callLine, uint32_t* physAddresses);
void *MmAllocateUnsafeD              (size_t size, const char* callFile, int callLine);
void *MmAllocateUnsafeKD             (size_t size, const char* callFile, int callLine);


void *MmAllocateSinglePagePhyD (uint32_t* pOut, const char* callFile, int callLine)
{
	cli;
	void* ptr = MmAllocateSinglePageUnsafePhyD (pOut, callFile, callLine);
	sti;
	return ptr;
}
void *MmAllocateSinglePageD(const char* callFile, int callLine)
{
	return MmAllocateSinglePagePhyD(NULL, callFile, callLine);
}
void *MmAllocatePhyD (size_t size, const char* callFile, int callLine, uint32_t* physAddresses)
{
	cli;
	void* ptr = MmAllocateUnsafePhyD (size, callFile, callLine, physAddresses);
	sti;
	return ptr;
}

void *MmAllocateUnsafeD (size_t size, const char* callFile, int callLine)
{
	return MmAllocateUnsafePhyD(size, callFile, callLine, NULL);
}
void *MmAllocateUnsafeKD (size_t size, const char* callFile, int callLine)
{
	Heap *pHeapBkp = g_pHeap;
	ResetToKernelHeapUnsafe();
	void* toReturn = MmAllocateUnsafePhyD(size, callFile, callLine, NULL);
	UseHeapUnsafe (pHeapBkp);
	return toReturn;
}

void *MmAllocateD (size_t size, const char* callFile, int callLine)
{
	cli;
	void* ptr = MmAllocateUnsafeD (size, callFile, callLine);
	sti;
	return ptr;
}
void *MmAllocateKD (size_t size, const char* callFile, int callLine)
{
	cli;
	void* ptr = MmAllocateUnsafeKD (size, callFile, callLine);
	sti;
	return ptr;
}

#endif

// MmFree wrappers
#if 1

void MmFreePageUnsafe(void* pAddr);
void MmFreeUnsafeK(void* pAddr);
void MmFreeUnsafe (void* pAddr);

void MmFreePage (void* pAddr)
{
	cli;
	MmFreePageUnsafe(pAddr);
	sti;
}
void MmFree (void* pAddr)
{
	cli;
	MmFreeUnsafe(pAddr);
	sti;
}
void MmFreeK (void* pAddr)
{
	cli;
	MmFreeUnsafeK(pAddr);
	sti;
}

#endif


// MMIO wrappers
#if 1

void *MmMapPhysMemFastUnsafe(uint32_t page);
void *MmMapPhysMemFast(uint32_t page)
{
	cli;
	void* ptr = MmMapPhysMemFastUnsafe(page);
	sti;
	return ptr;
}
void MmUnmapPhysMemFastUnsafe(void* pMem);
void MmUnmapPhysMemFast(void* pMem)
{
	cli;
	MmUnmapPhysMemFastUnsafe(pMem);
	sti;
}
uint32_t MmMapPhysicalMemoryRWUnsafe(uint32_t hint, uint32_t phys_start, uint32_t phys_end, bool bReadWrite);
uint32_t MmMapPhysicalMemoryRW(uint32_t hint, uint32_t phys_start, uint32_t phys_end, bool bReadWrite)
{
	cli;
	uint32_t result = MmMapPhysicalMemoryRWUnsafe(hint, phys_start, phys_end, bReadWrite);
	sti;
	return result;
}
uint32_t MmMapPhysicalMemory(uint32_t hint, uint32_t phys_start, uint32_t phys_end)
{
	return MmMapPhysicalMemoryRW(hint, phys_start, phys_end, false);
}



#endif

