/*****************************************
		NanoShell Operating System
		(C)2021-2022 iProgramInCpp

	     Interrupt handler module
******************************************/
#include <main.h>
#include <idt.h>
#include <keyboard.h>
#include <syscall.h>
#include <debug.h>
#include <misc.h>
#include <video.h>
#include <task.h>
#include <process.h>
#include <string.h>

#define KBDATA 0x60
#define KBSTAT 0x64
#define IDT_SIZE 256
#define INTGATE 0x8e //present, DPL=0, gate type = 0xe (32-bit interrupt gate)
#define TSSGATE 0x85 //present, DPL=0, gate type = 0x5 (task gate)
#define KECODESEG 0x08
#define KEFTSSSEG 0x28

IdtEntry g_idt [IDT_SIZE];

bool g_interruptsAvailable = false;

void KeHandleSsFailureC()
{
	LogMsg("Unrecoverable stack error");
}

#ifdef EXPERIMENTAL
int VbGuestGetInterruptNumber();//experimental/vboxguest.c
#endif
uint8_t gPicMask1 = 0xff, gPicMask2 = 0xff;

void SetupSoftInterrupt (int intNum, void *pIsrHandler)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits  = ((int)(pIsrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(pIsrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
}
void SetupPicInterrupt (int intNum, void* isrHandler)
{
	IdtEntry* pEntry = &g_idt[0x20 + intNum];
	pEntry->offset_lowerbits  = ((int)(isrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(isrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
	
	int picFlag = intNum & 7;
	int whichPic = intNum >= 8;
	
	*(whichPic ? &gPicMask2 : &gPicMask1) &= ~(1<<picFlag);
}
void SetupExceptionInterrupt (int intNum, void* isrHandler)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits  = ((int)(isrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(isrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
}
void SetupExcepTaskInterrupt (int intNum)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits  = 0;
	pEntry->offset_higherbits = 0;
	pEntry->zero = 0;
	pEntry->type_attr = TSSGATE;
	pEntry->selector = KEFTSSSEG;
}

void WaitMS(int ms);
void PlaySound (uint32_t frequency)
{
	uint32_t divisor; uint8_t tmp;
	
	// set the PIT to the desired frequency:
	divisor = 1193182 / frequency;
	
	WritePort (0x43, 0xB6);
	WritePort (0x42, (uint8_t)(divisor));
	WritePort (0x42, (uint8_t)(divisor>>8));
	
	// And play the sound
	tmp = ReadPort (0x61);
	if (tmp != (tmp | 3))
		WritePort (0x61, tmp | 3);
}
void StopSound (void)
{
	WritePort (0x61, ReadPort (0x61) & 0xFC);
}

void PerformBeep()
{
	PlaySound(600);
	WaitMS   (50);
	StopSound();
}

bool      g_killedTaskBecauseOfException = false;

CrashInfo g_taskKilledCrashInfo;
bool g_hasAlreadyThrownException = false;
bool g_hasAlreadyThrownException1 = false;

void KeAcknowledgeTaskCrash()
{
	g_killedTaskBecauseOfException = false;
	g_hasAlreadyThrownException1   = false;
}
bool KeDidATaskCrash()
{
	return g_killedTaskBecauseOfException;
}
const char* KeGetCrashedTaskTag()
{
	return g_taskKilledCrashInfo.m_tag;
}
CrashInfo* KeGetCrashedTaskInfo()
{
	return &g_taskKilledCrashInfo;
}

/**
 * Exception handlers.  They cause a bugcheck when we get 'em.
 */
extern Console *g_currentConsole, g_debugConsole;

void IsrExceptionCommon(int code, Registers* pRegs)
{
	//TODO SEVERE FIXME: if a task fucks up the ESP you can easily triple fault the system.
	//Don't let that happen to you.  Make the stack-segment-exception switch to an emergency
	//stack to salvage what's left of the system.
	
	//TODO As it turns out a #SS is not easily obtainable, I tried fuzzing up the ESP by stack underflow, overflow and
	//also just plain corruption.  Nothing.  The worst I got was a #GP.
	//Perhaps add some stack guards?  Perhaps isolate the stack from the rest of memory (example to 0x30000000)
	//so that any stack underflow /overflow would lead to a pagefault?
	
	//g_currentConsole = &g_debugConsole;
	g_currentConsole = &g_debugConsole;
	VidSetVBEData(NULL);
	VidSetFont (FONT_TAMSYN_BOLD);
	
	//If we're running a task:
	if (KeGetRunningTask() != NULL)
	{
		if (!g_hasAlreadyThrownException1)
		{
			g_hasAlreadyThrownException1 = true;
			
			memset(
				g_taskKilledCrashInfo.m_stackTrace,
				0,
				sizeof (g_taskKilledCrashInfo.m_stackTrace)
			);
			
			//If the task was not using the kernel heap, dispose of its heap.
			/*if (KeGetRunningTask()->m_pCurrentHeap != NULL)
			{
				//FreeHeap switches to the kernel heap after its done freeing everything.
				FreeHeap (KeGetRunningTask()->m_pCurrentHeap);
			}*/
			
			g_killedTaskBecauseOfException = true;
			g_taskKilledCrashInfo.m_pTaskKilled = KeGetRunningTask();
			g_taskKilledCrashInfo.m_regs        = *pRegs;
			g_taskKilledCrashInfo.m_nErrorCode  = code;
			
			strcpy (g_taskKilledCrashInfo.m_tag, KeGetRunningTask()->m_tag);
			if (strlen (g_taskKilledCrashInfo.m_tag) == 0)
			{
				strcpy (g_taskKilledCrashInfo.m_tag, "Generic task");
			}
			
			if (KeGetRunningTask()->m_pProcess)
			{
				strcpy (g_taskKilledCrashInfo.m_tag, ((Process*)KeGetRunningTask()->m_pProcess)->sIdentifier);
			}
			
			//Just kill the process
			// Mark the process to be killed eventually
			KeGetRunningTask()->m_bSuspended = false;
			
			
			//Get the stacktrace too
			StackFrame* stk = (StackFrame*)(pRegs->ebp);
			int sttri = 0;
			g_taskKilledCrashInfo.m_stackTrace[sttri++] = pRegs->eip;
			
			for (unsigned int frame = 0; stk && frame < 50; frame++)
			{
				g_taskKilledCrashInfo.m_stackTrace[sttri++] = stk->eip;
				stk = stk->ebp;
			}
			
			//Let a task switch come in
			sti;
			
			// Wait for a switch
			while (1) hlt;
		}
	}
	
	//kernel task or a task crashing baaaadly
	if (g_hasAlreadyThrownException)
	{
		LogMsg("Recursive exception detected.  Goodbye, cruel world!");
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
	
	//formula: 1193182/<your desired frequency in hz>
	
	/*
		 250 HZ: 4772
		 500 HZ: 2386
		1000 HZ: 1194
		2000 HZ: 597
		4000 HZ: 299
		8000 HZ: 149
	
	*/
	
	int pit_frequency = 2386;
	
	//1194;//65536/4096;//~ 74.573875 KHz
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
extern void IrqSb16A();
extern void IrqVirtualBoxA();
extern void IrqCascadeA();
extern void IrqSerialCom1A();
extern void IrqSerialCom2A();
extern void OnSyscallReceivedA();
void KeClockInit();

extern bool gInitializeVB;

/**
 * IDT initializer routines.  Also sets up the system call interrupt.
 */
void KeIdtLoad1(IdtPointer *ptr)
{
	__asm__ ("lidt %0" :: "m"(*ptr));
}
bool VmwDetect();
void VmwAbsCursorIrqA();
void KiIdtInit()
{	
	// Allow a bit of leeway before task switching is allowed
	SetupPicInterrupt (0x0, IrqTimerA);
	SetupPicInterrupt (0x1, IrqKeyboardA);
	SetupPicInterrupt (0x2, IrqClockA); // IRQ2: Cascade. Never triggered
	SetupPicInterrupt (0x3, IrqSerialCom2A);
	SetupPicInterrupt (0x4, IrqSerialCom1A);
	SetupPicInterrupt (0x5, IrqSb16A);
	SetupPicInterrupt (0x8, IrqClockA);
	SetupPicInterrupt (0xC, IrqMouseA);
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
	SetupExcepTaskInterrupt (0x0C);//<-- A task gate, because we don't want to push to an invalid esp
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
	
	// SPECIAL CASE: Vbox Guest driver
#ifdef EXPERIMENTAL
	int in = VbGuestGetInterruptNumber();
	if (in >= 0 && gInitializeVB)
		SetupPicInterrupt (in, IrqVirtualBoxA);
#endif
}

void KiPermitTaskSwitching()
{
	SetupPicInterrupt (0x0, IrqTaskA);
}

void KiSetupPic()
{
	
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
	
	WritePort (0x21, gPicMask1);
	WritePort (0xA1, gPicMask2);
	
	// Load the IDT
	
	IdtPointer ptr;
	ptr.limit = (sizeof(IdtEntry) * IDT_SIZE);
	ptr.base = (size_t)g_idt;
	
	KeIdtLoad1 (&ptr);
	
	KeTimerInit();
	KeClockInit();
	
	//flush the PICs
	for (int i=0; i<64; i++)
	{
		WritePort(0x20, 0x20);
		WritePort(0xA0, 0x20);
	}
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
	
	//32768>>(14-1) = 4 hz.
	//I'll enable this in order to get second-periodic interrupts
	int divisionRate = 13;
	WritePort(0x70, 0x8A);
	flags = ReadPort(0x71);
	WritePort(0x70, 0x8A);
	WritePort(0x71, (flags & 0xF0) | divisionRate);
	
	// hack: https://forum.osdev.org/viewtopic.php?f=1&t=30091
	WritePort(0x70, 0x0C);
	ReadPort(0x71);
	//sti;
}

extern int g_nRtcTicks;//misc.c
int  g_nSeconds;
bool g_gotTime = false;

/**
 * RTC interrupt routine.
 */
uint64_t g_tscOneSecondAgo, g_tscTwoSecondsAgo;
uint64_t ReadTSC();
bool g_trustRtcUpdateFinishFlag;
void IrqClock()
{
	//acknowledge interrupt
	WritePort(0x20, 0x20);
	WritePort(0xA0, 0x20);
	//also read register C, may be useful later:
	WritePort(0x70, 0x0C);
	
	UNUSED char flags = ReadPort(0x71);
	
	g_nRtcTicks++;
	if (g_nRtcTicks % RTC_TICKS_PER_SECOND == 0)
	{
		g_nSeconds++;
		g_tscTwoSecondsAgo = g_tscOneSecondAgo;
		g_tscOneSecondAgo  = ReadTSC();
		SLogMsg("TSC difference: %Q", g_tscOneSecondAgo - g_tscTwoSecondsAgo);
	}
}