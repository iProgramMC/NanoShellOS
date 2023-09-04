//  ***************************************************************
//  a_video.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

static THREAD_LOCAL struct tm s_tm;

void sleep(int ms)
{
	TmSleep(ms);
}

// This function tries to approximate unix time the best that it can.
// This does not take into account leap seconds. Despite that, it matches the time
// functions on my system (used gmtime, matches exactly with a call to time(NULL))
static int s_daysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int GetEpochTimeFromTimeStruct(TimeStruct* ts)
{
	int year = ts->year - 1970;
	int days = 365 * year + 1;
	
	// (almost) every 4 years since 1972 are leap years. Count them in
	// This should work again. (year - 2 + 4) / 4 = (year - 2) / 4 + 1.
	int leapYears = (year + 2) / 4;
	days += leapYears;
	
	// based on the month, determine the day
	for (int month = 1; month < ts->month; month++)
	{
		days += s_daysPerMonth[month];
		if (month == 2 && ts->year % 4 == 0) // February -- leap year
			days++;
	}
	
	return days * 86400 + ts->hours * 3600 + ts->minutes * 60 + ts->seconds;
}

int GetEpochTime()
{
	return GetEpochTimeFromTimeStruct(GetTime());
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
		month++;
	}

	pOut->day   = days + 1;
	pOut->month = month;
	pOut->year  = year;
}

void StructTmToTimeStruct(TimeStruct* out, struct tm* in)
{
	out->seconds  = in->tm_sec;
	out->minutes  = in->tm_min;
	out->hours    = in->tm_hour;
	out->weekday  = in->tm_wday;
	out->day      = in->tm_mday;
	out->month    = in->tm_mon;
	out->year     = in->tm_year;
	out->statusA = out->statusB = 0;
}

void TimeStructToStructTm(TimeStruct* in, struct tm* out)
{
	out->tm_sec  = in->seconds; 
	out->tm_min  = in->minutes; 
	out->tm_hour = in->hours; 
	out->tm_wday = in->weekday; 
	out->tm_mday = in->day; 
	out->tm_mon  = in->month; 
	out->tm_year = in->year;
	out->tm_gmtoff = 0;
	out->tm_zone = "GMT";
}

double difftime(time_t a, time_t b)
{
	return a - b;
}

time_t time(time_t* timer)
{
	time_t t = GetEpochTimeFromTimeStruct(GetTime());
	
	if (timer)
		*timer = t;
	
	return t;
}

time_t mktime (struct tm * ptr)
{
	TimeStruct ts;
	StructTmToTimeStruct(&ts, ptr);
	return GetEpochTimeFromTimeStruct(&ts);
}

struct tm* localtime_r(const time_t* timep, struct tm * result)
{
	TimeStruct ts;
	GetHumanTimeFromEpoch(*timep, &ts);
	TimeStructToStructTm(&ts, result);
	return result;
}

struct tm* localtime(const time_t* timep)
{
	return localtime_r(timep, &s_tm);
}

int gettimeofday(struct timeval * tv, struct timezone * tz)
{
	if (tz)
	{
		// no time zone
		tz->tz_minuteswest = 0;
		tz->tz_dsttime     = 0;
	}
	
	if (tv)
	{
		tv->tv_sec  = (int)time(NULL);
		tv->tv_usec = (GetTickCount() % 1000) * 1000;
	}
	
	return 0;
}

int settimeofday(UNUSED struct timeval * tv, UNUSED struct timezone * tz)
{
	// TODO: implement this
	SetErrorNumber(-EACCES);
	return -1;
}
