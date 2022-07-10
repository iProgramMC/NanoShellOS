/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        FILE SYSTEM: FAT 32 module
******************************************/

// TODO: We should probably include some garbage collection in this.
// Right now, it's quite possible to browse yourself to OoM using the cabinet
// if you have a sufficiently intricate directory structure.

// Structure definitions.
#include <fat.h>
FatFileSystem s_Fat32Structures[32];

// Internal FAT code
#if 1

void FatTrimSpaces(char* c) {
	char* pEnd = c + strlen(c) - 1;
	
	while (pEnd >= c)
	{
		if (*pEnd != ' ') return;
		*pEnd = 0;
		pEnd--;
	}
}
void FatGetSector(FatFileSystem* pFS, uint8_t* buffer, uint32_t sector, uint32_t count) {
	//FIXME: A less hacky way to write > 255 sectors.
	int count1 = count;
	if (count1 > 255) count1 = 255;
	StDeviceRead(sector + pFS->m_partStartSec, buffer, pFS->m_driveID, count1);
	if (count > 255)
	{
		FatGetSector(pFS, buffer + 255*512, sector + 255, count - 255);
	}
}
void FatPutSector(FatFileSystem* pFS, uint8_t* buffer, uint32_t sector, uint32_t count) {
	
	//FIXME: A less hacky way to write > 255 sectors.
	int count1 = count;
	if (count1 > 255) count1 = 255;
	StDeviceWrite(sector + pFS->m_partStartSec, buffer, pFS->m_driveID, count1);
	if (count > 255)
	{
		FatPutSector(pFS, buffer + 255*512, sector + 255, count - 255);
	}
}

void FatFlushFat (FatFileSystem *pFS)
{
	SLogMsg("Starting FAT flush. Please wait.");
	for (uint32_t i = 0; i < pFS->m_bpb.m_nSectorsPerFat32; i++)
	{
		if (pFS->m_pbFatModified[i])
		{
			FatPutSector(pFS, (uint8_t*)(pFS->m_pFat + i * 128), pFS->m_fatBeginSector + i, 1);
		}
	}
	memset (pFS->m_pbFatModified, 0, pFS->m_bpb.m_nSectorsPerFat32);
	SLogMsg("FAT flush done.");
}

void FatSetClusterInFAT (FatFileSystem *pFS, uint32_t cluster, uint32_t nextClus)
{
	pFS->m_pFat[cluster] = nextClus;
	pFS->m_pbFatModified[cluster / 128] = true;
}

// TODO: Need to fix this for big endian platforms.
__attribute__((always_inline))
static inline uint16_t FatGetU16(uint8_t* buffer, int offset)
{
	return *((uint16_t*)(buffer + offset));
}
__attribute__((always_inline))
static inline uint32_t FatGetU32(uint8_t* buffer, int offset)
{
	return *((uint32_t*)(buffer + offset));
}

//FatReadBpb
static void FatReadBpb (FatFileSystem *pFS)
{
	BiosParameterBlock *PBPB = &pFS->m_bpb;
	uint8_t Sector0[512];
	FatGetSector(pFS, Sector0, 0, 1);
	
	PBPB->m_nBytesPerSector      = FatGetU16(Sector0, 11);
	PBPB->m_nSectorsPerCluster   =           Sector0 [13];
	PBPB->m_nReservedSectors     = FatGetU16(Sector0, 14);
	PBPB->m_nFAT                 =           Sector0 [16]; // Usually 1, can be 2
	PBPB->m_nDirEntries          = FatGetU16(Sector0, 17);
	PBPB->m_nTotalSectors        = FatGetU16(Sector0, 19);
	PBPB->m_mediaDescType        =           Sector0 [21];
	PBPB->m_nSectorsPerFat12or16 = FatGetU16(Sector0, 22);
	PBPB->m_nSectorsPerTrack     = FatGetU16(Sector0, 24);
	PBPB->m_nHeadsOrSizesOnMedia = FatGetU16(Sector0, 26);
	PBPB->m_nHiddenSectors       = FatGetU32(Sector0, 28);
	PBPB->m_nLargeSectorsOnMedia = FatGetU32(Sector0, 32);
	
	// Extended boot record
	PBPB->m_nSectorsPerFat32     = FatGetU32(Sector0, 36);
	PBPB->m_nFlags               = FatGetU16(Sector0, 40);
	PBPB->m_nFatVersion          = FatGetU16(Sector0, 42);
	PBPB->m_nRootDirStartCluster = FatGetU32(Sector0, 44);
	PBPB->m_sectorNumberFsInfo   = FatGetU16(Sector0, 48);
	PBPB->m_sectorNmbrBckpBtSctr = FatGetU16(Sector0, 50);
	//skip 12 bytes
	PBPB->m_driveNumber          =           Sector0 [64];
	PBPB->m_WindowsFlags         =           Sector0 [65];
	PBPB->m_Signature            =           Sector0 [66];
	PBPB->m_nVolumeID            = FatGetU32(Sector0, 67);
	
	memcpy (PBPB->m_sVolumeLabel, Sector0 + 71, 11); PBPB->m_sVolumeLabel[11] = 0;
	memcpy (PBPB->m_sSystemID   , Sector0 + 82,  8); PBPB->m_sSystemID   [ 8] = 0;
}

uint32_t FatClusterToSector (FatFileSystem *pFS, uint32_t clusterNum)
{
	return pFS->m_clusBeginSector + ((clusterNum - 2) * pFS->m_bpb.m_nSectorsPerCluster);
}

void FatGetCluster (FatFileSystem *pFS, uint8_t* pBuffer, uint32_t clusterNum)
{
	if (clusterNum >= FAT_END_OF_CHAIN)
	{
		LogMsg("Can't get cluster, hit end of chain.  File %s, line %d", __FILE__, __LINE__);
		return;
	}
	uint32_t sector   = FatClusterToSector (pFS, clusterNum);
	uint32_t secCount = pFS->m_bpb.m_nSectorsPerCluster;
	FatGetSector (pFS, pBuffer, sector, secCount);
}

void FatPutCluster (FatFileSystem *pFS, uint8_t* pBuffer, uint32_t clusterNum)
{
	if (clusterNum >= FAT_END_OF_CHAIN)
	{
		LogMsg("Can't get cluster, hit end of chain.  File %s, line %d", __FILE__, __LINE__);
		return;
	}
	uint32_t sector   = FatClusterToSector (pFS, clusterNum);
	uint32_t secCount = pFS->m_bpb.m_nSectorsPerCluster;
	FatPutSector (pFS, pBuffer, sector, secCount);
}

uint32_t FatGetNextCluster (FatFileSystem *pFS, uint32_t cluster)
{
	return pFS->m_pFat[cluster] & 0x0FFFFFFF; // 28 bits.
}

//! Must allocate (1 + nEntries*13) bytes...
void FatParseLFN (char* pNameOut, uint8_t* pEntries, uint32_t nEntries)
{
	int j;
	for (uint32_t i = 0; i < nEntries; i++)
	{
		uint8_t* pEntry  = pEntries + (i * 32);
		uint8_t  nEntry  = (uint8_t)pEntry[0] & 0x0F;
		char   * nameOff = pNameOut + ((nEntry - 1) * 13);
		
		for (j = 1; j < 10; j += 2) //since these are unicode characters
		{
			if (pEntry[j] >= 32 && pEntry[j] <= 127)
				*nameOff = pEntry[j];
			else
				*nameOff = 0;
			nameOff ++;
		}
		for (j = 14; j < 25; j += 2) //since these are unicode characters
		{
			if (pEntry[j] >= 32 && pEntry[j] <= 127)
				*nameOff = pEntry[j];
			else
				*nameOff = 0;
			nameOff ++;
		}
		for (j = 28; j < 31; j += 2) //since these are unicode characters
		{
			if (pEntry[j] >= 32 && pEntry[j] <= 127)
				*nameOff = pEntry[j];
			else
				*nameOff = 0;
			nameOff ++;
		}
		//*nameOff = 0;
	}
}

void FatClearCluster(FatFileSystem* pFS, uint32_t cluster)
{
	uint8_t buffer [pFS->m_clusSize];
	memset (buffer, 0, pFS->m_clusSize);
	FatPutCluster (pFS, buffer, cluster);
}

uint32_t FatAllocateCluster (FatFileSystem *pFS)
{
	uint32_t i, intsPerFat = (512 * pFS->m_bpb.m_nSectorsPerFat32) / 4;
	for (i = pFS->m_clusAllocHint; i < intsPerFat; i++)
	{
		if (pFS->m_pFat[i] == 0)
		{
			FatSetClusterInFAT (pFS, i, FAT_END_OF_CHAIN);
			FatClearCluster    (pFS, i);
			pFS->m_clusAllocHint = i + 1;
			return i;
		}
	}
	for (i = 0; i < pFS->m_clusAllocHint; i++)
	{
		if (pFS->m_pFat[i] == 0)
		{
			FatSetClusterInFAT (pFS, i, FAT_END_OF_CHAIN);
			FatClearCluster    (pFS, i);
			pFS->m_clusAllocHint = i + 1;
			return i;
		}
	}
	return 0x00000000;
}

uint8_t FatFileNameChecksum(char* pFileName)
{
	uint8_t  checksum = pFileName[0];
	for (int i = 1; i < 11; i++)
	{
		checksum = ((checksum & 1) << 7) + (checksum >> 1) + pFileName [i];
	}
	return checksum;
}

static char ToUpper(char c)
{
	if (c >= 'a' && c <= 'z') return(c - 'a' + 'A');
	else return(c);
}
static char ToLower(char c)
{
	if (c >= 'A' && c <= 'Z') return(c - 'A' + 'a');
	else return(c);
}

int GetRandomWithSeed(int *seed)
{
	(*seed) += (int)0xe120fc15;
	uint64_t tmp = (uint64_t)(*seed) * 0x4a39b70d;
	uint32_t m1 = (tmp >> 32) ^ tmp;
	tmp = (uint64_t)m1 * 0x12fad5c9;
	uint32_t m2 = (tmp >> 32) ^ tmp;
	return m2 & 0x7FFFFFFF;//make it always positive.
}

void FatWrite83FileName(const char* pFileName, uint8_t* pBuffer, int seed)
{
	memset (pBuffer, ' ', 11);
	bool isDotOnly = strcmp (pFileName, "..") == 0 || strcmp (pFileName, ".") == 0;
	uint32_t nameLen = strlen (pFileName);
	int dotIndex = -1;
	for (int i = nameLen - 1; i >= 0; i--) {
		if (pFileName[i] == '.') {
			dotIndex = i;
			break; // only the last dot counts.
		}
	}
	
	if (isDotOnly) dotIndex = -1;
	if (dotIndex > 0)
	{
		for (int i = 0; i < 3; i++) {
			uint32_t cIndex = dotIndex + i + 1;
			char a = ToUpper(pFileName[cIndex]);
			uint8_t c = cIndex >= nameLen ? ' ' : a;
			pBuffer[8 + i] = c;
		}
	}
	
	uint32_t firstPartLen = nameLen;
	if (dotIndex > 0) {
		firstPartLen = dotIndex;
	}
	if (firstPartLen > 8) {
		for (int i = 0; i < 6; i++)
			pBuffer[i] = ToUpper(pFileName[i]);
		/*pBuffer[6] = '~';
		pBuffer[7] = '1'; // FIXME TODO: increment this if file already exists*/
		
		/*
			I created some testing files in a folder (testtest1-10.txt) today. Here are their SFNs:
			TESTTE~1.TXT
			TESTTE~2.TXT
			TESTTE~3.TXT
			TESTTE~4.TXT
			TE9A25~1.TXT
			TE9E25~1.TXT
			TE9235~1.TXT
			TE9635~1.TXT
			TE9A35~1.TXT
			TEC13B~1.TXT
			TEC13F~1.TXT
			TEC133~1.TXT
			
			It looks like Windows keeps only the first 2 characters after the first 4 files with the same SFN.
			
			Perhaps the SFN thing is bogus when a LFN entry is on top, so let's just add bogus characters like this.
		*/
		
		for (int i = 2; i < 6; i++)
			pBuffer[i] = "0123456789ABCDEF"[GetRandomWithSeed(&seed) % 16];
		pBuffer[6] = '~';
		pBuffer[7] = '1';
	}
	else
	{
		for (uint32_t i = 0; i < firstPartLen; i++) {
			pBuffer[i] = ToUpper(pFileName[i]);
		}
	}
}

uint8_t* FatLocateEntries (
	FatFileSystem *pFS,
	uint8_t       *pClusterBuffer,
	uint32_t       nStartCluster,
	uint32_t       nCount,
	uint32_t      *pFoundCluster,
	int           *pIndex
)
{
	uint32_t i;
	uint32_t dirsPerClus = pFS->m_clusSize / 32;
	int32_t  index       = -1;
	uint32_t cluster     = nStartCluster;
	
	while (1)
	{
		FatGetCluster (pFS, pClusterBuffer, cluster);
		
		uint32_t inARow = 0;
		for (i = 0; i < dirsPerClus; i++)
		{
			uint8_t *pEntry    = pClusterBuffer + (i * 32);
			uint8_t  firstByte = pEntry[0];
			
			if (firstByte == 0x00 || firstByte == 0xE5)
			{
				// empty directory.  Counting up the inARow variable.
				inARow++;
			}
			else
			{
				inARow = 0;
			}
			
			if (inARow >= nCount)
			{
				index = i - (inARow - 1);
			}
		}
		
		if (index >= 0)
			// We found a spot to put our crap.
			break;
		
		uint32_t nextClus = pFS->m_pFat[cluster];
		if (nextClus >= FAT_END_OF_CHAIN)
		{
			nextClus = FatAllocateCluster (pFS);
			if (nextClus == 0)
			{
				return NULL;
			}
			
			FatClearCluster   (pFS, nextClus);
			FatSetClusterInFAT(pFS, cluster,  nextClus);
			FatSetClusterInFAT(pFS, nextClus, FAT_END_OF_CHAIN);
		}
		cluster = nextClus;
	}
	*pFoundCluster = cluster;
	*pIndex        = index;
	return pClusterBuffer + (index * 32);
}	

void FatWriteLFNEntries(uint8_t *pStart, uint32_t numEntries, const char* pFName, int seed)
{
	char shortfname [12]; shortfname[11] = 0;
	FatWrite83FileName (pFName, (uint8_t*)shortfname, seed);
	uint8_t checksum = FatFileNameChecksum(shortfname);
	
	memset (pStart, 0, 32 * numEntries);
	
	// Write the LFN entries.
	
	uint32_t writtenChars = 0, nameLen = strlen (pFName);
	const char    *pNamePtr = pFName;
	uint8_t *pEntry   = NULL; 
	for (uint32_t i = 0; i < numEntries; i++)
	{
		// Reverse the entry order.
		pEntry = pStart + ((numEntries - 1 - i) * 32);
		
		// Set the entry number.
		pEntry[0]  = i + 1;
		pEntry[13] = checksum;
		
		bool wroteZeroAtEnd = false;
		
		// Characters are 16 bytes in LFN entries (j += 2)
        uint32_t j;
		for (j = 1; j < 10; j += 2)
		{
			if (writtenChars < nameLen)
			{
				pEntry[j] = *pNamePtr;
			}
			else
			{
				if (!wroteZeroAtEnd)
					pEntry[j] = 0;
				else
					*((uint16_t*)&pEntry[j]) = 0xffff;
				wroteZeroAtEnd = true;
			}
			pNamePtr++;
			writtenChars++;
		}
		for (j = 14; j < 25; j += 2)
		{
			if (writtenChars < nameLen)
			{
				pEntry[j] = *pNamePtr;
			}
			else
			{
				if (!wroteZeroAtEnd)
					pEntry[j] = 0;
				else
					*((uint16_t*)&pEntry[j]) = 0xffff;
				wroteZeroAtEnd = true;
			}
			pNamePtr++;
			writtenChars++;
		}
		for (j = 28; j < 31; j += 2)
		{
			if (writtenChars < nameLen)
			{
				pEntry[j] = *pNamePtr;
			}
			else
			{
				if (!wroteZeroAtEnd)
					pEntry[j] = 0;
				else
					*((uint16_t*)&pEntry[j]) = 0xffff;
				wroteZeroAtEnd = true;
			}
			pNamePtr++;
			writtenChars++;
		}
		
		// This is a LFN file entry comprising a LFN filename.
		pEntry[11] = FAT_LFN;
	}
	// Mark the last(first) entry with the end-of-LFN bit
	pStart[0] |= 0x40;
}

void FatPopulateDir (FatFileSystem* pFileSystem, FatDirectory* pDirectory, uint32_t cluster);
void FatPopulateRootDir (FatFileSystem* pFileSystem, FatDirectory* pDirectory)
{
	FatPopulateDir (pFileSystem, pDirectory, 2);
}

uint8_t* FatReadDirEntry (UNUSED FatFileSystem* pFS, uint8_t* pStart, uint8_t* pEnd, FatDirEntry*pDirEnt)
{
	if (pEnd == pStart) return NULL;
	
	uint8_t firstByte = pStart[0];
	uint8_t *entry = pStart;
    if (firstByte == 0x00 || firstByte == 0xE5)
	{
        // NOT A VALID ENTRY!
		LogMsg("FatReadDirEntry: Not valid entry");
		if (pFS->m_bReportErrors)
		{
			pFS->m_nReportedErrors++;
			LogMsg("Invalid file entry was found.");
		}
        return NULL;
    }

    uint32_t LFNCount = 0;
    while (entry[11] == FAT_LFN)
	{
		if (entry[0] & 0x40)
		{
			//First entry.  If we had other LFN entries before then, they're orphaned entries.
			LFNCount = 0;
		}
        LFNCount++;
		if (entry[0] == 0xe5)
		{
			LFNCount = 0;
		}
        entry += 32;
        if (entry == pEnd)
		{
			SLogMsg("FatReadDirEntry: LFN -- Need to load more I guess?");
            return NULL;
        }
		//if (LFNCount > 8) break;
    }
	if (entry[0] == 0xe5)
	{
		if (pFS->m_bReportErrors)
		{
			pFS->m_nReportedErrors++;
			LogMsg("An orphaned long file name entry was detected.");
		}
	}
    if (LFNCount > 0)
	{
		memset(pDirEnt->m_pName, 0, sizeof(pDirEnt->m_pName));
        FatParseLFN(pDirEnt->m_pName, pStart, LFNCount);
    }
    else
	{
		// There's no long file name.
		// Trim up the short filename.
		memcpy(pDirEnt->m_pName, entry, 11);
		pDirEnt->m_pName[11] = 0;
		
		char extension[4];
		memcpy(extension, pDirEnt->m_pName + 8, 3);
		extension[3] = 0;
		FatTrimSpaces(extension);
	
		pDirEnt->m_pName[8] = 0;
		FatTrimSpaces(pDirEnt->m_pName);
	
		if (strlen(extension) > 0) {
			strcat (pDirEnt->m_pName, ".");
			strcat (pDirEnt->m_pName, extension);
		}
		
		int  len = strlen(pDirEnt->m_pName);
		bool lowercase = (entry[12] & FAT_MSFLAGS_NAMELOWER) != 0;
		for (int i = 0; i < len; i++)
		{
			if (lowercase)
				pDirEnt->m_pName[i] = ToLower(pDirEnt->m_pName[i]);
			if (pDirEnt->m_pName[i] == '.')
			{
				lowercase = (entry[12] & FAT_MSFLAGS_EXTELOWER) != 0;
			}
		}
    }

    pDirEnt->m_dirFlags = entry[11];
    uint16_t first_cluster_high = FatGetU16(entry, 20);
    uint16_t first_cluster_low  = FatGetU16(entry, 26);
    pDirEnt->m_firstCluster     = first_cluster_high << 16 | first_cluster_low;
    pDirEnt->m_fileSize         = FatGetU32(entry, 28);
	
	//char debug[40];
	//sprintf(debug, "+%d",  pDirEnt->m_firstCluster);
	//strcat (pDirEnt->m_pName, debug);
		
    return entry + 32;
}

void
FatNextDirEntry (
	FatFileSystem *pFS,
	uint8_t       *pRootCluster,
	uint8_t       *pEntry,
	uint8_t      **pNextEntry,
	FatDirEntry   *pTargetDirEnt,
	uint32_t       cluster,
	uint32_t      *pSecondCluster
)
{
	uint8_t* pEndOfCluster = pRootCluster + pFS->m_clusSize;
	*pNextEntry = FatReadDirEntry (pFS, pEntry, pEndOfCluster, pTargetDirEnt);
	if (!(*pNextEntry))
	{
		// Something went wrong!
		// Either directory entry spans the bounds of a cluster, or
		// the directory entry is invalid.
		// Load the next cluster and retry.
		
		// Figure out how much of the last cluster to "replay"
		uint32_t bytesFromPrevChunk = pEndOfCluster - pEntry;
		*pSecondCluster = FatGetNextCluster (pFS, cluster);
		if (*pSecondCluster >= FAT_END_OF_CHAIN)
		{
			// There's no other directory cluster to load and the previous entry was invalid!
			// It's possible that the file system is corrupted, or that the software is buggy.
			// Either way, report the bug to debug console.
			
			SLogMsg ("ERROR: Bad directory entry (FatNextDirEntry, directory entry ends prematurely).\nDetails:");
			SLogMsg ("FS:%x    (file system pointer)", pFS);
			SLogMsg ("RC:%x %x (root cluster ptr, deref pointer)", pRootCluster, *pRootCluster);
			SLogMsg ("PE:%x %x (entry ptr, deref ptr)", pEntry, *pEntry);
			SLogMsg ("NE:%x %x (next entry ptr, deref pointer)", pNextEntry, *pNextEntry);
			SLogMsg ("TD:%x %x (target dirent ptr, deref pointer)", pTargetDirEnt, *pTargetDirEnt);
			SLogMsg ("SC:%x %x (second cluster ptr, deref pointer)", pSecondCluster, *pSecondCluster);
			SLogMsg ("AC:%x    (actual cluster number)", cluster);
			
			if (pFS->m_bReportErrors)
			{
				pFS->m_nReportedErrors++;
				LogMsg("A file directory is corrupted.");
			}
			
			return;
		}
		
		// Load the cluster after the previous entries.
		FatGetCluster(pFS, pRootCluster + pFS->m_clusSize, *pSecondCluster);
		
		// Set entry to its new location at the beginning of rootCluster.
		pEntry = pRootCluster + pFS->m_clusSize - bytesFromPrevChunk;
		
		// Retry reading the entry.
		*pNextEntry = FatReadDirEntry (pFS, pEntry, pEndOfCluster + pFS->m_clusSize, pTargetDirEnt);
		if (!(*pNextEntry))
		{
			SLogMsg ("ERROR: Bad directory entry (FatNextDirEntry, still cannot read the directory entry).\nDetails:");
			SLogMsg ("FS:%x    (file system pointer)", pFS);
			SLogMsg ("RC:%x %x (root cluster ptr, deref pointer)", pRootCluster, *pRootCluster);
			SLogMsg ("PE:%x %x (entry ptr, deref ptr)", pEntry, *pEntry);
			SLogMsg ("NE:%x %x (next entry ptr, deref pointer)", pNextEntry, *pNextEntry);
			SLogMsg ("TD:%x %x (target dirent ptr, deref pointer)", pTargetDirEnt, *pTargetDirEnt);
			SLogMsg ("SC:%x %x (second cluster ptr, deref pointer)", pSecondCluster, *pSecondCluster);
			SLogMsg ("AC:%x    (actual cluster number)", cluster);
			
			if (pFS->m_bReportErrors)
			{
				pFS->m_nReportedErrors++;
				LogMsg("A file directory is corrupted.");
			}
			
			return;
		}
	}
}

void FatPopulateDir (FatFileSystem* pFS, FatDirectory* pDir, uint32_t cluster)
{
	pDir->m_startCluster = cluster;
	uint32_t dirsPerClus = pFS->m_clusSize / 32;
	uint32_t maxDirs = 0;
	uint32_t entryCount = 0;
	pDir->m_nEntries = 0;
	pDir->m_pEntries = NULL;
	
	while (true)
	{
		maxDirs += dirsPerClus;
		if (pDir->m_pEntries)
		{
			//re-allocate:
			FatDirEntry* pNewDir = (FatDirEntry*)MmAllocateK(maxDirs * sizeof(FatDirEntry));
			memcpy (pNewDir, pDir->m_pEntries, (maxDirs - dirsPerClus) * sizeof(FatDirEntry));
			MmFreeK (pDir->m_pEntries);
			pDir->m_pEntries = pNewDir;
		}
		else
			pDir->m_pEntries = (FatDirEntry*)MmAllocateK(maxDirs * sizeof(FatDirEntry));
		
		// Double the size in case we need to read a dirent that spans the bounds of a cluster.
		uint8_t root_cluster [pFS->m_clusSize * 2];
		FatGetCluster (pFS, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		while ((uint32_t)(entry - root_cluster) < pFS->m_clusSize)
		{
			uint8_t first_byte = *entry;
			if (first_byte == 0x00 || first_byte == 0xE5)
			{
				// This directory entry has never been written or it has been deleted.
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry* target_dirent = pDir->m_pEntries + entryCount;
			FatNextDirEntry (pFS, root_cluster, entry, &nextEntry, target_dirent, cluster, &secondCluster);
			entry = nextEntry;
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			
			entryCount++;
		}
		cluster = FatGetNextCluster (pFS, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	pDir->m_nEntries = entryCount;
}

void FatZeroFatChain (FatFileSystem *pFS, uint32_t startCluster)
{
	uint32_t cluster = startCluster;
	while (cluster < FAT_END_OF_CHAIN && cluster != 0)
	{
		uint32_t next_cluster = pFS->m_pFat[cluster];
		FatSetClusterInFAT (pFS, cluster, 0);
		cluster = next_cluster;
	}
	FatFlushFat (pFS);
}
void FatFreeDirectory (UNUSED FatFileSystem *pFS, FatDirectory* pDir)
{
	//would free their name, but we don't need this
	pDir->m_nEntries = 0;
	pDir->m_pEntries = NULL;
}

bool FatDeleteFile (FatFileSystem* pFS, FatDirectory* pDir, const char* fileName)
{
	int len = strlen(fileName);
	char filenameUpper[len + 1];
	memset(filenameUpper, 0, len+1);
	for(int i = 0; i < len; i++) filenameUpper[i] = ToUpper(fileName[i]);
	
	uint32_t cluster = pDir->m_startCluster;
	uint8_t root_cluster [pFS->m_clusSize * 2];
	FatDirEntry targetDirent;
	
	while (true)
	{
		FatGetCluster (pFS, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		while ((uint32_t)(entry - root_cluster) < pFS->m_clusSize)
		{
			uint8_t first_byte = *entry;
			if (first_byte == 0x00 || first_byte == 0xE5)
			{
				// This directory entry has never been written or it has been deleted.
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatNextDirEntry (pFS, root_cluster, entry, &nextEntry, &targetDirent, cluster, &secondCluster);
			if (strcmp (targetDirent.m_pName, fileName) == 0)
			{
				// We found it! Invalidate all the entries.
				//entry[0] = 0xE5;
				uint8_t* entryptr = entry;
				while (entryptr != nextEntry)
				{
					entryptr[0] = 0xe5;
					entryptr += 32;
				}
				
				FatPutCluster (pFS, root_cluster, cluster);
				
				if (secondCluster)
				{
					FatPutCluster(pFS, root_cluster + pFS->m_clusSize, secondCluster);
				}
				
				FatZeroFatChain (pFS, targetDirent.m_firstCluster);
				//remove name
				FatFlushFat(pFS);
				return true;
			}
			entry = nextEntry;
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			//get rid of name
		}
		cluster = FatGetNextCluster (pFS, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	return false;
}

#endif

// Code to interact with the VFS (read/write/etc)
#if 1

// Files
typedef struct
{
	FatFileSystem* pOpenedIn;
	uint8_t      * pWorkCluster;
	bool           bLoadedWorkCluster;
	uint32_t       nClusterCurrent,  // Cluster number inside the FAT.  Used to continue reading beyond the work cluster
				   nClusterProgress, // Number of clusters that have been passed to reach this point.  Useful to check if "offset" has changed at all.
				   nClusterFirst;    // First cluster.  Used to re-resolve nClusterCurrent if offset has gone back.
	bool           bAllowWriting;
	bool           bIsStreamingFromDisk;//If false, the file is fully loaded within memory
	uint8_t      * pDataPointer;//If bIsStreamingFromDisk is true, this is NULL
	uint32_t       nDataSize;
	uint32_t       nDataCapacity;//how much before we need to reallocate the pDataPointer
	bool           bWasWrittenTo;
}
FatInode;

static FatInode s_FatInodes[FD_MAX];

bool FsFatOpen (FileNode* pFileNode, UNUSED bool read, bool write)
{
	// pFileNode->m_inode    BEFORE OPENING currently represents the first cluster.
	// pFileNode->m_implData represents the FatFileSystem pointer it's on
	
	FatFileSystem* pFS = (FatFileSystem*)pFileNode->m_implData;
	uint32_t nFirstCluster = pFileNode->m_inode;
	
	// look for a free inode:
	int freeInode = -1;
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!s_FatInodes[i].pOpenedIn)
		{
			freeInode = i;
			break;
		}
	}
	if (freeInode == -1) return false;//too many files!
	
	// Prepare the structure.
	FatInode* pNode = &s_FatInodes[freeInode];
	
	pNode->pOpenedIn          = pFS;
	pNode->nClusterCurrent    = pNode->nClusterFirst = nFirstCluster;
	pNode->nClusterProgress   = 0;
	pNode->bAllowWriting      = write;
	pNode->bLoadedWorkCluster = false;
	
	bool cache_all = false || write;//Experimental
	pNode->bWasWrittenTo = false;
	if (cache_all)
	{
		pNode->bIsStreamingFromDisk = true;
		int capacity = pFileNode->m_length;
		if (capacity < 4096)
			capacity = 4096;
		pNode->nDataSize            = pFileNode->m_length;
		pNode->nDataCapacity        = capacity;
		pNode->pDataPointer         = MmAllocateK(capacity);
		if (!pNode->pDataPointer)
		{
			pNode->pOpenedIn = NULL;
			pNode->nClusterCurrent = 0;
			pNode->nDataSize = pNode->nDataCapacity = 0;
			return false;
		}
		
		uint32_t nCurrentCluster = nFirstCluster;
		uint32_t nFileSize = pNode->nDataSize, nOffset = 0;
		do
		{
			uint32_t nSectorSize = nFileSize;
			if (nSectorSize >= pFS->m_clusSize)
				nSectorSize =  pFS->m_clusSize;
			
			FatGetCluster(pFS, &pNode->pDataPointer[nOffset], nCurrentCluster);
			nCurrentCluster = FatGetNextCluster(pFS, nCurrentCluster);
			
			nOffset   += pFS->m_clusSize;
			nFileSize -= pFS->m_clusSize;
		}
		while (nCurrentCluster < FAT_END_OF_CHAIN);
	}
	else
	{
		pNode->bIsStreamingFromDisk = false;
		pNode->pDataPointer         = NULL;
		pNode->nDataSize            = 0;
	}
	
	pFileNode->m_inode = freeInode;
	
	return true;
}

static bool FatUpdateFileEntry(FileNode *pFileNode, uint32_t newSize, uint32_t clusterInfo);

void FsFatClose (FileNode* pFileNode)
{
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return;
	
	if (pNode->bAllowWriting && pNode->bLoadedWorkCluster)
	{
		//Write one last sector
		DebugLogMsg("Writing last cluster!");
		FatPutCluster (pNode->pOpenedIn, pNode->pWorkCluster, pNode->nClusterCurrent);
	}
	
	// Free the pWorkCluster structure, if applicable.
	if (pNode->pWorkCluster)
	{
		MmFreeK(pNode->pWorkCluster);
		pNode->pWorkCluster = NULL;
	}
	
	if (pNode->pDataPointer)
	{
		if (pNode->bWasWrittenTo && pNode->bAllowWriting)
		{
			// Free this chain:
			FatZeroFatChain(pNode->pOpenedIn, pNode->nClusterFirst);
			
			// Allocate a NEW chain:
			uint32_t clustersToAllocate = (pNode->nDataSize + pNode->pOpenedIn->m_clusSize - 1) / pNode->pOpenedIn->m_clusSize;
			uint32_t nOffset = 0, nFirstCluster = 0, nLastCluster = 0;
			
			while (clustersToAllocate)
			{
				uint32_t size = pNode->nDataSize - nOffset;
				if (size > pNode->pOpenedIn->m_clusSize)
					size = pNode->pOpenedIn->m_clusSize;
				
				uint32_t cluster = FatAllocateCluster(pNode->pOpenedIn);
				if (!nFirstCluster) nFirstCluster = cluster;
				
				FatPutCluster(pNode->pOpenedIn, &pNode->pDataPointer[nOffset], cluster);
				
				if (nLastCluster)
					FatSetClusterInFAT(pNode->pOpenedIn, nLastCluster, cluster);
				
				FatSetClusterInFAT(pNode->pOpenedIn, cluster, FAT_END_OF_CHAIN);
				
				nLastCluster = cluster;
				
				nOffset += pNode->pOpenedIn->m_clusSize;
				clustersToAllocate--;
			}
			
			FatFlushFat(pNode->pOpenedIn);
			FatUpdateFileEntry(pFileNode, pNode->nDataSize, nFirstCluster);
			
			pNode->nClusterFirst = nFirstCluster;
		}
		MmFreeK(pNode->pDataPointer);
		pNode->pDataPointer = NULL;
	}
	pNode->bIsStreamingFromDisk = false;
	
	// Deinitialize this fatinode:
	pNode->pOpenedIn = NULL;
	
	// Reset the pFileNode to its default state:
	pFileNode->m_inode = pNode->nClusterFirst;
	pNode->nClusterCurrent = pNode->nClusterFirst = pNode->nClusterProgress = 0;
}


extern FileDescriptor g_FileNodeToDescriptor[FD_MAX];
int                   g_fatsMountedCount         = 0;

void FatWriteCurrentTimeToEntry (uint8_t* pActualEntry, bool creationDateToo)
{
	//Sometimes we may want to leave our fingerprints on the files.  The good thing is, the code works
	uint16_t date_data = 0;
	uint16_t time_data = 0;
	
#ifdef ADD_TIMESTAMP_TO_FILES

	TimeStruct strct = *TmReadTime();
	
	date_data |= (strct.year - 1980) << 9;
	date_data |= (strct.month) << 5;
	date_data |= (strct.day);
	
	time_data |= strct.hours << 11;
	time_data |= strct.minutes << 5;
	time_data |= strct.seconds >> 1;//Seconds are only recorded down to a 2 sec resolution
	
#endif
	if (creationDateToo)
	{
		*((uint16_t*)(&pActualEntry[0x0E])) = time_data; // Create time
		*((uint16_t*)(&pActualEntry[0x10])) = date_data; // Create date
	}
	*((uint16_t*)(&pActualEntry[0x12])) = date_data; // Last access date
	*((uint16_t*)(&pActualEntry[0x16])) = time_data; // Last modify time
	*((uint16_t*)(&pActualEntry[0x18])) = date_data; // Last modify date
}

uint32_t FsFatRead (UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer);
uint32_t FsFatWrite(UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer);

FileNode* FatCreateFileWithData(FileNode *pDirNode, const char* pFileName, uint8_t* pData, size_t size)
{
	FatFileSystem* pOpenedIn = (FatFileSystem*)pDirNode->m_implData;
	size_t fileNameLen = strlen (pFileName);
	uint32_t requiredEntriesLFN = (fileNameLen / 13);
	if (fileNameLen % 13 > 0)
	{
		requiredEntriesLFN++;
	}
	
	int dotIndex = 0;
	for (int i = fileNameLen - 1; i >= 0; i--)
	{
		if (pFileName[i] == '.')
		{
			dotIndex = i;
			break;//Only take the last dot in mind
		}
	}
	
	bool requiresLFN = !(dotIndex <= 8 && (fileNameLen - dotIndex - 1) <= 3);
	uint8_t extended_attrs = 0x00;
	if (!requiresLFN)
	{
		//Are you sure???
		bool firstLetterCap = pFileName[0] >= 'a' && pFileName[0] <= 'z';
		for (int i = 1; i < dotIndex; i++)
		{
			//do caps not match?
			bool a = ((pFileName[i] < 'a' || pFileName[i] > 'z') &&  firstLetterCap), b = ((pFileName[i] < 'A' || pFileName[i] > 'Z') && !firstLetterCap);
			if (a || b)
			{
				requiresLFN = true;
				break;
			}
		}
		
		if (!requiresLFN && firstLetterCap)
			extended_attrs |= FAT_MSFLAGS_NAMELOWER;
		
		firstLetterCap = pFileName[dotIndex+1] >= 'a' && pFileName[dotIndex+1] <= 'z';
		for (uint32_t i = dotIndex + 1; i < fileNameLen; i++)
		{
			//do caps not match?
			bool a = ((pFileName[i] < 'a' || pFileName[i] > 'z') &&  firstLetterCap), b = ((pFileName[i] < 'A' || pFileName[i] > 'Z') && !firstLetterCap);
			if (a || b)
			{
				requiresLFN = true;
				break;
			}
		}
		
		if (!requiresLFN && firstLetterCap)
			extended_attrs |= FAT_MSFLAGS_EXTELOWER;
	}
	
	// If the filename's length is < 8 AND the extension is less than 4 characters, then there shouldn't be any LFN entries
	if (!requiresLFN) {
		requiredEntriesLFN = 0;
	}
	int seed = GetRandom();
	uint8_t  rootCluster[pOpenedIn->m_clusSize];
	uint32_t cluster; int startEntryIndex;
	uint8_t* pStartEntries = FatLocateEntries(
		pOpenedIn,
		rootCluster,
		pDirNode->m_implData2,//Directory cluster
		requiredEntriesLFN + 1,
		&cluster,
		&startEntryIndex
	);
	if (startEntryIndex < 0) return NULL;
	if (requiredEntriesLFN > 0)
	{
		FatWriteLFNEntries(pStartEntries, requiredEntriesLFN, pFileName, seed);
	}
	uint8_t* pActualEntry = pStartEntries + requiredEntriesLFN * 32;
	
	memset (pActualEntry, 0, 32);
	FatWrite83FileName(pFileName, pActualEntry, seed);
	
	uint32_t requiredClusters = size / pOpenedIn->m_clusSize;
	if (size % pOpenedIn->m_clusSize != 0) requiredClusters++;
	
	uint32_t bytesToWriteTotal = size;
	
	uint32_t firstCluster = 0, prevCluster = 0;
	for (uint32_t i = 0; i < requiredClusters; i++)
	{
		uint32_t currCluster = FatAllocateCluster (pOpenedIn);
		if (!firstCluster)
			firstCluster = currCluster;
		
		uint8_t clusterBuffer [pOpenedIn->m_clusSize];
		memset (clusterBuffer, 0, pOpenedIn->m_clusSize);
		
		uint32_t bytesToWrite = bytesToWriteTotal;
		if (bytesToWrite > pOpenedIn->m_clusSize)
			bytesToWrite = pOpenedIn->m_clusSize;
		
		bytesToWriteTotal -= bytesToWrite;
		
		memcpy (clusterBuffer, pData, bytesToWrite);
		
		FatPutCluster(pOpenedIn, clusterBuffer, currCluster);
		
		if (prevCluster)
		{
			FatSetClusterInFAT(pOpenedIn, prevCluster, currCluster);
		}
		prevCluster = currCluster;
	}
	
	// Write other fields
	pActualEntry[11] = 0x00;
	pActualEntry[12] = extended_attrs;
	
	uint32_t fileCluster = 0;
	// 0-sized files aren't supposed to have a cluster
	pActualEntry[20] = (firstCluster >> 16) & 0xff;
	pActualEntry[21] = (firstCluster >> 24) & 0xff;
	pActualEntry[26] = (firstCluster) & 0xff;
	pActualEntry[27] = (firstCluster >> 8) & 0xff;
	
	FatWriteCurrentTimeToEntry (pActualEntry, true);

	*((uint32_t*)(&pActualEntry[0x1C])) = 0;//File length
	
	FatPutCluster(pOpenedIn, rootCluster, cluster); 
	
	FatFlushFat(pOpenedIn);
	
	// Add the new file entry:
	SLogMsg("Creating file node ...");
	FileNode* pEntry =  CreateFileNode (pDirNode);
	
	memset (pEntry, 0, sizeof (*pEntry));
	
	strcpy(pEntry->m_name, pFileName);
	
	pEntry->m_type = FILE_TYPE_FILE;
	
	pEntry->m_perms = PERM_READ | PERM_WRITE;
	
	if (EndsWith (pFileName, ".nse"))
		pEntry->m_perms |=  PERM_EXEC;
	
	pEntry->m_inode  = fileCluster;
	pEntry->m_length = 0;
	pEntry->m_implData = (int)pOpenedIn;
	pEntry->m_implData1 = pDirNode->m_implData2;//Root directory
	pEntry->m_implData2 = fileCluster;
	pEntry->m_flags    = 0;
	
	pEntry->Read  = FsFatRead;
	pEntry->Write = FsFatWrite;
	pEntry->Open  = FsFatOpen;
	pEntry->Close = FsFatClose;
	
	// Before I revamped the vfs, here would be the comment:
    // "Look through all the open fds and convert the old filenodes to the new ones."
	// But that's COMPLETELY redundant, because we use linked lists, so all filenodes should stay in place
	
	return pEntry;
}

FileNode* FatCreateEmptyFile(FileNode *pDirNode, const char* pFileName)
{
	return FatCreateFileWithData(pDirNode, pFileName, NULL, 0);
}

static bool FatUpdateFileEntry(FileNode *pFileNode, uint32_t newSize, uint32_t clusterInfo)
{
	pFileNode->m_length = newSize;
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	uint32_t cluster = pFileNode->m_implData1;
	int  index = 0;
	bool didSomething = false;
	
	while (true)
	{
		uint8_t root_cluster[pNode->pOpenedIn->m_clusSize * 2];
		if (cluster >= FAT_END_OF_CHAIN)
		{
			SLogMsg("Why did cluster get off the chain!? Weird, stopping");
			break;
		}
		FatGetCluster(pNode->pOpenedIn, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		while ((uint32_t)(entry - root_cluster) < pNode->pOpenedIn->m_clusSize)
		{
			uint8_t firstByte = *entry;
			if (firstByte == 0x00 || firstByte == 0xE5)
			{
				// This directory entry has never been written or it has been deleted.
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pNode->pOpenedIn, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			//SLogMsg("Comparing %s and %s", targetDirEnt.m_pName,pFileNode->m_name);
			if (strcmp(targetDirEnt.m_pName, pFileNode->m_name) == 0)
			{
				uint8_t* pEntry2 = entry;
				
				//while we have a LFN entry ready:
				while (pEntry2[11] == FAT_LFN)
				{
					//skip it, we're looking for the juicy sfn entry
					pEntry2 += 32;
				}
				
				//Found it!
				//Write stuff to it.
				*((uint32_t*)(pEntry2 + 28)) = newSize;
				FatWriteCurrentTimeToEntry (pEntry2, false);
				if (clusterInfo != 0)
				{
					clusterInfo &= 0x0FFFFFFF;
					pEntry2[20] = (clusterInfo >> 16) & 0xff;
					pEntry2[21] = (clusterInfo >> 24) & 0xff;
					pEntry2[26] = (clusterInfo) & 0xff;
					pEntry2[27] = (clusterInfo >> 8) & 0xff;
				}
				
				FatPutCluster(pNode->pOpenedIn, root_cluster, cluster);
				if (secondCluster)
					FatPutCluster(pNode->pOpenedIn, root_cluster + pNode->pOpenedIn->m_clusSize, secondCluster);
				
				return true;
			}
			
			entry = nextEntry;
			
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			index++;
		}
		cluster = FatGetNextCluster(pNode->pOpenedIn, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	return didSomething;
}

static bool FatUpdateFileEntryNotOpened(FileNode *pFileNode, uint32_t newSize, uint32_t newCluster, bool bDeleted)
{
	pFileNode->m_length = newSize;
	FatFileSystem *pOpenedIn = (FatFileSystem*) pFileNode->m_implData;
	uint32_t cluster = pFileNode->m_implData1;
	int  index = 0;
	bool didSomething = false;
	
	while (true)
	{
		uint8_t root_cluster[pOpenedIn->m_clusSize * 2];
		if (cluster >= FAT_END_OF_CHAIN)
		{
			SLogMsg("Why did cluster get off the chain!? Weird, stopping");
			break;
		}
		FatGetCluster(pOpenedIn, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		while ((uint32_t)(entry - root_cluster) < pOpenedIn->m_clusSize)
		{
			uint8_t firstByte = *entry;
			if (firstByte == 0x00 || firstByte == 0xE5)
			{
				// This directory entry has never been written or it has been deleted.
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pOpenedIn, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			if (strcmp(targetDirEnt.m_pName, pFileNode->m_name) == 0)
			{
				//while we have a LFN entry ready:
				while (entry[11] == FAT_LFN)
				{
					if (bDeleted)
						entry[0] = 0xE5;
					
					//skip it, we're looking for the juicy sfn entry
					entry += 32;
				}
				
				bool bEntryInSecondClusterToo = (entry > (root_cluster + pOpenedIn->m_clusSize));
				
				//Found it!
				//Write stuff to it.
				*((uint32_t*)(entry + 28)) = newSize;
				if (bDeleted)
					entry[0] = 0xE5;
				
				if (newCluster != 0)
				{
					newCluster &= 0x0FFFFFFF;
					// Update the cluster info.
					entry[20] = (newCluster >> 16) & 0xff;
					entry[21] = (newCluster >> 24) & 0xff;
					entry[26] = (newCluster >>  0) & 0xff;
					entry[27] = (newCluster >>  8) & 0xff;
				}
				
				
				FatWriteCurrentTimeToEntry (entry, false);
				
				FatPutCluster(pOpenedIn, root_cluster, cluster);
				if (bEntryInSecondClusterToo)
					FatPutCluster(pOpenedIn, root_cluster + pOpenedIn->m_clusSize, cluster + 1);
				
				didSomething = true;
				break;
			}
			
			entry = nextEntry;
			
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			index++;
		}
		cluster = FatGetNextCluster(pOpenedIn, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	return didSomething;
}

void FatEmptyExistingFile(FileNode* pFileNode)
{
	FatFileSystem* pSystem = (FatFileSystem*)pFileNode->m_implData;
	
	FatZeroFatChain (pSystem, pFileNode->m_inode);
	FatUpdateFileEntryNotOpened (pFileNode, 0, 0xF0000000, false);
	
	//Since we zeroed out a chain...
	FatFlushFat(pSystem);
}

int FatDeleteExistingFile(FileNode* pFileNode)
{
	//TODO: What if this is a directory
	if (pFileNode->m_type & FILE_TYPE_DIRECTORY)
	{
		LogMsg("FatDeleteExistingFile: %s is a directory. Uh oh!", pFileNode->m_name);
		
		return -EINVAL;
	}
	FatFileSystem* pSystem = (FatFileSystem*)pFileNode->m_implData;
	
	FatZeroFatChain (pSystem, pFileNode->m_inode);
	bool b = FatUpdateFileEntryNotOpened (pFileNode, 0, 0xF0000000, true);
	
	//Since we zeroed out a chain...
	FatFlushFat(pSystem);
	
	return b ? -ENOTHING : -EIO;
}

void FatZeroOutFile(FileNode *pDirectoryNode, char* pFileName)
{
	FatFileSystem* pSystem = (FatFileSystem*)pDirectoryNode->m_implData;
	FileNode* pFN = FsFindDir(pDirectoryNode, pFileName);
	// If file exists:
	if (pFN)
	{
		DebugLogMsg("Found! pFN: %x", (FatFileSystem*)pFN->m_implData);
		FatZeroFatChain    (pSystem,   pFN->m_inode);//TODO
		FatUpdateFileEntryNotOpened (pFN, 0, FatAllocateCluster(pSystem), false);
		FatFlushFat(pSystem);//Since we allocated a new cluster
	}
	else
	{
		FatCreateEmptyFile (pDirectoryNode, pFileName);
	}
}


uint32_t FsFatReadNotStreaming (UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return 0;
	
	if (offset > pNode->nDataSize)
		return 0;
	if (offset + size > pNode->nDataSize)
		size = pNode->nDataSize - offset;
	
	memcpy (pBuffer, &pNode->pDataPointer[offset], size);
	return size;
}

uint32_t FsFatReadStreaming (FileNode *pFileNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return 0;
	
	if (pNode->bIsStreamingFromDisk)
		return FsFatReadNotStreaming(pFileNode, offset, size, pBuffer);
	
	bool needsToReloadCluster = false;
	
	if (!pNode->pWorkCluster)
	{
		pNode->pWorkCluster  = (uint8_t*)MmAllocateK (pNode->pOpenedIn->m_clusSize);
		needsToReloadCluster = true;
		if (!pNode->pWorkCluster)
			return 0; //Out of memory
	}
	
	// Determine how much we can read
	if (offset + size > pFileNode->m_length)
	{
		size = pFileNode->m_length - offset;
	}
	
	// Cluster offset to read from.  NOT the cluster number, just the number of cluster steps to go through to reach this.
	uint32_t clusterOffsetCurrent = offset / pNode->pOpenedIn->m_clusSize;
	
	// Did it change from the current cluster? If yes, how?
	if (clusterOffsetCurrent < pNode->nClusterProgress)
	{
		// It went back.  Re-resolve from the start.
		pNode->nClusterCurrent  = pNode->nClusterFirst;
		needsToReloadCluster = true;
		
		// Navigate the singly linked list until we reach the cluster.
		for (pNode->nClusterProgress = 0; pNode->nClusterProgress < clusterOffsetCurrent; pNode->nClusterProgress++)
		{
			pNode->nClusterCurrent = FatGetNextCluster (pNode->pOpenedIn, pNode->nClusterCurrent);
		}
	}
	else if (clusterOffsetCurrent > pNode->nClusterProgress)
	{
		// It went forwards.  To speed forward reads, start resolving the current cluster
		// number right from the current offset stored.
		needsToReloadCluster = true;
		for (; pNode->nClusterProgress < clusterOffsetCurrent; pNode->nClusterProgress++)
		{
			pNode->nClusterCurrent = FatGetNextCluster (pNode->pOpenedIn, pNode->nClusterCurrent);
		}
	}
	else
	{
		// ... We're already at the needed cluster.
	}
	
	int insideClusterOffset = offset % pNode->pOpenedIn->m_clusSize;
	
	// Reload the cluster if needed.
	if (needsToReloadCluster)
	{
		FatGetCluster (pNode->pOpenedIn, pNode->pWorkCluster, pNode->nClusterCurrent);
	}
	
	int whereToEndReading = size + insideClusterOffset, howMuchToRead = size;
	if (whereToEndReading > (int)pNode->pOpenedIn->m_clusSize)
	{
		howMuchToRead -= (whereToEndReading - (int)pNode->pOpenedIn->m_clusSize);
		whereToEndReading = (int)pNode->pOpenedIn->m_clusSize;
	}
	
	// Place the data in it, finally.
	uint8_t* pointer = (uint8_t*)pBuffer;
	memcpy (pointer, pNode->pWorkCluster + insideClusterOffset, howMuchToRead);
	
	int result = howMuchToRead;
	if ((uint32_t)howMuchToRead < size) // More to read?
		// Yeah, read more.
		result += FsFatReadStreaming (pFileNode, offset + howMuchToRead, size - howMuchToRead, pointer + howMuchToRead);
	
	return result;
}

uint32_t FsFatRead (UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	uint32_t off1 = 0, size_left = size;
	uint32_t rv = 0;
	#define ITERATION_SIZE 16384
	while (size_left > ITERATION_SIZE)
	{
		rv += FsFatReadStreaming(pFileNode, offset + off1, ITERATION_SIZE, (uint8_t*)pBuffer + off1);
		off1 += ITERATION_SIZE;
		if (rv < off1)
			return rv;//TODO
		size_left -= ITERATION_SIZE;
	}
	if (size_left)
		return rv + FsFatReadStreaming(pFileNode, offset + off1, size_left, (uint8_t*)pBuffer + off1);
	return rv;
}

uint32_t FsFatWrite (FileNode *pFileNode, uint32_t offset, uint32_t size, void* pBuffer)
{
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return 0;
	
	if (offset > pNode->nDataSize)
		return 0;
	
	if (offset + size > pNode->nDataSize)
	{
		uint32_t newSize = offset + size;
		
		if (newSize > pNode->nDataCapacity)//Overrun the capacity?
		{
			uint32_t oldCap = pNode->nDataCapacity;
			pNode->nDataCapacity *= 2;
			
			// Still not enough?
			if (pNode->nDataCapacity < newSize)
				pNode->nDataCapacity = newSize;
			
			uint8_t* newDataPtr = MmAllocateK(pNode->nDataCapacity);
			memcpy (newDataPtr, pNode->pDataPointer, oldCap);
			
			MmFreeK(pNode->pDataPointer);
			pNode->pDataPointer = newDataPtr;
		}
		
		pNode->nDataSize = newSize;
	}
	
	if (size) pNode->bWasWrittenTo = true;
	
	//write!
	memcpy (&pNode->pDataPointer[offset], pBuffer, size);
	return size;
}

bool FsFatOpenNonRootDir(FileNode *pFileNode);
void FsFatCloseNonRootDir (FileNode* pFileNode);
static DirEnt*   FsFatReadNonRootDir(FileNode* pNode, uint32_t index);
static FileNode* FsFatFindNonRootDir(FileNode* pNode, const char* pName);
//generic purpose fillup directory entry thing

static void FsFatReadDirectoryContents(FatFileSystem* pSystem, FileNode* pDirNode, uint32_t startCluster)
{
	// Read the directory and count the entries.
	uint32_t cluster = startCluster;
	int index = 0;
	
	while (true)
	{
		uint8_t root_cluster[pSystem->m_clusSize * 2];
		FatGetCluster(pSystem, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		int dirEntsUntilReal = 0;
		while ((uint32_t)(entry - root_cluster) < pSystem->m_clusSize)
		{
			uint8_t firstByte = *entry;
			if (firstByte == 0x00 || firstByte == 0xE5 || (firstByte == (uint8_t)'.' && dirEntsUntilReal++ < 2))
			{
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pSystem, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			// turn this entry into a FileNode entry.
			FileNode *pCurrent = CreateFileNode (pDirNode);
			
			strcpy(pCurrent->m_name, targetDirEnt.m_pName);
			
			pCurrent->m_type = FILE_TYPE_FILE;
			if (targetDirEnt.m_dirFlags & FAT_DIRECTORY)
				pCurrent->m_type = FILE_TYPE_DIRECTORY;
			
			pCurrent->m_perms = PERM_READ | PERM_WRITE;
			if (targetDirEnt.m_dirFlags & FAT_READONLY)
				pCurrent->m_perms &= ~PERM_WRITE;
			
			if (EndsWith (targetDirEnt.m_pName, ".nse"))
				pCurrent->m_perms |=  PERM_EXEC;
			
			pCurrent->m_inode  = targetDirEnt.m_firstCluster;
			pCurrent->m_length = targetDirEnt.m_fileSize;
			pCurrent->m_implData = (int)pSystem;
			pCurrent->m_implData1 = startCluster;
			pCurrent->m_implData2 = targetDirEnt.m_firstCluster;
			pCurrent->m_implData3 = -1;
			pCurrent->m_flags    = 0;
			
			if (!(targetDirEnt.m_dirFlags & FAT_DIRECTORY))
			{
				pCurrent->Read  = FsFatRead;
				pCurrent->Write = FsFatWrite;
				pCurrent->Open  = FsFatOpen;
				pCurrent->Close = FsFatClose;
				pCurrent->EmptyFile  = FatEmptyExistingFile;
				pCurrent->RemoveFile = FatDeleteExistingFile;
			}
			else
			{
				pCurrent->OpenDir  = FsFatOpenNonRootDir;
				pCurrent->CloseDir = FsFatCloseNonRootDir;
				pCurrent->ReadDir  = FsFatReadNonRootDir;
				pCurrent->FindDir  = FsFatFindNonRootDir;
				pCurrent->CreateFile = FatCreateEmptyFile;
			}
			
			entry = nextEntry;
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			
			index++;
		}
		cluster = FatGetNextCluster(pSystem, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
}

bool FsFatOpenNonRootDir(FileNode *pFileNode)
{
	FatFileSystem* pFS = (FatFileSystem*)pFileNode->m_implData;
	uint32_t nFirstCluster = pFileNode->m_inode;
	
	if (pFileNode->m_implData3 == (uint32_t)-1)
	{
		// Read the directory entries.
		FsFatReadDirectoryContents(
			pFS,
			pFileNode,
			nFirstCluster
		);
	}
	
	pFileNode->m_implData3 = 1;
	
	// Finish!
	return true;
}

void FsFatCloseNonRootDir (UNUSED FileNode* pFileNode)
{
	//Does nothing. For now
}

static DirEnt  g_FatDirEnt;
static DirEnt* FsFatReadNonRootDir(FileNode* pDirNode, uint32_t index)
{
	int loaded = pDirNode->m_implData3;
	if (loaded == -1)
	{
		LogMsg("[FATAL] Warning: did you mean to `FsOpenDir` first?  Opening for you, but do keep in mind that this isn't how you do things.");
		if (!FsFatOpenNonRootDir(pDirNode))
		{
			LogMsg("[FATAL] Couldn't even open pNode.  What a shame.");
			return NULL;
		}
		return FsFatReadNonRootDir(pDirNode, index);
	}
	
	FileNode *pNode = pDirNode->children;
	//TODO: Optimize this, if you have consecutive indices
	uint32_t id = 0;
	while (pNode && id < index)
	{
		pNode = pNode->next;
		id++;
	}
	if (!pNode) return NULL;
	strcpy(g_FatDirEnt.m_name, pNode->m_name);
	g_FatDirEnt.m_inode      = pNode->m_inode;
	g_FatDirEnt.m_type       = pNode->m_type;
	return &g_FatDirEnt;
}
static FileNode* FsFatFindNonRootDir(FileNode* pDirNode, const char* pName)
{
	int loaded = pDirNode->m_implData3;
	if (loaded == -1)
	{
		LogMsg("[FATAL] Warning: did you mean to `FiOpenDir` first?  Opening for you, but do keep in mind that this isn't how you do things.");
		if (!FsFatOpenNonRootDir(pDirNode))
		{
			LogMsg("[FATAL] Couldn't even open pNode.  What a shame.");
			return NULL;
		}
		return FsFatFindNonRootDir(pDirNode, pName);
	}
	
	FileNode *pNode = pDirNode->children;
	while (pNode)
	{
		if (strcmp (pNode->m_name, pName) == 0)
			return pNode;
		pNode = pNode->next;
	}
	return NULL;
}

//Generic

static DirEnt* FsFatReadRootDir(FileNode* pDirNode, uint32_t index)
{
	FileNode *pNode = pDirNode->children;
	//TODO: Optimize this, if you have consecutive indices
	uint32_t id = 0;
	while (pNode && id < index)
	{
		pNode = pNode->next;
		id++;
	}
	if (!pNode) return NULL;
	
	strcpy(g_FatDirEnt.m_name, pNode->m_name);
	g_FatDirEnt.m_inode      = pNode->m_inode;
	g_FatDirEnt.m_type       = pNode->m_type;
	return &g_FatDirEnt;
}
static FileNode* FsFatFindRootDir(FileNode *pDirNode, const char* pName)
{
	FileNode *pNode = pDirNode->children;
	while (pNode)
	{
		if (strcmp (pNode->m_name, pName) == 0)
			return pNode;
		pNode = pNode->next;
	}
	return NULL;
}

static void FatMountRootDir(FatFileSystem* pSystem, char* pOutPath)
{
	FileNode *pFatRoot = CreateFileNode (FsGetRootNode());
	
	sprintf(pFatRoot->m_name, "Fat%d", g_fatsMountedCount);
	strcpy (pOutPath, pFatRoot->m_name);
	pFatRoot->m_type  = FILE_TYPE_DIRECTORY | FILE_TYPE_MOUNTPOINT;
	pFatRoot->m_flags = 0;
	pFatRoot->m_perms = PERM_READ|PERM_WRITE|PERM_EXEC;
	pFatRoot->m_inode = g_fatsMountedCount;
	pFatRoot->m_length = 0;
	pFatRoot->m_implData  = (int)pSystem;
	pFatRoot->m_implData2 = 2;
	
	pFatRoot->Read     = NULL;
	pFatRoot->Write    = NULL;
	pFatRoot->Open     = NULL;
	pFatRoot->Close    = NULL;
	pFatRoot->OpenDir  = NULL;
	pFatRoot->CloseDir = NULL;
	pFatRoot->CreateFile = FatCreateEmptyFile;
	pFatRoot->ReadDir  = FsFatReadRootDir;
	pFatRoot->FindDir  = FsFatFindRootDir;
	
	//uint32_t dirsPerClus = pSystem->m_clusSize / 32;
	uint32_t cluster = 2;
	
	int index = 0;
	
	while (true)
	{
		uint8_t root_cluster[pSystem->m_clusSize * 2];
		FatGetCluster(pSystem, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		while ((uint32_t)(entry - root_cluster) < pSystem->m_clusSize)
		{
			uint8_t firstByte = *entry;
			if (firstByte == 0x00 || firstByte == 0xE5)
			{
				// This directory entry has never been written or it has been deleted.
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pSystem, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			if (!(targetDirEnt.m_dirFlags & FAT_VOLUME_ID))
			{
				// turn this entry into a FileNode entry.
				FileNode *pCurrent = CreateFileNode (pFatRoot);//pFileNodes + index;
				
				strcpy(pCurrent->m_name, targetDirEnt.m_pName);
				
				pCurrent->m_type = FILE_TYPE_FILE;
				if (targetDirEnt.m_dirFlags & FAT_DIRECTORY)
					pCurrent->m_type = FILE_TYPE_DIRECTORY;
				
				pCurrent->m_perms = PERM_READ | PERM_WRITE;
				if (targetDirEnt.m_dirFlags & FAT_READONLY)
					pCurrent->m_perms &= ~PERM_WRITE;
				
				if (EndsWith (targetDirEnt.m_pName, ".nse"))
					pCurrent->m_perms |=  PERM_EXEC;
				
				pCurrent->m_inode  = targetDirEnt.m_firstCluster;
				pCurrent->m_length = targetDirEnt.m_fileSize;
				pCurrent->m_implData = (int)pSystem;
				pCurrent->m_implData1 = 2;//Root directory
				pCurrent->m_implData2 = targetDirEnt.m_firstCluster;
				pCurrent->m_implData3 = -1;
				pCurrent->m_flags    = 0;
				
				if (!(targetDirEnt.m_dirFlags & FAT_DIRECTORY))
				{
					pCurrent->Read  = FsFatRead;
					pCurrent->Write = FsFatWrite;
					pCurrent->Open  = FsFatOpen;
					pCurrent->Close = FsFatClose;
					pCurrent->EmptyFile  = FatEmptyExistingFile;
					pCurrent->RemoveFile = FatDeleteExistingFile;
				}
				else
				{
					pCurrent->OpenDir  = FsFatOpenNonRootDir;
					pCurrent->CloseDir = FsFatCloseNonRootDir;
					pCurrent->ReadDir  = FsFatReadNonRootDir;
					pCurrent->FindDir  = FsFatFindNonRootDir;
					pCurrent->CreateFile = FatCreateEmptyFile;
				}
			}
			
			entry = nextEntry;
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			
			index++;
		}
		cluster = FatGetNextCluster(pSystem, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	
	g_fatsMountedCount++;
}

#endif

// Mounting code
#if 1

int FsMountFatPartition(DriveID driveID, int partitionStart, int partitionSizeSec)
{
	// Find a Fat32 structure in the list of Fat32 structures.
	int FreeArea = -1;
	for (size_t i = 0; i < ARRAY_COUNT(s_Fat32Structures); i++)
	{
		if (!s_Fat32Structures[i].m_bMounted)
		{
			FreeArea = i;
			break;
		}
	}
	
	if (FreeArea == -1) return MOUNT_ERR_TOO_MANY_PARTS_MOUNTED;
	
	int RootInode = FreeArea;
	
	FatFileSystem* pFat32 = &s_Fat32Structures[RootInode];
	pFat32->m_partStartSec = partitionStart;
	pFat32->m_partSizeSec  = partitionSizeSec;
	pFat32->m_driveID      = driveID;
	pFat32->m_bMounted     = true;
	FatReadBpb   (pFat32);
	FatTrimSpaces(pFat32->m_bpb.m_sSystemID);
	
	// is this a FAT32 system?
	if (strcmp (pFat32->m_bpb.m_sSystemID, "FAT32") != 0)
	{
		//No, free this and return.
		//LogMsg("Probed drive %d partition %d for FAT32 file system.  It's not FAT32.", driveID, partitionStart);
		//LogMsg("Its system ID is '%s'.", pFat32->m_bpb.m_sSystemID);
		pFat32->m_bMounted = false;
		return MOUNT_ERR_NOT_FAT32;
	}
	
	pFat32->m_fatBeginSector  = pFat32->m_bpb.m_nReservedSectors;
	pFat32->m_clusBeginSector = pFat32->m_fatBeginSector + (pFat32->m_bpb.m_nFAT * pFat32->m_bpb.m_nSectorsPerFat32);
	pFat32->m_clusSize        = 512 * pFat32->m_bpb.m_nSectorsPerCluster;
	pFat32->m_clusAllocHint   = 0;
	
	uint32_t nBytesPerFAT     = 512 * pFat32->m_bpb.m_nSectorsPerFat32;
	pFat32->m_pFat            = (uint32_t*)MmAllocateK (nBytesPerFAT);
	pFat32->m_pbFatModified   = (bool*)MmAllocateK (pFat32->m_bpb.m_nSectorsPerFat32);
	memset(pFat32->m_pbFatModified, 0, pFat32->m_bpb.m_nSectorsPerFat32);
	
	//LogMsgNoCr ("Loading file system from drive(%d)partition(%d).  Please wait.", driveID, partitionStart);
	for (uint32_t sectorI = 0; sectorI < pFat32->m_bpb.m_nSectorsPerFat32; sectorI++)
	{
		FatGetSector (pFat32, (uint8_t*)(pFat32->m_pFat + sectorI*128), pFat32->m_fatBeginSector + sectorI, 1);
	}
	
	//LogMsg(" Done!");
	
	//LogMsgNoCr("Reading root directory to mount in the vfs...");
	
	char out_path[128];
	out_path[0] = '?', out_path[1] = 0;
	FatMountRootDir(pFat32, out_path);
	
	LogMsg("Mounted '%s'.", out_path, 512 * pFat32->m_bpb.m_nSectorsPerCluster);
	
	return MOUNT_SUCCESS;
}

void FatUnmountFileSystem(FatFileSystem* pFS)
{
	FatFlushFat (pFS);
	MmFreeK (pFS->m_pFat);
	MmFreeK (pFS->m_pbFatModified);
	pFS->m_pFat          = NULL;
	pFS->m_pbFatModified = NULL;
	pFS->m_bMounted      = false;
	LogMsg("File system unmounted.  Need to do it from the VFS side too. TODO!");
}

void FsFatInit ()
{
	// probe each drive
	MasterBootRecord record;
	for (int i = 0; i < 0x100; i++)
	{
		if (!StIsDriveAvailable(i)) continue;
		
		SLogMsg("Reading MBR from drive %x", i);
		
		// This drive is available.  Look at its MBR first.
		// TODO handle cases where there's only one main partition across
		// the whole disk
		StDeviceRead( 0, &record, i, 1);
		
		// probe each partition
		for (int pi = 0; pi < 4; pi++)
		{
			Partition* pPart = &record.m_Partitions[pi];
			
			if (pPart->m_PartTypeDesc != 0)
			{
				// This is a valid partition.  Mount it.
				FsMountFatPartition (i, pPart->m_StartLBA, pPart->m_PartSizeSectors);
			}
		}
	}
}

#endif
