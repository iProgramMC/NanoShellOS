// nanoshell/time_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _TIME_TYPES_H
#define _TIME_TYPES_H

#include <sys/types.h>

typedef long time_t;

struct timespec
{
	time_t tv_sec;
	long   tv_nsec;
};

struct timeval
{
	time_t tv_sec;
	long   tv_usec;
};

struct tm
{
	int tm_sec;           // Seconds after the minute. [0, 59]
	int tm_min;           // Minutes after the hour.   [0, 59]	
	int tm_hour;          // Hours since midnight.     [0, 23]
	int tm_mday;          // Day of the month.         [1, 31]
	int tm_mon;           // Months since January.     [0, 11]
	int tm_year;          // Years since 1900. (so, 2022 would be represented as 122)
	int tm_wday;          // Days since Sunday.        [0, 6]. Sunday = 0.
	int tm_yday;          // Days since January 1st.   [0, 365] (Day 365 = December 31st, on a leap year)
	int tm_isdst;         // Daylight savings time flag.
	long int tm_gmtoff;   // GNU extension: The offset from GMT in seconds.
	const char *tm_zone;  // GNU extension: The name of the time zone.
};

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

struct timezone {
    int tz_minuteswest; // minutes west of Greenwich
    int tz_dsttime;     // type of DST correction
};

#endif//_TIME_TYPES_H
