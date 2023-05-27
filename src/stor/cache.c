/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

	Storage Abstraction Caching module
******************************************/
#include <storabs.h>
#include <memory.h>
#include <time.h>

#define CACHE_DEBUG

#ifdef CACHE_DEBUG
#define CLogMsg(...) SLogMsg(__VA_ARGS__)
#else
#define CLogMsg(...)
#endif

// well, these are in storabs.c
unsigned StGetMinCacheUnits();
unsigned StGetMaxCacheUnits();

#define EVICT_WHEN_REACHING_MAX (4096) // Evict 4096 cache units when we're about to reach the max.

static uint32_t StCacheHash(const void* ptr)
{
	return (uint32_t)ptr;
}

static bool StCacheCompare(const void* key1, const void* key2)
{
	return key1 == key2;
}

static void StCacheOnErase(UNUSED const void* key, void* pDataVoid)
{
	CacheUnit* pUnit = pDataVoid;
	
	CacheRegister* pReg = pUnit->m_pParentReg;
	
	// If this has been modified, make sure to flush the data.
	if (pUnit->m_bModified)
	{
		DriveStatus d = StDeviceWriteNoCache(pUnit->m_lba, pUnit->m_pData, pReg->m_driveID, PAGE_SIZE / BLOCK_SIZE);
		if (d != DEVERR_SUCCESS)
		{
			SLogMsg("Delayed I/O write operation failed on drive %d.", pReg->m_driveID);
			ASSERT(!"Huh?");
		}
	}
	
	// Free the internal data pointer.
	MmFree(pUnit->m_pData);
	pUnit->m_pData = NULL;
}

// Initialize the caching system.
void StCacheInit(CacheRegister* pReg, DriveID driveID)
{
	ASSERT (!pReg->m_bUsed);
	pReg->m_bUsed          = true;
	pReg->m_CacheHashTable = HtCreate(StCacheHash, StCacheCompare, StCacheOnErase);
	pReg->m_driveID        = driveID;
}

static eHTForEachOp StCachePrint(const void* key, void* pDataVoid, UNUSED void* ctx)
{
	CacheUnit* pUnit = pDataVoid;
	
	SLogMsg("%d:{%d:%d:%d}", (int)key, pUnit->m_lba, pUnit->m_bModified, pUnit->m_lastAccess);
	
	return FOR_EACH_NO_OP;
}

void StDebugDump(CacheRegister* pReg)
{
	LogMsg("Dumping cache register for drive ID %d to debug console", pReg->m_driveID);
	
	HtForEach(pReg->m_CacheHashTable, StCachePrint, NULL);
}

CacheUnit* StLookUpCacheUnit(CacheRegister* pReg, uint32_t lba)
{
	// The LBA should be divisible by 8. This means we need to chop off the last 3 bits.
	lba &= ~7;

	return HtLookUp(pReg->m_CacheHashTable, (void*)lba);
}

CacheUnit* StAddCacheUnit(CacheRegister* pReg, uint32_t lba, void *pData)
{
	// The LBA should be divisible by 8. This means we need to chop off the last 3 bits.
	lba &= ~7;
	
	CacheUnit* pUnit = MmAllocate(sizeof *pUnit);
	if (!pUnit)
		return NULL;

	if (!HtSet(pReg->m_CacheHashTable, (void*)lba, pUnit))
	{
		MmFree(pUnit);
		return NULL;
	}
	
	// set it up:
	pUnit->m_lba        = lba;
	pUnit->m_lastAccess = GetTickCount();
	pUnit->m_bModified  = false;
	pUnit->m_pParentReg = pReg;
	
	// load the data in:
	pUnit->m_pData = MmAllocatePhy(4096, &pUnit->m_dataPhys);
	if (!pUnit->m_pData)
	{
		MmFree(pUnit);
		return NULL;
	}
	
	if (!pData)
	{
		DriveStatus d = StDeviceReadNoCache(pUnit->m_lba, pUnit->m_pData, pReg->m_driveID, PAGE_SIZE / BLOCK_SIZE);
		if (d != DEVERR_SUCCESS)
		{
			SLogMsg("I/O read operation failed on drive %d. This is bad!", pReg->m_driveID);
			ASSERT(!"Huh?");
		}
	}
	else
	{
		memcpy(pUnit->m_pData, pData, PAGE_SIZE);
	}
	
	return pUnit;
}

static eHTForEachOp StCacheFindLastUsedCacheUnit(UNUSED const void* key, void* pDataVoid, void* pCtx)
{
	CacheUnit** pLastUsedCacheUnit = pCtx;
	CacheUnit*  pCurrentUnit = pDataVoid;
	
	if (!*pLastUsedCacheUnit || (*pLastUsedCacheUnit)->m_lastAccess > pCurrentUnit->m_lastAccess)
	{
		*pLastUsedCacheUnit = pCurrentUnit;
	}
	
	return FOR_EACH_NO_OP;
}

static eHTForEachOp StCacheEraseLastUsedCacheUnit(UNUSED const void* key, void* pDataVoid, void* pCtx)
{
	CacheUnit* pCurrentUnit = pDataVoid;
	
	if (pCurrentUnit == pCtx)
		return FOR_EACH_ERASE;
	
	return FOR_EACH_NO_OP;
}

void StEvictLeastUsedCacheUnits(CacheRegister* pReg)
{
	CacheUnit* pLastUsedCacheUnit = NULL;
	
	HtForEach(pReg->m_CacheHashTable, StCacheFindLastUsedCacheUnit, &pLastUsedCacheUnit);
	
	HtForEach(pReg->m_CacheHashTable, StCacheEraseLastUsedCacheUnit, pLastUsedCacheUnit);
}

// erase everything indiscriminately!!
static eHTForEachOp StCacheFlushAllCacheUnits(UNUSED const void* key, UNUSED void* data, UNUSED void* ctx)
{
	return FOR_EACH_ERASE;
}

void StFlushAllCacheUnits(CacheRegister* pReg)
{
	if (!pReg->m_bUsed) return;
	
	HtForEach(pReg->m_CacheHashTable, StCacheFlushAllCacheUnits, NULL);
}

