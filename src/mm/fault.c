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

#define DAI_SCRUB_BYTE (0xCE)

#ifdef COW_DEBUG
#define CowDebugLogMsg(...) SLogMsg(__VA_ARGS__)
#else
#define CowDebugLogMsg(...)
#endif

#ifdef DAI_DEBUG
#define DaiDebugLogMsg(...) SLogMsg(__VA_ARGS__)
#else
#define DaiDebugLogMsg(...)
#endif

int g_nPageFaultsSoFar = 0;//just for statistics

int MmGetNumPageFaults()
{
	return g_nPageFaultsSoFar;
}

uint32_t* MuiGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable);
void IsrExceptionCommon(int code, Registers* pRegs);

void MmOnPageFault(Registers *pRegs)
{
	g_nPageFaultsSoFar++;
	
	UserHeap *pHeap = MuGetCurrentHeap();
	
	KeGetThreadStats()->m_pageFaults++;
	
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
			// Hrm, maybe it's part of the kernel page based allocator, that can be demand-paged too
			bIsKernelHeap = true;
			pPageEntry = MhGetPageEntry(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK);
			
			// if it's part of the kernel mapper
			if (!pPageEntry)
			{
				pPageEntry = MkiGetPageEntryAt(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK);
			}
		}
		
		if (pPageEntry)
		{
			if (*pPageEntry & PAGE_BIT_PRESENT)
			{
				// Hrm? But the error code said it wasn't present, whatever
				SLogMsg("The page entry said you were present");
				return;
			}
			
			if (*pPageEntry & PAGE_BIT_DAI)
			{
				DaiDebugLogMsg("Page not present, allocating %x%s...", pRegs->cr2, bIsKernelHeap?" on kernel heap" : " on user heap");
				
				// It's time to map a page here
				uint32_t frame = MpRequestFrame(bIsKernelHeap);
				
				if (frame == 0)
				{
					SLogMsg("Out of memory, d'oh!");
					goto _INVALID_PAGE_FAULT;
				}
				
				DaiDebugLogMsg("Got page %x", frame);
				
				// If the page entry had PAGE_BIT_SCRUB_ZERO set, scrub with zero instead of DAI_SCRUB_BYTE
				uint8_t nScrubByte = DAI_SCRUB_BYTE;
				if (*pPageEntry & PAGE_BIT_SCRUB_ZERO)
					nScrubByte = 0;
				
				*pPageEntry = *pPageEntry & 0xFFF;
				*pPageEntry |= frame;
				*pPageEntry |= PAGE_BIT_PRESENT;
				*pPageEntry &= ~PAGE_BIT_DAI;
				
				MmInvalidateSinglePage(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK);
				
				// TODO: figure out why removing this crashes programs
				memset((void*)(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK), nScrubByte, PAGE_SIZE);
				
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
					SLogMsg("Out of memory, d'oh!");
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
	SLogMsg("Invalid page fault at EIP: %x. CR2: %x. ErrorCode: %x", pRegs->eip, pRegs->cr2, pRegs->error_code);
	
	PrintBackTrace((StackFrame*)pRegs->ebp, (uintptr_t)pRegs->eip, NULL, NULL, false);
	
	// No Problem. Just trigger an bug check
	IsrExceptionCommon(BC_EX_PAGE_FAULT, pRegs);
}

