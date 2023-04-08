/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

	Storage Abstraction Caching module
******************************************/
#include <storabs.h>
#include <memory.h>
#include <misc.h>

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

// Initialize the caching system.
void StCacheInit(CacheRegister* pReg, DriveID driveID)
{
	ASSERT (!pReg->m_bUsed);
	pReg->m_bUsed              = true;
	pReg->m_pCacheUnits        = MmAllocateK(sizeof(CacheUnit) * StGetMinCacheUnits());
	pReg->m_nCacheUnitCapacity = StGetMinCacheUnits();
	pReg->m_nCacheUnitCount    = 0;
	pReg->m_driveID            = driveID;
}

void StDebugDump(CacheRegister* pReg)
{
	LogMsg("Cache unit count: %d, capacity: %d. Elements:", pReg->m_nCacheUnitCount, pReg->m_nCacheUnitCapacity);
	for (size_t i = 0; i < pReg->m_nCacheUnitCount; i++)
	{
		LogMsgNoCr("{%d:%d:%d} ", pReg->m_pCacheUnits[i].m_lba, pReg->m_pCacheUnits[i].m_bModified, pReg->m_pCacheUnits[i].m_lastAccess);
	}
	LogMsg("");
}

// evicts several units from (unit + 0, unit + 1, ... unit + count - 1)
void StEvictSeveralCacheUnits(CacheRegister* pReg, uint32_t unit, uint32_t count)
{
	if (pReg->m_nCacheUnitCount == 0)
		return;
	
	ASSERT(unit < pReg->m_nCacheUnitCount);
	ASSERT(unit + count <= pReg->m_nCacheUnitCount);

	if (count != 1)
		CLogMsg("Evicting cache units %d to %d",unit, unit+count-1);
	else
		CLogMsg("Evicting cache unit %d", unit);
	
	// free the data first
	for (size_t i = 0; i < count; i++)
	{
		// If it has been modified at all, perform a write.
		CacheUnit *pUnit = &pReg->m_pCacheUnits[unit + i];
		
		if (pUnit->m_bModified)
		{
			CLogMsg("Flushing cache unit with lba %d to disk %b", pUnit->m_lba, pReg->m_driveID);
			DriveStatus d = StDeviceWriteNoCache (pUnit->m_lba, pUnit->m_pData, pReg->m_driveID, PAGE_SIZE / BLOCK_SIZE);
			if (d != DEVERR_SUCCESS)
			{
				SLogMsg("I/O operation failed on this disk. This is bad!");
				ASSERT(!"Huh?");
			}
		}
		
		MmFreeK(pUnit->m_pData);
		pUnit->m_pData = NULL;
	}

	// then move all the other entries over it
	memmove(&pReg->m_pCacheUnits[unit], &pReg->m_pCacheUnits[unit + count], sizeof(CacheUnit) * (pReg->m_nCacheUnitCount - unit - count));

	// and decrease the count itself
	pReg->m_nCacheUnitCount -= count;
}

void StEvictCacheUnit(CacheRegister* pReg, uint32_t unit)
{
	StEvictSeveralCacheUnits(pReg, unit, 1);
}

void StEvictLeastUsedCacheUnit(CacheRegister* pReg)
{
	uint32_t earliestTickCount = ~0, unit = -1;

	for (size_t i = 0; i < pReg->m_nCacheUnitCount; i++)
	{
		if (earliestTickCount > pReg->m_pCacheUnits[i].m_lastAccess)
			earliestTickCount = pReg->m_pCacheUnits[i].m_lastAccess, unit = i;
	}

	if ((int)unit < 0) return;
	StEvictCacheUnit(pReg, unit);
}

void StExpandCacheUnitStorage(CacheRegister* pReg, size_t newSize)
{
	ASSERT(newSize >= pReg->m_nCacheUnitCapacity);
	ASSERT(newSize < StGetMaxCacheUnits());

	pReg->m_pCacheUnits = MmReAllocateK(pReg->m_pCacheUnits, newSize * sizeof(CacheUnit));

	// initialize the data following this
	memset(&pReg->m_pCacheUnits[pReg->m_nCacheUnitCapacity], 0, sizeof(CacheUnit) * (newSize - pReg->m_nCacheUnitCapacity));

	// update the capacity
	pReg->m_nCacheUnitCapacity = newSize;
}

void StShrinkCacheUnitStorage(CacheRegister* pReg, size_t newSize)
{
	ASSERT(newSize < pReg->m_nCacheUnitCapacity);

	pReg->m_pCacheUnits = MmReAllocateK(pReg->m_pCacheUnits, newSize * sizeof(CacheUnit));
	pReg->m_nCacheUnitCapacity = newSize;
}
//unsafe version, ordering must be ensured!
CacheUnit* StAddCacheUnitAt(CacheRegister* pReg, uint32_t lba, int where, void *pData)
{
	if ((size_t)where < pReg->m_nCacheUnitCount)
	{
		memmove(&pReg->m_pCacheUnits[where + 1], &pReg->m_pCacheUnits[where], sizeof(CacheUnit) * (pReg->m_nCacheUnitCount - where));
	}

	CacheUnit* pUnit = &pReg->m_pCacheUnits[where];

	pUnit->m_lba = lba;
	pUnit->m_bModified  = false;
	pUnit->m_dataPhys   = 0;
	pUnit->m_pData      = MmAllocateSinglePage();
	pUnit->m_lastAccess = GetTickCount();
	
	// Perform I/O to get the data there.
	if (pData == NULL)
	{
		StDeviceReadNoCache(pUnit->m_lba, pUnit->m_pData, pReg->m_driveID, PAGE_SIZE / BLOCK_SIZE);
	}
	else
	{
		memcpy (pUnit->m_pData, pData, PAGE_SIZE);
	}

	pReg->m_nCacheUnitCount++;
	
	return pUnit;
}

CacheUnit* StGetCacheUnit(CacheRegister* pReg, int unitNo)
{
	if (unitNo < 0) return NULL;
	
	size_t unit = (size_t)unitNo;
	if (unit >= pReg->m_nCacheUnitCount) return NULL;

	return &pReg->m_pCacheUnits[unit];
}

int StLookUpCacheUnit(CacheRegister* pReg, uint32_t lba)
{
	// The LBA should be divisible by 8. This means we need to chop off the last 3 bits.
	lba &= ~7;

	// Perform a binary search.
	int where = 0;
	int left = 0, right = pReg->m_nCacheUnitCount;
	while (left < right)
	{
		where = left + (right - left) / 2;

		if (pReg->m_pCacheUnits[where].m_lba < lba) //before this there are ONLY elements that have smaller LBA
			left = where + 1;
		else
			right = where;
	}

	where = right;

	// Last check if this cache unit actually matches.
	CacheUnit* pUnit = StGetCacheUnit(pReg, where);
	if (pUnit && pUnit->m_lba == lba)
		return where;

	// Not found, return nothing.
	return -1;
}

CacheUnit* StAddCacheUnit(CacheRegister* pReg, uint32_t lba, void *pData)
{
	// The LBA should be divisible by 8. This means we need to chop off the last 3 bits.
	lba &= ~7;

	// Do we need to expand the cache unit storage?
	ASSERT(pReg->m_nCacheUnitCount <= pReg->m_nCacheUnitCapacity);

	if (pReg->m_nCacheUnitCount == pReg->m_nCacheUnitCapacity)
	{
		// _should_ we expand it?
		if (pReg->m_nCacheUnitCapacity + StGetMinCacheUnits() >= StGetMaxCacheUnits())
		{
			// no, try evicting some less used cache units
			// TODO: optimize, right now it's basically O(n^2).. get better man
			for (int i = 0; i < EVICT_WHEN_REACHING_MAX; i++)
				StEvictLeastUsedCacheUnit(pReg);
		}
		else
		{
			StExpandCacheUnitStorage(pReg, pReg->m_nCacheUnitCapacity + StGetMinCacheUnits());
		}
	}

	// Determine where we need to add the cache unit using a binary search
	int where = 0;
	int left = 0, right = pReg->m_nCacheUnitCount;
	while (left < right)
	{
		where = left + (right - left) / 2;

		if (pReg->m_pCacheUnits[where].m_lba < lba) //before this there are ONLY elements that have smaller LBA
			left = where + 1;
		else
			right = where;
	}

	where = right;

	return StAddCacheUnitAt(pReg, lba, where, pData);

	//StDebugDump(pReg);
}

void StFlushAllCacheUnits(CacheRegister* pReg)
{
	if (!pReg->m_bUsed) return;
	CLogMsg("Evicting ALL cache units!");
	StEvictSeveralCacheUnits (pReg, 0, pReg->m_nCacheUnitCount);
}

