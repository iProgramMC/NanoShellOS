//  ***************************************************************
//  mm/pmm.c - Creation date: 11/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mp (Memory manager, Physical memory manager)

#include <memory.h>
#include "memoryi.h"

extern uint32_t e_placement;

#define FRAME_BITSET_SIZE_INTS  (32768)   //Enough to wrap the entirety of the address space
#define FRAME_BITSET_SIZE_BYTES (32768*4)
#define FRAME_BITSET_SIZE_BITS  (32768*32)

uint32_t g_frameBitset [FRAME_BITSET_SIZE_INTS];

int g_numPagesAvailable = 0;

int MpGetNumAvailablePages()
{
	return g_numPagesAvailable;
}

#define  INDEX_FROM_BIT(a) (a / 32)
#define OFFSET_FROM_BIT(a) (a % 32)

void MpSetFrame (uint32_t frameAddr)
{
	uint32_t frame = frameAddr >> 12;
	uint32_t idx =  INDEX_FROM_BIT (frame),
			 off = OFFSET_FROM_BIT (frame);
	g_frameBitset[idx] |= (1 << off);
}
void MpClearFrame (uint32_t frameAddr)
{
	ASSERT (frameAddr != 0 && "ERROR: MpClearFrame(0) probably shouldn't happen");
	
	uint32_t frame = frameAddr >> 12;
	uint32_t idx = INDEX_FROM_BIT (frame),
			 off = OFFSET_FROM_BIT (frame);
	g_frameBitset[idx] &= ~(1 << off);
}

uint32_t MpFindFreeFrame()
{
	for (uint32_t i = 0; i < FRAME_BITSET_SIZE_INTS; i++)
	{
		//Any bit free?
		if (g_frameBitset[i] != 0xFFFFFFFF)
		{
			//yes, which?
			for (int j = 0; j < 32; j++)
			{
				if (!(g_frameBitset[i] & (1 << j)))
				{
					return i*32 + j;
				}
			}
		}
		//no, continue
	}
	// Out of memory!
	return 0xffffffffu;
}

int MpGetNumFreePages()
{
	int result = 0;
	for (uint32_t i = 0; i < FRAME_BITSET_SIZE_INTS; i++)
	{
		// Any bit free?
		if (g_frameBitset[i] != 0xFFFFFFFF)
		{
			// yes, which?
			for (int j = 0; j < 32; j++)
			{
				if (!(g_frameBitset[i] & (1 << j)))
					result++;
			}
		}
		//no, continue
	}
	
	return result;
}

uintptr_t MpRequestFrame(bool bIsKernelHeap)
{
	uint32_t frame = MpFindFreeFrame();
	if (frame == 0xFFFFFFFFu)
	{
		// Out of memory.
		ILogMsg("Out of memory in MpRequestFrame");
		return 0;
	}
	
	uintptr_t result = frame << 12;
	MpSetFrame(result);
	
	// kernel heap doesn't do COW or anything so we don't need to track reference counts to it
	if (!bIsKernelHeap)
		MrReferencePage(result);
	
	return result;
}

void MmStartupStuff();

void MpInitialize(multiboot_info_t* mbi)
{
	MmStartupStuff();
	
	for (uint32_t i = 0; i < ARRAY_COUNT(g_frameBitset); i++)
	{
		g_frameBitset[i] = 0xFFFFFFFF;
	}
	
	//parse the multiboot mmap and mark the respective memory frames as free:
	int len, addr;
	len = mbi->mmap_length, addr = mbi->mmap_addr;
	
	//adding extra complexity is a no-go for now
	if (addr >= 0x100000)
	{
		ILogMsg("OS state not supported.  Mmap address: %x  Mmap len: %x", addr, len);
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
		{
			for (int i = 0; i < (int)pMemoryMap->len; i += 0x1000)
			{
				uint32_t addr = pMemoryMap->addr + i;
				if (addr >= e_placement)// || addr < 0x100000)
					MpClearFrame (addr);
			}
		}
	}
	
	// Clear out the kernel, from 0x100000 to 0x600000
	for (uint32_t i = 0x100; i < (e_placement >> 12) + 1; i++)
	{
		MpSetFrame(i << 12); // Set frame as not allocatable
	}
	
	if (mbi->mods_count > 0)
	{
		//Usually the mods table is below 1m. However we can still access it anyway
		//within the first 8m, because the identity mapping goes all the way to 8m.
		if (mbi->mods_addr >= 0x800000)
		{
			ILogMsg("Module table starts at %x.  OS state not supported", mbi->mods_addr);
			KeStopSystem();
		}
		
		//The initrd module is here.
		multiboot_module_t *pModules = (void*) (mbi->mods_addr + 0xc0000000);
		for (MultibootUInt32 i = 0; i < mbi->mods_count; i++)
		{
			// block out the frames this contains from being allocated by the PMM
			for (uint32_t pg = pModules[i].mod_start;
				          pg < pModules[i].mod_end + 0xFFF;
						  pg += 4096)
			{
				MpSetFrame (pg);
			}
		}
	}
	
	g_numPagesAvailable = MpGetNumFreePages();
}
