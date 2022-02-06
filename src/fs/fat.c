/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        FILE SYSTEM: FAT 32 module
******************************************/
#include <vfs.h>
#include <console.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>
#include <storabs.h>

// Credits: https://github.com/knusbaum/kernel/blob/master/fat32.c


// Structure definitions.
#if 1

// should use this when generating NanoShell-specific permissions for this file
#define FAT_READONLY (1<<0)

// A non-pussy operating system doesn't need to use these flags.
// TODO: Remove this comment when this goes public.
#define FAT_HIDDEN    (1<<1)
#define FAT_SYSTEM    (1<<2)

// In the root of a FAT file system, there's a special file whose name matches the volume ID.
// NanoShell should and does ignore any files with this bit set.
#define FAT_VOLUME_ID (1<<3)

// If this is a directory.  Pretty simple, really.
#define FAT_DIRECTORY (1<<4)

// Archive bit.  Usually it's used to determine whether a file has been backed up or not (this bit
// will be reset when you write to the file later on).  Not used by NanoShell.
#define FAT_ARCHIVE   (1<<5)

// Long file name bit.  Useful when we need LFN support.
#define FAT_LFN       (FAT_READONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID)

// FAT End of chain special magic number cluster.  Marks the end of a chain.
// This is just like the NULL pointer in most singly linked list implementations.
#define FAT_END_OF_CHAIN 0x0FFFFFF8

typedef struct
{
	uint16_t m_nBytesPerSector;     // usually 512
	uint8_t  m_nSectorsPerCluster;  // must be 1 or over.
	uint16_t m_nReservedSectors;    // before we reach the FAT
	uint8_t  m_nFAT;                // number of FATs (NanoShell will only use the first one)
	uint16_t m_nDirEntries;
	uint16_t m_nTotalSectors;
	uint8_t  m_mediaDescType;
	uint16_t m_nSectorsPerFat12or16;
	uint16_t m_nSectorsPerTrack;
	uint16_t m_nHeadsOrSizesOnMedia;//?
	uint32_t m_nHiddenSectors;
	uint32_t m_nLargeSectorsOnMedia;
	
	// Extended Boot Record
	uint32_t m_nSectorsPerFat32;
	uint16_t m_nFlags;
	uint16_t m_nFatVersion;
	uint32_t m_nRootDirStartCluster;// where our root logic should start
	uint16_t m_sectorNumberFsInfo;
	uint16_t m_sectorNmbrBckpBtSctr;
	uint8_t  m_driveNumber;
	uint8_t  m_WindowsFlags;        //used only by Windows
	uint8_t  m_Signature;
	uint32_t m_nVolumeID;           //something like AABB-CCDD
	char     m_sVolumeLabel[12];    //! NOT null terminated! Instead you have to `memcpy` it into a null termed string.
	char     m_sSystemID   [9];     //Same as above.
}
__attribute__((packed))
BiosParameterBlock;

typedef struct
{
	char     m_pName[128];
	uint8_t  m_dirFlags;
	uint32_t m_firstCluster,
	         m_fileSize;
}
FatDirEntry;

typedef struct
{
	uint32_t     m_startCluster;
	FatDirEntry* m_pEntries;
	uint32_t     m_nEntries;
}
FatDirectory;

typedef struct
{
	bool         m_bMounted;
	uint32_t*    m_pFat;
	bool*        m_pbFatModified;
	FileNode*    m_pRoot;
	BiosParameterBlock m_bpb;
	uint32_t     m_fatBeginSector,
	             m_clusBeginSector,
				 m_clusSize,
				 m_clusAllocHint, //! to avoid fragmentation
				 m_partStartSec,
				 m_partSizeSec,
				 m_driveID;
}
FatFileSystem;

typedef struct
{
	uint8_t  m_BootIndicator;
	uint8_t  m_StartCHS[3]; //not used
	uint8_t  m_PartTypeDesc;
	uint8_t  m_EndCHS[3]; //not used
	uint32_t m_StartLBA; //used
	uint32_t m_PartSizeSectors;
}
__attribute__((packed))
Partition;

typedef struct
{
	uint8_t   m_BootLoaderCode[446];
	Partition m_Partitions[4];
	uint16_t  m_BootSignature; //most of the time this is 0x55AA
}
__attribute__((packed))
MasterBootRecord;

static FatFileSystem s_Fat32Structures[32];

enum
{
	MOUNT_SUCCESS,
	MOUNT_ERR_TOO_MANY_PARTS_MOUNTED = 0x7000,
	MOUNT_ERR_NOT_FAT32,
};
#endif

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
	pFS->m_pbFatModified[cluster / 512] = true;
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
__attribute__((always_inline))
static inline int16_t FatGetS16(uint8_t* buffer, int offset)
{
	return *((int16_t*)(buffer + offset));
}
__attribute__((always_inline))
static inline int32_t FatGetS32(uint8_t* buffer, int offset)
{
	return *((int32_t*)(buffer + offset));
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
	uint8_t  checksum = 0;
	for (int i = 0; i < 11; i++)
	{
		uint8_t high_bit = (checksum & 0x1) << 7;
		checksum = ((checksum >> 1) & 0x7F) | high_bit;
		checksum += pFileName[i];
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

void FatWrite83FileName(const char* pFileName, uint8_t* pBuffer)
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
		pBuffer[6] = '~';
		pBuffer[7] = '1'; // FIXME TODO: increment this if file already exists
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
	FatDirectory  *pDir,
	uint32_t       nCount,
	uint32_t      *pFoundCluster,
	int           *pIndex
)
{
	uint32_t i;
	uint32_t dirsPerClus = pFS->m_clusSize / 32;
	int32_t  index       = -1;
	uint32_t cluster     = pDir->m_startCluster;
	
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
				inARow = 0;
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

void FatWriteLFNEntries(uint8_t *pStart, uint32_t numEntries, char* pFName)
{
	char shortfname [12]; shortfname[11] = 0;
	FatWrite83FileName (pFName, (uint8_t*)shortfname);
	uint8_t checksum = FatFileNameChecksum(shortfname);
	
	// Write the LFN entries.
	
	uint32_t writtenChars = 0, nameLen = strlen (pFName);
	char    *pNamePtr = pFName;
	uint8_t *pEntry   = NULL; 
	for (uint32_t i = 0; i < numEntries; i++)
	{
		// Reverse the entry order.
		pEntry = pStart + ((numEntries - 1 - i) * 32);
		
		// Set the entry number.
		pEntry[0]  = i + 1;
		pEntry[13] = checksum;
		
		// Characters are 16 bytes in LFN entries (j += 2)
        uint32_t j;
		for (j = 2; j < 10; j += 2)
		{
			if (writtenChars < nameLen)
			{
				pEntry[j] = *pNamePtr;
			}
			else
			{
				pEntry[j] = 0;
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
				pEntry[j] = 0;
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
				pEntry[j] = 0;
			}
			pNamePtr++;
			writtenChars++;
		}
		
		// Mark the attributes byte as LFN.
		pEntry[11] = FAT_LFN;
	}
	// Mark the last(first) entry with the end-of-LFN bit
	pEntry[0] |= 0x40;
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
        return NULL;
    }

    uint32_t LFNCount = 0;
    while (entry[11] == FAT_LFN)
	{
        LFNCount++;
        entry += 32;
        if (entry == pEnd)
		{
			SLogMsg("FatReadDirEntry: LFN -- Need to load more I guess?");
            return NULL;
        }
		//if (LFNCount > 8) break;
    }
    if (LFNCount > 0)
	{
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
		
		int len = strlen(pDirEnt->m_pName);
		for (int i = 0; i < len; i++)
		{
			pDirEnt->m_pName[i] = ToLower(pDirEnt->m_pName[i]);
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

void FatNextDirEntry (
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
			FatDirEntry* pNewDir = (FatDirEntry*)MmAllocate(maxDirs * sizeof(FatDirEntry));
			memcpy (pNewDir, pDir->m_pEntries, (maxDirs - dirsPerClus) * sizeof(FatDirEntry));
			MmFree (pDir->m_pEntries);
			pDir->m_pEntries = pNewDir;
		}
		else
			pDir->m_pEntries = (FatDirEntry*)MmAllocate(maxDirs * sizeof(FatDirEntry));
		
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
	/*
	for (uint32_t i = 0; i < pDir->m_nEntries; i++)
	{
		
	}*/
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
			if (strcmp (targetDirent.m_pName, filenameUpper) == 0)
			{
				// We found it! Invalidate all the entries.
				entry[0] = 0xE5;
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
	uint32_t       nClusterCurrent,  // Cluster number inside the FAT.  Used to continue reading beyond the work cluster
				   nClusterProgress, // Number of clusters that have been passed to reach this point.  Useful to check if "offset" has changed at all.
				   nClusterFirst;    // First cluster.  Used to re-resolve nClusterCurrent if offset has gone back.
}
FatInode;

static FatInode s_FatInodes[FD_MAX];

bool FsFatOpen (FileNode* pFileNode, UNUSED bool read, UNUSED bool write)
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
	
	pNode->pOpenedIn = pFS;
	pNode->nClusterCurrent = pNode->nClusterFirst = nFirstCluster;
	pNode->nClusterProgress = 0;
	
	pFileNode->m_inode = freeInode;
	
	return true;
}

void FsFatClose (FileNode* pFileNode)
{
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return;
	
	// Free the pWorkCluster structure, if applicable.
	if (pNode->pWorkCluster)
	{
		MmFree(pNode->pWorkCluster);
		pNode->pWorkCluster = NULL;
	}
	
	// Deinitialize this fatinode:
	pNode->pOpenedIn = NULL;
	
	// Reset the pFileNode to its default state:
	pFileNode->m_inode = pNode->nClusterFirst;
	pNode->nClusterCurrent = pNode->nClusterFirst = pNode->nClusterProgress = 0;
}

uint32_t FsFatRead1 (UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	//LogMsg("FsFatRead(%x, %d, %d, %x)", pFileNode,offset,size,pBuffer);
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return 0;
	
	bool needsToReloadCluster = false;
	
	if (!pNode->pWorkCluster)
	{
		pNode->pWorkCluster  = (uint8_t*)MmAllocate (pNode->pOpenedIn->m_clusSize);
		needsToReloadCluster = true;
		if (!pNode->pWorkCluster)
			return 0; //Out of memory
	}
	
	// Determine how much we can read
	if (offset + size > pFileNode->m_length)
	{
		size = pFileNode->m_length - offset;
	}
	
	if (size <= 0) return 0;
	
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
		result += FsFatRead1 (pFileNode, offset + howMuchToRead, size - howMuchToRead, pointer + howMuchToRead);
	
	return result;
}

uint32_t FsFatRead (UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	uint32_t off1 = 0, size_left = size;
	uint32_t rv = 0;
	#define ITERATION_SIZE 512
	while (size_left > ITERATION_SIZE)
	{
		rv += FsFatRead1(pFileNode, offset + off1, ITERATION_SIZE, (uint8_t*)pBuffer + off1);
		off1 += ITERATION_SIZE;
		if (rv < off1)
			return rv;//TODO
		size_left -= ITERATION_SIZE;
	}
	if (size_left)
		return rv + FsFatRead1(pFileNode, offset + off1, size_left, (uint8_t*)pBuffer + off1);
	return rv;
}

// Directories
typedef struct tagDirectoryCacheEntry
{
	bool      m_used;
	
	//The file system this directory is a part of.
	FatFileSystem* pFileSystem;
	
	//If the parent entry is NULL, it means that this is the root.
	struct tagDirectoryCacheEntry*  m_parent;
	
	//The filenode of this directory inside the parent directory.
	FileNode *m_thisFileNode;
	
	//If any child entry is NULL, that means:
	//1) It's not a directory
	//2) It has not been opened yet.
	struct tagDirectoryCacheEntry** m_children;
	
	//Cached files.
	FileNode* m_pFileNodes;
	int       m_nFileNodes;
	
	//Reference counting.  May be used later to clean up stuff, but for now, don't.
	int       m_referenceCount;
}
DirectoryCacheEntry;
DirectoryCacheEntry m_dceEntries [FD_MAX];

void FsListOpenedDirs()
{
	LogMsg("Listing open dirs");
	for (int i=0; i<FD_MAX; i++)
	{
		if (m_dceEntries[i].m_used)
		{
			LogMsg("- %4d: Opened in %x | Files: %5d | FileNodePtr: %x", i, m_dceEntries[i].pFileSystem, m_dceEntries[i].m_nFileNodes, m_dceEntries[i].m_thisFileNode);
		}
	}
}

bool FsFatOpenNonRootDir(FileNode *pFileNode);
void FsFatCloseNonRootDir (FileNode* pFileNode);
static DirEnt* FsFatReadNonRootDir(FileNode* pNode, uint32_t index);
static FileNode* FsFatFindNonRootDir(FileNode* pNode, const char* pName);
//generic purpose fillup directory entry thing
static void FsFatReadDirectoryContents(FatFileSystem* pSystem, FileNode* *whereToStoreFileNodes, int* whereToStoreFileNodeCount, uint32_t startCluster)
{
	// Read the directory and count the entries.
	int entryCount = 0;
	uint32_t cluster = startCluster;
	
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
			entry = nextEntry;
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			
			entryCount++;
		}
		cluster = FatGetNextCluster(pSystem, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	
	// Allocate a FileNode* pointer.
	FileNode* pFileNodes = (FileNode*)MmAllocate (sizeof(FileNode) * entryCount);
	memset (pFileNodes, 0, sizeof(FileNode) * entryCount);
	
	// Read the directory AGAIN and fill in the FileNodes
	cluster = startCluster;
	int index = 0;
	
	while (true)
	{
		uint8_t root_cluster[pSystem->m_clusSize * 2];
		FatGetCluster(pSystem, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		int dirEntsUntilReal = 0;
		while ((uint32_t)(entry - root_cluster) < pSystem->m_clusSize)
		{
			if (index >= entryCount)
			{
				LogMsg("WARNING: Still have entries?! STOPPING NOW! This is UNACCEPTABLE.");
				break;
			}
			
			uint8_t firstByte = *entry;
			while (firstByte == 0x00 || firstByte == 0xE5 || (firstByte == (uint8_t)'.' && dirEntsUntilReal++ < 2))
			{
				entry += 32;
				firstByte = *entry;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pSystem, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			// turn this entry into a FileNode entry.
			FileNode *pCurrent = pFileNodes + index;
			
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
			pCurrent->m_implData1 = -1;
			pCurrent->m_implData2 = -1;
			pCurrent->m_flags    = 0;
			
			if (!(targetDirEnt.m_dirFlags & FAT_DIRECTORY))
			{
				pCurrent->Read  = FsFatRead;
				pCurrent->Write = NULL;
				pCurrent->Open  = FsFatOpen;
				pCurrent->Close = FsFatClose;
			}
			
			//TODO: directory I/O
			pCurrent->OpenDir  = FsFatOpenNonRootDir;
			pCurrent->CloseDir = FsFatCloseNonRootDir;
			pCurrent->ReadDir  = FsFatReadNonRootDir;
			pCurrent->FindDir  = FsFatFindNonRootDir;
			
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
	
	//Done!
	*whereToStoreFileNodes     = pFileNodes;
	*whereToStoreFileNodeCount = entryCount;
}

bool FsFatOpenNonRootDir(FileNode *pFileNode)
{
	FatFileSystem* pFS = (FatFileSystem*)pFileNode->m_implData;
	uint32_t nFirstCluster = pFileNode->m_inode;
	
	// look for a free directory cache entry:
	int freeDce = -1;
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!m_dceEntries[i].m_used)
		{
			freeDce = i; break;
		}
	}
	
	if (freeDce == -1)
	{
		LogMsg("Could not open directory.  Might want to close some?");
		return false;//Too many directories cached! Free some! (Or cancel like the lazy bitch that you are)
	}
	//^^ TODO
	
	DirectoryCacheEntry* pDce = &m_dceEntries[freeDce];
	pDce->m_used = true;
	pDce->pFileSystem = pFS;
	//pDce->m_parent = --TODO
	pDce->m_thisFileNode = pFileNode;
	pDce->m_referenceCount = 0;
	
	// Read the directory entries.
	FsFatReadDirectoryContents(
		pFS,
		&pDce->m_pFileNodes,
		&pDce->m_nFileNodes,
		nFirstCluster
	);
	
	// Allocate the same number of tagDirectoryCacheEntries
	pDce->m_children = MmAllocate (sizeof (DirectoryCacheEntry*) * pDce->m_nFileNodes);
	memset (pDce->m_children, 0,   sizeof (DirectoryCacheEntry*) * pDce->m_nFileNodes);
	
	// This directory entry has been loaded already.  Mark it here.
	pFileNode->m_implData2 = freeDce;
	
	// Finished.
	return true;
}

void FsFatCloseNonRootDir (UNUSED FileNode* pFileNode)
{
	LogMsg("TODO");
	//Does nothing. For now
}

static DirEnt  g_FatDirEnt;
static DirEnt* FsFatReadNonRootDir(FileNode* pNode, uint32_t index)
{
	int dceIndex = pNode->m_implData2;
	if (dceIndex == -1)
	{
		LogMsg("[FATAL] Warning: did you mean to `FiOpenDir` first?  Opening for you, but do keep in mind that this isn't how you do things.");
		if (!FsFatOpenNonRootDir(pNode))
		{
			LogMsg("[FATAL] Couldn't even open pNode.  What a shame.  What the fuck?");
			return NULL;
		}
		return FsFatReadNonRootDir(pNode, index);
	}
	
	if (dceIndex < 0 || dceIndex >= FD_MAX) return NULL;
	
	DirectoryCacheEntry* pDce = &m_dceEntries[dceIndex];
	if (!pDce->m_used) 
	{
		LogMsg("[FATAL] Huh?? Tried to read a directory entry that had a fake cache implData2? Try again.");
		pNode->m_implData2 = -1;
		return FsFatReadNonRootDir(pNode, index);
	}
	
	if (index >= (uint32_t)pDce->m_nFileNodes) return NULL;//Out of bounds?
	
	strcpy (g_FatDirEnt.m_name, pDce->m_pFileNodes[index].m_name);
	g_FatDirEnt.m_inode = pDce->m_pFileNodes[index].m_inode;
	
	return &g_FatDirEnt;
}
static FileNode* FsFatFindNonRootDir(FileNode* pNode, const char* pName)
{
	int dceIndex = pNode->m_implData2;
	if (dceIndex == -1)
	{
		LogMsg("[FATAL] Warning: did you mean to `FiOpenDir` first?  Opening for you, but do keep in mind that this isn't how you do things.");
		if (!FsFatOpenNonRootDir(pNode))
		{
			LogMsg("Couldn't even open pNode.  What a shame.  What the fuck?");
			return NULL;
		}
	}
	
	if (dceIndex < 0 || dceIndex >= FD_MAX) return NULL;
	
	DirectoryCacheEntry* pDce = &m_dceEntries[dceIndex];
	if (!pDce->m_used) 
	{
		LogMsg("[FATAL] Huh?? Tried to read a directory entry that had a fake cache implData2? Try again.");
		pNode->m_implData2 = -1;
		return FsFatFindNonRootDir(pNode, pName);
	}
	
	for (int i = 0; i < pDce->m_nFileNodes; i++)
	{
		if (strcmp (pDce->m_pFileNodes[i].m_name, pName) == 0)
			return &pDce->m_pFileNodes[i];
	}
	
	//Not Found.
	return NULL;
}

//Generic
FileNode*             g_fatsMountedPointers     [32];
static FileNode*      g_fatsMountedListFilesPtrs[32];
static FatFileSystem* g_fatsMountedAsFileSystems[32];
int                   g_fatsMountedListFilesCnt [32];
int                   g_fatsMountedCount         = 0;

static DirEnt* FsFatReadRootDir(FileNode* pNode, uint32_t index)
{
	int findex = pNode->m_inode;
	if (findex < 0 || findex >= 32) return NULL;
	if (index >= (uint32_t)g_fatsMountedListFilesCnt[findex])
		return NULL;
	
	strcpy(g_FatDirEnt.m_name, g_fatsMountedListFilesPtrs[findex][index].m_name);
	g_FatDirEnt.m_inode = g_fatsMountedListFilesPtrs[findex][index].m_inode;
	return &g_FatDirEnt;
}
static FileNode* FsFatFindRootDir(FileNode *pNode, const char* pName)
{
	int findex = pNode->m_inode;
	if (findex < 0 || findex >= 32) return NULL;
	
	for (int i = 0; i < g_fatsMountedListFilesCnt[findex]; i++)
	{
		if (strcmp (g_fatsMountedListFilesPtrs[findex][i].m_name, pName) == 0)
			return &g_fatsMountedListFilesPtrs[findex][i];
	}
	return NULL;
}

static void FatMountRootDir(FatFileSystem* pSystem, char* pOutPath)
{
	// Read the directory and count the entries.
	int entryCount = 0;
	//uint32_t dirsPerClus = pSystem->m_clusSize / 32;
	uint32_t cluster = 2;
	
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
				entry += 32;
				continue;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pSystem, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			entry = nextEntry;
			if (secondCluster)
			{
				cluster = secondCluster;
			}
			
			entryCount++;
		}
		cluster = FatGetNextCluster(pSystem, cluster);
		if (cluster >= FAT_END_OF_CHAIN || cluster < 2) break;
	}
	
	// Allocate a FileNode* pointer.
	FileNode* pFileNodes = (FileNode*)MmAllocate (sizeof(FileNode) * entryCount);
	memset (pFileNodes, 0, sizeof(FileNode) * entryCount);
	
	//LogMsg("Counted %d File entries.", entryCount);
	
	// Read the directory AGAIN and fill in the FileNodes
	cluster = 2;
	int index = 0;
	
	while (true)
	{
		uint8_t root_cluster[pSystem->m_clusSize * 2];
		FatGetCluster(pSystem, root_cluster, cluster);
		uint8_t* entry = root_cluster;
		while ((uint32_t)(entry - root_cluster) < pSystem->m_clusSize)
		{
			if (index >= entryCount)
			{
				LogMsg("WARNING: Still have entries?! STOPPING NOW! This is UNACCEPTABLE.");
				break;
			}
			
			uint8_t firstByte = *entry;
			while (firstByte == 0x00 || firstByte == 0xE5)
			{
				entry += 32;
				firstByte = *entry;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pSystem, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			//LogMsg("> Loading file entry '%s' (index %d)", targetDirEnt.m_pName, index);
			
			// turn this entry into a FileNode entry.
			FileNode *pCurrent = pFileNodes + index;
			
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
			pCurrent->m_implData1 = -1;
			pCurrent->m_implData2 = -1;
			pCurrent->m_flags    = 0;
			
			if (!(targetDirEnt.m_dirFlags & FAT_DIRECTORY))
			{
				pCurrent->Read  = FsFatRead;
				pCurrent->Write = NULL;
				pCurrent->Open  = FsFatOpen;
				pCurrent->Close = FsFatClose;
			}
			
			//TODO: directory I/O
			pCurrent->OpenDir  = FsFatOpenNonRootDir;
			pCurrent->CloseDir = FsFatCloseNonRootDir;
			pCurrent->ReadDir  = FsFatReadNonRootDir;
			pCurrent->FindDir  = FsFatFindNonRootDir;
			
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
	
	// Make a root file node and add it to the initrd node.
	FileNode* pFatRoot = (FileNode*)MmAllocate(sizeof(FileNode));
	g_fatsMountedPointers     [g_fatsMountedCount] = pFatRoot;
	g_fatsMountedAsFileSystems[g_fatsMountedCount] = pSystem;
	g_fatsMountedListFilesPtrs[g_fatsMountedCount] = pFileNodes;
	g_fatsMountedListFilesCnt [g_fatsMountedCount] = entryCount;
	
	sprintf(pFatRoot->m_name, "Fat%d", g_fatsMountedCount);
	strcpy (pOutPath, pFatRoot->m_name);
	pFatRoot->m_type  = FILE_TYPE_DIRECTORY;
	pFatRoot->m_flags = 0;
	pFatRoot->m_perms = PERM_READ|PERM_WRITE|PERM_EXEC;
	pFatRoot->m_inode = g_fatsMountedCount;
	pFatRoot->m_length = 0;
	pFatRoot->m_implData = 0;
	
	pFatRoot->Read     = NULL;
	pFatRoot->Write    = NULL;
	pFatRoot->Open     = NULL;
	pFatRoot->Close    = NULL;
	pFatRoot->OpenDir  = NULL;
	pFatRoot->CloseDir = NULL;
	pFatRoot->ReadDir  = FsFatReadRootDir;
	pFatRoot->FindDir  = FsFatFindRootDir;
	
	g_fatsMountedCount++;
}
/*
static void FatUnmountRootDir(FatFileSystem* pSystem)
{
	//TODO
}
*/
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
	pFat32->m_pFat            = (uint32_t*)MmAllocate (nBytesPerFAT);
	pFat32->m_pbFatModified   = (bool*)MmAllocate (pFat32->m_bpb.m_nSectorsPerFat32);
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
	
	LogMsg("Mounted '%s'.  Cluster size: %d", out_path, 512 * pFat32->m_bpb.m_nSectorsPerCluster);
	
	return MOUNT_SUCCESS;
}

void FatUnmountFileSystem(FatFileSystem* pFS)
{
	FatFlushFat (pFS);
	MmFree (pFS->m_pFat);
	MmFree (pFS->m_pbFatModified);
	pFS->m_pFat          = NULL;
	pFS->m_pbFatModified = NULL;
	pFS->m_bMounted      = false;
	LogMsg("File system unmounted.  Need to do it from the VFS side too. TODO!");
}

void FsMountFatPartitions ()
{
	// probe each drive
	MasterBootRecord record;
	for (int i = 0; i < 0x100; i++)
	{
		if (!StIsDriveAvailable(i)) continue;
		
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