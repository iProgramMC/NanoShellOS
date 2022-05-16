/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Misc - timing module header file
******************************************/
#ifndef _MISC_H
#define _MISC_H

#define RTC_TICKS_PER_SECOND 8

#define C_UPDATE_IN_PROGRESS_FLAG  0X80

// (0x20 would represent 20, not 32)
#define BCD_TO_BIN(x) (x&0x0F)+((x>>4)*10)

typedef struct
{
	int seconds,
		minutes,
		hours,
		weekday,
		day,
		month,
		year,
		statusA,
		statusB;
}
TimeStruct;

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
void GetTimeStampCounter(uint32_t* high, uint32_t* low);

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
 * Waits a specified number of milliseconds.
 */
void WaitMS(int ms);

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

enum
{
	FORMAT_TYPE_FIXED, //hh:mm:ss
	FORMAT_TYPE_VAR,   //H hours, M minutes, S seconds
};

/**
 * Formats time as seconds into a string.  Recommend a size of at least 128.
 */
void FormatTime(char* output, int formatType, int seconds);

/**
 * Reads the time into a TimeStruct.  Use this in the RTC timer tick.
 */
void TmGetTime (TimeStruct* pStruct);

/**
 * Gets the current time.
 */
TimeStruct* TmReadTime();

#endif//_MISC_H