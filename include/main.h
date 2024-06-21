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

#define VersionNumber 102
#define VersionString "V1.02"

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

void StopwatchStart();
int  StopwatchEnd();

// LogMsg - Log a message to the current console. Must have interrupts enabled.
void LogMsg (const char* fmt, ...);
void LogMsgNoCr (const char* fmt, ...);

// LogMsg - Log a message to the screen. Can have interrupts disabled.
void ILogMsg (const char* fmt, ...);
void ILogMsgNoCr (const char* fmt, ...);

// SLogMsg - Log a message to the debug console (0xE9 port). Can have interrupts disabled.
void SLogMsg (const char* fmt, ...);
void SLogMsgNoCr (const char* fmt, ...);

STATIC_ASSERT(sizeof(long) == sizeof(uintptr_t), "Size of long and uintptr_t must match");

#define CONTAINING_RECORD(Pointer, Type, Field) ((Type*)((uintptr_t)(Pointer) - (uintptr_t)offsetof(Type, Field)))

#endif//_MAIN_H