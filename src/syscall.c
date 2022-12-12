/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

            System call module
******************************************/
#include <syscall.h>
#include <debug.h>
#include <memory.h>
#include <userspace/syscalls.h>

void OnSyscallReceived (UNUSED Registers* pRegs)
{
	/*
	switch (pRegs->esi)
	{
		case LOGMSG:
			LogMsg("%s", (char*)pRegs->eax);//avoid parsing %s's as something off the stack!
			break;
		case MALLOC:
			pRegs->eax = (int)MmAllocate (pRegs->ebx);
			break;
		case FREE:
			MmFree ((void*)pRegs->eax);
			break;
		case DUMPMEM:
			MmDebugDump();
			break;
		default:
			LogMsg("warning: undefined syscall");
			DumpRegisters(pRegs);
			break;
	}
	*/
	
	ILogMsg("WARNING: This system call protocol is unsupported for now.");
}