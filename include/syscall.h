/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

        Syscall module header file
******************************************/
#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <main.h>

/**
 * USAGE FOR THIS STRUCT:
 * When you get an interrupt (which pushes eflags, cs, and eip to the stack),
 * IMMEDIATELY push some error code (because our fault handlers need it, and
 * I want to make this as generic as possible). Then push ESP, and call the C
 * handler. (it'll be defined like `void someHandler(Registers* pRegs);`)
 *
 * Then when we return from the function, immediately `add esp, 4`, then do a `popa`
 * and `add esp, 4` again.
 * 
 * NOTE: Fault handlers are not supposed to return.  If they do anyway *DON'T*
 * just iretd out of them, they will be re-called (at best), or a different exception
 * will be thrown (at worst), because the EIP will be invalid (error code would replace it)
*/

typedef struct 
{
	uint32_t//:
		edi, esi, ebp, esp, ebx, edx, ecx, eax,
		error_code, 
		eip, cs, eflags;
}
Registers;

void OnSyscallReceived (Registers* pRegs);


#endif//_SYSCALL_H