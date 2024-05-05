/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp
		  
    Interrupt system module header file
******************************************/
#ifndef _IDT_H
#define _IDT_H

#include <debug.h>
#include <task.h>

typedef struct {
	unsigned short offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr; 
	unsigned short offset_higherbits;
}
__attribute__((packed)) 
IdtEntry;

typedef struct
{
	uint16_t limit;
	uint32_t base;
}
__attribute__((packed))
IdtPointer;

typedef struct CrashInfo
{
	struct CrashInfo* m_next;
	
	Registers m_regs;
	Task*     m_pTaskKilled;
	uint32_t  m_nErrorCode;
	char      m_tag[256];
	uint32_t  m_stackTrace[51];
}
CrashInfo;

extern void KiIdtInit();
extern void KiPicInit();
extern void KiPermitTaskSwitching();
extern void KeTimerInit();
extern void IrqKeyboardA(void);
extern void IrqTimerA(void);
extern void KeIdtLoad(IdtPointer *idt_ptr);
extern void WaitMS(int ms);
//extern void SetupPicInterrupt (int intNum, void* isrHandler);


// Registers an IRQ handler.
//
// Note that the IRQ might be fired for other reasons, e.g. if you have a network
// card and a sound card in the same slot.  So your dispatcher must check with the
// device it supports in order to see if it was even for your device.
typedef void(*InterruptDispatcher)();
void KeRegisterIrqHandler(int irqNo, InterruptDispatcher dispatcher, bool interruptsDisabled);

#define IRQ_TIMER    (0)
#define IRQ_KEYBOARD (1)
#define IRQ_CASCADE  (2)
#define IRQ_COM2     (3)
#define IRQ_COM1     (4)
#define IRQ_LPT1     (7)
#define IRQ_CLOCK    (8)
#define IRQ_MOUSE    (12)

#endif//_IDT_H