/*************************************************************
                  NanoShell Operating System
		            (C) 2022 iProgramInCpp
        Limine (C) 2019-2022 mintsuki and contributors

                   Limine installer module
*************************************************************/

/**
 * Abstract:
 * This module implements the Limine installer, as can be found at:
 * https://github.com/limine-bootloader/limine/tree/trunk/limine-install
 */
#include <main.h>
#include <memory.h>
#include <vfs.h>

#ifdef EXPERIMENTAL

// TODO
#if 0

uint8_t* g_pLimineHdd;

void LimineInstallTeardown ()
{
	if (g_pLimineHdd)
		MmFree(g_pLimineHdd);
	g_pLimineHdd = NULL;
}

bool LimineInstallLoadData ()
{
	int fd = FiOpen ("/limine-hdd.bin", O_RDONLY);
	
	if (fd < 0)
	{
		LogMsg("Cannot install limine: file not found in initrd.");
		return false;
	}
	
	return true;
}

void LimineNoMbrError()
{
	LogMsg("ERROR: Could not determine if the device has a valid partition table.  Please ensure the device has a valid MBR.");
}

#define NoMbrError return LimineNoMbrError

bool DeviceRead (void* _buffer, uint32_t loc, size_t count)
{
	uint8_t *pBuffer = _buffer;
	uint32_t progress = 0;
	while (progress < count)
	{
		uint32_t block = (loc + progress) / g_BlockSize;
		
		if (!DeviceCacheBlock(block))
		{
			LogMsg("ERROR: Read error.");
			return;
		}
		
		uint32_t chunk = count - progress;
		uint32_t offset = (loc + progress) % g_BlockSize;
		if (chunk > g_BlockSize - offset)
			chunk = g_BlockSize - offset;
		
		memcpy (pBuffer + progress, &cache[offset], chunk);
		progress += chunk;
	}
	return true;
}


//The passed in argument represents the drive number we should pass in to the
//storage abstraction module.
void LimineInstallMain(int arg)
{
	// Note: Unlike the real limine-install, this does not support GPT drives.
	// Only good old-fashioned MBR.
	
	
	// Before we move on: make sure the user really wants to install limine to this drive:
	LogMsg("Are you sure you want to install Limine on drive %d?  You could lose data if this goes wrong. (y/N)", arg);
	
	char c = CoGetChar();
	if (c != 'y')
	{
		LogMsg("Quitting");
		return;
	}
	
	// Perform sanity checks on mbr
	uint16_t hint = 0;
	DeviceRead (&hint, 218, sizeof (uint16_t));
	
	if (hint != 0)
	{
		NoMbrError();
	}
	
	DeviceRead (&hint, 444, sizeof (uint16_t));
	
	if (hint != 0x5A5A)
	{
		NoMbrError();
	}
	
	DeviceRead (&hint, 510, sizeof (uint16_t));
	
	if (hint != 0x55AA)
	{
		NoMbrError();
	}
	
	// Perform sanity checks on partition table
	DeviceRead (&hint, 446, sizeof (uint8_t));
	if ((uint8_t)hint != 0x00 && (uint8_t)hint != 0x80)
		NoMbrError();
	DeviceRead (&hint, 462, sizeof (uint8_t));
	if ((uint8_t)hint != 0x00 && (uint8_t)hint != 0x80)
		NoMbrError();
	DeviceRead (&hint, 478, sizeof (uint8_t));
	if ((uint8_t)hint != 0x00 && (uint8_t)hint != 0x80)
		NoMbrError();
	DeviceRead (&hint, 494, sizeof (uint8_t));
	if ((uint8_t)hint != 0x00 && (uint8_t)hint != 0x80)
		NoMbrError();
	
	// Check if there are any file systems on here.
	char hintc [64];
	
	DeviceRead (hintc, 4, 8);
	if (memcmp (hintc, "_ECH_FS_", 8) == 0)
		NoMbrError();
	
	DeviceRead (hintc, 54, 3);
	if (memcmp (hintc, "FAT", 3) == 0)
		NoMbrError();
}

#endif

#endif