/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

           ELF Executable module
******************************************/
#include <elf.h>
#include <string.h>
#include <memory.h>

//#define ELF_DEBUG
#ifdef ELF_DEBUG
#define EDLogMsg(...)  SLogMsg(__VA_ARGS__)
#else
#define EDLogMsg(...)
#endif

extern Heap *g_pHeap;

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
		MmFreeK (pProcess->m_pageTablesList[i]);
		pProcess->m_pageTablesList[i] = 0;
	}
	for (uint32_t i=0; i<pProcess->m_pageAllocationCount; i++)
	{
		MmFreeK (pProcess->m_pagesAllocated[i]);
		pProcess->m_pagesAllocated[i] = NULL;
	}
}
void ElfMapAddress(ElfProcess* pProc, void *virt, size_t size, void* data, size_t fileSize)
{
	uint32_t* pageDir = pProc->m_pageDirectory;
	uint32_t sizeToCopy = fileSize;
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
		for (uint32_t i = ptIndex; i < min; i++)
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
			
			if (sizeToCopy > 0)
			{
				memcpy (pageVirt, pointer, sizeToCopy > 4096 ? 4096 : sizeToCopy);
				sizeToCopy -= 4096;
			}
			
			//if we have copied everything, just reserve the pages,
			//I'm sure the executable will put them to good use :)
			
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

int ElfExecute (void *pElfFile, size_t size, const char* pArgs)
{
	EDLogMsg("Loading elf file");
	size += 0; //to circumvent unused warning
	ElfProcess proc;
	memset(&proc, 0, sizeof(proc));
	
	uint8_t* pElfData = (uint8_t*)pElfFile; //to do arithmetic with this
	//check the header.
	ElfHeader* pHeader = (ElfHeader*)pElfFile;
	
	Heap *pBackup = g_pHeap;
	ResetToKernelHeap();
	
	int errCode = ElfIsSupported(pHeader);
	if (errCode != 1) //not supported.
	{
		LogMsg("Got error %d while loading the elf.", errCode);
		return errCode;
	}
	//ElfDumpInfo(pHeader);
	
	bool failed = false;
	
	// Allocate a new page directory for the elf:
	EDLogMsg("(allocating heap...)");
	if (!AllocateHeap (&proc.m_heap, 4096))//2048))//256))
		return ELF_CANT_MAKE_HEAP;
	
	uint32_t* newPageDir = proc.m_heap.m_pageDirectory,
			  newPageDirP= proc.m_heap.m_pageDirectoryPhys;
	
	proc.m_pageDirectory     = newPageDir;
	proc.m_pageDirectoryPhys = newPageDirP;
	
	EDLogMsg("(loading prog hdrs into memory...)");
	for (int i = 0; i < pHeader->m_phNum; i++)
	{
		ElfProgHeader* pProgHeader = (ElfProgHeader*)(pElfData + pHeader->m_phOffs + i * pHeader->m_phEntSize);
		
		void *addr = (void*)pProgHeader->m_virtAddr;
		size_t size1 = pProgHeader->m_memSize, size2 = pProgHeader->m_fileSize;
		int offs = pProgHeader->m_offset;
		
		EDLogMsg("Mapping address %x with size %d", addr, size1);
		if (!addr) 
		{
			EDLogMsg("Found section that doesn't map to anything...  We won't map that.");
			continue;
		}
		
		// Check if the virtaddr is proper
		if (pProgHeader->m_virtAddr < 0xC00000 || pProgHeader->m_virtAddr + pProgHeader->m_memSize >= 0x10000000)
		{
			failed = true;
			break;
		}
		else
		{
			ElfMapAddress (&proc, addr, size1, &pElfData[offs], size2);
		}
	}
	
	if (!failed)
	{
		EDLogMsg("(loaded and mapped everything, activating heap!)");
		UseHeap (&proc.m_heap);
		
		EDLogMsg("(looking for NOBITS sections to zero out...)");
		for (int i = 0; i < pHeader->m_shNum; i++)
		{
			ElfSectHeader* pSectHeader = (ElfSectHeader*)(pElfData + pHeader->m_shOffs + i * pHeader->m_shEntSize);
			void *addr = (void*)pSectHeader->m_addr;
			if (pSectHeader->m_type == SHT_NOBITS)
			{
				//clear
				ZeroMemory(addr, pSectHeader->m_shSize);
			}
		}
		
		EDLogMsg("The ELF setup is done, jumping to the entry! Wish us luck!!!");
		//now that we have switched, call the entry func:
		ElfEntry entry = (ElfEntry)pHeader->m_entry;
		
		int e = entry(pArgs);
		
		EDLogMsg("The Elf Entry exited!!! Cleaning up and quitting...");
		
		g_lastReturnCode = e;
	}
	
	ResetToKernelHeap();
	ElfCleanup (&proc);
	
	if (pBackup)
		UseHeap( pBackup );
	
	return failed ? ELF_INVALID_SEGMENTS : ELF_ERROR_NONE;
}

const char *gElfErrorCodes[] =
{
	"I don't see why you needed to ElfGetErrorMsg, because the elf you tried to execute (%s) ran successfully!",
	"The image file %s is not a valid NanoShell32 application.",
	"The image file %s is valid, but is for a machine type other than the current machine.",
	"The image file %s is valid, but is for a machine type other than the current machine.",
	"The image file %s is valid, but is for a machine type other than the current machine.",
	"The image file %s is valid, but is not ELF Version 1, so not supported.",
	"The image file %s is not a valid NanoShell32 application.",
	"The image file %s is corrupted.\n\nUnable to apply relocations.",
	"Insufficient memory to run this application. Quit one or more NanoShell applications and then try again.",
	"The image file %s is corrupted.\n\nThe segment mapping is invalid.",
};

const char *ElfGetErrorMsg (int error_code)
{
	if (error_code < ELF_ERROR_NONE || error_code >= ELF_ERR_COUNT)
		return "Unknown Elf Execution Error";
	
	return gElfErrorCodes[error_code - ELF_ERROR_NONE];
}
