/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        FILE SYSTEM: FAT 32 module
******************************************/

//NOTE: The driver is still at an experimental state
//Notable issues include LFN support

#include <vfs.h>
#include <console.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>
#include <storabs.h>

#define FSDEBUG

#ifdef FSDEBUGX
#define DebugLogMsg LogMsg
#elif defined(FSDEBUG)
#define DebugLogMsg SLogMsg
#else
#define DebugLogMsg(...)
#endif

// Credits: https://github.com/knusbaum/kernel/blob/master/fat32.c

#define ADD_TIMESTAMP_TO_FILES

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

// Extensions that Microsoft uses
#define FAT_MSFLAGS_NAMELOWER (1 << 3) // 0x08
#define FAT_MSFLAGS_EXTELOWER (1 << 4) // 0x10

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
        return NULL;
    }

    uint32_t LFNCount = 0;
    while (entry[11] == FAT_LFN)
	{
		if (entry[0] & 0x40)
		{
			//Last entry.  If we had other LFN entries before then, they're orphaned entries.
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
		SLogMsg("Orphaned longfilename entry detected.  Skipping.");
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
	bool           bLoadedWorkCluster;
	uint32_t       nClusterCurrent,  // Cluster number inside the FAT.  Used to continue reading beyond the work cluster
				   nClusterProgress, // Number of clusters that have been passed to reach this point.  Useful to check if "offset" has changed at all.
				   nClusterFirst;    // First cluster.  Used to re-resolve nClusterCurrent if offset has gone back.
	bool           bAllowWriting;
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
	
	pFileNode->m_inode = freeInode;
	
	return true;
}

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
		MmFree(pNode->pWorkCluster);
		pNode->pWorkCluster = NULL;
	}
	
	// Deinitialize this fatinode:
	pNode->pOpenedIn = NULL;
	
	// Reset the pFileNode to its default state:
	pFileNode->m_inode = pNode->nClusterFirst;
	pNode->nClusterCurrent = pNode->nClusterFirst = pNode->nClusterProgress = 0;
}


extern FileDescriptor g_FileNodeToDescriptor[FD_MAX];
FileNode*             g_fatsMountedPointers     [32];
static FileNode*      g_fatsMountedListFilesPtrs[32];
static FatFileSystem* g_fatsMountedAsFileSystems[32];
int                   g_fatsMountedListFilesCnt [32];
int                   g_fatsMountedCount         = 0;

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

FileNode* FatCreateEmptyFile(FileNode *pDirNode, const char* pFileName)
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
		SLogMsg("Are you sure it doesn't require LFN? (Filename:%s)", pFileName);
		//Are you sure???
		bool firstLetterCap = pFileName[0] >= 'a' && pFileName[0] <= 'z';
		for (int i = 1; i < dotIndex; i++)
		{
			//do caps not match?
			bool a = ((pFileName[i] < 'a' || pFileName[i] > 'z') &&  firstLetterCap), b = ((pFileName[i] < 'A' || pFileName[i] > 'Z') && !firstLetterCap);
			if (a || b)
			{
				SLogMsg("Requires LFN because caps don't always match in name (A:%d B:%d C:%c)",a,b,pFileName[i]);
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
				SLogMsg("Requires LFN because caps don't always match in extension (A:%d B:%d, C:%c)",a,b,pFileName[i]);
				requiresLFN = true;
				break;
			}
		}
		
		if (!requiresLFN && firstLetterCap)
			extended_attrs |= FAT_MSFLAGS_EXTELOWER;
		
		if (requiresLFN)
			SLogMsg("I'm not so sure");
		else
			SLogMsg("Fuck yeah!!!");
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
	
	// Write other fields
	pActualEntry[11] = 0x00;
	pActualEntry[12] = extended_attrs;
	
	uint32_t fileCluster = 0;
	// 0-sized files aren't supposed to have a cluster
	pActualEntry[20] = 0;
	pActualEntry[21] = 0;
	pActualEntry[26] = 0;
	pActualEntry[27] = 0;
	
	FatWriteCurrentTimeToEntry (pActualEntry, true);

	*((uint32_t*)(&pActualEntry[0x1C])) = 0;//File length
	
	FatPutCluster(pOpenedIn, rootCluster, cluster); 
	
	// Is this root directory?
	if (pDirNode->m_implData2 == 2)
	{
		// Do the next few steps atomically.
		// TODO: A linked list would be easier to work with, and ideally, that's what I should have used...
		cli;
		
		// yes.  First, create a new and expanded file node list.
		FileNode* pFN    = g_fatsMountedListFilesPtrs[pDirNode->m_inode];
		int       nFNOld = g_fatsMountedListFilesCnt [pDirNode->m_inode];
		
		int       nFNNew = nFNOld + 1;//We added a file.
		FileNode* pFNNew = MmAllocate(sizeof(FileNode) * nFNNew);
		memcpy (pFNNew, pFN, sizeof (FileNode) * nFNOld);
		
		// Add the new file entry:
		FileNode* pEntry = &pFNNew[nFNOld];
		
		memset (pEntry, 0, sizeof (*pEntry));
			
		strcpy(pEntry->m_name, pFileName);
		
		pEntry->m_type = FILE_TYPE_FILE;
		
		pEntry->m_perms = PERM_READ | PERM_WRITE;
		
		if (EndsWith (pFileName, ".nse"))
			pEntry->m_perms |=  PERM_EXEC;
		
		pEntry->m_inode  = fileCluster;
		pEntry->m_length = 0;
		pEntry->m_implData = (int)pOpenedIn;
		pEntry->m_implData1 = 2;//Root directory
		pEntry->m_implData2 = fileCluster;
		pEntry->m_flags    = 0;
		
		pEntry->Read  = FsFatRead;
		pEntry->Write = FsFatWrite;
		pEntry->Open  = FsFatOpen;
		pEntry->Close = FsFatClose;
		
		// Look through all the open fds and convert the old filenodes to the new ones.
		uintptr_t nodes_start = (uintptr_t)pFN;
		uintptr_t nodes_end   = nodes_start + sizeof(FileNode) * nFNOld;
		for (uint32_t i = 0; i < ARRAY_COUNT(g_FileNodeToDescriptor); i++)
		{
			uintptr_t node = (uintptr_t)g_FileNodeToDescriptor[i].m_pNode;
			if (nodes_start <= node && node <= nodes_end)
			{
				int offset = g_FileNodeToDescriptor[i].m_pNode - pFN;
				g_FileNodeToDescriptor[i].m_pNode = pFNNew + offset;
			}
		}
		
		g_fatsMountedListFilesPtrs[pDirNode->m_inode] = pFNNew;
		g_fatsMountedListFilesCnt [pDirNode->m_inode] = nFNNew;
		
		sti;
		
		MmFree(pFN);
		
		return pFNNew + nFNOld;
	}
	else
	{
		DirectoryCacheEntry *pDCE = &m_dceEntries[pDirNode->m_implData3];
		
		// Do the next few steps atomically.
		// TODO: A linked list would be easier to work with, and ideally, that's what I should have used...
		cli;
		
		// yes.  First, create a new and expanded file node list.
		FileNode* pFN    = pDCE->m_pFileNodes;
		int       nFNOld = pDCE->m_nFileNodes;
		
		int       nFNNew = nFNOld + 1;//We added a file.
		FileNode* pFNNew = MmAllocate(sizeof(FileNode) * nFNNew);
		memcpy (pFNNew, pFN, sizeof (FileNode) * nFNOld);
		
		// Add the new file entry:
		FileNode* pEntry = &pFNNew[nFNOld];
		
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
		
		// Look through all the open fds and convert the old filenodes to the new ones.
		uintptr_t nodes_start = (uintptr_t)pFN;
		uintptr_t nodes_end   = nodes_start + sizeof(FileNode) * nFNOld;
		for (uint32_t i = 0; i < ARRAY_COUNT(g_FileNodeToDescriptor); i++)
		{
			uintptr_t node = (uintptr_t)g_FileNodeToDescriptor[i].m_pNode;
			if (nodes_start <= node && node <= nodes_end)
			{
				int offset = g_FileNodeToDescriptor[i].m_pNode - pFN;
				g_FileNodeToDescriptor[i].m_pNode = pFNNew + offset;
			}
		}
		
		pDCE->m_pFileNodes = pFNNew;
		pDCE->m_nFileNodes = nFNNew;
		
		sti;
		
		MmFree(pFN);
		
		return pFNNew + nFNOld;
	}
	
	return NULL;
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
			while (firstByte == 0x00 || firstByte == 0xE5)
			{
				entry += 32;
				firstByte = *entry;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pNode->pOpenedIn, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			if (strcmp(targetDirEnt.m_pName, pFileNode->m_name) == 0)
			{
				uint8_t* pEntry2 = entry;
				
				while (pEntry2[11] == FAT_LFN)
				{
					pEntry2 += 32;
					if (pEntry2 > root_cluster + sizeof(root_cluster))
					{
						SLogMsg("Can't overwrite entry! Filesystem may or may not be corrupted.  If you have windows installed be sure to chkdsk this disk");
						return false;
					}
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

static bool FatUpdateFileEntryNotOpened(FileNode *pFileNode, uint32_t newSize, uint32_t newCluster)
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
			while (firstByte == 0x00 || firstByte == 0xE5)
			{
				entry += 32;
				firstByte = *entry;
			}
			
			uint32_t secondCluster = 0;
			uint8_t* nextEntry = NULL;
			FatDirEntry targetDirEnt;
			FatNextDirEntry (pOpenedIn, root_cluster, entry, &nextEntry, &targetDirEnt, cluster, &secondCluster);
			
			if (strcmp(targetDirEnt.m_pName, pFileNode->m_name) == 0)
			{
				//Found it!
				//Write stuff to it.
				*((uint32_t*)(entry + 28)) = newSize;
				
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
	FatUpdateFileEntryNotOpened (pFileNode, 0, 0xF0000000);
	
	//Since we added a new cluster...
	FatFlushFat(pSystem);
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
		FatUpdateFileEntryNotOpened (pFN, 0, FatAllocateCluster(pSystem));
		FatFlushFat(pSystem);//Since we allocated a new cluster
	}
	else
	{
		FatCreateEmptyFile (pDirectoryNode, pFileName);
	}
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

uint32_t FsFatWrite (UNUSED FileNode *pFileNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void* pBuffer)
{
	DebugLogMsg("FsFatWrite(%x, %d, %d, %x)", pFileNode,offset,size,pBuffer);
	FatInode* pNode = &s_FatInodes[pFileNode->m_inode];
	
	if (!pNode->pOpenedIn) return 0;
	
	bool needsToReloadCluster = false;

	int remainderToExpandTo = 0;
	// Determine how much we can read
	if (offset + size > pFileNode->m_length)
	{
		int oldSize = size;
		size = pFileNode->m_length - offset;
		remainderToExpandTo = oldSize - size;
		
		//do we need to expand?
		if (remainderToExpandTo > 0)
		{
			//Expand here, and then FsFatWrite again.
			DebugLogMsg("Cluster Size is  %d.",  pNode->pOpenedIn->m_clusSize);
			DebugLogMsg("Want to expand by %d?", remainderToExpandTo);
			
			int newSize = pFileNode->m_length + remainderToExpandTo;
			int clusterSizeOld = (pFileNode->m_length + pNode->pOpenedIn->m_clusSize - 1) / pNode->pOpenedIn->m_clusSize;
			int clusterSizeNew = (newSize             + pNode->pOpenedIn->m_clusSize - 1) / pNode->pOpenedIn->m_clusSize;
			
			uint32_t newCluster1 = 0; //0 means don't update that
			// Do they differ?
			if (clusterSizeOld != clusterSizeNew)
			{
				// yes. Allocate however many cluster it takes:
				int clustersToAllocate = clusterSizeNew - clusterSizeOld;
				DebugLogMsg("Yes.  Need to allocate %d clusters.", clustersToAllocate);
				
				uint32_t nLastCluster = pNode->nClusterFirst;
				if (pNode->nClusterFirst == 0)
				{
					//Empty file: create one cluster
					pNode->nClusterFirst = nLastCluster = FatAllocateCluster(pNode->pOpenedIn);
					
					DebugLogMsg("FS First cluster is 0, changing it to our own NEW cluster %d!", nLastCluster);
					
					FatFlushFat(pNode->pOpenedIn);
					
					//We've allocated one cluster.  Remove one from the number of clusters to allocate.
					clustersToAllocate--;
					
					//When updating the file entry, make sure that the entry knows that the file starts here.
					newCluster1 = nLastCluster;
					
					//Also let the abstract FS know
					pFileNode->m_implData2 = nLastCluster;
					pNode->nClusterFirst   = nLastCluster;
					pNode->nClusterCurrent = nLastCluster;
					pNode->nClusterProgress = 0;
				}
				else
				{
					//Get the last cluster
					do
					{
						uint32_t clus = FatGetNextCluster(pNode->pOpenedIn, nLastCluster);
						if (clus < FAT_END_OF_CHAIN)
						{
							nLastCluster = clus;
						}
						else break;
					}
					while (1);
				}
				
				// Avoid fragmentation
				pNode->pOpenedIn->m_clusAllocHint = nLastCluster;
				while (clustersToAllocate)
				{
					DebugLogMsg("Last Cluster Number is %d.  Expanding by one cluster, %d left.", nLastCluster, clustersToAllocate);
					uint32_t nextCluster = FatAllocateCluster(pNode->pOpenedIn);
					if (!nextCluster) 
					{
						LogMsg("ERROR! Expansion did not work, drive full?");
						return 0;
					}
					DebugLogMsg("Allocated Cluster Number %d.", nextCluster);
					FatClearCluster   (pNode->pOpenedIn, nextCluster);
					FatSetClusterInFAT(pNode->pOpenedIn, nLastCluster, nextCluster);
					FatSetClusterInFAT(pNode->pOpenedIn, nextCluster,  FAT_END_OF_CHAIN);
					nLastCluster = nextCluster;
					clustersToAllocate--;
				}
			}
			
			//TODO: Write directory entry to update size.
			
			if (!FatUpdateFileEntry(pFileNode, newSize, newCluster1))
			{
				LogMsg("ERROR: Could Not Write Entry! FileSystem may be corrupted!!!");
				return 0;
			}
			else
				DebugLogMsg("Overwrote entry.");
			
			FatFlushFat(pNode->pOpenedIn);
			//return 0;
			
			return FsFatWrite(pFileNode, offset, oldSize, pBuffer);
		}
	}
	
	if (size <= 0) return 0;
	
	if (!pNode->pWorkCluster)
	{	
		DebugLogMsg("Didn't allocate work cluster, doing it now");
		pNode->pWorkCluster  = (uint8_t*)MmAllocate (pNode->pOpenedIn->m_clusSize);
		needsToReloadCluster = true;
		pNode->bLoadedWorkCluster = false;
		if (!pNode->pWorkCluster)
			return 0; //Out of memory
	}
	
	// Cluster offset to read from.  NOT the cluster number, just the number of cluster steps to go through to reach this.
	uint32_t clusterOffsetCurrent = offset / pNode->pOpenedIn->m_clusSize;
	
	// Did it change from the current cluster? If yes, how?
	uint32_t clusterToWrite = 0;
	if (clusterOffsetCurrent < pNode->nClusterProgress)
	{
		// It went back.  Re-resolve from the start.
		clusterToWrite = pNode->nClusterCurrent;
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
		clusterToWrite = pNode->nClusterCurrent;
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
	if (clusterToWrite > 0)
	{
		if (pNode->bLoadedWorkCluster)
		{
			DebugLogMsg("Writing some cluster!");
			FatPutCluster (pNode->pOpenedIn, pNode->pWorkCluster, clusterToWrite);
		}
	}
	if (needsToReloadCluster)
	{
		DebugLogMsg("Reloading cluster.");
		FatGetCluster (pNode->pOpenedIn, pNode->pWorkCluster, pNode->nClusterCurrent);
		pNode->bLoadedWorkCluster = true;
	}
	
	int whereToEndWriting = size + insideClusterOffset, howMuchToWrite = size;
	if (whereToEndWriting > (int)pNode->pOpenedIn->m_clusSize)
	{
		howMuchToWrite -= (whereToEndWriting - (int)pNode->pOpenedIn->m_clusSize);
		whereToEndWriting = (int)pNode->pOpenedIn->m_clusSize;
	}
	
	// Place the data in it, finally.
	uint8_t* pointer = (uint8_t*)pBuffer;
	memcpy (pNode->pWorkCluster + insideClusterOffset, pointer, howMuchToWrite);
	DebugLogMsg("Written to memory, waiting to write on disk.");
	pNode->bLoadedWorkCluster = true;
	
	int result = howMuchToWrite;
	if ((uint32_t)howMuchToWrite < size) // More to write?
		// Yeah, write more.
		result += FsFatWrite (pFileNode, offset + howMuchToWrite, size - howMuchToWrite, pointer + howMuchToWrite);
	else
	{
		// No more to write
		DebugLogMsg("No more to write");
	}
	
	return result;
}

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
				//LogMsg("WARNING: Still have entries?! STOPPING NOW! This is UNACCEPTABLE."); -- these entries tend to be garbage
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
			pCurrent->m_implData1 = startCluster;
			pCurrent->m_implData2 = targetDirEnt.m_firstCluster;
			pCurrent->m_flags    = 0;
			
			if (!(targetDirEnt.m_dirFlags & FAT_DIRECTORY))
			{
				pCurrent->Read  = FsFatRead;
				pCurrent->Write = FsFatWrite;
				pCurrent->Open  = FsFatOpen;
				pCurrent->Close = FsFatClose;
				pCurrent->EmptyFile = FatEmptyExistingFile;
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
	pFileNode->m_implData3 = freeDce;
	
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
	int dceIndex = pNode->m_implData3;
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
		pNode->m_implData3 = -1;
		return FsFatReadNonRootDir(pNode, index);
	}
	
	if (index >= (uint32_t)pDce->m_nFileNodes) return NULL;//Out of bounds?
	
	strcpy (g_FatDirEnt.m_name, pDce->m_pFileNodes[index].m_name);
	g_FatDirEnt.m_inode = pDce->m_pFileNodes[index].m_inode;
	
	return &g_FatDirEnt;
}
static FileNode* FsFatFindNonRootDir(FileNode* pNode, const char* pName)
{
	int dceIndex = pNode->m_implData3;
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
		pNode->m_implData3 = -1;
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
				//LogMsg("WARNING: Still have entries?! STOPPING NOW! This is UNACCEPTABLE.");--these entries tend to be garbage.
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
			pCurrent->m_implData1 = 2;//Root directory
			pCurrent->m_implData2 = targetDirEnt.m_firstCluster;
			pCurrent->m_flags    = 0;
			
			if (!(targetDirEnt.m_dirFlags & FAT_DIRECTORY))
			{
				pCurrent->Read  = FsFatRead;
				pCurrent->Write = FsFatWrite;
				pCurrent->Open  = FsFatOpen;
				pCurrent->Close = FsFatClose;
				pCurrent->EmptyFile = FatEmptyExistingFile;
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
	
	LogMsg("Mounted '%s'.", out_path, 512 * pFat32->m_bpb.m_nSectorsPerCluster);
	
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