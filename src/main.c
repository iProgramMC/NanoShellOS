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
#include <time.h>
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
#include <main.h>

// definitions we don't really want out there:
void MbSetup(uint32_t check, uint32_t mbaddr);
void MbReadCmdLine();
void MbCheckMem();
void MbCheckCmdLine();
void MmMarkStuffReadOnly();
void FsFatInit();
void FsExt2Init();
void FsTempInit();
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

static int s_stopwatchStart;

void StopwatchStart()
{
	s_stopwatchStart = GetTickCount();
}

int StopwatchEnd()
{
	return GetTickCount() - s_stopwatchStart;
}

/*
void TestFontInit()
{
	StatResult sr;
	const char * filename = "/Lat15-Terminus16.psf";
	
	int res = FiStat(filename, &sr);
	if (res < 0)
	{
		LogMsg("Could not load font '%s': %s", filename, GetErrNoString(res));
		return;
	}
	int fd = FiOpen(filename, O_RDONLY);
	if (fd < 0)
	{
		LogMsg("Could not load font '%s': %s", filename, GetErrNoString(fd));
		return;
	}
	
	uint8_t* pBytes = MmAllocate(sr.m_size);
	if (!pBytes)
	{
		LogMsg("The font '%s' could not be loaded due to a memory shortage.", filename);
		return;
	}
	
	FiRead(fd, pBytes, sr.m_size);
	FiClose(fd);
	
	int id = CreatePSFont(pBytes, sr.m_size);
	if (id == FONT_BASIC)
		LogMsg("The font '%s' could not be loaded.", filename);
	
	MmFree(pBytes);
	
	VidSetFont(id);
}
*/

NO_RETURN
void KiStartupSystem(uint32_t check, uint32_t mbaddr)
{
	CoKickOff();
	SLogMsg("NanoShell is starting up...");
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
	//FsFatInit();
	FsExt2Init();
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
	//TestFontInit();
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