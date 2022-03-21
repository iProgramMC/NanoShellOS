/*****************************************
        NanoShell Operating System
        (C)2021-2022 iProgramInCpp

 Kernel initialization and startup module
******************************************/
#include <elf.h>
#include <fpu.h>
#include <idt.h>
#include <keyboard.h>
#include <main.h>
#include <memory.h>
#include <misc.h>
#include <mouse.h>
#include <multiboot.h>
#include <pci.h>
#include <print.h>
#include <shell.h>
#include <storabs.h>
#include <task.h>
#include <vfs.h>
#include <vga.h>
#include <video.h>
#include <wcall.h>
#include <window.h>


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
	bool textMode = true;
	// TODO: Use serial debugging, instead of the E9h port?

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

	WindowCallDeinitialize();

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

	KiFpuInit();

	FpuTest();

	KiIdtInit();
	cli;
	// Initialize the Memory Management Subsystem
	MmInit(mbi);

	// Initialize eventual PCI devices.	This is useful to find the main BGA
	// controller, if there's one.
	PciInit();

	// Initialize the video subsystem
	VidInitialize(mbi);

	if (!VidIsAvailable())
	{
		SLogMsg("BGA device BAR0:%x", g_BGADeviceBAR0);
		
		// try this:
		#define DEFAULT_WIDTH 1024
		#define DEFAULT_HEIGHT 768
		
		BgaChangeScreenResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);
		mbi->flags |= MULTIBOOT_INFO_FRAMEBUFFER_INFO;
		mbi->framebuffer_type = 1;
		// TODO FIXME: We assume this is the address, but what if it isn't?
		// Furthermore, what if the pitch doesn't match the width*4?
		mbi->framebuffer_addr = g_BGADeviceBAR0;
		mbi->framebuffer_width = DEFAULT_WIDTH;
		mbi->framebuffer_pitch = DEFAULT_WIDTH * 4;
		mbi->framebuffer_height = DEFAULT_HEIGHT;
		mbi->framebuffer_bpp = 32;

		// and re-attempt init:
		VidInitialize(mbi);
	}
	
	if (strcmp (g_cmdline, "No!") == 0 || g_cmdline[0] == 0)
	{
		LogMsg("NanoShell cannot boot, because either:");
		LogMsg("- no cmdline was passed");
		LogMsg("- cmdline's address was %x%s", cmdlineaddr, cmdlineaddr >= 0x100000 ? " (was bigger than 1 MB)" : "");
		KeStopSystem();
	}

	//LogMsg("CmdLine: %s", g_cmdline);
	
	// Initialize the keyboard.
	KbInitialize();

	KePrintSystemVersion();

	// KePrintMemoryMapInfo();
	//	Initialize the task scheduler
	KiTaskSystemInitialize();
	
	// Initialize the file system
	FsSetup ();

	// Initialize the ramdisk
	if (g_pMultibootInfo->mods_count != 1)
		KeBugCheck(BC_EX_INITRD_MISSING, NULL);

	//Usually the mods table is below 1m.
	if (g_pMultibootInfo->mods_addr >= 0x100000)
	{
		LogMsg("Module table starts at %x.  OS state not supported", g_pMultibootInfo->mods_addr);
	}
	
	//The initrd module is here.
	multiboot_module_t *initRdModule = (void*) (g_pMultibootInfo->mods_addr + 0xc0000000);

	//Precalculate an address we can use
	uint32_t pInitrdAddress = 0xc0000000 + initRdModule->mod_start;
	
	/*if (initRdModule->mod_start >= 0x100000 && initRdModule->mod_start <= 0x500000)
	{
		LogMsg("OS State not supported.  Initrd module start: %x", initRdModule->mod_start);
		KeStopSystem();
	}*/
	
	// We should no longer have the problem of it hitting our frame bitset.
	
	LogMsg("Init Ramdisk module Start address: %x, End address: %x", initRdModule->mod_start, initRdModule->mod_end);
	//If the end address went beyond 1 MB:
	if (initRdModule->mod_end >= 0x100000)
	{
		//Actually go to the effort of mapping the initrd to be used.
		pInitrdAddress = MmMapPhysicalMemory (0x10000000, initRdModule->mod_start, initRdModule->mod_end);
	}
	
	LogMsg("Physical address that we should load from: %x", pInitrdAddress);

	// Initialize the IDE driver
	StIdeInitialize();

	// Initialize the FAT partitions.
	FsMountFatPartitions();
	
	// Load the initrd last so its entries show up last when we type 'ls'.
	FsInitializeInitRd((void*)pInitrdAddress);
	
	sti;

	LogMsg("Waiting to get time...");
	while (!g_gotTime)
	{
		hlt;
	}

	// Initialize the mouse driver too
	sti;
	if (VidIsAvailable())
	{
		LogMsg("Press any key to initialize the PS/2 mouse, or wait about 5 "
					 "seconds to control it by keyboard.");

		// Wait 5 seconds.
		bool choseToInit = false;
		for (int i = 0; i < 5; i++)
		{
			// Wait 1 second.
			int startTick = GetTickCount();
			while (startTick + 1000 > GetTickCount())
			{
				if (!KbIsBufferEmpty())
				{
					// They pressed a key.	Skip the mouse init.
					choseToInit = true;
					break;
				}
			}
			if (choseToInit)
				break;
			LogMsgNoCr(".");
		}

		if (choseToInit)
		{
			LogMsg("Initializing!	(If on real hardware, you have been warned!!)");
			MouseInit();
		}
	}

	// print the hello text, to see if the OS booted ok
	if (!VidIsAvailable())
	{
		LogMsg("\n\x01\x0CWARNING\x01\x0F: Running NanoShell in text mode is "
					 "deprecated and will be removed in the future.\n");
		textMode = true;
	}

	// KePrintSystemInfo();

	// LogMsg("Type 'w' to start up the GUI!");

	ShellInit();

	if (textMode)
		ShellRun(0);
	else
		WindowManagerTask(0);
	LogMsg("Kernel ready to shutdown.");
	KeStopSystem();
}
