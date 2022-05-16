/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

           ELF Executable module
******************************************/
#include <elf.h>
#include <string.h>
#include <memory.h>
#include <vfs.h>
#include <task.h>
#include <process.h>

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
	#define MAX_PAGE_ALLOC_COUNT 4096 //max: 16 MB, should be plenty.
	uint32_t  m_pageAllocationCount;
	uint32_t* m_pagesAllocated[MAX_PAGE_ALLOC_COUNT];
	
	Heap* m_heap;
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

void ElfCleanup (UNUSED ElfProcess* pProcess)
{
}
void ElfMapAddress(ElfProcess* pProc, void *virt, size_t size, void* data, size_t fileSize)
{
	uint32_t* pageDir = pProc->m_pageDirectory;
	uint32_t sizeToCopy = fileSize;
	size = (((size-1) >> 12) + 1) << 12;
	
	uint32_t pdIndex =  (uint32_t)virt >> 22;
	uint32_t ptIndex = ((uint32_t)virt >> 12) & 0x3FF;
	uint32_t piIndex = ((uint32_t)virt      ) & 0xFFF;
	
	// A page table maps 4 MB of memory, or (1 << 22) bytes.
	//uint32_t pageTablesNecessary = ((size1 - 1) >> 22) + 1;
	
	uintptr_t start = (uintptr_t)virt & ~0xFFF;
	uintptr_t end   = ((uintptr_t)virt + size);
	//ceil it
	end = ((end >> 12) + !!(end & 0xFFF)) << 12;
	
	uint32_t pagesNecessary      = 1 + (end >> 12) - (start >> 12) - 1;
	uint32_t pageTablesNecessary = 1 + (end >> 22) - (start >> 22);
	
	EDLogMsg("Virt: %x size: %x (%d)",virt,size,size);
	EDLogMsg("Pages necessary: %d   Page Tables necessary: %d", pagesNecessary, pageTablesNecessary);
	
	uint8_t* pointer = (uint8_t*)data;
	
	while (pageTablesNecessary)
	{
		EDLogMsg("Requesting page table %x.  Page Tables Left: %x", pdIndex, pageTablesNecessary);
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
		
		uint32_t min = 1024;
		if (min > pagesNecessary+ptIndex)
			min = pagesNecessary+ptIndex;
		
		int pagesAllocated = min - ptIndex;
		for (uint32_t i = ptIndex; i < min; i++)
		{
			EDLogMsg("Requesting page %x. Place to copy data from: %x", i, pointer);
			
			uint32_t phys2 = 0;
			void *pageVirt = MmAllocateSinglePagePhy(&phys2);
			ZeroMemory (pageVirt, 4096);
			pageTVirt[i].m_pAddress = phys2 >> 12;
			pageTVirt[i].m_bPresent = true;
			pageTVirt[i].m_bUserSuper = true;
			pageTVirt[i].m_bReadWrite = true;
			
			//register our new page in the elfprocess:
			pProc->m_pagesAllocated[pProc->m_pageAllocationCount++] = pageVirt;
			
			uint32_t me = 4096 - piIndex;
			piIndex = 0;
			
			int minSize = sizeToCopy > me ? me : sizeToCopy;
			if (minSize > 0)
			{
				memcpy (pageVirt + piIndex, pointer, minSize);
				sizeToCopy -= minSize;
			}
			
			//if we have copied everything, just reserve the pages,
			//I'm sure the executable will put them to good use :)
			
			pointer += minSize;
		}
		pageTablesNecessary--; 
		pagesNecessary -= pagesAllocated;
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

static int ElfExecute (void *pElfFile, UNUSED size_t size, const char* pArgs, int *pErrCodeOut)
{
	EDLogMsg("Loading elf file");
	
	// The heap associated with this process
	Heap *pHeap  = g_pHeap;
	
	ElfProcess proc;
	memset(&proc, 0, sizeof(proc));
	
	proc.m_heap = pHeap;
	
	uint8_t* pElfData = (uint8_t*)pElfFile; //to do arithmetic with this
	//check the header.
	ElfHeader* pHeader = (ElfHeader*)pElfFile;
	
	int errCode = ElfIsSupported(pHeader);
	if (errCode != 1) //not supported.
	{
		LogMsg("Got error %d while loading the elf.", errCode);
		return errCode;
	}
	
	proc.m_pageDirectory     = proc.m_heap->m_pageDirectory;
	proc.m_pageDirectoryPhys = proc.m_heap->m_pageDirectoryPhys;
	
	bool failed = false;
	
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
		if (pProgHeader->m_virtAddr < 0xB00000 || pProgHeader->m_virtAddr + pProgHeader->m_memSize >= 0x10000000)
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
		UseHeap (pHeap);
		
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
		
		EDLogMsg("Executable has exited.");
		
		*pErrCodeOut = g_lastReturnCode = e;
	}
	
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
	"The image file %s does not exist.",
	"The image file %s could not be opened due to unspecified I/O error.",
	"Insufficient memory to run this application. Quit one or more NanoShell applications and then try again.",
	"Insufficient memory to run this application. Quit one or more NanoShell applications and then try again.",
	"Execution of this executable was killed.",
};

const char *ElfGetErrorMsg (int error_code)
{
	if (error_code < ELF_ERROR_NONE || error_code >= ELF_ERR_COUNT)
		return "Unknown Elf Execution Error";
	
	return gElfErrorCodes[error_code - ELF_ERROR_NONE];
}

typedef struct
{
	void*  pFileData;
	size_t nFileSize;
	bool   bGui;        //false if ran from command shell
	bool   bAsync;      //false if the parent is waiting for this to finish
	bool   bExecDone;   //true if the execution of this process has completed
	int    nHeapSize;   //can be dictated by user settings later, for now it should be 512
	char   sArgs[1024];
	char   sFileName[PATH_MAX+5];
	int    nElfErrorCode;     // The error code that the ELF executive returns if the ELF file didn't run
	int    nElfErrorCodeExec; // The error code that the ELF file itself returns.
}
ElfLoaderBlock;

static void ElfExecThread(int pnLoaderBlock)
{
	// Load the pLoaderBlock
	ElfLoaderBlock block = *((ElfLoaderBlock*)pnLoaderBlock);
	
	// Make a clone of the elf data, so that in the event that this thread dies, it won't leak memory
	void *pMem = MmAllocate (block.nFileSize);
	memcpy (pMem, block.pFileData, block.nFileSize);
	MmFreeK(block.pFileData);
	block.pFileData = pMem;
	
	// Try to load the ELF in
	int erc = ElfExecute (block.pFileData, block.nFileSize, block.sArgs, &block.nElfErrorCodeExec);
	
	if (erc != ELF_ERROR_NONE)
	{
		// Show an error code, depending on the bRunFromGui
		SLogMsg("ELF Execution Error: %d", erc);
	}
	
	MmFreeK(block.pFileData);
	
	if (block.bAsync)
	{
		ExGetRunningProc()->pDetail = NULL;
		MmFreeK ((void*)pnLoaderBlock);
	}
	else
	{
		ExGetRunningProc()->pDetail = NULL; // the runner will handle us anyway.
		((ElfLoaderBlock*)pnLoaderBlock)->bExecDone         = true;
		((ElfLoaderBlock*)pnLoaderBlock)->nElfErrorCode     = erc;
		((ElfLoaderBlock*)pnLoaderBlock)->nElfErrorCodeExec = block.nElfErrorCodeExec;
		KeExit ();
	}
}

void ElfOnDeath(Process* pProc)
{
	if (pProc->pDetail)
	{
		ElfLoaderBlock* pBlk = (ElfLoaderBlock*)pProc->pDetail;
		if (pBlk->bAsync)
		{
			MmFreeK(pProc->pDetail);
			pProc->pDetail = NULL;
		}
		else
		{
			pBlk->bExecDone = true;
			pBlk->nElfErrorCode     = ELF_KILLED;
			pBlk->nElfErrorCodeExec = 0;
		}
	}
}

// bAsync : Does not wait for the process to die
// pElfErrorCodeOut : The error code returned by the spawned elf executable itself.
int ElfRunProgram(const char *pFileName, const char *pArgs, bool bAsync, bool bGui, int nHeapSize, int *pElfErrorCodeOut)
{
	int fd = FiOpen(pFileName, O_RDONLY);
	if (fd < 0)
	{
		// Show an error code, depending on the bRunFromGui
		SLogMsg("Couldn't open file: %d", fd);
		return (fd == -EEXIST) ? ELF_FILE_NOT_FOUND : ELF_FILE_IO_ERROR;
	}
	
	// Get the file size
	size_t nFileSize = FiTellSize (fd);
	
	void* pData = MmAllocateK (nFileSize);
	
	if (!pData)
		return ELF_OUT_OF_MEMORY;
	
	size_t nActualFileSz = FiRead (fd, pData, nFileSize);
	
	FiClose (fd);
	
	// Try to execute it.
	ElfLoaderBlock *pBlock = MmAllocateK (sizeof (ElfLoaderBlock));
	if (!pBlock)
		return ELF_OUT_OF_MEMORY;
	
	// Fill in the block
	memset (pBlock, 0, sizeof *pBlock);
	
	strcpy (pBlock->sFileName, pFileName);
	
	strcpy (pBlock->sArgs,     pFileName);
	
	// If we have arguments at all and we have something inside...
	if (pArgs && *pArgs)
	{
		strcat (pBlock->sArgs, " ");
		strcat (pBlock->sArgs, pArgs);
	}
	
	pBlock->bAsync    = bAsync;
	pBlock->bGui      = bGui;
	pBlock->nHeapSize = nHeapSize;
	pBlock->pFileData = pData;
	pBlock->nFileSize = nActualFileSz;
	pBlock->nElfErrorCode     = 0;
	pBlock->nElfErrorCodeExec = 0;
	
	// Create a new process
	int erc = 0;
	Process *pProc = ExCreateProcess(ElfExecThread, (int)pBlock, pFileName, pBlock->nHeapSize, &erc);
	if (!pProc)
	{
		MmFreeK (pBlock);
		return ELF_PROCESS_ERROR;
	}
	pProc->pDetail = pBlock;
	pProc->OnDeath = ElfOnDeath;
	
	// If this is an async execution, our job is done, and the KeExecThread will continue.
	if (bAsync) return ELF_ERROR_NONE;
	
	// Otherwise, wait until you're done.
	/*while (!pBlock->bExecDone)
	{
		KeTaskDone();
	}*/
	WaitProcess (pProc);
	
	// Ok, execution is complete. Free all related data
	int error_code_obtained = pBlock->nElfErrorCode;
	*pElfErrorCodeOut = pBlock->nElfErrorCodeExec;
	
	MmFreeK (pBlock);
	
	return error_code_obtained;
}
