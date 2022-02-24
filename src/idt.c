/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

	      Interrupt system module
******************************************/
#include <main.h>
#include <idt.h>
#include <keyboard.h>
#include <syscall.h>
#include <debug.h>
#include <misc.h>
#include <video.h>

#define KBDATA 0x60
#define KBSTAT 0x64
#define IDT_SIZE 256
#define INTGATE 0x8e
#define KECODESEG 0x08

IdtEntry g_idt [IDT_SIZE];

bool g_interruptsAvailable = false;

void SetupSoftInterrupt (int intNum, void *pIsrHandler)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits = ((int)(pIsrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(pIsrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
}
void SetupInterrupt (uint8_t *mask1, uint8_t* mask2, int intNum, void* isrHandler)
{
	IdtEntry* pEntry = &g_idt[0x20 + intNum];
	pEntry->offset_lowerbits = ((int)(isrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(isrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
	
	int picFlag = intNum & 7;
	int whichPic = intNum >= 8;
	
	*(whichPic ? mask2 : mask1) &= ~(1<<picFlag);
}
void SetupExceptionInterrupt (int intNum, void* isrHandler)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits = ((int)(isrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(isrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
}

/**
 * Exception handlers.  They cause a bugcheck when we get 'em.
 */
bool g_hasAlreadyThrownException = false;
extern Console *g_currentConsole, g_debugConsole;
void IsrExceptionCommon(int code, Registers* pRegs) {
	g_debugConsole.color = 0x4F;
	g_currentConsole = &g_debugConsole;
	VidSetVBEData(NULL);
	VidSetFont(FONT_TAMSYN_BOLD);
	
	if (g_hasAlreadyThrownException)
	{
		LogMsg("SEVERE ERROR: Already threw an exception.");
		KeStopSystem();
	}
	g_hasAlreadyThrownException = true;
	KeBugCheck((BugCheckReason)code, pRegs);
}
#define HAS_EXCEPTION_HANDLERS
#ifdef HAS_EXCEPTION_HANDLERS


extern void IsrStub0 ();
extern void IsrStub1 ();
extern void IsrStub2 ();
extern void IsrStub3 ();
extern void IsrStub4 ();
extern void IsrStub5 ();
extern void IsrStub6 ();
extern void IsrStub7 ();
extern void IsrStub8 ();
extern void IsrStub9 ();
extern void IsrStub10();
extern void IsrStub11();
extern void IsrStub12();
extern void IsrStub13();
extern void IsrStub14();
extern void IsrStub15();
extern void IsrStub16();
extern void IsrStub17();
extern void IsrStub18();
extern void IsrStub19();
extern void IsrStub20();
extern void IsrStub21();
extern void IsrStub22();
extern void IsrStub23();
extern void IsrStub24();
extern void IsrStub25();
extern void IsrStub26();
extern void IsrStub27();
extern void IsrStub28();
extern void IsrStub29();
extern void IsrStub30();
extern void IsrStub31();
#endif

/**
 * PIT initializer routine.
 */
void KeTimerInit() 
{
	WritePort(0x43, 0x34); // generate frequency
	
	// set frequency
	//int pitMaxFreq = 1193182;
	
	//note that the PIT frequency divider has been hardcoded to 65535
	//for testing.
	
	/*
		1000 HZ: 1194
		2000 HZ: 597
		4000 HZ: 299
		8000 HZ: 149
	
	*/
	
	int pit_frequency = 597;//65536/4096;//~ 74.573875 KHz
	WritePort(0x40, (uint8_t)( pit_frequency       & 0xff));
	WritePort(0x40, (uint8_t)((pit_frequency >> 8) & 0xff));
}
/**
 * PIT interrupt routine
 */
void IrqTimer()
{
	//LogMsg("Timer!");
	WritePort(0x20, 0x20);
	WritePort(0xA0, 0x20);
}
unsigned long idtPtr[2];

// some forward declarations
extern void IrqTaskA();
extern void IrqClockA();
extern void IrqMouseA();
extern void IrqCascadeA();
extern void OnSyscallReceivedA();
void KeClockInit();

/**
 * IDT initializer routines.  Also sets up the system call interrupt.
 */
void KeIdtLoad1(IdtPointer *ptr)
{
	__asm__ ("lidt %0" :: "m"(*ptr));
}
void KiIdtInit()
{
	uint8_t mask1 = 0xff, mask2 = 0xff;
	
	SetupInterrupt (&mask1, &mask2, 0x0, IrqTaskA);//IrqTimerA);
	SetupInterrupt (&mask1, &mask2, 0x1, IrqKeyboardA);
	SetupInterrupt (&mask1, &mask2, 0x2, IrqClockA); // IRQ2: Cascade. Never triggered
	SetupInterrupt (&mask1, &mask2, 0x8, IrqClockA);
	SetupInterrupt (&mask1, &mask2, 0xC, IrqMouseA);
	//prim and sec IDE drives.  Enable IRQs to avoid spending all the
	//CPU time polling and heating up the shit out of our CPU.
	//SetupInterrupt (&mask1, &mask2, 0xE, IrqCascadeA);
	//SetupInterrupt (&mask1, &mask2, 0xF, IrqCascadeA);
	
#ifdef HAS_EXCEPTION_HANDLERS
	SetupExceptionInterrupt (0x00, IsrStub0 );
	SetupExceptionInterrupt (0x01, IsrStub1 );
	SetupExceptionInterrupt (0x02, IsrStub2 );
	SetupExceptionInterrupt (0x03, IsrStub3 );
	SetupExceptionInterrupt (0x04, IsrStub4 );
	SetupExceptionInterrupt (0x05, IsrStub5 );
	SetupExceptionInterrupt (0x06, IsrStub6 );
	SetupExceptionInterrupt (0x07, IsrStub7 );
	SetupExceptionInterrupt (0x08, IsrStub8 );
	SetupExceptionInterrupt (0x09, IsrStub9 );
	SetupExceptionInterrupt (0x0A, IsrStub10);
	SetupExceptionInterrupt (0x0B, IsrStub11);
	SetupExceptionInterrupt (0x0C, IsrStub12);
	SetupExceptionInterrupt (0x0D, IsrStub13);
	SetupExceptionInterrupt (0x0E, IsrStub14);
	SetupExceptionInterrupt (0x0F, IsrStub15);
	SetupExceptionInterrupt (0x10, IsrStub16);
	SetupExceptionInterrupt (0x11, IsrStub17);
	SetupExceptionInterrupt (0x12, IsrStub18);
	SetupExceptionInterrupt (0x13, IsrStub19);
	SetupExceptionInterrupt (0x14, IsrStub20);
	SetupExceptionInterrupt (0x15, IsrStub21);
	SetupExceptionInterrupt (0x16, IsrStub22);
	SetupExceptionInterrupt (0x17, IsrStub23);
	SetupExceptionInterrupt (0x18, IsrStub24);
	SetupExceptionInterrupt (0x19, IsrStub25);
	SetupExceptionInterrupt (0x1A, IsrStub26);
	SetupExceptionInterrupt (0x1B, IsrStub27);
	SetupExceptionInterrupt (0x1C, IsrStub28);
	SetupExceptionInterrupt (0x1D, IsrStub29);
	SetupExceptionInterrupt (0x1E, IsrStub30);
	SetupExceptionInterrupt (0x1F, IsrStub31);
#endif
	
	SetupSoftInterrupt (0x80, OnSyscallReceivedA);
	SetupSoftInterrupt (0x81, IrqTaskA);
	
	//initialize the pics
	WritePort (0x20, 0x11);
	WritePort (0xa0, 0x11);
	
	//set int num offsets, because fault offsets are hardcoded
	//inside the CPU. We dont want a crash triggering a keyboard
	//interrupt do we?!
	WritePort (0x21, 0x20); //main PIC
	WritePort (0xa1, 0x28); //secondary PIC, unused
	
	//no need to initialize the second PIC, but I'll still
	//do it anyway just to be safe
	WritePort (0x21, 0x04);
	WritePort (0xA1, 0x02);
	
	//something
	WritePort (0x21, 0x01);
	WritePort (0xA1, 0x01);
	
	WritePort (0x21, mask1);
	WritePort (0xA1, mask2);
	
	// Load the IDT
	
	IdtPointer ptr;
	ptr.limit = (sizeof(IdtEntry) * IDT_SIZE);
	ptr.base = (size_t)g_idt;
	
	g_interruptsAvailable = true;// sti was set in LoadIDT
	KeIdtLoad1 (&ptr);
	
	KeTimerInit();
	KeClockInit();
	
	//flush the PICs
	for (int i=0; i<64; i++)
	{
		WritePort(0x20, 0x20);
		WritePort(0xA0, 0x20);
	}
	
	//sti;
}

/**
 * RTC initialization routine.
 */
void KeClockInit()
{
	cli;
	WritePort(0x70, 0x8A);
	WritePort(0x71, 0x20);
	
	WritePort(0x70, 0x8B);
	char flags = ReadPort(0x71);
	WritePort(0x70, 0x8B);
	WritePort(0x71, flags | 0x40);
	
	//32768>>(5-1) = 2048 hz.  The fastest you can pick is a division rate of
	//3, which gets you a 8192Hz interrupt rate.
	int divisionRate = 8;
	WritePort(0x70, 0x8A);
	flags = ReadPort(0x71);
	WritePort(0x70, 0x8A);
	WritePort(0x71, (flags & 0xF0) | divisionRate);
	
	//sti;
}

extern int g_nRtcTicks;//misc.c

/**
 * RTC interrupt routine.
 */
void IrqClock()
{
	//acknowledge interrupt
	WritePort(0x20, 0x20);
	WritePort(0xA0, 0x20);
	//also read register C, may be useful later:
	WritePort(0x70, 0x0C);
	
	char flags = ReadPort(0x71);
	if (flags & (1 << 4))
	{
		//HACK: Done so that it wouldn't drift anymore.
		g_nRtcTicks = ((g_nRtcTicks / RTC_TICKS_PER_SECOND)+1) * RTC_TICKS_PER_SECOND;
		TmGetTime(TmReadTime());
	}
	g_nRtcTicks++;
}
