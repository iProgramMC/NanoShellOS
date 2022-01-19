/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

        Storage Abstraction module
******************************************/

/**
 * A short description of what this module is responsible for:
 *
 * This module is responisble for parsing away drive numbers, and using
 * the drivers meant for them.
 *
 * The mapping of the drive numbers is as follows:
 *
 *    - 0x00 - 0x03: IDE drives
 *    - 0x10 - 0x11: Floppy drives (won't add because they don't support FAT32)
 *    - 0xF0 - 0xFF: RAM disk drives 
 *    - 0xE0 - 0xEF: USB flash drives (?)
 *    - 0xAA       : Special reserved flag. This drive ID is always empty.
 *    - 0x20 - 0x2F: AHCI drives (?)
 */
 
#include <storabs.h>

extern void KeTaskDone (void);

// Resolving drive types
#if 1
static DriveType StGetDriveType(DriveID driveNum)
{
	if (                    driveNum <= 0x03) return DEVICE_IDE;
	if (driveNum >= 0x10 && driveNum <= 0x11) return DEVICE_FLOPPY;
	if (driveNum >= 0xF0)                     return DEVICE_RAMDISK;
	return DEVICE_UNKNOWN;
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
	int offset = lba * SECTOR_SIZE;
	
	//then write!
	fast_memcpy (&(g_RAMDisks[driveID].m_pDriveContents[offset]), pSrc, nBlocks * SECTOR_SIZE);
	return DEVERR_SUCCESS;
}
static DriveStatus StRamDiskRead(uint32_t lba, void* pDest, uint8_t driveID, uint8_t nBlocks)
{
	if (!StIsRamDiskMounted(driveID))
	{
		return DEVERR_NOTFOUND;
	}
	
	//get offset for a memcpy:
	int offset = lba * SECTOR_SIZE;
	
	//then read!
	fast_memcpy (pDest, &(g_RAMDisks[driveID].m_pDriveContents[offset]), nBlocks * SECTOR_SIZE);
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
#endif

// ATA driver
#if 1

#define CMD_READ  0x20
#define CMD_WRITE 0x30
#define CMD_GETID 0xEC

#define BSY_FLAG 0x80
#define DRQ_FLAG 0x08
#define ERR_FLAG 0x01

bool g_ideDriveAvailable[] = { false, false, false, false };//the 4 IDE drives

static bool StIdeWaitBusy (uint16_t base)
{
	int countWait = 0;
	while (ReadPort (base + 7) & BSY_FLAG)
	{
		countWait++;
		if (countWait >= 1000000)
		{
			return false;
		}
		//KeTaskDone();
	}
	return true;
}/*
static bool StIdeWaitBusyDrq (uint16_t base)
{
	int countWait = 0;
	while (ReadPort (base + 7) & (BSY_FLAG | DRQ_FLAG))
	{
		countWait++;
		if (countWait >= 2000000)
		{
			return false;
		}
		//KeTaskDone();
	}
	return true;
}
*/
static DriveStatus StIdeDriveRead(uint32_t lba, void* pDest, uint8_t driveID, uint8_t nBlocks)
{
	if (g_ideDriveAvailable[driveID] == false)
		return DEVERR_NOTFOUND;
	
	uint16_t base, driveType = 0xE0;
	switch (driveID)
	{
		case 0:
		case 1: base = 0x01F0; break;
		case 2:
		case 3: base = 0x0170; break;
		default: return DEVERR_HARDWARE_ERROR;
	}
	if (driveID % 2) driveType |= 0x10;
	
	if (!StIdeWaitBusy(base))
	{
		g_ideDriveAvailable[driveID] = false;
		return DEVERR_HARDWARE_ERROR;
	}
	
	//LBA: 28 bits
	WritePort (base+6, (uint8_t)((lba & 0x0F000000)>>24) | driveType);
	WritePort (base+1, 0x00);
	WritePort (base+2, nBlocks);
	WritePort (base+3, (uint8_t)((lba & 0x000000FF)));
	WritePort (base+4, (uint8_t)((lba & 0x0000FF00)>>8));
	WritePort (base+5, (uint8_t)((lba & 0x00FF0000)>>16));
	WritePort (base+7, CMD_READ);
	
	uint16_t* pDestW = (uint16_t*)pDest;
	
	for (int i = 0; i < nBlocks; i++)
	{
		if (!StIdeWaitBusy(base))
		{
			LogMsg("Drive %d is not responding? Marking drive as unavailable!", driveID);
			g_ideDriveAvailable[driveID] = false;
			return DEVERR_HARDWARE_ERROR;
		}
		
		for (int as = 0; as < 256; as++)
			*(pDestW++) = ReadPortW (base);
	}
	
	return DEVERR_SUCCESS;
}

static DriveStatus StIdeDriveWrite(uint32_t lba, const void* pDest, uint8_t driveID, uint8_t nBlocks)
{
	if (g_ideDriveAvailable[driveID] == false)
		return DEVERR_NOTFOUND;
	
	uint16_t base, driveType = 0xE0;
	switch (driveID)
	{
		case 0:
		case 1: base = 0x01F0; break;
		case 2:
		case 3: base = 0x0170; break;
		default: return DEVERR_HARDWARE_ERROR;
	}
	if (driveID % 2) driveType |= 0x10;
	
	if (!StIdeWaitBusy(base))
	{
		LogMsg("Before sending Write command: drive %d is not responding? Marking drive as unavailable!", driveID);
		g_ideDriveAvailable[driveID] = false;
		return DEVERR_HARDWARE_ERROR;
	}
	
	//LBA: 28 bits
	WritePort (base+6, (uint8_t)((lba & 0x0F000000)>>8) | driveType);
	WritePort (base+2, nBlocks);
	WritePort (base+3, (uint8_t)((lba & 0x000000FF)));
	WritePort (base+4, (uint8_t)((lba & 0x0000FF00)>>8));
	WritePort (base+5, (uint8_t)((lba & 0x00FF0000)>>8));
	WritePort (base+7, CMD_WRITE);
	
	uint16_t* pDestW = (uint16_t*)pDest;
	
	for (int i = 0; i < nBlocks; i++)
	{
		if (!StIdeWaitBusy(base))
		{
			LogMsg("Drive %d is not responding? Marking drive as unavailable!", driveID);
			g_ideDriveAvailable[driveID] = false;
			return DEVERR_HARDWARE_ERROR;
		}
		
		for (int as = 0; as < 256; as++)
		{
			WritePortW(base, *(pDestW++));
			WritePort (0x80, 0x00); //Delay
		}
	}
	// Flush cache
	WritePort (base + 7, 0xE7);
	
	if (!StIdeWaitBusy(base))
	{
		LogMsg("FLUSH CACHE: Drive %d is not responding? Marking drive as unavailable!", driveID);
		g_ideDriveAvailable[driveID] = false;
		return DEVERR_HARDWARE_ERROR;
	}
	
	return DEVERR_SUCCESS;
}

typedef struct AtaIdentifyData
{
	uint16_t DataStructRev;
	uint16_t MultiwordDmaSupport;
	uint16_t UltraDmaSupport;
	uint64_t MaxLba;
	uint16_t FeatureSet;
	uint16_t SataFeatureSet;
	uint16_t SataReserved;
	uint16_t Reserved[10];
	uint16_t FeatureSet2;
	uint16_t FeatureSet3;
	uint16_t Reserved2[233];
	uint8_t IntegrityByte;
	uint8_t CheckSum;
}
__attribute__((packed))
AtaIdentifyData;

static DriveStatus StIdeSendIDCommand(uint8_t driveID, AtaIdentifyData* pData)
{
	uint16_t base, driveType = 0xE0;
	switch (driveID)
	{
		case 0:
		case 1: base = 0x01F0; break;
		case 2:
		case 3: base = 0x0170; break;
		default: return DEVERR_HARDWARE_ERROR;
	}
	if (driveID % 2) driveType |= 0x10;
	
	WritePort (base + 6, driveType);
	WritePort (base + 2, 0);
	WritePort (base + 3, 0);
	WritePort (base + 4, 0);
	WritePort (base + 5, 0);
	WritePort (base + 7, CMD_GETID);
	if (ReadPort(base + 7) == 0)
		return DEVERR_NOTFOUND;
	
	int timeout = 200;
	while (ReadPort(base + 7) & BSY_FLAG)
	{
		timeout--;
		if (timeout <= 0)
		{
			LogMsg("Trying to find drive %d, timeout reached, assuming not found");
			return DEVERR_NOTFOUND;
		}
		//KeTaskDone();
	}
	
	uint8_t state = 0x00;
	timeout = 20000;
	do
	{
		state = ReadPort (base + 7);
		timeout --;
		if (timeout <= 0)
		{
			LogMsg("Trying to find drive %d, timeout reached, assuming not found");
			return DEVERR_NOTFOUND;
		}
	}
	while ((state & (DRQ_FLAG | ERR_FLAG)) == 0);
	
	if (!(state & ERR_FLAG))
	{
		// Read Data
		uint16_t* ids = (uint16_t*)pData;
		for (int i = 0; i < 256; i++)
		{
			*ids++ = ReadPortW(base);
		}
		return DEVERR_SUCCESS;
	}
	return DEVERR_NOTFOUND;
}

void StIdeInitialize()
{
	LogMsg("Probing for available IDE drives...");
	
	for (int i = 0; i < 4; i++)
	{
		AtaIdentifyData data;
		int error_code = StIdeSendIDCommand (i, &data);
		
		if (error_code != DEVERR_SUCCESS)
		{
			LogMsg("Could not locate drive %d.  Assuming unavailable.  Error Code: %x", i, error_code);
		}
		else
		{
			LogMsg("Drive %d found, marking as available.", i);
			g_ideDriveAvailable[i] = true;
			
			LogMsg("Info about drive %d", i);
			LogMsg(" * DataStructRev:       %d", data.DataStructRev);
			LogMsg(" * MultiwordDmaSupport: %d", data.MultiwordDmaSupport);
			LogMsg(" * UltraDmaSupport:     %d", data.UltraDmaSupport);
			LogMsg(" * MaxLba:              %d", data.MaxLba);
			LogMsg(" * FeatureSet:          %d", data.FeatureSet);
			LogMsg(" * SataFeatureSet:      %d", data.SataFeatureSet);
			LogMsg(" * SataReserved:        %d", data.SataReserved);
			LogMsg(" * FeatureSet2:         %d", data.FeatureSet2);
			LogMsg(" * FeatureSet3:         %d", data.FeatureSet3);
			LogMsg(" * IntegrityByte:       %d", data.IntegrityByte);
			LogMsg(" * CheckSum:            %d", data.CheckSum);
		}
	}
	
	LogMsg("Probing done.");
}

static bool StIdeIsAvailable (uint8_t did)
{
	return g_ideDriveAvailable[did];
}

#endif

// Abstracted out drive callbacks
#if 1

static DriveReadCallback g_ReadCallbacks[] = {
	StNoDriveRead, //Unknown
	StIdeDriveRead, //IDE -- TODO!
	StNoDriveRead, //Floppy -- TODO!
	StRamDiskRead, //RAM disk,
	StNoDriveRead, //Count
};
static DriveWriteCallback g_WriteCallbacks[] = {
	StNoDriveWrite, //Unknown
	StIdeDriveWrite, //IDE -- TODO!
	StNoDriveWrite, //Floppy -- TODO!
	StRamDiskWrite, //RAM disk,
	StNoDriveWrite, //Count
};
static DriveGetSubIDCallback g_GetSubIDCallbacks[] = {
	StNoDriveGetSubID, //Unknown
	StIdeGetSubID,     //IDE -- TODO!
	StNoDriveGetSubID, //Floppy -- TODO!
	StRamDiskGetSubID, //RAM disk,
	StNoDriveGetSubID, //Count
};
static DriveIsAvailableCallback g_IsAvailableCallbacks[] = {
	StNoDriveIsAvailable, //Unknown
	StIdeIsAvailable,     //IDE -- TODO!
	StNoDriveIsAvailable, //Floppy -- TODO!
	StRamDiskIsAvailable, //RAM disk,
	StNoDriveIsAvailable, //Count
};

DriveStatus StDeviceRead(uint32_t lba, void* pDest, DriveID driveId, uint8_t nBlocks)
{
	DriveType driveType = StGetDriveType(driveId);
	if (driveType == DEVICE_UNKNOWN)
		return DEVERR_NOTFOUND;
	
	uint8_t driveSubId = g_GetSubIDCallbacks[driveType](driveId);
	
	return g_ReadCallbacks[driveType](lba, pDest, driveSubId, nBlocks);
}
DriveStatus StDeviceWrite(uint32_t lba, const void* pSrc, DriveID driveId, uint8_t nBlocks)
{
	DriveType driveType = StGetDriveType(driveId);
	if (driveType == DEVICE_UNKNOWN)
		return DEVERR_NOTFOUND;
	
	uint8_t driveSubId = g_GetSubIDCallbacks[driveType](driveId);
	
	return g_WriteCallbacks[driveType](lba, pSrc, driveSubId, nBlocks);
}
bool StIsDriveAvailable (DriveID driveId)
{
	DriveType driveType = StGetDriveType(driveId);
	if (driveType == DEVICE_UNKNOWN)
		return false;
	
	uint8_t driveSubId = g_GetSubIDCallbacks[driveType](driveId);
	
	return g_IsAvailableCallbacks[driveType](driveSubId);
}
#endif
