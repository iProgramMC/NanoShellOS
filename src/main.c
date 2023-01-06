/*****************************************
        NanoShell Operating System
        (C)2021-2022 iProgramInCpp

 Kernel initialization and startup module
******************************************/
#include <clip.h>
#include <config.h>
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
#include <sb.h>
#include <shell.h>
#include <storabs.h>
#include <task.h>
#include <uart.h>
#include <vfs.h>
#include <vga.h>
#include <video.h>
#include <wcall.h>
#include <window.h>

// definitions we don't really want out there:
void MbSetup(uint32_t check, uint32_t mbaddr);
void MbReadCmdLine();
void MbCheckMem();
void MbCheckCmdLine();
void MmMarkStuffReadOnly();
void FsFatInit();
void FsExt2Init();
void FsInitRdInit();
void CfgLoadFromCmdLine();
void CfgLoadFromMainFile();
void AcpiInitIfApplicable();
void VbInitIfApplicable();
void VmwInitIfApplicable();
void BgaInitIfApplicable();
void KiIrqEnable();
void KeCPUID();
void KiTimingWait();
void ShellInit();
void CrashReporterCheckNoWindow();
void KiLaunch(TaskedFunction func);
bool KiEmergencyMode();

NO_RETURN
void KiStartupSystem(uint32_t check, uint32_t mbaddr)
{
	CoKickOff();
	MbSetup(check, mbaddr);
	WindowCallInit();
	MbReadCmdLine();
	MbCheckMem();
	MpInitialize(KiGetMultibootInfo());
	MhInitialize();
	MmMarkStuffReadOnly();
	KeCPUID();
	KiTaskSystemInit();
	KiIdtInit();
	KiPicInit();
	PciInit();
	VidInit();
	CbInit();
	BgaInitIfApplicable();
	MbCheckCmdLine();
	KiPermitTaskSwitching();
	KbInit();
	KiIrqEnable();  // After this, interrupts are enabled.
	KePrintSystemVersion();
	KiTimingWait();
	CfgInit();
	CfgLoadFromCmdLine();
	FsInit();
	StIdeInit();
	StAhciInit();
	FsExt2Init();
	FsFatInit();
	FsInitRdInit();
	UartInit(0);
	SbInit();
	CfgLoadFromMainFile();
	#ifdef EXPERIMENTAL_RSDPTR
	AcpiInitIfApplicable();
	#endif
	#ifdef EXPERIMENTAL
	VbInitIfApplicable();
	#endif
	MouseInit();
	VmwInitIfApplicable();
	ShellInit();
	SLogMsg("System ready to roll!");
	if (KiEmergencyMode())
		KiLaunch(ShellRun);
	else
		KiLaunch(WindowManagerTask);
	while (true)
	{
		CrashReporterCheckNoWindow();
		hlt;
	}
}