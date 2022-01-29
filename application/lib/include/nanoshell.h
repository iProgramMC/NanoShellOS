#ifndef _NANOSHELL_H
#define _NANOSHELL_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

typedef uint8_t BYTE;
typedef uint8_t bool;
#define false 0
#define true 1

#define ARRAY_COUNT(array) (sizeof(array)/sizeof(*array))

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

//basic memoryoperations:
int memcmp(const void* ap, const void* bp, size_t size);
void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size);
void fmemcpy32 (void* restrict dest, const void* restrict src, size_t size);
void* memmove(void* restrict dstptr, const void* restrict srcptr, size_t size);
void* memset(void* bufptr, BYTE val, size_t size);
void* fast_memset(void* bufptr, BYTE val, size_t size);
int atoi(const char* str);
void memtolower(char* as, int w);
void memtoupper(char* as, int w);
void ZeroMemory (void* bufptr1, size_t size);
//basic string operations:
size_t strgetlento(const char* str, char chr);
size_t strlen(const char* str);
void* strcpy(const char* ds, const char* ss);
void strtolower(char* as);
void strtoupper(char* as);
int strcmp(const char* as, const char* bs);
void strcat(char* dest, char* after);

//our own functions that call into the system
void PutString(const char* text);
void LogMsg(const char* fmt, ...);
void LogMsgNoCr(const char* fmt, ...);
void *Allocate(int size);
void Free (void* ptr);
void MmDebugDump();
char ReadChar();
void ReadString(char* pOutBuffer, int MaxSize);
const char* GetCpuType();
const char* GetCpuName();
void* GetConsole();//TODO
TimeStruct* GetTime();
int GetTickCount();//time since startup

#endif//_NANOSHELL_H