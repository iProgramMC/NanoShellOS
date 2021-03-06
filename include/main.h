/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

    NanoShell Main module header file
******************************************/
#ifndef _MAIN_H
#define _MAIN_H

#include<stddef.h>
#include<stdarg.h>
#include<stdint.h>
#include<stdbool.h>

//#define EXPERIMENTAL
#define EXPERIMENTAL_RSDPTR
#define EXPERIMENTAL_VMWARE

typedef char byte;
typedef char BYTE;
typedef unsigned uint;

#define USE_SSE_FXSAVE

#define PATH_MAX (260)

extern void KeTaskDone();

#define asm __asm__ volatile

#define hlt __asm__ volatile("hlt\n\t")
#define cli __asm__ volatile("cli\n\t")//do{__asm__("cli\n\t");SLogMsg("CLI request at " __FILE__ ":%d",__LINE__);}while(0)
#define sti __asm__ volatile("sti\n\t")//do{__asm__("sti\n\t");SLogMsg("STI request at " __FILE__ ":%d",__LINE__);}while(0)

#define VersionNumber 92
#define VersionString "V0.92"

#define UNUSED __attribute__((unused))

#define KERNEL_MEM_START 0xC0000000

//note: needs to be used for arrays only (So no pointers, otherwise that will be UB)
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(*array))

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#define UNUSED __attribute__((unused))
#define ALWAYS_INLINE __attribute__((always_inline))
#define NO_RETURN __attribute__((noreturn))

//SAI = Static and Always Inlined
#define SAI static ALWAYS_INLINE inline

bool OnAssertionFail (const char *pStr, const char *pFile, const char *pFunc, int nLine);
#define ASSERT(condition) ((condition) || OnAssertionFail(#condition, __FILE__, __FUNCTION__, __LINE__))

extern void WritePort(unsigned short port, unsigned char data);
extern unsigned char ReadPort(unsigned short port);
extern void WritePortW(unsigned short port, unsigned short data);
extern unsigned short ReadPortW(unsigned short port);
extern void WritePortL(unsigned short port, unsigned int data);
extern unsigned int ReadPortL(unsigned int port);

__attribute__((noreturn))
void KeStopSystem();
void KePrintSystemVersion();

#include <console.h>

#endif//_MAIN_H