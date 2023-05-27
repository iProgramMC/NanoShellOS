/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

        Storage Abstraction module
******************************************/
#ifndef _STORABS_H
#define _STORABS_H

#include <main.h>
#include <string.h>
#include <ht.h>
#include <pci.h>
#include <lock.h>

// Enums
enum {
	DEVICE_UNKNOWN,
	DEVICE_IDE,
	DEVICE_FLOPPY,
	DEVICE_RAMDISK,
	DEVICE_AHCI,
	DEVICE_COUNT,
};
enum {
	DEVERR_SUCCESS,
	DEVERR_NOTFOUND = 0x2000,
	DEVERR_NOWRITE,
	DEVERR_HARDWARE_ERROR,
};

#define RAMDISK_MAX 16
#define BLOCK_SIZE 512

// Type definitions
typedef unsigned int  DriveType;
typedef unsigned int  DriveStatus;
typedef unsigned char DriveID;

typedef DriveStatus(*DriveReadCallback)       (uint32_t lba,       void* pDest, uint8_t driveID, uint8_t numBlocks);
typedef DriveStatus(*DriveWriteCallback)      (uint32_t lba, const void* pSrc,  uint8_t driveID, uint8_t numBlocks);
typedef bool       (*DriveIsAvailableCallback)(uint8_t driveID);
typedef uint8_t    (*DriveGetSubIDCallback)   (DriveID driveId);


typedef struct
{
	bool     m_bMounted;
	bool     m_bReadOnly;
	uint32_t m_CapacityBlocks;
	uint8_t* m_pDriveContents;
}
RamDisk;

/**
 * Writes to a storage device.
 */
DriveStatus StDeviceWriteNoCache(uint32_t lba, const void* pSrc,  DriveID driveId, uint8_t nBlocks);
DriveStatus StDeviceWrite       (uint32_t lba, const void* pSrc,  DriveID driveId, uint8_t nBlocks);

/**
 * Reads into pDest from a storage device.
 */
DriveStatus StDeviceReadNoCache (uint32_t lba,       void* pDest, DriveID driveId, uint8_t nBlocks);
DriveStatus StDeviceRead        (uint32_t lba,       void* pDest, DriveID driveId, uint8_t nBlocks);

/**
 * Checks if a drive ID is available or not.
 */
bool StIsDriveAvailable (DriveID driveId);

/**
 * Mounts a testing RAM disk and returns its ID.
 */
DriveID StMountTestRamDisk (void);

/**
 * Initializes IDE driver.
 */
void StIdeInit(void);

// IDE
#if 1

DriveStatus StIdeDriveRead(uint32_t lba, void* pDest, uint8_t driveID, uint8_t nBlocks);
DriveStatus StIdeDriveWrite(uint32_t lba, const void* pDest, uint8_t driveID, uint8_t nBlocks);
bool StIdeIsAvailable (uint8_t did);

#endif

// AHCI
#if 1

#include <ahci.h>

DriveStatus StAhciRead(uint32_t lba, void* pDest, uint8_t driveID, uint8_t nBlocks);
DriveStatus StAhciWrite(uint32_t lba, const void* pDest, uint8_t driveID, uint8_t nBlocks);
bool StAhciIsAvailable (uint8_t did);


#endif

// Caching
#if 1

// A cache unit can store 8 sectors' worth of information (4096 sectors)

struct CacheRegister;

typedef struct
{
	struct CacheRegister* m_pParentReg;
	uint32_t m_lba;        //the LBA of the 8 sectors on disk (LBA + 0 -> LBA + 7)
	uint32_t m_lastAccess; //GetTickCount() of the last access (read/write).
	bool     m_bModified;  //if this section was ever written to during its lifetime

	uint8_t* m_pData;      //the place where the data itself is located (1 page)
	uint32_t m_dataPhys;   //in case it's useful to someone, here's the physical address of the data.
}
CacheUnit;

typedef struct CacheRegister
{
	bool       m_bUsed;
	DriveID    m_driveID;
	HashTable* m_CacheHashTable;
	SafeLock   m_lock;
}
CacheRegister;

void StCacheInit              (CacheRegister* pReg, DriveID driveID);
void StDebugDump              (CacheRegister* pReg);
void StEvictLeastUsedCacheUnit(CacheRegister* pReg);
CacheUnit* StLookUpCacheUnit  (CacheRegister* pReg, uint32_t lba);
CacheUnit* StAddCacheUnit     (CacheRegister* pReg, uint32_t lba, void *pData /* = NULL */);
void StFlushAllCacheUnits     (CacheRegister* pReg);

#endif

void StFlushAllCaches();
void StDebugDumpAll();

#endif//_STORABS_H