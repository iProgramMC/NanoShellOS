/*****************************************
        NanoShell Operating System
        (C)2021-2022 iProgramInCpp

 Kernel initialization and startup module
******************************************/
#include <idt.h>
#include <keyboard.h>
#include <main.h>
#include <memory.h>
#include <misc.h>
#include <mouse.h>
#include <multiboot.h>
#include <print.h>
#include <shell.h>
#include <task.h>
#include <string.h>
#include <vga.h>
#include <video.h>


// TODO FIXME: Add a global VFS lock.

extern void FsMountFatPartitions(void);

char g_cmdline [1024];

__attribute__((noreturn)) void KeStopSystem()
{
	cli;
	while (1)
		hlt;
}

extern uint32_t e_placement;
int g_nKbExtRam = 0;

void KePrintSystemVersion()
{
	LogMsg("NanoShell (TM), March 2022 - " VersionString);
	LogMsg("[%d Kb System Memory, %d Kb Usable Memory]", g_nKbExtRam,
				 GetNumPhysPages() * 4);
	LogMsg("Built on: %s %s", __DATE__, __TIME__);
}

void KiPerformRamCheck()
{
	if (g_nKbExtRam < 8192)
	{
		SwitchMode(0);
		CoInitAsText(&g_debugConsole);
		LogMsg("NanoShell has not found enough extended memory.	8Mb of extended "
		       "memory is\nrequired to run NanoShell.   You may need to upgrade "
		       "your computer.");
		KeStopSystem();
	}
}

extern void KeCPUID();			 // io.asm
extern void ShellInit(void); // shell.c

#define VERBOSE_START 1
#if VERBOSE_START
#define VerboseLogMsg LogMsg
#else
#define VerboseLogMsg
#endif

void OnInaccessibleInitrdModule(uint32_t mods_addr)
{
	LogMsg("The initrd module is located above 1M! (Specifically at %x)  This is currently not supported by NanoShell.  Tips to avoid this error:\n"
	       "- Reduce the initrd size.  Some things may bloat it\n"
		   "- Use a different bootloader.  This may or may not fix the issue\n"
	       "- Change the kernel code so that the initrd is always mapped at, for example, 0x10000000", mods_addr);
	KeStopSystem();
}

extern VBEData *g_vbeData;

extern bool g_IsBGADevicePresent; // pci.c

multiboot_info_t *g_pMultibootInfo;

extern bool g_gotTime; // idt.c
void FpuTest();

extern uint32_t e_temporary1, e_temporary2;
extern uint32_t g_BGADeviceBAR0;

__attribute__((noreturn))
void KiStartupSystem(unsigned long check, unsigned long mbaddr)
{
	// Initially, both debug consoles are initialized as E9 hacks/serial.
	CoInitAsE9Hack(&g_debugConsole);
	CoInitAsE9Hack(&g_debugSerialConsole);

	// Initialise the terminal.
	g_debugConsole.color = DefaultConsoleColor;

	// Check the multiboot checknum
	if (check != 0x2badb002)
	{
		LogMsg("NanoShell has not booted from a Multiboot-compatible bootloader.  A bootloader such as GRUB is required to run NanoShell.");
		KeStopSystem();
	}

	// Read the multiboot data:
	multiboot_info_t *mbi = (multiboot_info_t *)(mbaddr + BASE_ADDRESS);
	g_pMultibootInfo = mbi;
	
	uint32_t cmdlineaddr = mbi->cmdline;
	if (!(mbi->flags & MULTIBOOT_INFO_CMDLINE))
	{
		strcpy (g_cmdline, "No!");
	}
	else if (cmdlineaddr < 0x100000)
	{
		strcpy (g_cmdline, ((char*)0xC0000000 + cmdlineaddr));
	}
	else
	{
		strcpy (g_cmdline, "No!");
	}

	g_nKbExtRam = mbi->mem_upper;
	KiPerformRamCheck();
	MmFirstThingEver();

	// grab the CPUID
	KeCPUID();

	KiIdtInit();
	cli;
	// Initialize the Memory Management Subsystem
	MmInit(mbi);

	// Initialize the video subsystem
	VidInitialize(mbi);
	
	if (strcmp (g_cmdline, "No!") == 0 || g_cmdline[0] == 0)
	{
		LogMsg("NanoShell cannot boot, because either:");
		LogMsg("- no cmdline was passed");
		LogMsg("- cmdline's address was %x%s", cmdlineaddr, cmdlineaddr >= 0x100000 ? " (was bigger than 1 MB)" : "");
		KeStopSystem();
	}

	// Initialize the keyboard.
	KbInitialize();

	KePrintSystemVersion();

	// KePrintMemoryMapInfo();
	//	Initialize the task scheduler
	KiTaskSystemInitialize();
	
	if (VidIsAvailable())
	{
		LogMsg("Initializing mouse driver");
		MouseInit();
	}

	// print the hello text, to see if the OS booted ok
	if (!VidIsAvailable())
	{
		LogMsg("\n\x01\x0CWarning\x01\x0F: NanoShell has fallen back to emergency text mode\n");
	}
	
	ShellInit();
	ShellRun(0);
	
	LogMsg("Kernel ready to shutdown.");
	KeStopSystem();
}
