/*****************************************
		NanoShell Operating System
		(C)2021-2022 iProgramInCpp

           ELF Executable module
******************************************/
#include <elf.h>
#include <string.h>
#include <memory.h>
#include "mm/memoryi.h"
#include <vfs.h>
#include <task.h>
#include <process.h>
#include <config.h>

//#define ELF_DEBUG
//#define ELFSYM_DEBUG

#ifdef ELF_DEBUG
#define EDLogMsg(...)  SLogMsg(__VA_ARGS__)
#else
#define EDLogMsg(...)
#endif

typedef struct 
{
	UserHeap* m_heap;
}
ElfProcess;

const char* ElfGetArchitectureString(uint16_t machine, uint8_t word_size)
{
	switch (machine)
	{
		case 3:
			if (word_size == 2)
				return "x86 (64-bit)";
			else
				return "i686";
		case 0: return "Unspecified";
		case 2: return "SPARC";
		case 4: return "Motorola 68000";
		case 5: return "Motorola 88000";
		case 8: return "MIPS";
		case 0x28: return "ARM";
		case 0x32: return "Intel Itanium (IA-64)";
		case 0x3E: return "AMD64";
	}
	
	return "Unknown Architecture";
}

const char* ElfGetOSABIString(uint8_t abi)
{
	switch (abi)
	{
		case 0: return "System V";
		case 2: return "NetBSD";
		case 3: return "Linux";
		case 4: return "The GNU Hurd";
		case 6: return "Solaris";
		case 8: return "IRIX";
		case 9: return "FreeBSD";
		case 0xC: return "OpenBSD";
	}
	
	return "Unknown System ABI";
}

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
	uintptr_t virtHint = (uintptr_t)virt & ~(PAGE_SIZE - 1), virtOffset = (uintptr_t)virt & (PAGE_SIZE - 1);
	
	size_t sizePages = (((size + virtOffset - 1) >> 12) + 1);
	
	MuMapMemoryFixedHint(pProc->m_heap, virtHint, sizePages, NULL, true, CLOBBER_SKIP, false, PAGE_BIT_SCRUB_ZERO);
	
	//memset((void*)virtHint, 0, sizePages * PAGE_SIZE);
	memcpy(virt, data, fileSize);
}

void ElfDumpInfo(ElfHeader* pHeader)
{
	LogMsg("ELF info:");
	LogMsg("entry: %x  phoff:  %x  shoff:   %x  phnum: %x", pHeader->m_entry, pHeader->m_phOffs, pHeader->m_shOffs,    pHeader->m_phNum);
	LogMsg("flags: %x  ehsize: %x  phentsz: %x  shnum: %x", pHeader->m_flags, pHeader->m_ehSize, pHeader->m_phEntSize, pHeader->m_shNum);
}

extern int g_lastReturnCode;

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
	void*  pSymTab;
	void*  pStrTab;
	bool   bSetUpSymTab;
	int    nSymTabEntries;
	uint64_t nParentTaskRID;
}
ElfLoaderBlock;

// TODO: Maybe update this to outside of the elf bubble? It could work for other executable files too

ElfSymbol* ElfGetSymbolAtIndex(ElfLoaderBlock* pLBlock, int index)
{
	if (!pLBlock)          return NULL;
	if (!pLBlock->pSymTab) return NULL;
	if (index < 0 || index >= pLBlock->nSymTabEntries) return NULL;
	
	ElfSymbol* pSymbols = (ElfSymbol*)pLBlock->pSymTab;
	return &pSymbols[index];
}

ElfSymbol* ElfGetSymbolAtAddress(ElfLoaderBlock* pLBlock, uintptr_t address)
{
	if (!pLBlock)          return NULL;
	if (!pLBlock->pSymTab) return NULL;
	
	ElfSymbol* pSymbols = (ElfSymbol*)pLBlock->pSymTab;
	
	// is it right after the end?
	if (pSymbols[pLBlock->nSymTabEntries - 1].m_stValue <= address)
		return NULL;
	
	// is it right before the beginning? if yes, we probably don't want that
	if (address < pSymbols[0].m_stValue)
		return NULL;
	
	// nope, so binary search time! :^)
	int left = 0, right = pLBlock->nSymTabEntries - 1;
	while (left < right)
	{
		int where = (left + right) / 2;
		//this is why we needed to check the end: we're checking if
		//our address is in between these two addresses
		ElfSymbol *pSym = &pSymbols[where], *pNextSym = &pSymbols[where + 1];
		
		if (pSym->m_stValue <= address && pNextSym->m_stValue > address)
		{
			// found!
			return pSym;
		}
		
		// before this, there are ONLY elements that have a smaller address.
		if (pSym->m_stValue < address)
			left = where + 1;
		else
			right = where;
	}
	
	return NULL;
}

ElfSymbol* ExLookUpSymbol(Process* pProc, uintptr_t address)
{
	if (!pProc) return NULL;
	
	// make up a fake loader block
	ElfLoaderBlock lb;
	memset(&lb, 0, sizeof lb);
	lb.pSymTab = pProc->pSymTab;
	lb.pStrTab = pProc->pStrTab;
	lb.nSymTabEntries = pProc->nSymTabEntries;
	
	return ElfGetSymbolAtAddress(&lb, address);
}

bool ElfSymbolLessThan(ElfSymbol* p1, ElfSymbol* p2)
{
	if (p1->m_stValue != p2->m_stValue) return p1->m_stValue < p2->m_stValue;
	if (p1->m_stSize  != p2->m_stSize)  return p1->m_stSize  < p2->m_stSize;
	if (p1->m_stInfo  != p2->m_stInfo)  return p1->m_stInfo  < p2->m_stInfo;
	if (p1->m_stName  != p2->m_stName)  return p1->m_stName  < p2->m_stName;
	return false;
}

// merge sort implementation
void ElfSortSymbolsSub(ElfSymbol* pSymbols, ElfSymbol* pTempStorage, int left, int right)
{
	// do we have a trivial problem?
	if (left >= right) return;
	
	int mid = (left + right) / 2;
	
	// sort the first half
	ElfSortSymbolsSub(pSymbols, pTempStorage, left, mid);
	
	// sort the second half
	ElfSortSymbolsSub(pSymbols, pTempStorage, mid + 1, right);
	
	// interleave the halves
	int indexL = left, indexR = mid + 1, indexI = 0;
	while (indexL <= mid && indexR <= right)
	{
		if (ElfSymbolLessThan(&pSymbols[indexL], &pSymbols[indexR]))
			pTempStorage[indexI++] = pSymbols[indexL++];
		else
			pTempStorage[indexI++] = pSymbols[indexR++];
	}
	
	// append the other parts
	while (indexL <=   mid) pTempStorage[indexI++] = pSymbols[indexL++];
	while (indexR <= right) pTempStorage[indexI++] = pSymbols[indexR++];
	
	// copy the elements back into the array
	memcpy(&pSymbols[left], pTempStorage, (right - left + 1) * sizeof (ElfSymbol));
}

void ElfSortSymbols(ElfSymbol* pSymbols, int nEntries)
{
	ElfSymbol* pIntermediateStorage = MmAllocate(nEntries * sizeof (ElfSymbol));
	
	ElfSortSymbolsSub(pSymbols, pIntermediateStorage, 0, nEntries - 1);
	
	MmFree(pIntermediateStorage);
}

void ElfSetupSymTabEntries(ElfSymbol** pSymbolsPtr, const char* pStrTab, int* pnEntries)
{
	ElfSymbol* pSymbols = *pSymbolsPtr;
	int nEntries = 0, nAllEntries = *pnEntries;
	ElfSymbol* pNewSymbols = MmAllocate(nAllEntries * sizeof *pSymbols);
	
	for (int i = 0; i < nAllEntries; i++)
	{
		ElfSymbol* pSym = &pSymbols[i];
		
		//we already loaded this
		if (pSym->m_stOther == 0xFF) continue;
		
		//ignore symbols with no value, or symbols with no name
		if (pSym->m_stValue == 0) continue;
		if (pSym->m_stName  == 0) continue;
		
		int stName = pSym->m_stName;
		pSym->m_pName   = &pStrTab[stName];
		pSym->m_stOther = 0xFF;
		
		pNewSymbols[nEntries++] = *pSym;
	}
	
	MmFree(pSymbols);
	
	// Sort the symbols for easy load.
	ElfSortSymbols(pNewSymbols, nEntries);
	
	#ifdef ELFSYM_DEBUG
	SLogMsg("Printing symbols ....");
	
	// Dump them
	for (int i = 0; i < nEntries; i++)
	{
		ElfSymbol* pSym = &pNewSymbols[i];
		
		SLogMsgNoCr("Loaded symbol %x", pSym->m_stValue);
		SLogMsg(": '%s'", pSym->m_pName);
	}
	#endif
	// Try to get the address
	
	*pnEntries   = nEntries;
	*pSymbolsPtr = pNewSymbols;
}

static int ElfExecute (void *pElfFile, UNUSED size_t size, const char* pArgs, int *pErrCodeOut, ElfLoaderBlock* pLoaderBlock)
{
	EDLogMsg("Loading elf file");
	
	// The heap associated with this process
	UserHeap *pHeap  = MuGetCurrentHeap();
	
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
		MuUseHeap (pHeap);
		
		int strTabShLink = -1, symTabIndex = -1;
		
		for (int i = 0; i < pHeader->m_shNum; i++)
		{
			ElfSectHeader* pSectHeader = (ElfSectHeader*)(pElfData + pHeader->m_shOffs + i * pHeader->m_shEntSize);
			
			if (pSectHeader->m_type == SHT_SYMTAB)
			{
				strTabShLink = pSectHeader->m_shLink;
				symTabIndex  = i;
				break;
			}
		}
		
		ElfSectHeader* pShStrTabHdr = (ElfSectHeader*)(pElfData + pHeader->m_shOffs + pHeader->m_shStrNdx * pHeader->m_shEntSize);
		
		for (int i = 0; i < pHeader->m_shNum; i++)
		{
			ElfSectHeader* pSectHeader = (ElfSectHeader*)(pElfData + pHeader->m_shOffs + i * pHeader->m_shEntSize);
			if (pSectHeader->m_type == SHT_NOBITS)
			{
				
			}
			if (pSectHeader->m_type == SHT_PROGBITS)
			{
				// check the header's name
				const char* pName = (const char*)(pElfData + pShStrTabHdr->m_offset + pSectHeader->m_name);
				
				if (strcmp(pName, ".nanoshell") == 0)
					ExSetProgramInfo((ProgramInfo*)pSectHeader->m_addr);
				else if (strcmp(pName, ".nanoshell_resources") == 0)
					ExLoadResourceTable((void*)pSectHeader->m_addr);
			}
			if (pSectHeader->m_type == SHT_SYMTAB)
			{
				SLogMsg("Found SymTab. m_offset: %x Size: %x", pSectHeader->m_offset, pSectHeader->m_shSize);
				
				uint8_t* pTableStart = &pElfData[pSectHeader->m_offset];
				size_t   nTableSize  = pSectHeader->m_shSize;
				
				// allocate the symbol table
				void *pTableMem = MmAllocate(nTableSize);
				
				// copy the contents from the ELF data
				memcpy(pTableMem, pTableStart, nTableSize);
				
				// set the loader block's relevant fields
				pLoaderBlock->pSymTab = pTableMem;
				pLoaderBlock->nSymTabEntries = nTableSize / sizeof(ElfSymbol);
				
				// have we loaded the strtab?
				if (pLoaderBlock->pStrTab && !pLoaderBlock->bSetUpSymTab)
				{
					ElfSetupSymTabEntries((ElfSymbol**)&pLoaderBlock->pSymTab, (const char*)pLoaderBlock->pStrTab, &pLoaderBlock->nSymTabEntries);
					pLoaderBlock->bSetUpSymTab = true;
				}
			}
			if (pSectHeader->m_type == SHT_STRTAB && i == strTabShLink)
			{
				SLogMsg("Found StrTab. m_offset: %x Size: %x", pSectHeader->m_offset, pSectHeader->m_shSize);
				
				uint8_t* pTableStart = &pElfData[pSectHeader->m_offset];
				size_t   nTableSize  = pSectHeader->m_shSize;
				
				// allocate the symbol table
				void *pTableMem = MmAllocate(nTableSize);
				
				// copy the contents from the ELF data
				memcpy(pTableMem, pTableStart, nTableSize);
				
				// set the loader block's relevant fields
				pLoaderBlock->pStrTab = pTableMem;
				
				// have we loaded the symtab?
				if (pLoaderBlock->pSymTab && !pLoaderBlock->bSetUpSymTab)
				{
					ElfSetupSymTabEntries((ElfSymbol**)&pLoaderBlock->pSymTab, (const char*)pLoaderBlock->pStrTab, &pLoaderBlock->nSymTabEntries);
					pLoaderBlock->bSetUpSymTab = true;
				}
			}
		}
		
		// now, copy the symtab and strtab data into the process' structure
		Process* pThisProcess = ExGetRunningProc();
		pThisProcess->pSymTab = pLoaderBlock->pSymTab;
		pThisProcess->pStrTab = pLoaderBlock->pStrTab;
		pThisProcess->nSymTabEntries = pLoaderBlock->nSymTabEntries;
		
		EDLogMsg("The ELF setup is done, jumping to the entry! Wish us luck!!!");
		
		//now that we have switched, call the entry func:
		ElfEntry entry = (ElfEntry)pHeader->m_entry;
		
		// this is a bit complex, but I'll break it down. This does a couple things:
		// - push `pArgs` as the only argument (%1)
		// - call `entry` (%2)
		// - restore esp to normal
		// - mark ebx, esi, and edi as being clobbered
		// - the return value is in %0 (eax)
		int returnValue = 0;
		asm("pushl %1\ncall *%2\nadd $4, %%esp" : "=a"(returnValue) : "r"(pArgs), "r"(entry) : "ebx", "esi", "edi");
		
		EDLogMsg("Executable has exited.");
		
		*pErrCodeOut = g_lastReturnCode = returnValue;
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
	"The execution of this program was terminated.",
	"The execution of this file is not permitted at the moment.",
	"The execution of program files is not permitted. Please contact your system administrator.",
};

const char *ElfGetErrorMsg (int error_code)
{
	if (error_code < 0)
		return GetErrNoString(error_code);
	
	if (error_code < ELF_ERROR_NONE || error_code >= ELF_ERR_COUNT)
		return "Unknown Elf Execution Error";
	
	return gElfErrorCodes[error_code - ELF_ERROR_NONE];
}

extern Console* g_currentConsole, g_debugSerialConsole;

static void ElfExecThread(int pnLoaderBlock)
{
	// Load the pLoaderBlock
	ElfLoaderBlock *pBlock = (ElfLoaderBlock*)pnLoaderBlock;
	
	// Create a local copy
	ElfLoaderBlock block = *pBlock;
	
	// If async, pipe all output to the serial console
	if (block.bAsync)
	{
		g_currentConsole = &g_debugSerialConsole;
	}
	
	// Try to load the ELF in
	int erc = ElfExecute (block.pFileData, block.nFileSize, block.sArgs, &block.nElfErrorCodeExec, pBlock);
	
	if (erc != ELF_ERROR_NONE)
	{
		// Show an error code, depending on the bRunFromGui
		SLogMsg("ELF Execution Error: %d", erc);
	}
	
	// free it ourselves, so that ElfOnDeath doesn't try to free this again
	MmFreeK(pBlock->pFileData);
	pBlock->pFileData = NULL;
	block.pFileData = NULL;
	
	//-- the process will free this stuff when it's time
	//SAFE_FREE(pBlock->pSymTab);
	//SAFE_FREE(pBlock->pStrTab);
	
	if (block.bAsync)
	{
		ExGetRunningProc()->pDetail = NULL;
		MmFreeK ((void*)pnLoaderBlock);
	}
	else
	{
		pBlock->bExecDone         = true;
		pBlock->nElfErrorCode     = erc;
		pBlock->nElfErrorCodeExec = block.nElfErrorCodeExec;
		KeExit ();
	}
}

void ElfOnDeath(Process* pProc)
{
	if (pProc->pDetail)
	{
		ElfLoaderBlock* pBlk = (ElfLoaderBlock*)pProc->pDetail;
		
		SAFE_FREE(pBlk->pFileData);
		
		//-- the process will free this stuff when it's time
		//SAFE_FREE(pBlk->pSymTab);
		//SAFE_FREE(pBlk->pStrTab);
		
		if (pBlk->bAsync)
		{
			MmFreeK(pProc->pDetail);
			pProc->pDetail = NULL;
		}
		else
		{
			if (!pBlk->bExecDone)
			{
				pBlk->bExecDone = true;
				pBlk->nElfErrorCode     = ELF_KILLED;
				pBlk->nElfErrorCodeExec = 0;
			}
			
			//if the parent thread doesn't exist, there is no point in keeping the detail here
			if (!KeGetThreadByRID(pBlk->nParentTaskRID))
			{
				MmFreeK(pBlk);
				pProc->pDetail = NULL;
			}
		}
	}
}

// bAsync : Does not wait for the process to die
// pElfErrorCodeOut : The error code returned by the spawned elf executable itself.
int ElfRunProgram(const char *pFileName, const char *pArgs, bool bAsync, bool bGui, int nHeapSize, int *pElfErrorCodeOut)
{
	int fd = FiOpen(pFileName, O_RDONLY | O_EXEC);
	if (fd < 0)
	{
		// Show an error code, depending on the bRunFromGui
		SLogMsg("Couldn't open file: %d", fd);
		return fd;
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
	pBlock->nParentTaskRID    = KeGetRunningTask()->m_nIdentifier;
	
	SLogMsg("Block : %p", pBlock);
	
	// Create a new process
	int erc = 0;
	Process *pProc = ExCreateProcess(ElfExecThread, (int)pBlock, pFileName, pBlock->nHeapSize, &erc, pBlock);
	if (!pProc)
	{
		MmFreeK (pBlock);
		return ELF_PROCESS_ERROR;
	}
	pProc->OnDeath = ElfOnDeath;
	
	// If this is an async execution, our job is done, and the KeExecThread will continue.
	if (bAsync) return ELF_ERROR_NONE;
	
	// Otherwise, wait until you're done.
	WaitProcess (pProc);
	
	// Ok, execution is complete. Free all related data
	int error_code_obtained = pBlock->nElfErrorCode;
	*pElfErrorCodeOut = pBlock->nElfErrorCodeExec;
	
	MmFree(pBlock);
	
	return error_code_obtained;
}
