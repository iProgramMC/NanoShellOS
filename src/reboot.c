/*****************************************
		NanoShell Operating System
		  (C) 2023 iProgramInCpp

        Shutdown and reboot module
******************************************/
#include <main.h>
#include <storabs.h>

void KeOnShutDownSaveData()
{
	SLogMsg("Flushing ALL cache units before rebooting!");
	StFlushAllCaches();
}

__attribute__((noreturn))
void KeRestartSystem(void)
{
	KeOnShutDownSaveData();
	
    uint8_t good = 0x02;
    while (good & 0x02)
        good = ReadPort(0x64);
    WritePort(0x64, 0xFE);
	
	// Still running.
	if (true)
	{
		// Try a triple fault instead.
		asm("mov $0, %eax\n\t"
			"mov %eax, %cr3\n");
		
		// If all else fails, declare defeat:
		KeStopSystem();
	}
}
