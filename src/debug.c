/*****************************************
		NanoShell Operating System
		(C)2021-2022 iProgramInCpp

             Debugging module
******************************************/
#include <debug.h>
#include <task.h>
#include <keyboard.h>
#include <print.h>
#include <icon.h>
#include <process.h>
#include <elf.h>

bool g_bAreInterruptsEnabled       = false;
bool g_bAreInterruptsEnabledBackup = false;
int  g_nInterruptRecursionCount    = 0;

extern Console *g_focusedOnConsole, *g_currentConsole, g_debugConsole;

NO_RETURN void KeStopSystem()
{
	SLogMsg("System has been stopped!");
	
	//PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, true);
	
	asm ("cli");
	
	while (1)
		hlt;
}
void DumpRegisters (Registers* pRegs)
{
	ILogMsg("Registers:");
	ILogMsg("EAX=%x CS=%b EIP=%x EFLGS=%x", pRegs->eax, pRegs->cs, pRegs->eip, pRegs->eflags);
	ILogMsg("EBX=%x       ESP=%x EBP=%x",   pRegs->ebx,            pRegs->esp, pRegs->ebp);
	ILogMsg("ECX=%x       ESI=%x CR2=%x",   pRegs->ecx,            pRegs->esi, pRegs->cr2);
	ILogMsg("EDX=%x       EDI=%x",          pRegs->edx,            pRegs->edi);
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
	"KERNEL_ASSERTION_FAILED",
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

bool OnAssertionFail (const char *pStr, const char *pFile, const char *pFunc, int nLine)
{
	// TODO weird sync issue. Make sure only one thing can assert at a time, somehow.
	// This is called in both interrupt and non interrupt contexts....
	
	if (!KeCheckInterruptsDisabled())
		cli;
	
	Registers regs;
	regs.eax = (uint32_t)pStr;
	regs.ebx = (uint32_t)pFile;
	regs.ecx = (uint32_t)pFunc;
	regs.edx = (uint32_t)nLine;
	
	SLogMsg("Assertion failed: %s", pStr);
	
	g_currentConsole = &g_debugConsole;
	
	KeBugCheck(BC_EX_ASSERTION_FAILED, &regs);
	
	PrintBackTrace((StackFrame*)KeGetEBP(), KeGetEIP(), "", NULL, true);
	
	ILogMsg("The system has been stopped.");
	
	KeStopSystem();
	
	return false;
}

#define MAX_FRAMES 12
typedef void (*PLogMsg) (const char* fmt, ...);

void PrintBackTrace (StackFrame* stk, uintptr_t eip, const char* pTag, void* pProcVoid, bool bPrintToScreen)
{
	Process *pProc = (Process*)pProcVoid;
	PLogMsg logMsg = SLogMsg;
	if (bPrintToScreen)
		logMsg = ILogMsg;
	
	const char* pFunctionCrashedInside = "";
	
	ElfSymbol* pSym = ExLookUpSymbol(pProc, eip);
	if (pSym)
	{
		pFunctionCrashedInside = pSym->m_pName;
	}
	
	logMsg("Stack trace:");
	logMsg("-> 0x%x %s\t%s", eip, TransformTag (pTag, eip), pFunctionCrashedInside);
	int count = 10;
	for (unsigned int frame = 0; stk && frame < MAX_FRAMES; frame++)
	{
		const char* thing = "";
		
		if (pProc)
		{
			ElfSymbol* pSym = ExLookUpSymbol(pProc, stk->eip);
			if (pSym)
			{
				thing = pSym->m_pName;
			}
		}
		
		logMsg(" * 0x%x %s\t%s", stk->eip, TransformTag (pTag, stk->eip), thing);
		
		stk = stk->ebp;
		count--;
		if (count == 0)
		{
			logMsg("(And so on. Cutting it off here. Remove this if you need it.)");
			break;
		}
	}
}

void KeLogExceptionDetails (BugCheckReason reason, Registers* pRegs, void* pProcVoid)
{
	Process *pProc = (Process*)pProcVoid;
	ILogMsg("*** STOP: %x", reason);
	
	if (reason == BC_EX_ASSERTION_FAILED)
	{
		const char *pAssertionMsg  = (const char*)pRegs->eax;
		const char *pAssertionFile = (const char*)pRegs->ebx;
		const char *pAssertionFunc = (const char*)pRegs->ecx;
		int         pAssertionLine =              pRegs->edx;
		
		ILogMsg("Assertion failed:");
		ILogMsg("'%s' at %s:%d [%s]", pAssertionMsg,pAssertionFile,pAssertionLine,pAssertionFunc);
		
		return;
	}
	
	if (pRegs)
		DumpRegisters(pRegs);
	
	if (pRegs)
	{
		// navigate the stack:
		StackFrame* stk = (StackFrame*)(pRegs->ebp);
		PrintBackTrace(stk, pRegs->eip, "", pProc, true);
	}
	else
	{
		ILogMsg("No stack trace is available.");
	}
}

void ShellExecuteCommand(char* p, bool* pbExit);
void KeBugCheck (BugCheckReason reason, Registers* pRegs)
{
	g_focusedOnConsole = &g_debugConsole;
	
	ILogMsg("A problem has been detected and NanoShell has shut down to prevent damage to your computer.\n\n");
	ILogMsg("%s\n", g_pBugCheckReasonText[reason]);
	if (reason <= BC_EX_RESERVED8)
	{
		ILogMsg("\nYou may now restart your computer, or log the crash details somewhere to use later.\n");
	}
	ILogMsg("\nTechnical Information:");
	
	KeLogExceptionDetails (reason, pRegs, ExGetRunningProc());
	
	//enough text, draw the icon:
	
	int x_mid = (GetScreenSizeX() - 96) / 2;
	int y_mid = (GetScreenSizeY() - 32) / 2;
	
	VidFillRect(0x00FFFF, x_mid - 5, y_mid - 5, x_mid + 105, y_mid + 41);
	
	RenderIcon(ICON_COMPUTER_PANIC, x_mid, y_mid);
	
#ifdef ALLOW_POST_CRASH_DEBUG
	ILogMsg("NanoShell has been put into debug mode.");
	ILogMsg("Press 'M' to list memory allocations, or '?' for help.");
	
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
			ILogMsg("Bug check help:");
			ILogMsg("- ? or /: list available commands");
			ILogMsg("- m or M: list allocated memory regions");
			ILogMsg("- h or H: halt system");
			ILogMsg("- r or R: re-print exception details");
			ILogMsg("- s or S: execute a shell command");
		}
		else if (buf == 's' || buf == 'S')
		{
			char shellcmd[256];
			ILogMsgNoCr("DEBUGSHELL>");
			CoGetString (shellcmd, 256);
			
			bool bExit = false;
			ShellExecuteCommand(shellcmd, &bExit);
		}
		else if (buf == 'm' || buf == 'M')
		{
			MmDebugDump ();
		}
		else if (buf == 'r' || buf == 'R')
		{
			KeLogExceptionDetails (reason, pRegs, ExGetRunningProc());
		}
		else if (buf == 'h' || buf == 'H')
		{
			break ;
		}
		else
		{
			ILogMsg("Press '?' to list a list of commands.");
		}
	}
	
	cli;
#endif
	
	ILogMsg("System halted.");
	
	KeStopSystem();
	while (1) hlt;
}

void KeInterruptSanityCheck()
{
	// if debug:
	uint32_t eFlags = KeGetEFlags();
	bool bAreInterruptsActuallyOn = !!(eFlags & EFLAGS_IF);
	
	if (bAreInterruptsActuallyOn != g_bAreInterruptsEnabled)
	{
		ILogMsg("bAreInterruptsActuallyOn: %d  g_bAreInterruptsEnabled: %d  EFLAGS: %x", bAreInterruptsActuallyOn, g_bAreInterruptsEnabled, eFlags);
		ASSERT(false);
	}
}

bool KeCheckInterruptsDisabled()
{
	//KeInterruptSanityCheck();
	
	return !g_bAreInterruptsEnabled;
}

void KeVerifyInterruptsDisabledD(const char *file, int line)
{
	if (!KeCheckInterruptsDisabled())
	{
		SLogMsg("Interrupts are NOT disabled! (called from %s:%d)", file, line);
		PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
		ASSERT(!"Hmm, interrupts shouldn't be enabled here");
	}
}

void KeVerifyInterruptsEnabledD(const char *file, int line)
{
	if (KeCheckInterruptsDisabled())
	{
		SLogMsg("Interrupts are disabled, but they should be enabled! (called from %s:%d)", file, line);
		PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
		ASSERT(!"Hmm, interrupts should be enabled here");
	}
}

void KeDisableInterrupts()
{
	//on debug, also check if we've already disabled the interrupts
	if (!g_bAreInterruptsEnabled)
	{
		SLogMsg("Interrupts are already disabled!");
		PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
		KeStopSystem();
	}
	
	asm("cli");
	g_bAreInterruptsEnabled = false;
}

void KeEnableInterrupts()
{
	//on debug, also check if we've already enabled the interrupts
	if (g_bAreInterruptsEnabled)
	{
		SLogMsg("Interrupts are already enabled!");
		PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
		KeStopSystem();
	}
	
	g_bAreInterruptsEnabled = true;
	asm("sti");
}

void KeOnEnterInterrupt()
{
	// if we only entered once so far
	if (++g_nInterruptRecursionCount == 1)
	{
		// save the interrupt state
		g_bAreInterruptsEnabledBackup = g_bAreInterruptsEnabled;
	}
	
	if (g_nInterruptRecursionCount > 10)
	{
		SLogMsg("WARNING: Interrupt nesting too deep at %d levels?", g_nInterruptRecursionCount);
		PrintBackTrace((StackFrame*)KeGetEBP(), (uintptr_t)KeGetEIP(), NULL, NULL, false);
	}
	
	g_bAreInterruptsEnabled = false;
}

void KeOnExitInterrupt()
{
	// If we're going to return to normal execution rather
	// than to an interrupt handler...
	if (--g_nInterruptRecursionCount == 0)
	{
		g_bAreInterruptsEnabled = g_bAreInterruptsEnabledBackup;
	}
	
	ASSERT(g_nInterruptRecursionCount >= 0);
}


