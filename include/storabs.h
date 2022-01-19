/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

        Storage Abstraction module
******************************************/
#ifndef _STORABS_H
#define _STORABS_H

#include <main.h>
#include <string.h>

// Enums
enum {
	DEVICE_UNKNOWN,
	DEVICE_IDE,
	DEVICE_FLOPPY,
	DEVICE_RAMDISK,
	DEVICE_COUNT,
};
enum {
	DEVERR_SUCCESS,
	DEVERR_NOTFOUND = 0x2000,
	DEVERR_NOWRITE,
	DEVERR_HARDWARE_ERROR,
};

#define RAMDISK_MAX 16
#define SECTOR_SIZE 512

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
DriveStatus StDeviceWrite(uint32_t lba, const void* pSrc,  DriveID driveId, uint8_t nBlocks);

/**
 * Reads into pDest from a storage device.
 */
DriveStatus StDeviceRead (uint32_t lba,       void* pDest, DriveID driveId, uint8_t nBlocks);

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
void StIdeInitialize(void);


#endif//_STORABS_H