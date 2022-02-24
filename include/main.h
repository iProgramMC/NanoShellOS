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

typedef char bool;
typedef char byte;
typedef char BYTE;
typedef unsigned uint;

extern void KeTaskDone();

#define false 0
#define true 1

#define asm __asm__ volatile

#define hlt __asm__ volatile("hlt\n\t")
#define cli __asm__ volatile("cli\n\t")//do{__asm__("cli\n\t");SLogMsg("CLI request at " __FILE__ ":%d",__LINE__);}while(0)
#define sti __asm__ volatile("sti\n\t")//do{__asm__("sti\n\t");SLogMsg("STI request at " __FILE__ ":%d",__LINE__);}while(0)

#define VersionNumber 20
#define VersionString "V0.20"

#define UNUSED __attribute__((unused))

#define crash __asm__ volatile("int $0x10\n\t") // Int 0x10 doesn't work in pmode! Might as well make use of it.

#define KERNEL_MEM_START 0xC0000000

//note: needs to be used for SIZED arrays only
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(*array))

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#define UNUSED __attribute__((unused))

#ifdef MULTITASKED_WINDOW_MANAGER

#define ACQUIRE_LOCK(lock_var) do {\
	while (lock_var) \
		KeTaskDone(); \
	lock_var = 1;\
} while (0)

#define FREE_LOCK(lock_var) do {\
	lock_var = 0;\
} while (0);

#else
	
#define ACQUIRE_LOCK(var) do {} while(0)
#define FREE_LOCK(var) do {} while(0)

#endif

extern void WritePort(unsigned short port, unsigned char data);
extern unsigned char ReadPort(unsigned short port);
extern void WritePortW(unsigned short port, unsigned short data);
extern unsigned short ReadPortW(unsigned short port);

__attribute__((noreturn))
void KeStopSystem();
void KePrintSystemVersion();

#include <console.h>

#endif//_MAIN_H