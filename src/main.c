/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

 Kernel initialization and startup module
******************************************/
#include <main.h>
#include <memory.h>
#include <vga.h>
#include <video.h>
#include <print.h>
#include <idt.h>
#include <keyboard.h>
#include <elf.h>
#include <multiboot.h>
#include <task.h>
#include <shell.h>
#include <mouse.h>
#include <storabs.h>
#include <misc.h>
#include <vfs.h>
#include <fpu.h>
#include <wcall.h>
#include <window.h>

//TODO FIXME: Add a global VFS lock.

extern void FsMountFatPartitions (void);

__attribute__((noreturn))
void KeStopSystem()
{
	cli;
	while (1)
		hlt;
}

extern uint32_t e_placement;
int g_nKbExtRam = 0;

void KePrintSystemVersion()
{
	LogMsg("NanoShell (TM), January 2022 - " VersionString);
	LogMsg("[%d Kb System Memory, %d Kb Usable Memory]", g_nKbExtRam, GetNumPhysPages() * 4);
}
void KiPerformRamCheck()
{
	if (g_nKbExtRam < 8192)
	{
		SwitchMode(0);
		CoInitAsText(&g_debugConsole);
		LogMsg("NanoShell has not found enough extended memory.  8Mb of extended memory is\nrequired to run NanoShell.  You may need to upgrade your computer.");
		KeStopSystem();
	}
}

extern void KeCPUID();//io.asm
extern void ShellInit(void);//shell.c

extern char g_initrdStart[];

#define VERBOSE_START 1
#if VERBOSE_START
#define VerboseLogMsg LogMsg
#else
#define VerboseLogMsg
#endif

extern VBEData* g_vbeData;

multiboot_info_t* g_pMultibootInfo;

extern uint8_t g_TestingFloppyImage[];

void FpuTest();
__attribute__((noreturn))
void KiStartupSystem (unsigned long check, unsigned long mbaddr)
{
	bool textMode = true;
	//TODO: Serial debugging?
	
	// Initially, both debug consoles are initialized as E9 hacks/serial.
	CoInitAsE9Hack (&g_debugConsole);
	CoInitAsE9Hack (&g_debugSerialConsole);
	
	// Initialise the terminal.
	g_debugConsole.color = DefaultConsoleColor;
	
	// Check the multiboot checknum
	if (check != 0x2badb002)
	{
		SwitchMode(0);
		CoInitAsText(&g_debugConsole);
		LogMsg("NanoShell has not booted from a Multiboot-compatible bootloader.  A bootloader such as GRUB is required to run NanoShell.");
		KeStopSystem();
	}
	
	WindowCallDeinitialize();
	
	// Read the multiboot data:
	multiboot_info_t  *mbi = (multiboot_info_t*)(mbaddr + BASE_ADDRESS);
	g_pMultibootInfo = mbi;
	
	g_nKbExtRam = mbi->mem_upper;
	KiPerformRamCheck();
	MmFirstThingEver();
	
	//grab the CPUID
	KeCPUID();
	
	KiFpuInit();
	
	FpuTest();
	
	KiIdtInit();
	cli;
	// Initialize the Memory Management Subsystem
	MmInit(mbi);
	// Initialize the video subsystem
	VidInitialize (mbi);
	
	KePrintSystemVersion();
	
	//KePrintMemoryMapInfo();
	// Initialize the task scheduler
	KiTaskSystemInitialize();
	
	
	// Initialize the ramdisk
	FsInitializeInitRd(g_initrdStart);
	// Initialize the IDE driver
	StIdeInitialize ();
	
	//Initialize the mouse driver too
	sti;
	if (VidIsAvailable())
		MouseInit();
	// Initialize the FAT partitions.
	FsMountFatPartitions();
	
	//print the hello text, to see if the OS booted ok
	if (!VidIsAvailable())
	{
		LogMsg("\n\x01\x0CWARNING\x01\x0F: Running NanoShell in text mode is deprecated and will be removed in the future.\n");
		textMode = true;
	}
	
	KePrintSystemInfo();
	
	LogMsg("Type 'w' to start up the GUI!");
	
	ShellInit();
	
	if (textMode)
		ShellRun(0);
	else
		WindowManagerTask (0);
	LogMsg("Kernel ready to shutdown.");
	KeStopSystem();
}