/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Misc - timing module header file
******************************************/
#ifndef _MISC_H
#define _MISC_H

#include <multiboot.h>

typedef struct
{
	unsigned char m_steppingID : 4;
	unsigned char m_model : 4;
	unsigned char m_familyID : 4;
	unsigned char m_processorType : 2;
	unsigned char m_reserved : 2;
	unsigned char m_extModelID: 4;
	unsigned char m_extendedFamilyID;
	unsigned char m_reserved1 : 4;
}
__attribute__((packed))
CPUIDFeatureBits;


/**
 * Returns a random number between 0 and 2147483647.
 */
int GetRandom();

/**
 * Gets the CPU type string. (e.g. "GenuineIntel", "AuthenticAMD" etc)
 *
 * Note:  MUST call KeCPUID().  KiStartupSystem already does that though.
 */
const char* GetCPUType();

/**
 * Gets the CPU name string.
 *
 * Note:  MUST call KeCPUID().  KiStartupSystem already does that though.
 */
const char* GetCPUName();

/**
 * Gets the CPU feature bits.
 *
 * Note:  MUST call KeCPUID().  KiStartupSystem already does that though.
 */
CPUIDFeatureBits GetCPUFeatureBits();

/**
 * Prints memory ranges in the system.
 */
void KePrintMemoryMapInfo();

/**
 * Prints info about the system.
 */
void KePrintSystemInfo();

/**
 * Prints more info about the system.
 */
void KePrintSystemInfoAdvanced();

/**
 * Triggers a restart of the system.
 */
__attribute__((noreturn))
void KeRestartSystem(void);

/**
 * Stops all kernel activity forcefully. Only use this if something
 * terrible happened and we need to shutdown immediately.
 */
NO_RETURN void KeStopSystem();

/**
 * Performs the cache flushing operation before shutting down the computer.
 */
void KeOnShutDownSaveData();

/**
 * Get the multiboot_info_t passed into KiStartupSystem.
 */
multiboot_info_t* KiGetMultibootInfo();

#endif//_MISC_H