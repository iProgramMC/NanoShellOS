// time.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _TIME___H
#define _TIME___H

#include <nanoshell/time_types.h>

double difftime(time_t a, time_t b);
time_t time(time_t* timer);
time_t mktime (struct tm * ptr);
struct tm* localtime_r(const time_t* timep, struct tm * result);
struct tm* localtime(const time_t* timep);
int gettimeofday(struct timeval * tv, struct timezone * tz);
int settimeofday(struct timeval * tv, struct timezone * tz);

// NanoShell specifics
TimeStruct* GetTime();
void StructTmToTimeStruct(TimeStruct* out, struct tm* in);
void TimeStructToStructTm(TimeStruct* in, struct tm* out);
int GetEpochTimeFromTimeStruct(TimeStruct* ts);
int GetEpochTime();
void GetHumanTimeFromEpoch(int utime, TimeStruct* pOut);

#endif//_TIME___H
