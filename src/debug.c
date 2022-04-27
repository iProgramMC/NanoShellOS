/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

             Debugging module
******************************************/
#include <debug.h>
#include <task.h>
#include <keyboard.h>
#include <print.h>
#include <icon.h>

void DumpRegisters (Registers* pRegs)
{
	LogMsg("Registers:");
	LogMsg("EAX=%x CS=%b EIP=%x EFLGS=%x", pRegs->eax, pRegs->cs, pRegs->eip, pRegs->eflags);
	LogMsg("EBX=%x       ESP=%x EBP=%x",   pRegs->ebx,            pRegs->esp, pRegs->ebp);
	LogMsg("ECX=%x       ESI=%x CR2=%x",   pRegs->ecx,            pRegs->esi, pRegs->cr2);
	LogMsg("EDX=%x       EDI=%x",          pRegs->edx,            pRegs->edi);
}
//WORK: make sure the string you pass in here is large enough!!!
void DumpRegistersToString (char* pStr, Registers* pRegs)
{
	sprintf(pStr, 
		"Registers:\n"
		"EAX=%x CS=%b EIP=%x EFLGS=%x\n"
		"EBX=%x       ESP=%x EBP=%x\n"
		"ECX=%x       ESI=%x CR2=%x\n"
		"EDX=%x       EDI=%x",
		pRegs->eax, pRegs->cs, pRegs->eip, pRegs->eflags, pRegs->ebx, pRegs->esp, pRegs->ebp, pRegs->ecx, pRegs->esi, pRegs->cr2, pRegs->edx, pRegs->edi);
}

const char* g_pBugCheckReasonText[] = {
	// first 32: x86 exceptions
	/*Fault*/"Division by zero",
	/*Trap */"Unknown debugging exception",
	/*Abort*/"Non maskable interrupt",
	/*Trap */"Breakpoint",
	/*Trap */"Overflow",
	/*Fault*/"Not the case",//Bound range exceeded
	/*Fault*/"INVALID_OPCODE",
	/*Fault*/"FPU_NOT_AVAILABLE",
	/*Abort*/"The kernel has triggered an irrepairable double-fault and now needs to restart.",
	/*Abort*/"Not the case",//FPU segment overrun
	/*Fault*/"INVALID_TSS",//Invalid TSS
	/*Fault*/"SEGMENT_NOT_PRESENT",
	/*Fault*/"STACK_SEGMENT_FAULT",
	/*Fault*/"GENERAL_PROTECTION_FAULT",
	/*Fault*/"PAGE_FAULT_IN_NONPAGED_AREA",
	/*Abort*/"Reserved",
	/*Fault*/"FLOATING_POINT_EXCEPTION",
	/*Fault*/"ALIGNMENT_CHECK_EXCEPTION",
	/*Abort*/"MACHINE_CHECK_EXCEPTION",
	/*Fault*/"SIMD_FPU_EXCEPTION",//SIMD FPU exception
	/*Abort*/"Not the case",//Virtualization exception
	/*Abort*/"Not the case",
	/*Abort*/"Reserved",
	/*Abort*/"Reserved",
	/*Abort*/"Reserved",
	/*Abort*/"Reserved",
	/*Abort*/"Reserved",
	/*Abort*/"Reserved",
	/*Abort*/"Not the case",
	/*Abort*/"Not the case",
	/*Abort*/"Not the case",
	/*Abort*/"Reserved",
	
	// miscellaneous failures you may encounter
	"INACCESSIBLE_INIT_DEVICE",
	"FILE_SYSTEM",
	"INACCESSIBLE_BOOT_DEVICE",
	"INIT_NOT_SPAWNABLE",
	"CRITICAL_PROCESS_DIED",
};

const char* GetBugCheckReasonText(BugCheckReason reason)
{
	if (reason < 0 || reason > (BugCheckReason)ARRAY_COUNT (g_pBugCheckReasonText)) return "UNKNOWN";
	
	return g_pBugCheckReasonText[reason];
}
const char* GetMemoryRangeString(uint32_t range)
{
	if (range >= 0xD0000000) return "Framebuffer (huh?!)";
	if (range >= 0xC0000000) return "Kernel Executable Data";
	if (range >= 0x80000000) return "Kernel Heap";
	if (range >= 0x40000000) return "User Heap";
	if (range >= 0x00C00000) return "User Application";
	return "Unknown (perhaps null page?)";
}
const char* TransformTag(const char* tag, uint32_t range)
{
	if (range >= 0x80000000) return "<kernel>";
	return tag;
}

#define MAX_FRAMES 10
void KeLogExceptionDetails (BugCheckReason reason, Registers* pRegs)
{
	LogMsg("*** STOP: %x", reason);
	
	if (pRegs)
		DumpRegisters(pRegs);
	
	const char* pTag = KeTaskGetTag(KeGetRunningTask());
	
	if (pRegs)
	{
		// navigate the stack:
		StackFrame* stk = (StackFrame*)(pRegs->ebp);
		//__asm__ volatile ("movl %%ebp, %0"::"r"(stk));
		LogMsg("Stack trace:");
		LogMsg("-> 0x%x %s", pRegs->eip, TransformTag (pTag, pRegs->eip), GetMemoryRangeString (pRegs->eip));
		int count = 40;
		for (unsigned int frame = 0; stk && frame < MAX_FRAMES; frame++)
		{
			LogMsgNoCr(" * 0x%x %s\t%s", stk->eip, TransformTag (pTag, stk->eip), GetMemoryRangeString (stk->eip));
			// TODO: addr2line implementation?
			LogMsg("");
			stk = stk->ebp;
			count--;
			if (count == 0)
			{
				LogMsg("(And so on. Cutting it off here. Remove this if you need it.)");
				break;
			}
		}
	}
	else
		LogMsg("No stack trace is available.");
}
void ShellExecuteCommand(char* p);
extern Console* g_focusedOnConsole, g_debugConsole;
void KeBugCheck (BugCheckReason reason, Registers* pRegs)
{
	g_focusedOnConsole = &g_debugConsole;
	
	LogMsg("A problem has been detected and NanoShell has shut down to prevent damage to your computer.\n\n");
	LogMsg("%s\n", g_pBugCheckReasonText[reason]);
	if (reason <= BC_EX_RESERVED8)
	{
		LogMsg("\nYou may now restart your computer, or log the crash details somewhere to use later.\n");
	}
	LogMsg("\nTechnical Information:");
	
	KeLogExceptionDetails (reason, pRegs);
	
	//enough text, draw the icon:
	
	int x_mid = (GetScreenSizeX() - 96) / 2;
	int y_mid = (GetScreenSizeY() - 32) / 2;
	
	VidFillRect(0x00FFFF, x_mid - 5, y_mid - 5, x_mid + 105, y_mid + 41);
	
	RenderIcon(ICON_COMPUTER_PANIC, x_mid, y_mid);
	
#ifdef ALLOW_POST_CRASH_DEBUG
	LogMsg("NanoShell has been put into debug mode.");
	LogMsg("Press 'M' to list memory allocations, or '?' for help.");
	
	//remap PICs to only enable keyboard interrupt:
	WritePort (0x21, 0b11111101);
	WritePort (0xA1, 0b11111111);
	
	//enable interrupts
	sti;
	
	//wait for a key to show up:
	while (1)
	{
		//q&d debugger
		while (CoInputBufferEmpty()) hlt;
		
		char buf = CoGetChar();
		if (buf == '/' || buf == '?')
		{
			LogMsg("Bug check help:");
			LogMsg("- ? or /: list available commands");
			LogMsg("- m or M: list allocated memory regions");
			LogMsg("- h or H: halt system");
			LogMsg("- r or R: re-print exception details");
			LogMsg("- s or S: execute a shell command");
		}
		else if (buf == 's' || buf == 'S')
		{
			char shellcmd[256];
			LogMsgNoCr("DEBUGSHELL>");
			CoGetString (shellcmd, 256);
			
			ShellExecuteCommand(shellcmd);
		}
		else if (buf == 'm' || buf == 'M')
		{
			MmDebugDump ();
		}
		else if (buf == 'r' || buf == 'R')
		{
			KeLogExceptionDetails (reason, pRegs);
		}
		else if (buf == 'h' || buf == 'H')
		{
			break ;
		}
		else
		{
			LogMsg("Press '?' to list a list of commands.");
		}
	}
	
	cli;
#endif
	
	LogMsg("System halted.");
	while (1) hlt;
}
