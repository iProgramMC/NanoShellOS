//  ***************************************************************
//  rt.c - Creation date: 20/02/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// This module's purpose: handle the resource table of a process.
// A resource table can contain icons, bitmaps, and strings.

#include <process.h>
#include <elf.h>
#include <vfs.h>

// pVirtAddr = the section's starting virtual address
static const char* RstNeutralizeOffset(const char* pAddr, const char* pVirtAddr, const char* pRealAddr, int nSectSize)
{
	const char* nothing = "(nothing)";
	int off = pAddr - pVirtAddr;
	if (off < 0 || off >= nSectSize) return nothing;
	return pRealAddr + off;
}

ProgramInfo* RstRetrieveProgramInfoFromFile(const char* pFileName)
{
	int fd = FiOpen(pFileName, O_RDONLY);
	if (fd < 0)
		return NULL;
	
	// read the header
	ElfHeader hdr;
	FiRead(fd, &hdr, sizeof hdr);
	
	if (!ElfCheckHeader(&hdr))
		return NULL;
	
	// read the section headers
	ElfSectHeader* pSectHeaders = MmAllocate(sizeof(ElfSectHeader) * hdr.m_shNum);
	FiSeek(fd, hdr.m_shOffs, SEEK_SET);
	for (int i = 0; i < hdr.m_shNum; i++)
	{
		FiRead(fd, &pSectHeaders[i], sizeof(ElfSectHeader));
		
		int diff = hdr.m_shEntSize - sizeof(ElfSectHeader);
		if (diff > 0)
			FiSeek(fd, diff, SEEK_CUR);
	}
	
	// read the section string table
	ElfSectHeader* pShStrTabHdr = &pSectHeaders[hdr.m_shStrNdx];
	
	char* pShStrTab = MmAllocate(pShStrTabHdr->m_shSize);
	FiSeek(fd, pShStrTabHdr->m_offset, SEEK_SET);
	FiRead(fd, pShStrTab, pShStrTabHdr->m_shSize);
	
	ProgramInfo* pProgInfo = NULL;
	
	// now go through each section header
	for (int i = 0; i < hdr.m_shNum; i++)
	{
		const char* pSectionName = pShStrTab + pSectHeaders[i].m_name;
		if (strcmp(pSectionName, ".nanoshell") == 0)
		{
			// dump the program info we have
			int sizeDifference = sizeof(ProgramInfo) - sizeof(ProgramInfoBlock);
			char* pProgramInfoSection = MmAllocate(pSectHeaders[i].m_shSize + sizeDifference);
			ProgramInfo* pinfo = (ProgramInfo*)pProgramInfoSection;
			FiSeek(fd, pSectHeaders[i].m_offset, SEEK_SET);
			FiRead(fd, pProgramInfoSection + sizeDifference, pSectHeaders[i].m_shSize);
			
			// neutralize the addresses, since they're virtual.
			// This sucks, maybe we should shrink it down a bit
			pinfo->m_info.m_AppName      = RstNeutralizeOffset(pinfo->m_info.m_AppName,      (char*)pSectHeaders[i].m_addr, pProgramInfoSection+sizeDifference, pSectHeaders[i].m_shSize);
			pinfo->m_info.m_AppAuthor    = RstNeutralizeOffset(pinfo->m_info.m_AppAuthor,    (char*)pSectHeaders[i].m_addr, pProgramInfoSection+sizeDifference, pSectHeaders[i].m_shSize);
			pinfo->m_info.m_AppCopyright = RstNeutralizeOffset(pinfo->m_info.m_AppCopyright, (char*)pSectHeaders[i].m_addr, pProgramInfoSection+sizeDifference, pSectHeaders[i].m_shSize);
			pinfo->m_info.m_ProjName     = RstNeutralizeOffset(pinfo->m_info.m_ProjName,     (char*)pSectHeaders[i].m_addr, pProgramInfoSection+sizeDifference, pSectHeaders[i].m_shSize);
			
			// also copy info from the Elf header
			pinfo->m_word_size  = hdr.m_ident[EI_CLASS];
			pinfo->m_byte_order = hdr.m_ident[EI_DATA];
			pinfo->m_os_abi     = hdr.m_ident[EI_OSABI];
			pinfo->m_machine    = hdr.m_machine;
			
			pProgInfo = pinfo;
		}
	}
	
	MmFree(pSectHeaders);
	MmFree(pShStrTab);
	pSectHeaders = NULL;
	pShStrTab = NULL;
	FiClose(fd);
	
	return pProgInfo;
}

Resource* RstLookUpResource(ResourceTable* pTable, int id)
{
	int left = 0, right = pTable->m_nResources - 1, mid;
	
	while (left <= right)
	{
		mid = (left + right) / 2;
		
		Resource* pResource = pTable->m_pResources[mid];
		if (pResource->m_id == id) return pResource;
		
		if (pResource->m_id < id)
			left = mid + 1;
		else
			right = mid - 1;
	}
	
	return NULL;
}

const char* GetStringResource(int resID)
{
	Resource* pRes = ExLookUpResource(resID);
	
	if (!pRes) return NULL;
	if (pRes->m_type != RES_STRING) return NULL;
	
	return (const char*)pRes->m_data;
}

