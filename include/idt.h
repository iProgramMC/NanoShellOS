/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp
		  
    Interrupt system module header file
******************************************/
#ifndef _IDT_H
#define _IDT_H

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

extern void KiIdtInit();
extern void KeTimerInit();
extern void IrqKeyboardA(void);
extern void IrqTimerA(void);
extern void KeIdtLoad(IdtPointer *idt_ptr);
extern void WaitMS(int ms);

#endif//_IDT_H