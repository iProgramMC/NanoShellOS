/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        FILE SYSTEM: FAT 32 module
******************************************/

#include <fat.h>
#include <task.h>

// Forward Declarations
#if 1

uint32_t FsFat32CountClusters(Fat32FileSystem *pFS, uint32_t cluster);

static inline void FsFat32AssertFileType(File *pFile);
uint32_t FsFat32GetNextCluster  (Fat32FileSystem *pFS, uint32_t cluster);
void     FsFat32ReadCluster     (Fat32FileSystem *pFS, uint32_t cluster, void *pDataOut);

static inline void FsFat32AssertDirectoryType(Directory *pFile);

File*      FsFat32OpenInt        (FileSystem *pFSBase, DirectoryEntry* entry);
Directory* FsFat32OpenDir        (FileSystem *pFSBase, uint32_t dirID);
bool       FsFat32LocateFileInDir(FileSystem *pFSBase, uint32_t dirID, const char *pFN, DirectoryEntry *pEntry, StatResult* result);
uint32_t   FsFat32GetRootDirID   (FileSystem *pFSBase);

#endif

// Utilities
#if 1
// WORK: Need to fix this for Big Endian platforms
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

static inline uint16_t GetU16(uint8_t* buffer, int offset)
{
	return *((uint16_t*)(buffer + offset));
}
static inline uint32_t GetU32(uint8_t* buffer, int offset)
{
	return *((uint32_t*)(buffer + offset));
}
static inline bool FsFat32IsEOF(uint32_t clusNum)
{
	return clusNum >= 0x0FFFFFF0;
}
static inline int FsFat32GetClusterSize (Fat32FileSystem *pFS)
{
	return pFS->clusSize;
}
static inline bool FsFat32IsValidPartType(uint8_t partType)
{
	//https://en.wikipedia.org/wiki/Partition_type
	switch (partType)
	{
	case 0x0B: // FAT32 with CHS addressing
	case 0x0C: // FAT32 with LBA addressing
		return true;
	}
	return false;
}
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
#endif

// File I/O
#if 1

bool FsFat32ClusterChainRegenerateCache(Fat32ClusterChain *pChain)
{
	int clusSize = FsFat32GetClusterSize(pChain->pFS);
	if ((pChain->offsetBytes % clusSize == 0))
	{
		if (pChain->offsetBytes == 0)
			pChain->currentCluster = pChain->firstCluster;
		else
		{
			pChain->currentCluster = FsFat32GetNextCluster(pChain->pFS, pChain->currentCluster);
			if (FsFat32IsEOF(pChain->currentCluster))
				return true;
		}
	}

	// load a new cluster
	pChain->clusterCacheLoaded = true;

	FsFat32ReadCluster(pChain->pFS, pChain->currentCluster, pChain->clusterCache);
	return false;
}
int FsFat32ClusterChainTell(File* file)
{
	FsFat32AssertFileType(file);
	Fat32ClusterChain *pChain = (Fat32ClusterChain*)file;
	
	return pChain->offsetBytes;
}
int FsFat32ClusterChainTellSize(File* file)
{
	FsFat32AssertFileType(file);
	Fat32ClusterChain *pChain = (Fat32ClusterChain*)file;
	
	if (pChain->entry.file_length >= 0)
	{
		SLogMsg("Returning file length as %d", pChain->entry.file_length);
		return pChain->entry.file_length;
	}
	else
	{
		// auto-calculate size
		uint32_t size = FsFat32GetClusterSize(pChain->pFS) * FsFat32CountClusters(pChain->pFS, pChain->firstCluster);
		
		pChain->entry.file_length = size;
		
		SLogMsg("Returning file length as %d", pChain->entry.file_length);
		return size;
	}
}
bool FsFat32ClusterChainSeek(File* file, int position, int whence, bool bAllowExpansion)
{
	FsFat32AssertFileType(file);
	
	Fat32ClusterChain *pChain = (Fat32ClusterChain*)file;
	int clusSize  = FsFat32GetClusterSize(pChain->pFS);
	switch (whence)
	{
		case SEEK_END:
			position += pChain->entry.file_length;
		case SEEK_SET:
			pChain->currentCluster = pChain->firstCluster;
			pChain->offsetBytes    = 0;
		case SEEK_CUR:
			if (pChain->offsetBytes == 0 && !pChain->clusterCacheLoaded) // TODO: optimize this!
			{
				if (FsFat32ClusterChainRegenerateCache(pChain))
					return true;
			}
			
			if (FsFat32IsEOF(pChain->currentCluster))
				return true;
			
			// TODO: Optimize this quite a bit
			for (int i = 0; i < position; i++)
			{
				if (FsFat32IsEOF(pChain->currentCluster)) return true;
				
				//TODO: Allow expansion of file
				if (!bAllowExpansion)
				{
					if (pChain->offsetBytes >= (uint32_t)pChain->entry.file_length  &&  pChain->entry.file_length >= 0)
						return true;
				}
				
				if ((pChain->offsetBytes % clusSize == 0) || !pChain->clusterCacheLoaded)
				{
					if (FsFat32ClusterChainRegenerateCache(pChain))
						return true;
				}
				
				pChain->offsetBytes++;
			}
			break;
	}
	return false;
}
uint32_t FsFat32ClusterChainRead(File* file, void *pOut, uint32_t size)
{
	FsFat32AssertFileType(file);
	
	Fat32ClusterChain *pChain = (Fat32ClusterChain*)file;
	
	int clusSize  = FsFat32GetClusterSize(pChain->pFS);
	uint8_t* pData = (uint8_t*)pOut;
	uint32_t i = 0;
	if (FsFat32ClusterChainSeek(file, 0, SEEK_CUR, false))
		return 0;
	
	for (i = 0; i != size; i++)
	{
		if (pChain->offsetBytes >= (uint32_t)pChain->entry.file_length && pChain->entry.file_length >= 0)
			return i;
		
		pData[i] = pChain->clusterCache[pChain->offsetBytes % clusSize];
		
		if (FsFat32ClusterChainSeek(file, 1, SEEK_CUR, false))
			return i;
	}
	
	return i;
}
void FsFat32ClusterChainClose(File* file)
{
	FsFat32AssertFileType(file);
	
	Fat32ClusterChain *pChain = (Fat32ClusterChain*)file;
	
	pChain->taken = false;
	pChain->clusterCacheLoaded = false;
	pChain->currentCluster = pChain->firstCluster = pChain->offsetBytes = 0;
	
	if (pChain->clusterCache)
		MmFreeK(pChain->clusterCache);
	pChain->clusterCache = NULL;
}
static inline void FsFat32AssertFileType(File *pFile)
{
	ASSERT(pFile->Read     == FsFat32ClusterChainRead      &&  "This is not a file on a FAT32 file system.");
	ASSERT(pFile->Close    == FsFat32ClusterChainClose     &&  "This is not a file on a FAT32 file system.");
	ASSERT(pFile->Seek     == FsFat32ClusterChainSeek      &&  "This is not a file on a FAT32 file system.");
	ASSERT(pFile->Tell     == FsFat32ClusterChainTell      &&  "This is not a file on a FAT32 file system.");
	ASSERT(pFile->TellSize == FsFat32ClusterChainTellSize  &&  "This is not a file on a FAT32 file system.");
}

#endif

// Fat32 Directory
#if 1

void FsFat32DirectoryClose(struct Directory* pDirBase)
{
	FsFat32AssertDirectoryType(pDirBase);
	
	Fat32Directory *pDir = (Fat32Directory*)pDirBase;
	
	pDir->taken = false;
	pDir->file->Close((File*)pDir->file);
}
bool FsFat32ReadDirEntry(Fat32ClusterChain *pChain, DirectoryEntry *pDirEnt, StatResult *pStatResultOut)
{
	uint8_t lfnData[1024], *lfnDataHead = lfnData;
	uint8_t data[32];//CLUSTER_SIZE_DECL(pFS)];
	
	// read the first directory entry
try_again:;
	uint32_t bytesRead = FsFileRead((File*)pChain, data, 32);
	if (!bytesRead)
		return 0;
	
	uint8_t* pEntry = data;
	FatDirectoryEntry* pDirData = (FatDirectoryEntry*)pEntry;
	
	if (*pEntry == 0x00 || *pEntry == 0xE5) goto try_again;
	
	uint32_t lfnCount = 0;
	while (pDirData->attrib == FAT_LFN)
	{
		if (pDirData->sfn[0] & 0x40)
		{
			// this is the first entry
			lfnCount = 0;
		}
		lfnCount++;
		if (pDirData->sfn[0] == (char)0xE5)
		{
			// a deleted file in the middle of our LFN stuff?
			if (lfnCount)
				SLogMsg("Warning: Orphaned LFN entry");
			lfnCount = 0;
		}
		memcpy (lfnDataHead, data, 32);
		lfnDataHead += 32;
		
		if (lfnDataHead >= lfnData + sizeof lfnData)
		{
			SLogMsg("WARNING!  lfnDataHead went over the allocated size!  This is probably a sign of the file name too big.  Don't want to risk an overflow, so quitting.");
			return false;
		}
		
		// load some more
		bytesRead = FsFileRead((File*)pChain, data, 32);
		if (bytesRead == 0)
		{
			SLogMsg("Warning: Orphaned LFN entry to end of directory");
			return false;
		}
	}
	
	if (*pEntry == 0x00 || *pEntry == 0xE5) goto try_again;
	
	// treat this seperately, because this flag is set on LFN entries too, which we do NOT want to ignore
	if (pDirData->attrib & FAT_VOLUME_ID) goto try_again;
	
	// if it's one of the "." or ".." entries, skip it.
	if (pDirData->sfn[0] == '.') goto try_again;
	
	if (pDirEnt)
	{
		memset(pDirEnt, 0, sizeof (*pDirEnt));
		
		if (lfnCount > 0)
		{
			FatParseLFN(pDirEnt->name, lfnData, lfnCount);
		}
		else
		{
			// There is no LFN, trim up the SFN
			
			memcpy(pDirEnt->name, pDirData, 11);
			pDirEnt->name[11] = 0;
	
			char ext[4]; int i;
	
			memcpy(ext, pDirEnt->name + 8, 3);
	
			// trim spaces from extension
			ext[3] = 0;
			i = 2;
			while (i >= 0 && ext[i] == ' ')
				ext[i--] = 0;
	
			// trim spaces from name
			pDirEnt->name[8] = 0;
			i = 7;
			while (i >= 0 && pDirEnt->name[i] == ' ')
				pDirEnt->name[i--] = 0;
	
			if (strlen(ext) != 0)
			{
				strcat(pDirEnt->name, ".");
				strcat(pDirEnt->name, ext);
			}
	
			int nameLen = strlen(pDirEnt->name);
	
			bool lowercase = (pEntry[12] & FAT_MSFLAGS_NAMELOWER) != 0;
			for (int i = 0; i < nameLen; i++)
			{
				if (lowercase)
					pDirEnt->name[i] = ToLower(pDirEnt->name[i]);
				if (pDirEnt->name[i] == '.')
					lowercase = (pEntry[12] & FAT_MSFLAGS_EXTELOWER) != 0;
			}
		}
		
		uint32_t cluster_id = pDirData->clusNumHigh << 16 | pDirData->clusNumLow;
	
		//pDirEnt->attributes       = pSFNEntry->attrib;
		pDirEnt->file_id          = FS_MAKE_ID(pChain->pFS->fsID, cluster_id);
		pDirEnt->file_length      = pDirData->fileSize;
		pDirEnt->type             = FILE_TYPE_FILE;
		//pDirEnt->perms            = (PERM_READ | PERM_WRITE | PERM_EXEC);
		
		//if (pDirData->attrib & FAT_READONLY)
			//pDirEnt->perms &= ~PERM_WRITE;
		
		if (pDirData->attrib & FAT_DIRECTORY)
			pDirEnt->type |= FILE_TYPE_DIRECTORY;
	}
	
	if (pStatResultOut)
	{
		memset(pStatResultOut, 0, sizeof (*pStatResultOut));
		
		pStatResultOut->m_type  = FILE_TYPE_FILE;
		pStatResultOut->m_perms = (PERM_READ | PERM_WRITE | PERM_EXEC);
		
		if (pDirData->attrib & FAT_READONLY)
			pStatResultOut->m_perms &= ~PERM_WRITE;
		
		if (pDirData->attrib & FAT_DIRECTORY)
			pStatResultOut->m_type |= FILE_TYPE_DIRECTORY;
		
		uint32_t cluster_id = pDirData->clusNumHigh << 16 | pDirData->clusNumLow;
		
		pStatResultOut->m_size   = pDirData->fileSize;
		pStatResultOut->m_inode  = cluster_id;
		pStatResultOut->m_blocks = FsFat32CountClusters(pChain->pFS,  cluster_id) / FsFat32GetClusterSize(pChain->pFS);
		pStatResultOut->m_modifyTime = 0;
		pStatResultOut->m_createTime = 0;
		pStatResultOut->m_flags = 0;
		pStatResultOut->m_fs_id = pChain->pFS->fsID;
		pStatResultOut->m_dev_type = 0;
	}
	
	return true;
}
bool FsFat32DirectoryReadEntry(Directory* pDirBase, DirectoryEntry* pOut, StatResult *pStatResultOut)
{
	FsFat32AssertDirectoryType(pDirBase);
	Fat32Directory *pDir = (Fat32Directory*)pDirBase;
	
	bool b = FsFat32ReadDirEntry(pDir->file, pOut, pStatResultOut);
	if (b)
		pDir->told++;
	return b;
}
void FsFat32DirectorySeek(Directory* pDirBase, int position)
{
	FsFat32AssertDirectoryType(pDirBase);
	
	Fat32Directory *pDir = (Fat32Directory*)pDirBase;
	
	FsFileSeek((File*)pDir->file, 0, SEEK_SET, false);
	pDir->told = 0;
	
	DirectoryEntry unused;
	while (position--)
	{
		if (!FsDirectoryReadEntry((Directory*)pDir, &unused, NULL))
			return;
	}
}
int FsFat32DirectoryTell(Directory* pDirBase)
{
	FsFat32AssertDirectoryType(pDirBase);
	Fat32Directory *pDir = (Fat32Directory*)pDirBase;
	return pDir->told;
}
static inline void FsFat32AssertDirectoryType(Directory *dir)
{
	ASSERT(dir->ReadEntry == FsFat32DirectoryReadEntry &&  "This is not a directory on a FAT32 file system.");
	ASSERT(dir->Close     == FsFat32DirectoryClose     &&  "This is not a directory on a FAT32 file system.");
	ASSERT(dir->Seek      == FsFat32DirectorySeek      &&  "This is not a directory on a FAT32 file system.");
	ASSERT(dir->Tell      == FsFat32DirectoryTell      &&  "This is not a directory on a FAT32 file system.");
}

#endif

// File system I/O
#if 1

static inline void FsFat32AssertType(FileSystem *pFS)
{
	ASSERT(pFS->OpenInt         == FsFat32OpenInt         &&  "This is not a FAT32 file system.");
	ASSERT(pFS->OpenDir         == FsFat32OpenDir         &&  "This is not a FAT32 file system.");
	ASSERT(pFS->LocateFileInDir == FsFat32LocateFileInDir &&  "This is not a FAT32 file system.");
	ASSERT(pFS->GetRootDirID    == FsFat32GetRootDirID    &&  "This is not a FAT32 file system.");
}
void FsFat32ReadBpb(Fat32FileSystem *pFS)
{
	uint8_t data[512];
	StDeviceRead (pFS->partStart, data, pFS->diskID, 1);

	BiosParameterBlock * PBPB = &pFS->bpb;
	PBPB->m_nBytesPerSector      = GetU16(data, 11);
	PBPB->m_nSectorsPerCluster   =        data [13];
	PBPB->m_nReservedSectors     = GetU16(data, 14);
	PBPB->m_nFAT                 =        data [16]; // Usually 1, can be 2
	PBPB->m_nDirEntries          = GetU16(data, 17);
	PBPB->m_nTotalSectors        = GetU16(data, 19);
	PBPB->m_mediaDescType        =        data [21];
	PBPB->m_nSectorsPerFat12or16 = GetU16(data, 22);
	PBPB->m_nSectorsPerTrack     = GetU16(data, 24);
	PBPB->m_nHeadsOrSizesOnMedia = GetU16(data, 26);
	PBPB->m_nHiddenSectors       = GetU32(data, 28);
	PBPB->m_nLargeSectorsOnMedia = GetU32(data, 32);
	
	// Extended boot record
	PBPB->m_nSectorsPerFat32     = GetU32(data, 36);
	PBPB->m_nFlags               = GetU16(data, 40);
	PBPB->m_nFatVersion          = GetU16(data, 42);
	PBPB->m_nRootDirStartCluster = GetU32(data, 44);
	PBPB->m_sectorNumberFsInfo   = GetU16(data, 48);
	PBPB->m_sectorNmbrBckpBtSctr = GetU16(data, 50);
	//skip 12 bytes
	PBPB->m_driveNumber          =        data [64];
	PBPB->m_WindowsFlags         =        data [65];
	PBPB->m_Signature            =        data [66];
	PBPB->m_nVolumeID            = GetU32(data, 67);
	
	memcpy (PBPB->m_sVolumeLabel, data + 71, 11); PBPB->m_sVolumeLabel[11] = 0;
	memcpy (PBPB->m_sSystemID   , data + 82,  8); PBPB->m_sSystemID   [ 8] = 0;
}
Fat32FileSystem *FsConstructFat32FileSystem(uint32_t fsID, uint8_t diskID, uint32_t PartitionStart, uint32_t PartitionSize)
{
	Fat32FileSystem *pFS = MmAllocateK(sizeof (Fat32FileSystem));
	if (!pFS) return pFS;
	
	// clear it out
	memset (pFS, 0, sizeof *pFS);
	
	pFS->fsID = fsID;
	
	// setup the functions
	pFS->OpenInt         = FsFat32OpenInt;
	pFS->OpenDir         = FsFat32OpenDir;
	pFS->LocateFileInDir = FsFat32LocateFileInDir;
	pFS->GetRootDirID    = FsFat32GetRootDirID;
	
	pFS->partStart = PartitionStart;
	pFS->partSize  = PartitionSize;
	pFS->diskID    = diskID;
	
	// read the BPB
	FsFat32ReadBpb(pFS);
	
	SLogMsg("Volume serial is %08x", pFS->bpb.m_nVolumeID);
	
	// initialize other things
	pFS->fatBeginSector  = pFS->bpb.m_nReservedSectors                     + pFS->partStart;
	pFS->clusBeginSector = pFS->bpb.m_nSectorsPerFat32   * pFS->bpb.m_nFAT + pFS->fatBeginSector;
	pFS->clusSize        = pFS->bpb.m_nSectorsPerCluster * SECTOR_SIZE;
	
	SLogMsg("Cluster size  is %d",  pFS->clusSize);
	
	int clusSize = pFS->clusSize;
	pFS->clusSizeLog2 = -1;
	while (clusSize)
	{
		pFS->clusSizeLog2++;
		clusSize >>= 1;
	}
	
	// Allocate some memory to store the FAT and stuff
	pFS->pFat         = MmAllocateK(pFS->bpb.m_nSectorsPerFat32 * SECTOR_SIZE);
	pFS->pFatModified = MmAllocateK(pFS->bpb.m_nSectorsPerFat32 / 8 + 1);
	pFS->pFatLoaded   = MmAllocateK(pFS->bpb.m_nSectorsPerFat32 / 8 + 1);
	
	memset (pFS->pFat,         0xFF, pFS->bpb.m_nSectorsPerFat32 * SECTOR_SIZE);
	memset (pFS->pFatModified, 0x00, pFS->bpb.m_nSectorsPerFat32 / 8 + 1);
	memset (pFS->pFatLoaded,   0x00, pFS->bpb.m_nSectorsPerFat32 / 8 + 1);
	
	return pFS;
}
FileSystem *FsConstructFat32FileSystemBase(uint32_t fsID, uint8_t diskID, uint32_t PartitionStart, uint32_t PartitionSize)
{
	return (FileSystem*)FsConstructFat32FileSystem(fsID, diskID, PartitionStart, PartitionSize);
}
uint32_t FsFat32GetNextCluster(Fat32FileSystem *pFS, uint32_t cluster)
{
	// Check if we have loaded this FAT sector:
	uint32_t fat_sector = cluster / (SECTOR_SIZE / 4);

	uint32_t fat_flag_index1 = fat_sector / 8;
	uint32_t fat_flag_index2 = fat_sector % 8;

	LockAcquire(&pFS->lock);
	if (!(pFS->pFatLoaded[fat_flag_index1] & (1 << fat_flag_index2)))
	{
		// load the FAT at this sector
		uint8_t data[SECTOR_SIZE];
		StDeviceRead(pFS->fatBeginSector + fat_sector, data, pFS->diskID, 1);

		memcpy(&pFS->pFat[fat_sector * (SECTOR_SIZE / 4)], data, sizeof data);

		pFS->pFatLoaded[fat_flag_index1] |= (1 << fat_flag_index2);
	}
	LockFree(&pFS->lock);

	return pFS->pFat[cluster];
}
void FsFat32DestroyFileSystem(Fat32FileSystem * pFS)
{
	LockAcquire(&pFS->lock);
	if (pFS->pFat)         MmFreeK(pFS->pFat);
	if (pFS->pFatModified) MmFreeK(pFS->pFatModified);
	if (pFS->pFatLoaded)   MmFreeK(pFS->pFatLoaded);
	
	// To whoever's trying to access this dying file system, tough luck. Sorry
	MmFreeK(pFS);
}
Fat32ClusterChain* FsFat32OpenIntInt (Fat32FileSystem *pFS, DirectoryEntry* entry)
{
	uint32_t firstCluster = (uint32_t)entry->file_id;
	
	// Look for a free handle
	int freeArea = -1;
	for (int i = 0; i < C_MAX_CLUSCHAIN_HANDLES; i++)
	{
		if (!pFS->fileHandles[i].taken)
		{
			freeArea = i;
			break;
		}
	}
	
	if (freeArea < 0)
	{
		LockFree(&pFS->lock);
		return NULL;
	}
	
	Fat32ClusterChain *pChain = &pFS->fileHandles[freeArea];
	pChain->taken  = true;
	pChain->pFS    = pFS;
	pChain->fileID = entry->file_id;
	pChain->entry  = *entry;
	pChain->currentCluster = pChain->firstCluster = firstCluster;
	pChain->offsetBytes    = 0;
	pChain->clusterCacheLoaded = 0;
	pChain->clusterCache       = MmAllocateK(FsFat32GetClusterSize(pFS));
	
	pChain->Read  = FsFat32ClusterChainRead;
	pChain->Write = NULL;//FsFat32ClusterChainWrite;
	pChain->Seek  = FsFat32ClusterChainSeek;
	pChain->Tell  = FsFat32ClusterChainTell;
	pChain->Close = FsFat32ClusterChainClose;
	pChain->TellSize = FsFat32ClusterChainTellSize;
	
	return pChain;
}
File* FsFat32OpenInt (FileSystem *pFSBase, DirectoryEntry* entry)
{
	// Check that the type is the same.
	FsFat32AssertType (pFSBase);
	
	// Convert to specific type.
	Fat32FileSystem *pFS = (Fat32FileSystem *)pFSBase;
	
	// Check that the directory entry's file ID is the same as this file system's ID
	ASSERT((entry->file_id >> 32) == pFS->fsID);
	
	// Lock the file system for a bit
	LockAcquire(&pFS->lock);
	File* pFile = (File*)FsFat32OpenIntInt(pFS, entry);
	LockFree (&pFS->lock);
	
	return pFile;
}
Directory* FsFat32OpenDir (FileSystem *pFSBase, uint32_t dirID)
{
	// Check that the type is the same.
	FsFat32AssertType (pFSBase);
	
	// Convert to specific type.
	Fat32FileSystem *pFS = (Fat32FileSystem *)pFSBase;
	
	// Lock the file system for a bit
	LockAcquire(&pFS->lock);
	
	// Look for a free handle
	int freeArea = -1;
	for (int i = 0; i < C_MAX_DIRECTORY_HANDLES; i++)
	{
		if (!pFS->dirHandles[i].taken)
		{
			freeArea = i;
			break;
		}
	}
	
	if (freeArea < 0)
	{
		LockFree(&pFS->lock);
		return NULL;
	}
	
	
	DirectoryEntry entry;
	entry.type         = FILE_TYPE_FILE | FILE_TYPE_DIRECTORY;
	entry.file_length  = -1;
	entry.file_id      = FS_MAKE_ID(pFS->fsID, dirID);
	strcpy (entry.name, "");
	
	Fat32Directory *pDir = &pFS->dirHandles[freeArea];
	
	pDir->taken = true;
	pDir->told  = 0;
	pDir->pFS   = pFS;
	pDir->file  = FsFat32OpenIntInt(pFS, &entry);
	FsFileSeek((File*)pDir->file, 0, SEEK_SET, false);
	
	pDir->ReadEntry  = FsFat32DirectoryReadEntry;
	pDir->Seek       = FsFat32DirectorySeek;
	pDir->Tell       = FsFat32DirectoryTell;
	pDir->Close      = FsFat32DirectoryClose;
	
	LockFree (&pFS->lock);
	
	return (Directory*)pDir;
}
void FsFat32ReadCluster(Fat32FileSystem *pFS, uint32_t cluster, void *pDataOut)
{
	// Get sector number
	uint32_t sector_number = pFS->clusBeginSector + ((cluster - 2) * pFS->bpb.m_nSectorsPerCluster);
	
	// read the data
	StDeviceRead(sector_number,  pDataOut,  pFS->diskID,  pFS->bpb.m_nSectorsPerCluster);
}
uint32_t FsFat32CountClusters(Fat32FileSystem *pFS, uint32_t cluster)
{
	uint32_t num = 0;
	while (!FsFat32IsEOF(cluster))
	{
		num++;
		cluster = FsFat32GetNextCluster(pFS, cluster);
	}
	return num;
}
uint32_t FsFat32GetRootDirID(FileSystem *pFSBase)
{
	// Check that the type is the same.
	FsFat32AssertType (pFSBase);
	
	// Convert to specific type.
	Fat32FileSystem *pFS = (Fat32FileSystem *)pFSBase;
	
	// Get the root directory itself.
	return pFS->bpb.m_nRootDirStartCluster;
}
bool FsFat32LocateFileInDir(FileSystem *pFSBase, uint32_t dirID, const char *pFN, DirectoryEntry *pEntry, StatResult* result)
{
	// Check that the type is the same.
	FsFat32AssertType (pFSBase);
	
	// Convert to specific type.
	Fat32FileSystem *pFS = (Fat32FileSystem *)pFSBase;
	
	// Open a directory
	Fat32Directory *pDirectory = (Fat32Directory*)pFS->OpenDir(pFSBase, dirID);
	if (!pDirectory) return NULL;
	
	while (FsDirectoryReadEntry((Directory*)pDirectory, pEntry, result))
	{
		if (strcmp (pEntry->name, pFN) == 0)
		{
			pDirectory->Close((Directory*)pDirectory);
			return true;
		}
	}
	
	pDirectory->Close((Directory*)pDirectory);
	return false;
}

#endif

// Mounting code
#if 1

int FsMountFatPartition(DriveID driveID, int partitionStart, int partitionSizeSec, int partitionNum)
{
	SLogMsg("Attempting to mount potential FAT32 partition on drive %b, partition %b...", driveID, partitionNum);
	uint32_t fsID = 0x200000 | (driveID * 4) | partitionNum;
	SLogMsg("fsID: %x",fsID);
	
	FileSystem *pFS = FsConstructFat32FileSystemBase(fsID, (uint8_t)driveID, partitionStart, partitionSizeSec);
	SLogMsg("pFS->fsID: %x",pFS->fsID);
	if (!pFS)
	{
		SLogMsg("Could not allocate file system");
		return 1;
	}
	
	if (!FsMountFileSystem(pFS))
	{
		SLogMsg("Could not mount file system");
		return 1;
	}
	
	if (!FsGetGlobalRoot())
	{
		uint64_t rootID = pFS->GetRootDirID(pFS);
		uint32_t fisyID = pFS->fsID;
		
		FileID fileID = FS_MAKE_ID(pFS->fsID,  pFS->GetRootDirID(pFS));
		
		SLogMsg("Setting this file system as root (the root dir's ID is %Q, stuff1: %x  stuff2: %x)", fileID, rootID, fisyID);
		
		FsSetGlobalRoot(fileID);
	}
	
	return 0;
}
void FatUnmountFileSystem(FatFileSystem* pFS)
{
	//TODO
}
void FsFatInit ()
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
			
			if (FsFat32IsValidPartType(pPart->m_PartTypeDesc))
			{
				// This is a valid partition.  Mount it.
				if (FsMountFatPartition (i, pPart->m_StartLBA, pPart->m_PartSizeSectors, pi))
					SLogMsg("Could not mount potential FAT32 partition on drive %b, partition %b", i, pi);
			}
		}
	}
}

#endif
