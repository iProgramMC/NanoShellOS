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
//#define EXPERIMENTAL_RSDPTR
#define EXPERIMENTAL_VMWARE

typedef char byte;
typedef char BYTE;
typedef unsigned uint;

#define USE_SSE_FXSAVE

#define PATH_MAX (260)

extern void KeTaskDone();

#define asm __asm__ volatile

#define hlt asm("hlt")

/*
void KeDisableInterruptsD(const char * file, int line);
void KeEnableInterruptsD(const char * file, int line);
#define cli KeDisableInterruptsD(__FILE__, __LINE__)  //asm("cli")
#define sti KeEnableInterruptsD (__FILE__, __LINE__)  //asm("sti")
*/

void KeDisableInterrupts();
void KeEnableInterrupts();
#define cli KeDisableInterrupts()  //asm("cli")
#define sti KeEnableInterrupts ()  //asm("sti")

// December 4, 2022. This marks Version 1.00 of the operating system,
// simply because I decided to make it 1.00 now. :-)
#define VersionNumber 101
#define VersionString "V1.01"

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

// The function that gets called when an assertion fails.
bool OnAssertionFail (const char *pStr, const char *pFile, const char *pFunc, int nLine);
#define ASSERT(condition) ((condition) || OnAssertionFail(#condition, __FILE__, __FUNCTION__, __LINE__))

extern void WritePort(unsigned short port, unsigned char data);
extern unsigned char ReadPort(unsigned short port);
extern void WritePortW(unsigned short port, unsigned short data);
extern unsigned short ReadPortW(unsigned short port);
extern void WritePortL(unsigned short port, unsigned int data);
extern unsigned int ReadPortL(unsigned int port);

// Stops the system immediately.
__attribute__((noreturn))
void KeStopSystem();

void KePrintSystemVersion();

// Gets the contents of the EFLAGS register.
uint32_t KeGetEFlags(void);

// Checks if interrupts are disabled right now.
bool KeCheckInterruptsDisabled(void);

// Asserts if interrupts are disabled.
void KeVerifyInterruptsDisabledD(const char * file, int line);
#define KeVerifyInterruptsDisabled KeVerifyInterruptsDisabledD(__FILE__, __LINE__)

// Asserts if interrupts are enabled.
void KeVerifyInterruptsEnabledD(const char * file, int line);
#define KeVerifyInterruptsEnabled KeVerifyInterruptsEnabledD(__FILE__, __LINE__)

#include <console.h>

void StopwatchStart();
int  StopwatchEnd();

#endif//_MAIN_H