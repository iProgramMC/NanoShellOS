/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

        Storage Abstraction module
******************************************/

/**
 * A short description of what this module is responsible for:
 *
 * This module is responsible for parsing away drive numbers, and using
 * the drivers meant for them.
 *
 * The mapping of the drive numbers is as follows:
 *
 *    - 0x00 - 0x03: IDE drives
 *    - 0x10 - 0x11: Floppy drives (won't add because they don't support FAT32)
 *    - 0xF0 - 0xFF: RAM disk drives 
 *    - 0xE0 - 0xEF: USB flash drives (?)
 *    - 0xAA       : Special reserved flag. This drive ID is always empty.
 *    - 0x20 - 0x5F: AHCI drives
 */
 
#include <storabs.h>
#include <time.h>

#define ENABLE_CACHING

#define CACHE_DEBUG

#ifdef CACHE_DEBUG
#define CLogMsg(...) SLogMsg(__VA_ARGS__)
#else
#define CLogMsg(...)
#endif

extern void KeTaskDone (void);

static SafeLock      s_driveLocks    [0x100];

// Caching interface
#ifdef ENABLE_CACHING
static CacheRegister s_cacheRegisters[0x100];//max driveID = 0xFF.

static int s_NumCacheRegisters;

UNUSED static CacheRegister *GetCacheRegister(DriveID driveID)
{
	if (!s_cacheRegisters[driveID].m_bUsed)
	{
		++s_NumCacheRegisters;
		
		StCacheInit(&s_cacheRegisters[driveID], driveID);
	}
	
	return &s_cacheRegisters[driveID];
}
#endif

// Resolving drive types
#if 1
static DriveType StGetDriveType(DriveID driveNum)
{
	if (                    driveNum <= 0x03) return DEVICE_IDE;
	if (driveNum >= 0x10 && driveNum <= 0x11) return DEVICE_FLOPPY;
	if (driveNum >= 0x20 && driveNum <= 0x5F) return DEVICE_AHCI;
	if (driveNum >= 0xF0)                     return DEVICE_RAMDISK;
	return DEVICE_UNKNOWN;
}

int MpGetNumAvailablePages(); // mm/pmm.c

unsigned StGetMaxCacheUnits()
{
	// a function of the amount of memory available.
	// We never want this to exceed 1/4 of the available memory. (why? Good question)
	// Since a cache unit stores a page's worth of data from the disk, this means that
	// 16384 (the old default) units would correspond to 64 MB (!!) of memory at most
	// used by the cache system. For each drive that has to cache stuff.
	// So our new method will basically take the amount of pages we have available
	// on the system, divide those by 4, and then by the amount of cache registers we have.
	
	int nCacheRegs = s_NumCacheRegisters;
	if (nCacheRegs == 0)
		nCacheRegs = 1; // Avoid a division by 0 error.
	
	return (unsigned int)MpGetNumAvailablePages() / 4 / nCacheRegs;
}

unsigned StGetMinCacheUnits()
{
	// The same as StGetMaxCacheUnits, but we have a maximum of 256, and we divide by 8.
	// So basically, it's MIN(MAX_CACHE_UNITS / 8, 256).
	
	int num = StGetMaxCacheUnits() / 8;
	if (num > 256)
		num = 256;
	
	return num;
}

//note: driveID is internal.
static DriveStatus StNoDriveRead(
	__attribute__((unused)) uint32_t lba, 
	__attribute__((unused)) void* pDest, 
	__attribute__((unused)) uint8_t driveID, 
	__attribute__((unused)) uint8_t nBlocks
)
{
	//does not do anything
	return DEVERR_NOTFOUND;
}
static DriveStatus StNoDriveWrite(
	__attribute__((unused)) uint32_t lba, 
	__attribute__((unused)) const void* pSrc, 
	__attribute__((unused)) uint8_t driveID, 
	__attribute__((unused)) uint8_t nBlocks
)
{
	//does not do anything
	return DEVERR_NOTFOUND;
}
static uint8_t StNoDriveGetSubID (__attribute__((unused)) DriveID did)
{
	return 0xFF;
}
static bool StNoDriveIsAvailable (__attribute__((unused)) uint8_t did)
{
	return false;
}
#endif

// Unused ram disks
#if 1
static RamDisk g_RAMDisks[RAMDISK_MAX];
bool StIsRamDiskMounted (int num)
{
	return(g_RAMDisks[num].m_bMounted);
}
bool StIsRamDiskReadOnly (int num)
{
	return(g_RAMDisks[num].m_bReadOnly);
}

static bool StRamDiskIsAvailable (uint8_t did)
{
	return StIsRamDiskMounted (did);
}
static DriveStatus StRamDiskWrite(uint32_t lba, const void* pSrc, uint8_t driveID, uint8_t nBlocks)
{
	if (!StIsRamDiskMounted(driveID))
		return DEVERR_NOTFOUND;
	if (!StIsRamDiskReadOnly(driveID))
		return DEVERR_NOWRITE;
	
	//get offset for a memcpy:
	int offset = lba * BLOCK_SIZE;
	
	//then write!
	fast_memcpy (&(g_RAMDisks[driveID].m_pDriveContents[offset]), pSrc, nBlocks * BLOCK_SIZE);
	return DEVERR_SUCCESS;
}
static DriveStatus StRamDiskRead(uint32_t lba, void* pDest, uint8_t driveID, uint8_t nBlocks)
{
	if (!StIsRamDiskMounted(driveID))
	{
		return DEVERR_NOTFOUND;
	}
	
	//get offset for a memcpy:
	int offset = lba * BLOCK_SIZE;
	
	//then read!
	fast_memcpy (pDest, &(g_RAMDisks[driveID].m_pDriveContents[offset]), nBlocks * BLOCK_SIZE);
	return DEVERR_SUCCESS;
}
static uint8_t StRamDiskGetSubID (DriveID did)
{
	return (int)did & 0xF;
}
static uint8_t StIdeGetSubID (DriveID did)
{
	return (int)did & 0x3;
}
static uint8_t StAhciGetSubID(DriveID did)
{
	return (int)did - 0x20;
}
#endif

// Abstracted out drive callbacks
#if 1

static DriveReadCallback g_ReadCallbacks[] = {
	StNoDriveRead, //Unknown
	StIdeDriveRead, //IDE
	StNoDriveRead, //Floppy - Scrapped
	StRamDiskRead, //RAM disk
	StAhciRead,    //AHCI
	StNoDriveRead, //Count
};
static DriveWriteCallback g_WriteCallbacks[] = {
	StNoDriveWrite, //Unknown
	StIdeDriveWrite, //IDE
	StNoDriveWrite, //Floppy
	StRamDiskWrite, //RAM disk
	StAhciWrite,    //AHCI
	StNoDriveWrite, //Count
};
static DriveGetSubIDCallback g_GetSubIDCallbacks[] = {
	StNoDriveGetSubID, //Unknown
	StIdeGetSubID,     //IDE
	StNoDriveGetSubID, //Floppy
	StRamDiskGetSubID, //RAM disk
	StAhciGetSubID,    //AHCI
	StNoDriveGetSubID, //Count
};
static DriveIsAvailableCallback g_IsAvailableCallbacks[] = {
	StNoDriveIsAvailable, //Unknown
	StIdeIsAvailable,     //IDE
	StNoDriveIsAvailable, //Floppy
	StRamDiskIsAvailable, //RAM disk
	StAhciIsAvailable,    //AHCI
	StNoDriveIsAvailable, //Count
};

DriveStatus StDeviceReadNoCache(uint32_t lba, void* pDest, DriveID driveId, uint8_t nBlocks)
{
	LockAcquire(&s_driveLocks[driveId]);
	
	DriveType driveType = StGetDriveType(driveId);
	if (driveType == DEVICE_UNKNOWN)
	{
		LockFree(&s_driveLocks[driveId]);
		return DEVERR_NOTFOUND;
	}
	
	uint8_t driveSubId = g_GetSubIDCallbacks[driveType](driveId);
	
	DriveStatus status = g_ReadCallbacks[driveType](lba, pDest, driveSubId, nBlocks);
	
	LockFree(&s_driveLocks[driveId]);
	
	return status;
}

DriveStatus StDeviceWriteNoCache(uint32_t lba, const void* pSrc, DriveID driveId, uint8_t nBlocks)
{
	LockAcquire(&s_driveLocks[driveId]);
	
	DriveType driveType = StGetDriveType(driveId);
	if (driveType == DEVICE_UNKNOWN)
	{
		LockFree(&s_driveLocks[driveId]);
		return DEVERR_NOTFOUND;
	}
	
	uint8_t driveSubId = g_GetSubIDCallbacks[driveType](driveId);
	
	DriveStatus status = g_WriteCallbacks[driveType](lba, pSrc, driveSubId, nBlocks);
	
	LockFree(&s_driveLocks[driveId]);
	
	return status;
}

bool StIsDriveAvailable (DriveID driveId)
{
	LockAcquire(&s_driveLocks[driveId]);
	
	DriveType driveType = StGetDriveType(driveId);
	if (driveType == DEVICE_UNKNOWN)
	{
		LockFree(&s_driveLocks[driveId]);
		return false;
	}
	
	uint8_t driveSubId = g_GetSubIDCallbacks[driveType](driveId);
	
	bool b = g_IsAvailableCallbacks[driveType](driveSubId);
	LockFree(&s_driveLocks[driveId]);
	
	return b;
}

DriveStatus StDeviceRead(uint32_t lba, void* pDest, DriveID driveId, uint8_t nBlocks)
{
	#ifdef ENABLE_CACHING
	uint8_t* pDestBytes = pDest;
	
	CacheRegister *pReg = &s_cacheRegisters[driveId];
	LockAcquire(&pReg->m_lock);
	
	if (!pReg->m_bUsed)
	{
		++s_NumCacheRegisters;
		
		StCacheInit(pReg, driveId);
	}
	
	DriveStatus ds = DEVERR_SUCCESS;
	
	// Ok, now perform the read itself.
	uint32_t lastLbaRead = ~0; CacheUnit *pUnit = NULL;
	for (uint32_t clba = lba, index = 0; index < nBlocks; clba++, index++)
	{
		if (lastLbaRead != (clba & ~7)  ||  !pUnit)
		{
			lastLbaRead  = (clba & ~7);
			pUnit = StLookUpCacheUnit(pReg, clba);
			if (!pUnit)
			{
				pUnit = StAddCacheUnit(pReg, clba, NULL, &ds);
			}
		}
		
		if (!pUnit) break;
		
		int blockNo = clba & 7;
		pUnit->m_lastAccess = GetTickCount();
		memcpy (pDestBytes + index * BLOCK_SIZE, pUnit->m_pData + blockNo * BLOCK_SIZE, BLOCK_SIZE);
	}
	
	LockFree(&pReg->m_lock);
	
	return ds;
	
	#else
	return StDeviceReadNoCache(lba, pDest, driveId, nBlocks);
	#endif
}

void StDumpInterestingThing()
{
	StDebugDump(&s_cacheRegisters[0]);
}
DriveStatus StDeviceWrite(uint32_t lba, const void* pSrc, DriveID driveId, uint8_t nBlocks)
{
	#ifdef ENABLE_CACHING
	uint8_t* pSrcBytes = (uint8_t*)pSrc;
	
	CacheRegister *pReg = &s_cacheRegisters[driveId];
	LockAcquire(&pReg->m_lock);
	
	if (!pReg->m_bUsed)
	{
		++s_NumCacheRegisters;
		
		StCacheInit(pReg, driveId);
	}
	
	DriveStatus ds = DEVERR_SUCCESS;
	
	// Ok, now perform the read itself.
	uint32_t lastLbaRead = ~0; CacheUnit *pUnit = NULL;
	for (uint32_t clba = lba, index = 0; index < nBlocks; clba++, index++)
	{
		if (lastLbaRead != (clba & ~7)  ||  !pUnit)
		{
			lastLbaRead  = (clba & ~7);
			pUnit = StLookUpCacheUnit(pReg, clba);
			if (!pUnit)
			{
				//CLogMsg("caching unit %d", lastLbaRead);
				pUnit = StAddCacheUnit(pReg, clba, NULL, &ds);
			}
		}
		
		if (!pUnit) break;
		
		int blockNo = clba & 7;
		pUnit->m_bModified  = true;
		pUnit->m_lastAccess = GetTickCount();
		memcpy (pUnit->m_pData + blockNo * BLOCK_SIZE, pSrcBytes + index * BLOCK_SIZE, BLOCK_SIZE);
	}
	
	LockFree(&pReg->m_lock);
	
	return ds;
	
	#else
	return StDeviceWriteNoCache(lba, pSrc, driveId, nBlocks);
	#endif
}

void StFlushAllCaches()
{
	for (int id = 0; id < 0x100; id++)
	{
		CacheRegister *pReg = &s_cacheRegisters[id];
		LockAcquire(&pReg->m_lock);
		if (pReg->m_bUsed)
			StFlushAllCacheUnits(pReg);
		LockFree(&pReg->m_lock);
	}
}

void StDebugDumpAll()
{
	LogMsg("Capacity min: %d max: %d", StGetMinCacheUnits(), StGetMaxCacheUnits());
	
	for (int id = 0; id < 0x100; id++)
	{
		CacheRegister *pReg = &s_cacheRegisters[id];
		LockAcquire(&pReg->m_lock);
		if (pReg->m_bUsed)
		{
			LogMsg("Info for drive ID %b", id);
			StDebugDump (pReg);
		}
		LockFree(&pReg->m_lock);
	}
}

#endif
