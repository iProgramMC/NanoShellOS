//  ***************************************************************
//  time.h - Creation date: 15/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#ifndef _TIME_H
#define _TIME_H

#define RTC_TICKS_PER_SECOND 8

#define C_UPDATE_IN_PROGRESS_FLAG 0x80
// (0x20 would represent 20, not 32)
#define BCD_TO_BIN(x) (((x) & 0x0F) + (((x) >> 4) * 10))

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

// for FormatTime.
enum
{
	FORMAT_TYPE_FIXED, //hh:mm:ss
	FORMAT_TYPE_VAR,   //H hours, M minutes, S seconds
};

/**
 * Gets the TSC from the CPU.  It returns the number of cycles the CPU went
 * through after the last reset, in "high" and "low".
 * The "high" or "low" pointers can be NULL, but if you pass in both as NULL, this does not use rdtsc.
 */
void GetTimeStampCounter(uint32_t* high, uint32_t* low);

/**
 * Gets the number of milliseconds since boot, assuming interrupts are disabled.
 */
int GetTickCountUnsafe();

/**
 * Gets the number of milliseconds since boot, assuming interrupts are enabled.
 */
int GetTickCount();

/**
 * Gets the number of times the RTC interrupt handler was called.
 */
int GetRawTickCount();

/**
 * Gets the number of microseconds since boot, assuming interrupts are disabled.
 */
uint64_t GetUsecCountUnsafe();

/**
 * Gets the number of microseconds since boot, assuming interrupts are enabled.
 */
uint64_t GetUsecCount();

/**
 * Gets the number of CPU ticks since boot.
 */
uint64_t ReadTSC();

/**
 * Waits a specified number of milliseconds.
 */
void WaitMS(int ms);

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

/**
 * Gets the current time in seconds since 1 January 1970.
 */
int GetEpochTime();

/**
 * Converts epoch time into human readable time (TimeStruct)
 */
void GetHumanTimeFromEpoch(int utime, TimeStruct* pOut);

#endif//_TIME_H