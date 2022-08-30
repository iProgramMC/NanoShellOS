//  ***************************************************************
//  mm/fault.c - Creation date: 12/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mf (Memory manager, page Fault)

#include <string.h>
#include <memory.h>
#include "memoryi.h"

//#define COW_DEBUG
//#define DAI_DEBUG

#ifdef COW_DEBUG
#define CowDebugLogMsg(...) LogMsg(__VA_ARGS__)
#else
#define CowDebugLogMsg(...)
#endif

#ifdef DAI_DEBUG
#define DaiDebugLogMsg(...) LogMsg(__VA_ARGS__)
#else
#define DaiDebugLogMsg(...)
#endif

int g_nPageFaultsSoFar = 0;//just for statistics

int MmGetNumPageFaults()
{
	return g_nPageFaultsSoFar;
}

uint32_t* MuiGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable);

void MmOnPageFault(Registers *pRegs)
{
	g_nPageFaultsSoFar++;
	
	UserHeap *pHeap = MuGetCurrentHeap();
	
	//SLogMsg("Page fault happened at %x (error code: %x) on Heap %p", pRegs->cr2, pRegs->error_code, pHeap);
	
	union
	{
		struct
		{
			bool bPresent   : 1;
			bool bWrite     : 1;
			bool bUser      : 1;
			bool bResWrite  : 1;
			//...
		};
		
		uint32_t value;
	}
	errorCode;
	
	errorCode.value = pRegs->error_code;
	
	// if the page wasn't marked as PRESENT
	if (!errorCode.bPresent)
	{
		// Wasn't present...
		uint32_t* pPageEntry = NULL;
		bool bIsKernelHeap = false;
		if (pHeap)
		{
			pPageEntry = MuiGetPageEntryAt(pHeap, pRegs->cr2 & PAGE_BIT_ADDRESS_MASK, false);
		}
		if (!pPageEntry)
		{
			// Hrm, maybe it's part of the kernel heap, that can be demand-paged too
			pPageEntry = MhGetPageEntry(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK);
			bIsKernelHeap = true;
		}
		
		if (pPageEntry)
		{
			if (*pPageEntry & PAGE_BIT_PRESENT)
			{
				// Hrm? But the error code said it wasn't present, whatever
				return;
			}
			
			if (*pPageEntry & PAGE_BIT_DAI)
			{
				DaiDebugLogMsg("Page not present, allocating %s...", bIsKernelHeap?" on kernel heap" : "on user heap");
				
				// It's time to map a page here
				uint32_t frame = MpRequestFrame(bIsKernelHeap);
				
				if (frame == 0)
				{
					LogMsg("Out of memory, d'oh!");
					goto _INVALID_PAGE_FAULT;
				}
				
				DaiDebugLogMsg("Got page %x", frame);
				
				*pPageEntry = *pPageEntry & 0xFFF;
				*pPageEntry |= frame;
				*pPageEntry |= PAGE_BIT_PRESENT;
				*pPageEntry &= ~PAGE_BIT_DAI;
				
				MmInvalidateSinglePage(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK);
				
				// Let's go!
				return;
			}
		}
		
		// Uh oh
		goto _INVALID_PAGE_FAULT;
	}
	
	if (errorCode.bWrite)
	{
		// Should be a COW field. Was present...
		uint32_t* pPageEntry = NULL;
		if (pHeap)
		{
			pPageEntry = MuiGetPageEntryAt(pHeap, pRegs->cr2 & PAGE_BIT_ADDRESS_MASK, false);
		}
		if (!pPageEntry)
		{
			// Da hell? But the error code said that the page was present
			// Kernel heap can't CoW, so fault here
			goto _INVALID_PAGE_FAULT;
		}
		
		if (*pPageEntry & PAGE_BIT_COW)
		{
			// It's show time...
			uint32_t oldFrame = *pPageEntry & PAGE_BIT_ADDRESS_MASK;
			
			CowDebugLogMsg("Page supposed to be copied on write, was frame %x", oldFrame);
			
			uint32_t refCount = MrGetReferenceCount(oldFrame);
			if (refCount == 1)
			{
				CowDebugLogMsg("Since its reference count is one, there's no need to duplicate this");
				
				// this is the only reference, don't duplicate
				*pPageEntry |= PAGE_BIT_READWRITE | PAGE_BIT_PRESENT;
				*pPageEntry &= ~PAGE_BIT_COW;
				
				MmInvalidateSinglePage(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK); // Refresh
			}
			else if (refCount == 0)
			{
				CowDebugLogMsg("Huh? Page's reference count is zero??");
			}
			else
			{
				CowDebugLogMsg("Since its reference count is %d, we need to duplicate this", refCount);
				
				MrUnreferencePage(oldFrame);
				
				uint8_t data[PAGE_SIZE];
				memcpy(data, (void*)(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK), PAGE_SIZE);
				
				uint32_t frame = MpRequestFrame(false);
				
				if (frame == 0)
				{
					LogMsg("Out of memory, d'oh!");
					goto _INVALID_PAGE_FAULT;
				}
				
				*pPageEntry = *pPageEntry & 0xFFF;
				*pPageEntry |= frame;
				*pPageEntry |= PAGE_BIT_READWRITE;
				*pPageEntry &= ~PAGE_BIT_COW;
				
				MmInvalidateSinglePage(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK); // Refresh
				
				memcpy ((void*)(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK), data, PAGE_SIZE);
			}
			
			// Go!
			return;
		}
	}
	
_INVALID_PAGE_FAULT:
	LogMsg("Invalid page fault at EIP: %x. CR2: %x. ErrorCode: %x", pRegs->eip, pRegs->cr2, pRegs->error_code);
	LogMsg("Registers:");
	LogMsg("EAX: %x  EBX: %x  ECX: %x  EDX: %x  ESP: %x  EBP: %x",pRegs->eax, pRegs->ebx, pRegs->ecx, pRegs->edx, pRegs->esp, pRegs->ebp);
	KeStopSystem();
}

