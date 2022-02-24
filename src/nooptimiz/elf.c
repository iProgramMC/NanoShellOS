/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

           ELF Executable module
******************************************/
#include <elf.h>
#include <string.h>
#include <memory.h>

typedef struct 
{
	uint32_t* m_pageDirectory;
	uint32_t  m_pageDirectoryPhys;
	uint32_t* m_pageTablesList[1024];
	#define MAX_PAGE_ALLOC_COUNT 4096
	uint32_t  m_pageAllocationCount;
	uint32_t* m_pagesAllocated[MAX_PAGE_ALLOC_COUNT];
	
	Heap m_heap;
}
ElfProcess;

bool ElfCheckHeader(ElfHeader* pHeader)
{
	if (!pHeader) return false;
	
	if (pHeader->m_ident[EI_MAG0] != ELFMAG0) return false;
	if (pHeader->m_ident[EI_MAG1] != ELFMAG1) return false;
	if (pHeader->m_ident[EI_MAG2] != ELFMAG2) return false;
	if (pHeader->m_ident[EI_MAG3] != ELFMAG3) return false;
	return true;
}
int ElfIsSupported(ElfHeader* pHeader)
{
	if (!ElfCheckHeader(pHeader)) return ELF_HEADER_INCORRECT;
	
	if (pHeader->m_ident[EI_CLASS] != ELF_CLASS_32BIT) 		return ELF_CLASS_INCORRECT; //The image file %s is for a different address-width, which we don't understand.
	if (pHeader->m_ident[EI_DATA ] != ELF_DATA_BYTEORDER)	return ELF_ENDIANNESS_INCORRECT; //The image file %s is valid, but is for a machine type other than the current machine.
	if (pHeader->m_ident[EI_DATA ] != ELF_VER_CURRENT)		return ELF_VERSION_INCORRECT; //The image file %s is ELF Version %d, currently unsupported.
	if (pHeader->m_machine != ELF_MACH_386)					return ELF_MACHINE_INCORRECT; //The image file %s is valid, but is for a machine type other than the current machine.
	if (pHeader->m_type != ELF_TYPE_RELOC &&
		pHeader->m_type != ELF_TYPE_EXEC)					return ELF_FILETYPE_INCORRECT; //The image file %s is a type of executable we don't understand.
	
	return true;
}

void ElfCleanup (ElfProcess* pProcess)
{
	FreeHeap (&pProcess->m_heap);
	pProcess->m_pageDirectory = NULL;
	for (int i=0; i<1024; i++) {
		MmFree (pProcess->m_pageTablesList[i]);
		pProcess->m_pageTablesList[i] = 0;
	}
	for (uint32_t i=0; i<pProcess->m_pageAllocationCount; i++)
	{
		MmFree (pProcess->m_pagesAllocated[i]);
		pProcess->m_pagesAllocated[i] = NULL;
	}
}
void ElfMapAddress(ElfProcess* pProc, void *virt, size_t size, void* data)
{
	uint32_t* pageDir = pProc->m_pageDirectory;
	size = (((size-1) >> 12) + 1) << 12;
	
	uint32_t pdIndex =  (uint32_t)virt >> 22;
	uint32_t ptIndex = ((uint32_t)virt >> 12) & 0x3FF;
	uint32_t size1 = size;
	
	// A page table maps 4 MB of memory, or (1 << 22) bytes.
	uint32_t pageTablesNecessary = ((size1 - 1) >> 22) + 1;
	uint32_t pagesNecessary = ((size1 - 1) >> 12) + 1;
	
	uint32_t* pointer = (uint32_t*)data;
	
	while (pageTablesNecessary)
	{
		uint32_t phys = 0;
		PageEntry *pageTVirt;
		if (pProc->m_pageTablesList[pdIndex])
		{
			pageTVirt = (PageEntry*)pProc->m_pageTablesList[pdIndex];
		}
		else 
		{
			pageTVirt = (PageEntry*)MmAllocateSinglePagePhy(&phys);
			pageDir[pdIndex] = phys | 0x3; //present and read/write
			pProc->m_pageTablesList[pdIndex] = (uint32_t*)pageTVirt;
			ZeroMemory (pageTVirt, 4096);
		}
		
		uint32_t min = 4096;
		if (min > pagesNecessary+ptIndex)
			min = pagesNecessary+ptIndex;
		for (uint32_t i=ptIndex; i<min; i++)
		{
			uint32_t phys2 = 0;
			void *pageVirt = MmAllocateSinglePagePhy(&phys2);
			ZeroMemory (pageVirt, 4096);
			pageTVirt[i].m_pAddress = phys2 >> 12;
			pageTVirt[i].m_bPresent = true;
			pageTVirt[i].m_bUserSuper = true;
			pageTVirt[i].m_bReadWrite = true;
			
			//register our new page in the elfprocess:
			pProc->m_pagesAllocated[pProc->m_pageAllocationCount++] = pageVirt;
			
			memcpy (pageVirt, pointer, 4096);
			//LogMsg("Copied section, need some more?");
			pointer += 1024;
		}
		pageTablesNecessary--; 
		pagesNecessary -= 4096-ptIndex;
		ptIndex = 0;
		pdIndex++;
	}
}

void ElfDumpInfo(ElfHeader* pHeader)
{
	LogMsg("ELF info:");
	LogMsg("entry: %x  phoff:  %x  shoff:   %x  phnum: %x", pHeader->m_entry, pHeader->m_phOffs, pHeader->m_shOffs,    pHeader->m_phNum);
	LogMsg("flags: %x  ehsize: %x  phentsz: %x  shnum: %x", pHeader->m_flags, pHeader->m_ehSize, pHeader->m_phEntSize, pHeader->m_shNum);
}

extern int g_lastReturnCode;

int ElfExecute (void *pElfFile, size_t size)
{
	size += 0; //to circumvent unused warning
	ElfProcess proc;
	memset(&proc, 0, sizeof(proc));
	
	uint8_t* pElfData = (uint8_t*)pElfFile; //to do arithmetic with this
	//check the header.
	ElfHeader* pHeader = (ElfHeader*)pElfFile;
	
	int errCode = ElfIsSupported(pHeader);
	if (errCode != 1) //not supported.
	{
		LogMsg("Got error %d while loading the elf.", errCode);
		return errCode;
	}
	//ElfDumpInfo(pHeader);
	// Allocate a new page directory for the elf:
	
	if (!AllocateHeap (&proc.m_heap, 256))
		return ELF_CANT_MAKE_HEAP;
	
	uint32_t* newPageDir = proc.m_heap.m_pageDirectory,
			  newPageDirP= proc.m_heap.m_pageDirectoryPhys;
	
	proc.m_pageDirectory     = newPageDir;
	proc.m_pageDirectoryPhys = newPageDirP;
	
	for (int i = 0; i < pHeader->m_phNum; i++)
	{
		ElfProgHeader* pProgHeader = (ElfProgHeader*)(pElfData + pHeader->m_phOffs + i * pHeader->m_phEntSize);
		
		void *addr = (void*)pProgHeader->m_virtAddr;
		size_t size1 = pProgHeader->m_memSize;
		int offs = pProgHeader->m_offset;
		
		ElfMapAddress (&proc, addr, size1, &pElfData[offs]);
		//MmUsePageDirectory(newPageDir, newPageDirP);
		//LogMsg("TEST!");
		//MmRevertToKernelPageDir();
	}
	
	//now that we have switched, call the entry func:
	ElfEntry entry = (ElfEntry)pHeader->m_entry;
	//MmUsePageDirectory(newPageDir, newPageDirP);
	
	UseHeap (&proc.m_heap);
	
	//LogMsg("Loaded ELF successfully! Executing it now.");
	int e = entry();
	
	//LogMsg("(executable returned: %d)", e);
	
	g_lastReturnCode = e;
	ResetToKernelHeap();
	ElfCleanup (&proc);
	
	return ELF_ERROR_NONE;
}
