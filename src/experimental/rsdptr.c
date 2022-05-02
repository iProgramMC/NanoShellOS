#include <main.h>
#include <console.h>
#include <print.h>
#include <string.h>
#include <memory.h>
#ifdef EXPERIMENTAL_RSDPTR

typedef struct
{
	char     m_signature[8];
	uint8_t  m_checksum;
	char 	 m_oemID[6];
	uint8_t  m_revision;
	uint32_t m_rsdtAddress; // A physical address
	
	//for RSDP V2.0
	uint32_t m_length;
	uint64_t m_xsdtAddress; // A physical address
	uint8_t  m_checksumExt;
	uint8_t  m_reserved[3];
}
__attribute__ ((packed))
RsdpDesc;

typedef struct
{
	char     m_signature[4];
	uint32_t m_length;
	uint8_t  m_revNum;
	uint8_t  m_checksum;
	char     m_oemID[6];
	char     m_oemTableID[8];
	uint32_t m_oemRev;
	uint32_t m_creatorId;
	uint32_t m_creatorRev;
}
__attribute__ ((packed))
AcpiSdtHeader;

typedef struct
{
	AcpiSdtHeader m_header;
	uint32_t      m_otherSdt[1]; //for easy access
}
__attribute__((packed))
RsdtTable;

void *MmMapPhysMemFast(uint32_t page);
void MmUnmapPhysMemFast(void* pMem);


uint32_t RsdpChecksum (RsdpDesc* pDesc)
{
	//only check ACPI V1.0 checksum for now
	uint8_t* pDesc1 = (uint8_t*)pDesc;
	uint8_t* pDesc2 = (uint8_t*)(&pDesc->m_length);
	
	size_t sz = pDesc2 - pDesc1;
	
	ASSERT (sz == 20 && "RsdpChecksum fatal error: Why isn't the RSDP V1.0 structure size 20 bytes?");
	
	uint32_t cksum = 0;
	for (int i = 0; i < (int)sz; i++)
	{
		cksum += pDesc1[i];
	}
	
	return cksum;
}

RsdtTable* g_pRSDTTable;


int RsdtGetOtherSdtCount(RsdtTable *pTable)
{
	return (pTable->m_header.m_length - sizeof (pTable->m_header)) / sizeof (uint32_t);
}

void RsdtAnalyze (RsdtTable *pTable)
{
	int sdtCount = RsdtGetOtherSdtCount(pTable);
	
	for (int i = 0; i < sdtCount; i++)
	{
		AcpiSdtHeader *pMem = MmMapPhysMemFast(pTable->m_otherSdt[i]);
		if (!pMem)
		{
			LogMsg("Could not read other SDTs - memory allocation failed.");
			return;
		}
		
		SLogMsg("SDT found: %c%c%c%c", pMem->m_signature[0], pMem->m_signature[1],
				 pMem->m_signature[2], pMem->m_signature[3]);
		
		MmUnmapPhysMemFast(pMem);
	}
}

void RsdpAnalyze(RsdpDesc* pDesc)
{
	ASSERT(g_pRSDTTable == NULL);
	
	//map it to some high memory address (like 0x3FFFF000)
	
	RsdtTable* pMem = (RsdtTable*)MmMapPhysMemFast(pDesc->m_rsdtAddress);
	
	ASSERT (pMem && "Error mapping RSDT Address. Who know's what's going to happen now!");
	LogMsg("Mapped. pMem: %x", pMem);
	g_pRSDTTable = pMem;
	
	LogMsg("RSDT signature: %c%c%c%c",
			pMem->m_header.m_signature[0], pMem->m_header.m_signature[1],
			pMem->m_header.m_signature[2], pMem->m_header.m_signature[3]);
	
	RsdtAnalyze(g_pRSDTTable);
}

void AttemptLocateRsdPtr()
{
	if (g_pRSDTTable)
	{
		LogMsg("It's already initialized, mate");
		return;
	}
	
	//scan EBDA and BIOS memory
	//TODO: a better way to locate RSD PTR?
	//TODO: will this work on UEFI systems with CSM?
	
	bool bFound = false;
	
	uint8_t* matches[16];
	int matchCount = 0;
	
	for (uint8_t* ptr = (uint8_t*)0xC0080000; ptr != (uint8_t*)(0xC00A0000 - 16); ptr += 16)
	{
		//TODO: maybe just do a *((uint64_t*)ptr) == ... check?
		if (memcmp (ptr, "RSD PTR ", 8) == 0) {
			bFound = true;
			
			if (matchCount < 16)
				matches[matchCount++] = ptr;
			else
				LogMsg("Found RSD PTR at %x (in EBDA). Couldn't fit this many matches so logging them out instead.", ptr);
		}
	}
	
	for (uint8_t* ptr = (uint8_t*)0xC00C0000; ptr != (uint8_t*)(0xC0100000 - 16); ptr += 16)
	{
		if (memcmp (ptr, "RSD PTR ", 8) == 0) {
			bFound = true;
			
			if (matchCount < 16)
				matches[matchCount++] = ptr;
			else
				LogMsg("Found RSD PTR at %x (in EBDA). Couldn't fit this many matches so logging them out instead.", ptr);
		}
	}
	
	LogMsg("Initializing experimental ACPI driver ...");
	
	for (int i = 0; i < matchCount; i++)
	{
		RsdpDesc* pDesc = (RsdpDesc*)matches[i];
		
		//check checksum
		uint32_t calcCksum = RsdpChecksum (pDesc);
		
		if ((calcCksum & 0xFF) != 0)
		{
			LogMsg("This %x ain't a valid rsdp.  calcCksum: %x  pDesc->m_checksum: %x", pDesc, calcCksum);
			continue;
		}
		
		RsdpAnalyze(pDesc);
	}
}

#endif
