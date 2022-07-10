/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

           Memory Manager module
******************************************/
#include <memory.h>
#include <print.h>
#include <string.h>
#include <vga.h>
#include <misc.h>

uint32_t g_frameBitset [32768];//Enough to wrap the entirety of the address space

extern uint32_t* e_frameBitsetVirt;
extern uint32_t e_frameBitsetSize;
extern uint32_t e_placement;

extern void MmStartupStuff(); //io.asm

void MmInitPrimordial()
{
	MmStartupStuff();
	e_frameBitsetSize = 131072;
	e_frameBitsetVirt = g_frameBitset;
}

int g_numPagesAvailable = 0;

int GetNumPhysPages()
{
	return g_numPagesAvailable;
}

#define  INDEX_FROM_BIT(a) (a / 32)
#define OFFSET_FROM_BIT(a) (a % 32)
void MmSetFrame (uint32_t frameAddr)
{
	uint32_t frame = frameAddr >> 12;
	uint32_t idx = INDEX_FROM_BIT (frame),
			 off =OFFSET_FROM_BIT (frame);
	e_frameBitsetVirt[idx] |= (0x1 << off);
}
void MmClrFrame (uint32_t frameAddr)
{
	uint32_t frame = frameAddr >> 12;
	uint32_t idx = INDEX_FROM_BIT (frame),
			 off =OFFSET_FROM_BIT (frame);
	e_frameBitsetVirt[idx] &=~(0x1 << off);
}

uint32_t MmFindFreeFrame()
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
