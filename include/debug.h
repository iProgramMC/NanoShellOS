/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

       Debugging module header file
******************************************/
#ifndef _DEBUG_H
#define _DEBUG_H

#include <syscall.h>

enum {
	BC_EX_DIV_BY_ZERO,
	BC_EX_DEBUG,
	BC_EX_NMI,
	BC_EX_BREAKPOINT,
	BC_EX_OVERFLOW,
	BC_EX_BOUND_RANGE_EXCEEDED,
	BC_EX_INVALID_OPCODE,
	BC_EX_DEVICE_NOT_AVAIL,
	BC_EX_DOUBLE_FAULT,
	BC_EX_COPROCESSOR_UNUSED,
	BC_EX_INVALID_TSS,
	BC_EX_SEGMENT_NOT_PRESENT,
	BC_EX_SS_FAULT,//TODO: Handle this seperately and switch to an emergency stack.
	BC_EX_GENERAL_FAULT,
	BC_EX_PAGE_FAULT,
	BC_EX_RESERVED,
	BC_EX_FPU_EXCEPTION,
	BC_EX_ALIGN_CHECK,
	BC_EX_MACHINE_CHECK,
	BC_EX_SIMD_FPU_EXCEPTION,
	BC_EX_VIRTUALIZATION,
	BC_EX_CONTROL_PROT_EX,
	BC_EX_RESERVED2,
	BC_EX_RESERVED3,
	BC_EX_RESERVED4,
	BC_EX_RESERVED5,
	BC_EX_RESERVED6,
	BC_EX_RESERVED7,
	BC_EX_HYPERV_INJECTION,
	BC_EX_VMM_COMM,
	BC_EX_SECURITY,
	BC_EX_RESERVED8,
	BC_EX_INITRD_MISSING,
	BC_EX_FILE_SYSTEM,
	BC_EX_INACCESSIBLE_BOOT_DEVICE,
	BC_EX_INIT_NOT_SPAWNABLE,
	BC_EX_CRITICAL_PROCESS_DIED,
	BC_EX_ASSERTION_FAILED,
};

/* Assume, as is often the case, that EBP is the first thing pushed. If not, we are in trouble. */
typedef struct StackFrame {
	struct StackFrame* ebp;
	uint32_t eip;
} StackFrame;

typedef int BugCheckReason;
void KeBugCheck (BugCheckReason reason, Registers* pRegs);
void DumpRegisters (Registers*);
//WORK: make sure the string you pass in here is large enough!!!
void DumpRegistersToString (char* pStr, Registers* pRegs);

#endif//_DEBUG_H