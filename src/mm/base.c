/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

           Memory Manager module
******************************************/

//#define MULTITASKED_WINDOW_MANAGER

//TODO FIXME: Make this thread safe in the future.  The problem is if
//you enable MULTITASKED_WINDOW_MANAGER (so, the locks), the system
//gets stack overflowed for whatever reason.

#include <memory.h>
#include <print.h>
#include <string.h>
#include <vga.h>
#include <misc.h>

extern bool g_interruptsAvailable;

extern uint32_t g_kernelPageDirectory[];
extern PageEntry g_pageTableArray[];
extern uint32_t* e_frameBitsetVirt;
extern uint32_t e_frameBitsetSize;
extern uint32_t e_placement;

uint32_t g_frameBitset [32768];//Enough to wrap the entirety of the address space

uint32_t* g_pageDirectory = NULL;
uint32_t* g_curPageDir = NULL;
uint32_t  g_curPageDirP = 0;

void MmFreePageUnsafe(void* pAddr);
void MmFreeUnsafe    (void* pAddr);
void MmFreeUnsafeK   (void* pAddr);

void MmTlbInvalidate() {
	__asm__("movl %cr3, %ecx\n\tmovl %ecx, %cr3\n\t");
}
void MmUsePageDirectory(uint32_t* curPageDir, uint32_t phys)
{
	g_curPageDir = curPageDir;
	g_curPageDirP = phys;
	__asm__ volatile ("mov %0, %%cr3"::"r"((uint32_t*)phys));
}
extern void MmStartupStuff(); //io.asm
void MmInitPrimordial()
{
	MmStartupStuff();
	e_frameBitsetSize = 131072;
	e_frameBitsetVirt = g_frameBitset;
}

// Mark the code and rodata segments as read-only.
extern char l_code_and_rodata_start[], l_code_and_rodata_end[];
void MmMarkStuffReadOnly()
{
	LogMsg("Code and rodata: %p - %p",l_code_and_rodata_start,l_code_and_rodata_end);
	
	uint32_t crap;
	for (crap = (uint32_t) l_code_and_rodata_start;
		 crap < (uint32_t) l_code_and_rodata_end;
		 crap += 4096)
	{
		g_pageTableArray[(crap >> 12) & 0x3FF].m_bReadWrite = 0;
	}
}

int g_numPagesAvailable = 0;

int GetNumPhysPages()
{
	return g_numPagesAvailable;
}

/* Quick memory map one page trick */
#if 1

/* This maps addresses to addresses in the high 0xFFs (i.e. 0xFFC00000 or higher) */
PageEntry g_LastPageTable [1024] __attribute__((aligned(4096)));

void MmInvalidateSinglePage(UNUSED uintptr_t add);
void *MmMapPhysMemFastUnsafeRW(uint32_t page, bool bReadWrite)
{
	int free_spot = -1;
	for (int i = 0; i < 1024; i++)
	{
		if (!g_LastPageTable[i].m_bPresent)
		{
			free_spot = i;
			break;
		}
	}
	
	if (free_spot == -1)
		return NULL; // Too many memory mappings, try to free up some
	
	PageEntry *pEntry = &g_LastPageTable[free_spot];
	
	pEntry->m_pAddress = page >> 12;
	
	pEntry->m_bPresent       = 1;
	pEntry->m_bWriteThrough  =
	pEntry->m_bCacheDisabled = 0;
	pEntry->m_bUserSuper     = 1;
	pEntry->m_bReadWrite     = bReadWrite;
	
	pEntry->m_bAccessed      =
	pEntry->m_bDirty         = 0;
	
	void *pAddr =  (void*)(0xFFC00000 | (free_spot << 12) | (page & 0xFFF));
	
	MmInvalidateSinglePage( (uintptr_t) pAddr );
	
	return pAddr;
}
void MmUnmapPhysMemFastUnsafe(void* pMem)
{
	uintptr_t mem = (uintptr_t)pMem;
	if (mem < 0xFFC00000) return;
	
	int i = (mem >> 12) & 0x3FF;
	
	g_LastPageTable[i].m_bPresent = false;
	
	MmInvalidateSinglePage ( (uintptr_t)pMem );
}

#endif

/* Kernel heap. */
 
#if 1
// Can allocate up to 256 MB of RAM.  No need for more I think,
// but if there is a need, just increase this. Recommend a Power of 2
#define PAGE_ENTRY_TOTAL 65536

PageEntry g_kernelPageEntries   [PAGE_ENTRY_TOTAL] __attribute__((aligned(4096)));

// Simple heap: The start PageEntry of a big memalloc also includes the number of pageEntries that also need to be freed
// For this reason freeing just one page from a larger allocation is _not_ enough, you need to use MmFree instead
// (unless you used MmAllocageSinglePage to allocate it)
int g_kernelMemoryAllocationSize[PAGE_ENTRY_TOTAL];

//if debug only
const char* g_kernelMemoryAllocationAuthor[PAGE_ENTRY_TOTAL];
int  g_kernelMemoryAllocationAuthorLine[PAGE_ENTRY_TOTAL];

#define PAGE_BITS 0X3 //present and read/write
#define PAGE_ALLOCATION_BASE 0x80000000

#endif

//forward declaration because we need this function before it is declared 
void *MmAllocatePhyD (size_t size, const char* callFile, int callLine, uint32_t* physAddresses);

/**
 * Heap variables (can switch in and out of the kernel heap)
 */
#if 1
PageEntry  * g_pageEntries = NULL;
int        * g_memoryAllocationSize = NULL;
const char** g_memoryAllocationAuthor = NULL;
int        * g_memoryAllocationAuthorLine = NULL;
int          g_heapSize = 0;
Heap       * g_pHeap = NULL;
uint32_t     g_pageAllocationBase = PAGE_ALLOCATION_BASE;
#endif

uint32_t g_memoryStart;

/**
 * Physical memory manager.  This hands out 4KB chunks of physical memory so that they can be mapped
 * by the virtual memory manager below this.
 */
#if 1

#define  INDEX_FROM_BIT(a) (a / 32)
#define OFFSET_FROM_BIT(a) (a % 32)
static void MmSetFrame (uint32_t frameAddr)
{
	uint32_t frame = frameAddr >> 12;
	uint32_t idx = INDEX_FROM_BIT (frame),
			 off =OFFSET_FROM_BIT (frame);
	e_frameBitsetVirt[idx] |= (0x1 << off);
}
static void MmClrFrame (uint32_t frameAddr)
{
	uint32_t frame = frameAddr >> 12;
	uint32_t idx = INDEX_FROM_BIT (frame),
			 off =OFFSET_FROM_BIT (frame);
	e_frameBitsetVirt[idx] &=~(0x1 << off);
}

static uint32_t MmFindFreeFrame()
{
	for (uint32_t i=0; i<INDEX_FROM_BIT(e_frameBitsetSize); i++)
	{
		//Any bit free?
		if (e_frameBitsetVirt[i] != 0xFFFFFFFF) {
			//yes, which?
			for (int j=0; j<32; j++)
			{
				if (!(e_frameBitsetVirt[i] & (1<<j)))
				{
					//FREE_LOCK (g_memoryPmmLock);
					return i*32 + j;
				}
			}
		}
		//no, continue
	}
	//what
	SLogMsg("No more physical memory page frames. This can and will go bad!!");
	return 0xffffffffu;
}
int GetNumFreePhysPages()
{
	int result = 0;
	for (uint32_t i=0; i<INDEX_FROM_BIT(e_frameBitsetSize); i++)
	{
		//Any bit free?
		if (e_frameBitsetVirt[i] != 0xFFFFFFFF) {
			//yes, which?
			for (int j=0; j<32; j++)
			{
				if (!(e_frameBitsetVirt[i] & (1<<j)))
					result++;
			}
		}
		//no, continue
	}
	
	//what
	return result;
}

void MmInitializePMM(multiboot_info_t* mbi)
{
	for (uint32_t i=0; i<INDEX_FROM_BIT(e_frameBitsetSize); i++)
	{
		e_frameBitsetVirt[i] = 0xFFFFFFFF;
	}
	
	//parse the multiboot mmap and mark the respective memory frames as free:
	int len, addr;
	len = mbi->mmap_length, addr = mbi->mmap_addr;
	
	//adding extra complexity is a no-go for now
	if (addr >= 0x100000)
	{
		SwitchMode(0);
		CoInitAsText(&g_debugConsole);
		LogMsg("OS state not supported.  Mmap address: %x  Mmap len: %x", addr, len);
		KeStopSystem();
	}
	
	//turn this into a virt address:
	addr += 0xC0000000;
	
	multiboot_memory_map_t* pMemoryMap;
	
	//logmsg's are for debugging and should be removed.
	for (pMemoryMap = (multiboot_memory_map_t*)addr;
		 (unsigned long) pMemoryMap < addr + mbi->mmap_length;
		 pMemoryMap = (multiboot_memory_map_t*) ((unsigned long) pMemoryMap + pMemoryMap->size + sizeof(pMemoryMap->size)))
	{
		// if this memory range is not reserved AND it doesn't bother our kernel space, free it
		if (pMemoryMap->type == MULTIBOOT_MEMORY_AVAILABLE)
			for (int i = 0; i < (int)pMemoryMap->len; i += 0x1000)
			{
				uint32_t addr = pMemoryMap->addr + i;
				if (addr >= e_placement)// || addr < 0x100000)
					MmClrFrame (addr);
			}
	}
	
	// Clear out the kernel, from 0x100000 to 0x600000
	for (int i = 0x100; i < (e_placement >> 12) + 1; i++)
	{
		MmSetFrame(i << 12); // Set frame as not allocatable
	}
	
	multiboot_info_t* pInfo = KiGetMultibootInfo();
	if (pInfo->mods_count > 0)
	{
		//Usually the mods table is below 1m.
		if (pInfo->mods_addr >= 0x100000)
		{
			LogMsg("Module table starts at %x.  OS state not supported", pInfo->mods_addr);
			KeStopSystem();
		}
		
		//The initrd module is here.
		multiboot_module_t *pModules = (void*) (pInfo->mods_addr + 0xc0000000);
		for (int i = 0; i < pInfo->mods_count; i++)
		{
			SLogMsg("Blocking out frames from module");
			// block out the frames this contains from being allocated by the PMM
			for (uint32_t pg = pModules[i].mod_start;
				          pg < pModules[i].mod_end + 0xFFF;
						  pg += 4096)
			{
				MmSetFrame (pg);
			}
		}
	}
	
	g_numPagesAvailable = GetNumFreePhysPages();
}

uint32_t MmMapPhysicalMemoryRWUnsafe(uint32_t hint, uint32_t phys_start, uint32_t phys_end, bool bReadWrite)
{
	SLogMsg("MmMapPhysicalMemoryRWUnsafe(%x, %x, %x, %d)", hint, phys_start, phys_end, bReadWrite);
	
	// Get the hint offset in the page tables we should start out at
	int page_table_offset = (int)((hint >> 12) & 0xFFF);
	int page_direc_offset = (int)((hint >> 22));
	
	uint32_t phys_page_start = (phys_start >> 12), phys_page_curr = phys_page_start;
	uint32_t phys_page_end   = (phys_end   >> 12) + ((phys_end & 0xFFF) != 0);
	
	uint32_t num_pages_to_map = phys_page_end - phys_page_start;
	
	// Every 1024 pages (4096 kb) we require a page table.
	PageEntry* pPageTable = NULL;
	uint32_t num_pages_mapped_so_far = 0;
	
	while (num_pages_mapped_so_far != num_pages_to_map)
	{
		// On a 1024-page boundary?
		if ((page_table_offset & 0x3FF) == 0 || !pPageTable)
		{
			if (page_table_offset > 0x3FF)
			{
				page_table_offset &= 0x3FF;
				page_direc_offset++;
			}
			//Allocate a new page table using MmAllocateSinglePagePhyD, or use a pre-existing one
			
			uint32_t p_addr = 0;
			
			if (g_kernelPageDirectory[page_direc_offset])
			{
				p_addr = g_kernelPageDirectory[page_direc_offset] & 0xFFFFF000;
				
				//This may seem like a hack, and that's because it _is_ one.
				//If the physical address > e_placement, assume it is allocated by the heap
				//Otherwise, it's available in the kernel's tiny mem space already
				if (p_addr >= e_placement)
					pPageTable = (PageEntry*)(p_addr + 0x80000000);
				else
					pPageTable = (PageEntry*)(p_addr + 0xC0000000);
			}
			else
			{
				pPageTable = (PageEntry*)MmAllocateSinglePagePhyD (&p_addr, "src/memory.c -- phys-map", __LINE__);
				
				if (pPageTable)
				{
					memset_ints (pPageTable, 0x00000000, 1024);
					
					// Also register it inside the directory
					g_kernelPageDirectory[page_direc_offset] = 1 | 2 | 4 | (p_addr & 0xFFFFF000);
				}
			}
			
			if (!pPageTable)
			{
				//panic
				 LogMsg("Error trying to MmMapPhysicalMemory(%x, %x, %x). Out of memory? Who knows what's going to happen now!", hint, phys_start, phys_end);
				SLogMsg("Error trying to MmMapPhysicalMemory(%x, %x, %x). Out of memory? Who knows what's going to happen now!", hint, phys_start, phys_end);
				KeStopSystem();
			}
		}
		
		// Fill out an entry in the current page table
		PageEntry *pEntry = &pPageTable[page_table_offset];
		pEntry->m_bPresent   = 1;
		pEntry->m_bReadWrite = bReadWrite;
		pEntry->m_bUserSuper = 1;
		pEntry->m_bAccessed  = 0;
		pEntry->m_bDirty     = 0;
		pEntry->m_pAddress   = phys_page_curr++;
		
		page_table_offset++;
		
		num_pages_mapped_so_far ++;
	}
	
	// Mark the frames as occupied
	for (uint32_t i = phys_start; i < phys_end + 0xFFF; i += 0x1000)
	{
		MmSetFrame (i);
	}
	
	MmTlbInvalidate ();
	
	return hint + (phys_start & 0xFFF);
}
#endif


/**
 * Heap management code.  What comes after 0x80000000 is always mapped regardless of which
 * user heap we're currently using - it's just that we can't allocate to the kernel heap
 * while we are using the user heap.
 */
#if 1
int GetHeapSize()
{
	return g_heapSize;
}

void ResetToKernelHeapUnsafe()
{
	g_pageAllocationBase = PAGE_ALLOCATION_BASE;
	g_heapSize = PAGE_ENTRY_TOTAL;
	g_pageEntries = g_kernelPageEntries;
	g_pageDirectory = g_kernelPageDirectory;
	g_memoryAllocationAuthor = g_kernelMemoryAllocationAuthor;
	g_memoryAllocationAuthorLine = g_kernelMemoryAllocationAuthorLine;
	g_memoryAllocationSize = g_kernelMemoryAllocationSize;
	g_pHeap = NULL;
	MmUsePageDirectory(g_kernelPageDirectory, (uint32_t)g_kernelPageDirectory - BASE_ADDRESS);
}

void UseHeapUnsafe (Heap* pHeap)
{
	if (!pHeap) {
		ResetToKernelHeapUnsafe();
		return;
	}
	
	g_pHeap = pHeap;
	g_heapSize                   = pHeap->m_pageEntrySize;
	g_pageEntries                = pHeap->m_pageEntries;
	g_pageDirectory              = pHeap->m_pageDirectory;
	g_memoryAllocationAuthor     = pHeap->m_memoryAllocAuthor;
	g_memoryAllocationAuthorLine = pHeap->m_memoryAllocAuthorLine;
	g_memoryAllocationSize       = pHeap->m_memoryAllocSize;
	
	//all user heap allocations start at 0x40000000
	g_pageAllocationBase = 0x40000000;
	
	
	MmUsePageDirectory(pHeap->m_pageDirectory, pHeap->m_pageDirectoryPhys);
}

// Frees a heap that was allocated on the kernel heap.
void FreeHeapUnsafe (Heap* pHeap)
{
	// free all the elements allocated inside the heap:
	for (int i = 0; i < pHeap->m_pageEntrySize; i++)
	{
		if (pHeap->m_pageEntries[i].m_bPresent)
		{
			//allocated, free it.
			//MmFreePageUnsafe(ptr);
			MmClrFrame(pHeap->m_pageEntries[i].m_pAddress << 12);
		}
	}
	
	Heap* bkp = g_pHeap;
	
	ResetToKernelHeapUnsafe();
	
	MmFreeUnsafe(pHeap->m_pageEntries);
	MmFreeUnsafe(pHeap->m_pageDirectory);
	MmFreeUnsafe(pHeap->m_memoryAllocAuthor);
	MmFreeUnsafe(pHeap->m_memoryAllocAuthorLine);
	MmFreeUnsafe(pHeap->m_memoryAllocSize);
	pHeap->m_pageEntries           = NULL;
	pHeap->m_pageDirectory         = NULL;
	pHeap->m_memoryAllocAuthor     = NULL;
	pHeap->m_memoryAllocAuthorLine = NULL;
	pHeap->m_memoryAllocSize       = NULL;
	pHeap->m_pageEntrySize         = 0;
	
	if (bkp != pHeap)
		UseHeapUnsafe (bkp);
}

void MmRevertToKernelPageDir()
{
	ResetToKernelHeap ();
}

//! ONLY call this for the kernel heap!
static void MmSetupKernelHeapPages()
{
	int heapSize = GetHeapSize();
	for (int i = 0; i < heapSize; i++)
	{
		*((uint32_t*)(g_pageEntries + i)) = 0;
		g_memoryAllocationSize[i] = 0;
	}
	int index = 0x200;//2;
	for (int i = 0; i < heapSize; i += 1024)
	{
		uint32_t pPageTable = (uint32_t)&g_pageEntries[i];
		
		g_curPageDir[index] = (pPageTable-BASE_ADDRESS) | PAGE_BITS;//present + readwrite
		index++;
	}
	
	//set the last entry of the page directory to use our mmio focused page table:
	g_curPageDir[1023] = ((int)g_LastPageTable - BASE_ADDRESS) | PAGE_BITS;
	
	MmTlbInvalidate();
}

static void MmSetupUserHeapPages(Heap* pHeap)
{
	int heapSize = pHeap->m_pageEntrySize;
	for (int i = 0; i < heapSize; i++)
	{
		*((uint32_t*)(pHeap->m_pageEntries + i)) = 0;
		pHeap->m_memoryAllocSize[i] = 0;
	}
	int index = 0x100, jindex = 0;//start at 0x40000000
	for (int i = 0; i < heapSize; i += 1024)
	{
		uint32_t pPageTable = pHeap->m_pageEntriesPhysical[jindex];
		
		pHeap->m_pageDirectory[index] = pPageTable | PAGE_BITS;//present + readwrite
		
		index++;
		jindex++;
	}
}

void *MmAllocateUnsafePhyD (size_t size, const char* callFile, int callLine, uint32_t* physAddresses);
void* MmAllocateSinglePageUnsafePhyD(uint32_t* pPhysOut, const char* callFile, int callLine);
void *MmAllocateUnsafeD (size_t size, const char* callFile, int callLine);
bool AllocateHeapUnsafeD (Heap* pHeap, int size, const char* callerFile, int callerLine)
{
	SLogMsg("Creating a heap with a size of %d", size);
	
	//PAGE_ENTRIES_PHYS_MAX_SIZE represents how many pagedirectories can we create at one time
	if (size > PAGE_ENTRIES_PHYS_MAX_SIZE * 1024)
	{
		//can't:
		SLogMsg("Can't allocate a heap bigger than %d pages big.  That may change in a future update.", PAGE_ENTRIES_PHYS_MAX_SIZE * 1024);
		return false;
	}
	
	//initialize stuff to null, this way we can FreeHeap already
	pHeap->m_pageEntrySize         = 0;
	pHeap->m_pageEntries           = NULL;
	pHeap->m_pageDirectory         = NULL;
	pHeap->m_memoryAllocAuthor     = NULL;
	pHeap->m_memoryAllocAuthorLine = NULL;
	pHeap->m_memoryAllocSize       = NULL;
	pHeap->m_pageDirectoryPhys     = 0;
	
	ResetToKernelHeapUnsafe();
	
	uint32_t phys;
	pHeap->m_pageEntrySize         = size;
	pHeap->m_pageEntries           = MmAllocateUnsafePhyD(sizeof (int) * size, callerFile, callerLine, pHeap->m_pageEntriesPhysical);
	pHeap->m_pageDirectory         = MmAllocateSinglePageUnsafePhyD(&phys, callerFile, callerLine);
	pHeap->m_memoryAllocAuthor     = MmAllocateUnsafeD(sizeof (int) * size, callerFile, callerLine);
	pHeap->m_memoryAllocAuthorLine = MmAllocateUnsafeD(sizeof (int) * size, callerFile, callerLine);
	pHeap->m_memoryAllocSize       = MmAllocateUnsafeD(sizeof (int) * size, callerFile, callerLine);
	pHeap->m_pageDirectoryPhys     = phys;
	
	// check if we failed to allocate something
	if (!pHeap->m_pageEntries           ||
		!pHeap->m_pageDirectory         ||
		!pHeap->m_memoryAllocAuthor     ||
		!pHeap->m_memoryAllocAuthorLine ||
		!pHeap->m_memoryAllocSize       ||
		!pHeap->m_pageDirectoryPhys)
	{
		SLogMsg("Couldn't allocate heap of size %d pages, out of memory?", size);
		//NULL frees do nothing, so it should be fine.
		MmFreeUnsafe(pHeap->m_pageEntries);
		MmFreeUnsafe(pHeap->m_pageDirectory);
		MmFreeUnsafe(pHeap->m_memoryAllocAuthor);
		MmFreeUnsafe(pHeap->m_memoryAllocAuthorLine);
		MmFreeUnsafe(pHeap->m_memoryAllocSize);
		MmFreeUnsafe(pHeap->m_pageDirectoryPhys);
		return false;
	}
	
	// copy everything from the kernel heap:
	memcpy (pHeap->m_pageDirectory, g_kernelPageDirectory, 4096);
	
	// initialize this heap's pageentries:
	MmSetupUserHeapPages(pHeap);
	
	ResetToKernelHeapUnsafe();
	
	return true;
}
bool AllocateHeapD (Heap* pHeap, int size, const char* callerFile, int callerLine)
{
	cli;
	bool b = AllocateHeapUnsafeD(pHeap, size, callerFile, callerLine);
	sti;
	return b;
}

#endif


int g_offset = 0;
void MmInit()
{
	multiboot_info_t* pInfo = KiGetMultibootInfo();
	
	e_placement += 0x1000;
	e_placement &= ~0xFFF;
	
	g_offset = e_placement >> 12;
	
	g_memoryStart = e_placement;
	
	MmInitializePMM(pInfo);
	MmRevertToKernelPageDir();
	MmSetupKernelHeapPages();
}

void MmInvalidateSinglePage(UNUSED uintptr_t add)
{
	//__asm__ volatile ("invlpg (%0)\n\t"::"r"(add):"memory");
	(void)add;
	MmTlbInvalidate();
}

static void* MmSetupPage(int i, uint32_t* pPhysOut, const char* callFile, int callLine)
{
	uint32_t frame = MmFindFreeFrame();
	if (frame == 0xffffffffu)
	{
		return NULL;
	}

	MmSetFrame(frame << 12);
	
	// Yes. Let's mark this as present, and return a made-up address from the index.
	g_pageEntries[i].m_bPresent = true;
	g_pageEntries[i].m_bReadWrite = true;
	g_pageEntries[i].m_bUserSuper = true;
	g_pageEntries[i].m_pAddress = frame;// + g_offset;
	
	g_memoryAllocationSize[i] = 0;
	g_memoryAllocationAuthor[i] = callFile;
	g_memoryAllocationAuthorLine[i] = callLine;
	
	// if pPhysOut is not null, we would probably want that too:
	if (pPhysOut)
		*pPhysOut = g_pageEntries[i].m_pAddress << 12;
	
	uint32_t retaddr = (g_pageAllocationBase + (i << 12));
	MmInvalidateSinglePage(retaddr);
	
#ifdef RANDOMIZE_MALLOCED_MEMORY
	for (int i = 0; i < 512; i++)
	{
		*((uint64_t*)retaddr + i) = 0x4b435546424d5544;//DUMBFUCK
	}
#endif
	
	//FREE_LOCK (g_memoryPageLock);
	return (void*)retaddr;
}
void* MmAllocateSinglePageUnsafePhyD(uint32_t* pPhysOut, const char* callFile, int callLine)
{
	// find a free pageframe.
	// For 4096 bytes we can use ANY hole in the pageframes list, and we
	// really do not care.
	
	int heapSize = GetHeapSize();
	for (int i = 0; i < heapSize; i++)
	{
		if (!g_pageEntries[i].m_bPresent) // A non-allocated pageframe?
		{
			return MmSetupPage(i, pPhysOut, callFile, callLine);
		}
	}
	// No more page frames?!
	SLogMsg("WARNING: No more page entries");
	
	return NULL;
}
void MmFreePageUnsafe(void* pAddr)
{
	if (!pAddr) return;
	
	// Turn this into a g_pageEntries index.
	uint32_t addr = (uint32_t)pAddr;
	
	//safety measures:
	if (addr >= 0xC0000000)
	{
		SLogMsg("Can't free kernel memory!");
		return;
	}
	if (g_pHeap && addr >= 0x80000000)
	{
		SLogMsg("Can't free from the kernel heap while you're using a user heap!");
		return;
	}
	if (!g_pHeap && addr < 0x80000000)
	{
		SLogMsg("You aren't using a user heap!");
		return;
	}
	
	addr -= g_pageAllocationBase;
	addr >>= 12;
	
	if (!g_pageEntries[addr].m_bPresent)
	{
		SLogMsg("Double free attempt on %x", pAddr);
		return;
	}
	
	//also clear the pmm's frame:
	MmClrFrame(g_pageEntries[addr].m_pAddress << 12);
	
	g_pageEntries[addr].m_bPresent = false;
	g_memoryAllocationSize[addr] = 0;
}
bool MmIsPageMapped(uint32_t pageAddr)
{
	//uint32_t pageTableNum = (pageAddr >> 22);
	//uint32_t pageEntryNum = (pageAddr >> 12) & 0x3FF;
	
	if (pageAddr >= 0xC0000000 && pageAddr < 0xC0700000)
		return true;
	
	if (pageAddr >= 0x80000000 && pageAddr < 0x80000000 + (PAGE_ENTRY_TOTAL << 12))
	{
		return (g_kernelPageEntries[((pageAddr - 0x80000000) >> 12)].m_bPresent);
	}
	
	//TODO
	return false;
}

//be sure to provide a decently sized physAddresses, or NULL
void *MmAllocateUnsafePhyD (size_t size, const char* callFile, int callLine, uint32_t* physAddresses)
{
	if (size <= 0x1000) //worth one page:
		return MmAllocateSinglePageUnsafePhyD(physAddresses, callFile, callLine);
	else
	{
		//ACQUIRE_LOCK (g_memoryPriLock);
		//more than one page, take matters into our own hands:
		int numPagesNeeded = ((size - 1) >> 12) + 1;
		//ex: if we wanted 6100 bytes, we'd take 6100-1=6099, then divide that by 4096 (we get 1) and add 1
		//    if we wanted 8192 bytes, we'd take 8192-1=8191, then divide that by 4096 (we get 1) and add 1 to get 2 pages
		
		int heapSize = GetHeapSize();
		for (int i = 0; i < heapSize; i++)
		{
			// A non-allocated pageframe?
			if (!g_pageEntries[i].m_bPresent)
			{
				// Yes.  Are there at least numPagesNeeded holes?
				int jfinal = i + numPagesNeeded;
				for (int j = i; j < jfinal; j++)
				{
					//Are there any already taken pages before we reach the end.
					if (g_pageEntries[j].m_bPresent)
					{
						//Yes.  This hole isn't large enough.
						//NOTE that THIS is why we need more levels of break than just "break". 
						//I'll just use a goto then.
						i = j;
						goto _label_continue;
					}
				}
				// Nope! We have space here!  Let's map all the pages, and return the address of the first one.
				uint32_t* pPhysOut = physAddresses;
				
				void* pointer = MmSetupPage(i, pPhysOut, callFile, callLine);
				if (!pointer)
				{
					// Uh oh!  This ain't good, but thankfully, we failed on the first page, so just gracefully return.
					return NULL;
				}
				
				// if not null, increment by 4.
				// if it WERE null and we DID increment by four, it would throw a pagefault
				if (pPhysOut)
					pPhysOut++;
				
				// Not to forget, set the memory allocation size below:
				g_memoryAllocationSize[i] = numPagesNeeded - 1;
				
				int nPagesToFree = 1;
				for (int j = i + 1; j < jfinal; j++)
				{
					void *ptr = MmSetupPage (j, pPhysOut, callFile, callLine);
					
					// if not null, increment by 4.
					if (!ptr)
					{
						// All, hell naw
						// We don't fit in physical memory!!! Let's roll back and return
						for (int npage = 0; npage < nPagesToFree; npage++)
						{
							MmFreePageUnsafe((uint8_t*)pointer + 4096 * npage);
						}
						return NULL;
					}
					
					nPagesToFree++;
					if (pPhysOut)
						pPhysOut++;
				}
				return pointer;
			}
		_label_continue:;
		}
		
		return NULL; //no continuous addressed pages are left.
	}
}

void *MmReAllocateUnsafeD(void* old_ptr, size_t size, const char* CallFile, int CallLine)
{
	// step 1: If the pointer is null, just allocate a new array of size `size`.
	if (!old_ptr)
		return MmAllocateUnsafeD(size, CallFile, CallLine);
	
	// step 2: get the first page's number
	uint32_t addr = (uint32_t)old_ptr;
	addr -= g_pageAllocationBase;
	addr >>= 12;
	if (addr >= (uint32_t)GetHeapSize()) return NULL;
	
	// step 3: figure out the old size of this block.
	int* pSubsequentAllocs = &g_memoryAllocationSize[addr];
	
	size_t old_size = 4096 * (*pSubsequentAllocs + 1);
	
	// If the provided size is smaller, return the same block, but shrunk.
	if (old_size >= size)
	{
		size_t numPagesNeeded =  ((size - 1) >> 12) + 1;
		
		int oldPages = *pSubsequentAllocs  + 1;
		*pSubsequentAllocs = numPagesNeeded - 1;
		
		// where freeing pages starts:
		uint8_t* addr = (uint8_t*)old_ptr + numPagesNeeded * 0x1000;
		
		for (int i = numPagesNeeded; i < oldPages; i++)
		{
			MmFreePageUnsafe(addr);
			addr += 0x1000;
		}
		
		return old_ptr;
	}
	else
	{
		// Check if there are this many free blocks available at all.
		size_t numPagesNeeded =  ((size - 1) >> 12) + 1;
		int oldPages = *pSubsequentAllocs  + 1;
		
		bool spaceAvailable = true;
		
		for (int i = oldPages; i < numPagesNeeded; i++)
		{
			if (g_pageEntries[addr + i].m_bPresent)
			{
				spaceAvailable = false;
				break;
			}
		}
		
		if (spaceAvailable)
		{
			// Proceed with the expansion.
			
			for (int i = oldPages; i < numPagesNeeded; i++)
			{
				void *ptr = MmSetupPage (addr+i, NULL, CallFile, CallLine);
				
				// if not null, increment by 4.
				if (!ptr)
				{
					// All, hell naw
					//TODO: rollback
					ASSERT(!"TODO: Roll back from failed MmReAllocate?");
					return NULL;
				}
			}
			
			*pSubsequentAllocs = numPagesNeeded - 1;
			
			return old_ptr;
		}
	}
	
	// If nothing else works, just allocate and memcpy().
	
	void* new_ptr = MmAllocateUnsafeD(size, CallFile, CallLine);
	if (!new_ptr)
		return NULL;
	
	size_t min_size = size;
	if (min_size > old_size)
		min_size = old_size;
	memcpy(new_ptr, old_ptr, min_size);
	
	MmFreeUnsafe(old_ptr);
	
	return new_ptr;
}

void MmFreeUnsafe(void* pAddr)
{
	//handle (hopefully) accidental NULL freeing
	if (!pAddr)
		return;
	
	// Free the first page, but before we do, save its g_memoryAllocationSize.
	uint32_t addr = (uint32_t)pAddr;
	addr -= g_pageAllocationBase;
	addr >>= 12;
	if (addr >= (uint32_t)GetHeapSize()) return;
	
	int nSubsequentAllocs = g_memoryAllocationSize[addr];
	
	MmFreePageUnsafe(pAddr);
	pAddr = (void*)((uint8_t*)pAddr+0x1000);
	for (int i = 0; i<nSubsequentAllocs; i++)
	{
		MmFreePageUnsafe(pAddr);
		pAddr = (void*)((uint8_t*)pAddr+0x1000);
	}
}
void MmFreeUnsafeK(void* pAddr)
{
	Heap *pHeapBkp = g_pHeap;
	ResetToKernelHeapUnsafe();
	MmFreeUnsafe(pAddr);
	UseHeapUnsafe (pHeapBkp);
}

uint32_t* MmGetKernelPageDir()
{
	return g_pageDirectory;
}
uint32_t MmGetKernelPageDirP()
{
	return (uint32_t)g_pageDirectory - BASE_ADDRESS;
}
void MmDebugDump()
{
	int entryCount = 0;
	LogMsg("MmDebugDump: dumping memory allocations on heap 0x%x:", g_pHeap);
	for(int i = 0; i < GetHeapSize(); i++)
	{
		if (g_pageEntries[i].m_bPresent) {
			//S: subsequent mempages, A: author file, AL: author line
			LogMsg("%x:  virt:%x  phys:%x  P%s%s%s%s S:%d A:%s AL:%d",
				i,
				(g_pageAllocationBase + (i << 12)),
				g_pageEntries[i].m_pAddress<<12,
				&" \0W\0"[g_pageEntries[i].m_bReadWrite<<1],
				&" \0S\0"[g_pageEntries[i].m_bUserSuper<<1],
				&" \0A\0"[g_pageEntries[i].m_bAccessed<<1],
				&" \0D\0"[g_pageEntries[i].m_bDirty<<1],
				g_memoryAllocationSize[i],
				g_memoryAllocationAuthor[i],
				g_memoryAllocationAuthorLine[i]
			);
			entryCount += 1 + g_memoryAllocationSize[i];
			i += g_memoryAllocationSize[i];
		}
	}
	LogMsgNoCr("There are %d unfreed allocations.", entryCount);
	if (entryCount)
		LogMsg("");
	else
		LogMsg(" Either you're leak-free or you haven't actually allocated anything.");
	
	LogMsg("Memory usage state: H:%d Kb U:%d Kb F:%d Kb A:%d Kb", entryCount * 4, (GetNumPhysPages()-GetNumFreePhysPages()) * 4, GetNumFreePhysPages() * 4, GetNumPhysPages() * 4);
	LogMsg("H - Heap usage, F - Free Memory, A - Available Memory");
}
