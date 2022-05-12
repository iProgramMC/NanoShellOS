/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

  Storage Abstraction - ATA (PIO) Driver
******************************************/

#include <storabs.h>
#include <task.h>


#define CMD_READ  0x20
#define CMD_WRITE 0x30
#define CMD_GETID 0xEC

#define BSY_FLAG 0x80
#define DRQ_FLAG 0x08
#define ERR_FLAG 0x01

SafeLock g_ideDriveLocks[4];

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
}


//TODO: Instead of using port I/O to read drives (which is slow), use DMA instead.

DriveStatus StIdeDriveRead(uint32_t lba, void* pDest, uint8_t driveID, uint8_t nBlocks)
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
	// Lock this
	LockAcquire(&g_ideDriveLocks[driveID]);
	
	if (driveID % 2) driveType |= 0x10;
	
	if (!StIdeWaitBusy(base))
	{
		g_ideDriveAvailable[driveID] = false;
		LockFree(&g_ideDriveLocks[driveID]);
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
			LockFree(&g_ideDriveLocks[driveID]);
			return DEVERR_HARDWARE_ERROR;
		}
		
		for (int as = 0; as < 256; as++)
			*(pDestW++) = ReadPortW (base);
	}
	
	LockFree(&g_ideDriveLocks[driveID]);
	return DEVERR_SUCCESS;
}

DriveStatus StIdeDriveWrite(uint32_t lba, const void* pDest, uint8_t driveID, uint8_t nBlocks)
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
	
	LockAcquire(&g_ideDriveLocks[driveID]);
	if (!StIdeWaitBusy(base))
	{
		LogMsg("Before sending Write command: drive %d is not responding? Marking drive as unavailable!", driveID);
		g_ideDriveAvailable[driveID] = false;
		LockFree(&g_ideDriveLocks[driveID]);
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
			LockFree(&g_ideDriveLocks[driveID]);
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
		LockFree(&g_ideDriveLocks[driveID]);
		return DEVERR_HARDWARE_ERROR;
	}
	
	LockFree(&g_ideDriveLocks[driveID]);
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
	LockAcquire(&g_ideDriveLocks[driveID]);
	
	WritePort (base + 6, driveType);
	WritePort (base + 2, 0);
	WritePort (base + 3, 0);
	WritePort (base + 4, 0);
	WritePort (base + 5, 0);
	WritePort (base + 7, CMD_GETID);
	if (ReadPort(base + 7) == 0)
	{
		LockFree(&g_ideDriveLocks[driveID]);
		return DEVERR_NOTFOUND;
	}
	
	int timeout = 200;
	while (ReadPort(base + 7) & BSY_FLAG)
	{
		timeout--;
		if (timeout <= 0)
		{
			LogMsg("Trying to find drive %d, timeout reached, assuming not found", driveID);
			LockFree(&g_ideDriveLocks[driveID]);
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
			LockFree(&g_ideDriveLocks[driveID]);
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
		LockFree(&g_ideDriveLocks[driveID]);
		return DEVERR_SUCCESS;
	}
	LockFree(&g_ideDriveLocks[driveID]);
	return DEVERR_NOTFOUND;
}

void StIdeInitialize()
{
	//LogMsg("Probing for available IDE drives...");
	
	for (int i = 0; i < 4; i++)
	{
		AtaIdentifyData data;
		int error_code = StIdeSendIDCommand (i, &data);
		
		if (error_code != DEVERR_SUCCESS)
		{
			//LogMsg("Could not locate drive %d.  Assuming unavailable.  Error Code: %x", i, error_code);
		}
		else
		{
			//LogMsg("Drive %d found, marking as available.", i);
			g_ideDriveAvailable[i] = true;
			
			/*LogMsg("Info about drive %d", i);
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
			LogMsg(" * CheckSum:            %d", data.CheckSum);*/
		}
	}
	
	//LogMsg("Probing done.");
}

bool StIdeIsAvailable (uint8_t did)
{
	return g_ideDriveAvailable[did];
}

