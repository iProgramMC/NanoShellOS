//  ***************************************************************
//  time.c - Creation date: 15/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <multiboot.h>
#include <main.h>
#include <time.h>
#include <string.h>
#include <print.h>
#include <idt.h>

#define CURRENT_YEAR 2022

TimeStruct g_time;
int gEarliestTickCount;
int g_nRtcTicks = 0;
int g_nEpochTime = 0;
bool g_bRtcInitialized = false;
int  g_nSeconds;
bool g_gotTime = false;
uint64_t g_tscOneSecondAgo, g_tscTwoSecondsAgo;
bool g_trustRtcUpdateFinishFlag;

void MonitorSystem();

TimeStruct* TmReadTime()
{
	return &g_time;
}

int TmCmosReadRegister(int reg)
{
	WritePort(0x70,reg);
	return ReadPort(0x71);
}

void TmCmosWriteRegister(int reg, int data)
{
	WritePort(0x70,reg);
	WritePort(0x71,data);
}

// I don't remember exactly where I copied this from
int TmGetWeekDay(TimeStruct* pStruct)
{
	int day = pStruct->day, month = pStruct->month, year = pStruct->year;
	
    int wday = (day  + ((153 * (month + 12 * ((14 - month) / 12) - 3) + 2) / 5)
				+ (365 * (year + 4800 - ((14 - month) / 12)))
				+ ((year + 4800 - ((14 - month) / 12)) / 4)
				- ((year + 4800 - ((14 - month) / 12)) / 100)
				+ ((year + 4800 - ((14 - month) / 12)) / 400)
				- 32045
      ) % 7;
	
    return (wday + 1) % 7; // so that Sunday is day 0
}

void TmGetTime (TimeStruct* pStruct)
{
	do 
	{
		pStruct->statusA = TmCmosReadRegister(0x0A);
	} 
	while (pStruct->statusA & C_UPDATE_IN_PROGRESS_FLAG);
	
	pStruct->seconds = TmCmosReadRegister(0x00);
	pStruct->minutes = TmCmosReadRegister(0x02);
	pStruct->hours   = TmCmosReadRegister(0x04);
	pStruct->day     = TmCmosReadRegister(0x07);
	pStruct->month   = TmCmosReadRegister(0x08);
	pStruct->year    = TmCmosReadRegister(0x09);
	TimeStruct placeholder;
	do 
	{
		placeholder.seconds = pStruct->seconds; 
		placeholder.minutes = pStruct->minutes; 
		placeholder.hours   = pStruct->hours  ; 
		placeholder.day     = pStruct->day    ; 
		placeholder.month   = pStruct->month  ; 
		placeholder.year    = pStruct->year   ;
		
		do 
		{
			pStruct->statusA = TmCmosReadRegister(0x0A);
		} 
		while (pStruct->statusA & C_UPDATE_IN_PROGRESS_FLAG);
		
		pStruct->seconds = TmCmosReadRegister(0x00);
		pStruct->minutes = TmCmosReadRegister(0x02);
		pStruct->hours   = TmCmosReadRegister(0x04);
		pStruct->day     = TmCmosReadRegister(0x07);
		pStruct->month   = TmCmosReadRegister(0x08);
		pStruct->year    = TmCmosReadRegister(0x09);
		
	}
	while (!(
		placeholder.seconds == pStruct->seconds &&
		placeholder.minutes == pStruct->minutes &&
		placeholder.hours   == pStruct->hours   &&
		placeholder.day     == pStruct->day     &&
		placeholder.month   == pStruct->month   &&
		placeholder.year    == pStruct->year
	));
	
	pStruct->statusB = TmCmosReadRegister(0x0B);
	if (!(pStruct->statusB & 0x04))//BCD mode
	{
		pStruct->seconds = BCD_TO_BIN(pStruct->seconds);
		pStruct->minutes = BCD_TO_BIN(pStruct->minutes);
		pStruct->day     = BCD_TO_BIN(pStruct->day);
		pStruct->month   = BCD_TO_BIN(pStruct->month);
		pStruct->year    = BCD_TO_BIN(pStruct->year);
		pStruct->hours   = ((pStruct->hours & 0xF) + (((pStruct->hours & 0x70) / 16) * 10)) | (pStruct->hours & 0x80);
	}
	
	// convert 12h to 24h
	if (!(pStruct->statusB & 0x02) && (pStruct->hours & 0x80))
		pStruct->hours = ((pStruct->hours & 0x7f) + 12) % 24;
	
	// calculate full year
	pStruct->year += 2000;
	if (pStruct->year < CURRENT_YEAR)
		pStruct->year += 100;
	
	// calculate the weekday parameter. The RTC's weekday may not be entirely reliable.
	pStruct->weekday = TmGetWeekDay(pStruct);
}

const char* g_monthNamesShort = 
	"Non\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0";

void TmPrintTime(TimeStruct* pStruct)
{
	char hr[3],mn[3],sc[3];
	hr[0] = '0' + pStruct->hours/10;
	hr[1] = '0' + pStruct->hours%10;
	hr[2] = 0;
	mn[0] = '0' + pStruct->minutes/10;
	mn[1] = '0' + pStruct->minutes%10;
	mn[2] = 0;
	sc[0] = '0' + pStruct->seconds/10;
	sc[1] = '0' + pStruct->seconds%10;
	sc[2] = 0;
	
	LogMsg("Current time: %d %s %d  %s:%s:%s", 
		pStruct->day, &g_monthNamesShort[4*pStruct->month],
		pStruct->year,
		hr,mn,sc);
}

void TmPrintTimeFormatted(char* buffer, TimeStruct* pStruct)
{
	char hr[3],mn[3],sc[3];
	hr[0] = '0' + pStruct->hours/10;
	hr[1] = '0' + pStruct->hours%10;
	hr[2] = 0;
	mn[0] = '0' + pStruct->minutes/10;
	mn[1] = '0' + pStruct->minutes%10;
	mn[2] = 0;
	sc[0] = '0' + pStruct->seconds/10;
	sc[1] = '0' + pStruct->seconds%10;
	sc[2] = 0;
	
	sprintf(
		buffer,
		"Current time: %d %s %d  %s:%s:%s", 
		pStruct->day, &g_monthNamesShort[4*pStruct->month],
		pStruct->year,
		hr,mn,sc);
}

void GetTimeStampCounter(uint32_t* high, uint32_t* low)
{
	if (!high && !low) return; //! What's the point?
	int edx, eax;
	__asm__ volatile ("rdtsc":"=a"(eax),"=d"(edx));
	if (high) *high = edx;
	if (low ) *low  = eax;
}

uint64_t ReadTSC()
{
	uint32_t hi, lo;
	GetTimeStampCounter(&hi, &lo);
	return ((uint64_t)hi << 32LL) | (uint64_t)(lo);
}

//not accurate at all, use GetTickCount() instead.
int GetRtcBasedTickCount()
{
	return g_nRtcTicks * 1000 / RTC_TICKS_PER_SECOND;
}

int GetRawTickCount()
{
	return g_nRtcTicks;
}

// This function tries to approximate unix time the best that it can.
// This does not take into account leap seconds. Despite that, it matches the time
// functions on my system (used gmtime, matches exactly with a call to time(NULL))
static int s_daysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int CalculateEpochTime()
{
	TimeStruct ts = *TmReadTime();
	int year = ts.year - 1970;
	int days = 365 * year + (ts.day - 1);
	
	// (almost) every 4 years since 1972 are leap years. Count them in
	// This should work again. (year - 2 + 4) / 4 = (year - 2) / 4 + 1.
	int leapYears = ((year - 1) + 2) / 4;
	
	days += leapYears;
	
	// based on the month, determine the day
	for (int month = 1; month < ts.month; month++)
	{
		days += s_daysPerMonth[month];
		if (month == 2 && ts.year % 4 == 0) // February -- leap year
			days++;
	}
	
	return days * 86400 + ts.hours * 3600 + ts.minutes * 60 + ts.seconds;
}

int GetEpochTime()
{
	return g_nEpochTime;
}

void GetHumanTimeFromEpoch(int utime, TimeStruct* pOut)
{
	// Separate the amount of days from the rest of the time.
	int days = utime / 86400, secs = utime % 86400;

	// Turn the secs into hours, minutes and seconds. This is trivial.
	pOut->hours = secs / 3600;
	pOut->minutes = secs / 60 % 60;
	pOut->seconds = secs % 60;

	// Get the year number.
	int year = 1970;
	while (days >= 365 + (year % 4 == 0))
	{
		days -= 365 + (year % 4 == 0);
		year++;
	}

	// Get the month number.
	int month = 1;
	while (days >= s_daysPerMonth[month])
	{
		days -= s_daysPerMonth[month];
		if (month == 2 && year % 4 == 0) days--;
		month++;
	}

	pOut->day   = days + 1;
	pOut->month = month;
	pOut->year  = year;
}

uint64_t gEarliestUsecCount;

uint64_t GetUsecCountUnsafe()
{
	uint64_t tscNow     = ReadTSC();
	uint64_t difference = tscNow - g_tscOneSecondAgo;             // the difference
	uint64_t tscPerSec  = g_tscOneSecondAgo - g_tscTwoSecondsAgo; // a full complete second has passed
	uint64_t nSeconds = g_nSeconds;
	
	//SLogMsg("TSC per second: %q   Difference: %q   Tsc Now: %q", tscPerSec, difference, tscNow);
	
	uint64_t us_count = 0;
	
	if (tscPerSec)
		us_count = difference * 1000000 / tscPerSec;// I hope this works :)
	
	uint64_t newUsecCount = nSeconds * 1000000ULL + us_count;
	if (gEarliestUsecCount < newUsecCount)
		gEarliestUsecCount = newUsecCount;
	return gEarliestUsecCount;
}

// Only call this if you have interrupts disabled!!!
int GetTickCountUnsafe()
{
	uint64_t tscNow     = ReadTSC();
	uint64_t difference = tscNow - g_tscOneSecondAgo;             // the difference
	uint64_t tscPerSec  = g_tscOneSecondAgo - g_tscTwoSecondsAgo; // a full complete second has passed
	int nSeconds = g_nSeconds;
	
	//SLogMsg("TSC per second: %q   Difference: %q   Tsc Now: %q", tscPerSec, difference, tscNow);
	
	int ms_count = 0;
	
	if (tscPerSec)
		ms_count = difference * 1000 / tscPerSec;
	
	int newTickCount = nSeconds * 1000 + ms_count;
	if (gEarliestTickCount < newTickCount)
		gEarliestTickCount = newTickCount;
	return gEarliestTickCount;
}

uint64_t GetUsecCount()
{
	if (!g_bRtcInitialized)
		return 0;
	
	KeVerifyInterruptsEnabled;
	cli;
	uint64_t tc = GetUsecCountUnsafe();
	sti;
	return tc;
}

int GetTickCount()
{
	if (!g_bRtcInitialized)
		return 0;
	
	KeVerifyInterruptsEnabled;
	cli;
	int tc = GetTickCountUnsafe();
	sti;
	
	return tc;
}

//note: recommend an output buffer of at least 50 chars
void FormatTime(char* output, int formatType, int seconds)
{
	switch (formatType)
	{
		case FORMAT_TYPE_FIXED: {
			//sprintf(output, "SECONDS: %05d", seconds);
			int sec = seconds % 60;
			int min = seconds / 60 % 60;
			int hrs = seconds / 3600;
			sprintf(output, "%02d:%02d:%02d", hrs, min, sec);
			break;
		}
		case FORMAT_TYPE_VAR: {
			int sec = seconds % 60;
			int min = seconds / 60 % 60;
			int hrs = seconds / 3600;
			
			char buf[50];
			if (hrs)
			{
				sprintf(buf, "%d hour%s", hrs, hrs == 1 ? "" : "s");
				strcat (output, buf);
			}
			if (min)
			{
				if (hrs)
					sprintf(buf, ", %d min%s", min, min == 1 ? "" : "s");
				else
					sprintf(buf,   "%d min%s", min, min == 1 ? "" : "s");
				strcat (output, buf);
			}
			if (sec || !seconds)
			{
				if (min)
					sprintf(buf, ", %d sec%s", sec, sec == 1 ? "" : "s");
				else
					sprintf(buf,   "%d sec%s", sec, sec == 1 ? "" : "s");
				strcat (output, buf);
			}
			
			break;
		}
	}
}

void KiTimingWait()
{
	TmGetTime(TmReadTime());
	g_nEpochTime = CalculateEpochTime();
	
	// wait 1 second for system to initialize, so that we can wait and stuff
	while (GetTickCount() < 1000)
	{
		asm("hlt":::"memory");
	}
}

void TimerInterruptHandler()
{
	//also read register C, may be useful later:
	WritePort(0x70, 0x0C);
	
	UNUSED char flags = ReadPort(0x71);
	
	g_nRtcTicks++;
	if (g_nRtcTicks % RTC_TICKS_PER_SECOND == 0)
	{
		g_nSeconds++;
		g_tscTwoSecondsAgo = g_tscOneSecondAgo;
		g_tscOneSecondAgo  = ReadTSC();
		//SLogMsg("TSC difference: %Q", g_tscOneSecondAgo - g_tscTwoSecondsAgo);
		
		MonitorSystem();
	}
	
	// 1 real second passed. Don't count on it, as some emulators don't send this flag
	// VirtualBox comes to mind...
	if (flags & (1 << 4))
	{
		g_trustRtcUpdateFinishFlag = true; // yeah, trust me from now on
		
		TmGetTime(TmReadTime());
		g_nEpochTime = CalculateEpochTime();
	}
	//(temporarily) load time like so, until (and if) a flag UPDATE_FINISHED interrupt comes
	if (!g_trustRtcUpdateFinishFlag)
	{
		//1 second passed!
		if (g_nRtcTicks % RTC_TICKS_PER_SECOND == 0)
		{
			TmGetTime(TmReadTime());
			
			g_nEpochTime = CalculateEpochTime();
		}
	}
}

/**
 * RTC initialization routine.
 */
void KeClockInit()
{
	WritePort(0x70, 0x8A);
	WritePort(0x71, 0x20);
	
	WritePort(0x70, 0x8B);
	char flags = ReadPort(0x71);
	WritePort(0x70, 0x8B);
	WritePort(0x71, flags | 0x40);
	
	//32768>>(14-1) = 4 hz.
	//I'll enable this in order to get second-periodic interrupts
	int divisionRate = 13;
	WritePort(0x70, 0x8A);
	flags = ReadPort(0x71);
	WritePort(0x70, 0x8A);
	WritePort(0x71, (flags & 0xF0) | divisionRate);
	
	// hack: https://forum.osdev.org/viewtopic.php?f=1&t=30091
	WritePort(0x70, 0x0C);
	ReadPort(0x71);
	//sti;
	
	KeRegisterIrqHandler(IRQ_CLOCK, TimerInterruptHandler, true);
}