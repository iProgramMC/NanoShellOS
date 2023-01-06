//  ***************************************************************
//  fat32.c - Creation date: 06/01/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <fat.h>
#include <debug.h>

#define CEIL_DIV_PO2(thing, divisor) ((thing + (1 << (divisor)) - 1) >> (divisor))

#define C_MAX_FAT32_FILE_SYSTEMS (16)
static const char s_fatLetters[] = "0123456789ABCDEF";
#define ENSURE_READ_OP(op, message) ASSERT(op == DEVERR_SUCCESS && message);

Fat32FileSystem s_fat32FileSystems[C_MAX_FAT32_FILE_SYSTEMS];

DriveStatus Fat32ReadClusters(Fat32FileSystem* pFS, uint32_t clusNo, uint32_t clusCount, void* pMem)
{
	uint32_t lbaStart = pFS->m_lbaStart + (clusNo - 2) * pFS->m_sectorsPerCluster, sectorCount = clusCount * pFS->m_sectorsPerCluster;
	uint8_t* pMem1 = (uint8_t*)pMem;
	
	while (sectorCount)
	{
		// read!
		DriveStatus ds = StDeviceRead( lbaStart, pMem1, pFS->m_driveID, 1 );
		if (ds != DEVERR_SUCCESS)
			return ds;
		
		lbaStart++;
		sectorCount--;
		pMem1 += BLOCK_SIZE;
	}
	
	return DEVERR_SUCCESS;
}

DriveStatus Fat32WriteClusters(Fat32FileSystem* pFS, uint32_t clusNo, uint32_t clusCount, const void* pMem)
{
	uint32_t lbaStart = pFS->m_lbaStart + (clusNo - 2) * pFS->m_sectorsPerCluster, sectorCount = clusCount * pFS->m_sectorsPerCluster;
	const uint8_t* pMem1 = (const uint8_t*)pMem;
	
	while (sectorCount)
	{
		// write!
		DriveStatus ds = StDeviceWrite( lbaStart, pMem1, pFS->m_driveID, 1 );
		if (ds != DEVERR_SUCCESS)
			return ds;
		
		lbaStart++;
		sectorCount--;
		pMem1 += BLOCK_SIZE;
	}
	
	return DEVERR_SUCCESS;
}








void FatReadBPB(Fat32FileSystem* pFS)
{
	union {
		uint8_t sector0[512];
		BiosParameterBlock bpb;
	} a;
	
	StDeviceRead( pFS->m_lbaStart, a.sector0, pFS->m_driveID, 1 );
	
	pFS->m_bpb = a.bpb;
}

bool FatValidateBpb(Fat32FileSystem* pFS)
{
	return memcmp(pFS->m_bpb.m_sSystemID, "FAT32", 5) == 0;
}

void FsMountFat32Partition(DriveID driveID, int partitionStart, int partitionSizeSec)
{
	// Find a Fat32 structure in the list of Fat32 structures.
	int FreeArea = -1;
	for (size_t i = 0; i < ARRAY_COUNT(s_fat32FileSystems); i++)
	{
		if (!s_fat32FileSystems[i].m_bMounted)
		{
			FreeArea = i;
			break;
		}
	}
	
	if (FreeArea == -1) return;
	
	Fat32FileSystem* pFS = &s_fat32FileSystems[FreeArea];
	pFS->m_lbaStart    = partitionStart;
	pFS->m_sectorCount = partitionSizeSec;
	pFS->m_driveID     = driveID;
	FatReadBPB(pFS);
	
	if (!FatValidateBpb(pFS))
	{
		// this isn't even good! Bleh, go away.
		return;
	}
	
	LogMsg("Can mount  this partition");
}

#define SAFE_DELETE(thing) do { if (thing) { MmFree(thing); thing = NULL; } } while (0)

void FatFreeFaTable(FaTable* pFat)
{
	if (pFat->m_pSecGrps)
	{
		for (uint32_t i = 0; i < pFat->m_nSecGrps; i++)
		{
			SAFE_DELETE(pFat->m_pSecGrps[i]);
		}
	}
	
	pFat->m_nSecGrps = 0;
	SAFE_DELETE(pFat->m_pSecGrps);
}

void FsFat32Cleanup(Fat32FileSystem* pFS)
{
	FatFreeFaTable(&pFS->m_fat);
}

void FsFatInit()
{
	// probe each drive
	for (int i = 0; i < 0x100; i++)
	{
		if (!StIsDriveAvailable(i)) continue;
		
		union
		{
			uint8_t bytes[512];
			MasterBootRecord s;
		} mbr;
		
		// read one sector from block 0
		StDeviceRead( 0, mbr.bytes, i, 1 );
		
		// probe each partition
		for (int pi = 0; pi < 4; pi++)
		{
			Partition* pPart = &mbr.s.m_Partitions[pi];
			
			if (pPart->m_PartTypeDesc != 0)
			{
				// This is a valid partition.  Mount it.
				FsMountFat32Partition (i, pPart->m_StartLBA, pPart->m_PartSizeSectors);
			}
		}
		
		// well, we could try without an MBR here too, but I'm too lazy to get the size of the drive itself right now.
	}
}
