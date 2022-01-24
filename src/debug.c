/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

             Debugging module
******************************************/
#include <debug.h>
#include <task.h>

void DumpRegisters (Registers* pRegs)
{
	LogMsg("Registers:");
	LogMsg("EAX=%x CS=%x "    "EIP=%x EFLGS=%x", pRegs->eax, pRegs->cs, pRegs->eip, pRegs->eflags);
	LogMsg("EBX=%x             ESP=%x EBP=%x",   pRegs->ebx,            pRegs->esp, pRegs->ebp);
	LogMsg("ECX=%x             ESI=%x", pRegs->ecx,            pRegs->esi);
	LogMsg("EDX=%x             EDI=%x", pRegs->edx,            pRegs->edi);
}

const char* g_pBugCheckReasonText[] = {
	// first 32: x86 exceptions
	"Divide by zero",
	"Unknown debugging exception",
	"Non maskable interrupt",
	"Breakpoint",
	"Overflow",
	"Boundary Range Exceeded",
	"Invalid opcode",
	"Device not available",
	"Double-fault",
	"Not the case",//FPU segment overrun
	"Not the case",//Invalid TSS
	"Segment not present",
	"Stack segment fault",
	"General protection fault",
	"Page fault",
	"Reserved",
	"x87 FPU exception",
	"Alignment check",
	"Machine check",
	"Not the case",//SIMD FPU exception
	"Not the case",//Virtualization exception
	"Not the case",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Not the case",
	"Not the case",
	"Not the case",
	"Reserved",
	
	// miscellaneous failures you may encounter
};

/* Assume, as is often the case, that EBP is the first thing pushed. If not, we are in trouble. */
typedef struct StackFrame {
	struct StackFrame* ebp;
	uint32_t eip;
} StackFrame;

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
void KeBugCheck (BugCheckReason reason, Registers* pRegs)
{
	LogMsg("*** STOP: %x", reason);
	LogMsg("%s", g_pBugCheckReasonText[reason]);
	
	if (!pRegs)
		LogMsg("No register information was provided.");
	else
		DumpRegisters(pRegs);
	
	const char* pTag = KeTaskGetTag(KeGetRunningTask());
	
	// navigate the stack:
	StackFrame* stk = (StackFrame*)(pRegs->ebp);
	//__asm__ volatile ("movl %%ebp, %0"::"r"(stk));
	LogMsg("Stack trace:");
	LogMsg("-> 0x%x %s", pRegs->eip, TransformTag (pTag, pRegs->eip), GetMemoryRangeString (pRegs->eip));
	int count = 50;
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
	LogMsg("DEBUG: Use 'addr2line' to figure out the calls.");
	LogMsg("You can type 'addr2line -e kernel.bin', then type in each of these addresses.");
	
	
	cli;
	while (1) hlt;
}
