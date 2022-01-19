/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Misc - timing module header file
******************************************/
#ifndef _MISC_H
#define _MISC_H

#define RTC_TICKS_PER_SECOND 256

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
 * Gets the TSC from the CPU.  It returns the number of cycles the CPU went
 * through after the last reset, in "high" and "low".
 * The "high" or "low" pointers can be NULL, but if you pass in both as NULL, this does not use rdtsc.
 */
void GetTimeStampCounter(int* high, int* low);

/**
 * Gets the number of milliseconds since boot.
 */
int GetTickCount();

/**
 * Gets the number of times the RTC interrupt handler was called.
 */
int GetRawTickCount();
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
const char* GetCPUType();

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

enum
{
	FORMAT_TYPE_FIXED, //hh:mm:ss
	FORMAT_TYPE_VAR,   //H hours, M minutes, S seconds
};

/**
 * Formats time as seconds into a string.  Recommend a size of at least 128.
 */
void FormatTime(char* output, int formatType, int seconds);

#endif//_MISC_H