/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

           Misc - timing module
******************************************/
#include <main.h>
#include <misc.h>
#include <memory.h>
#include <video.h>
#include <print.h>
#include <string.h>

#define CURRENT_YEAR 2022

// Real Time
#if 1
TimeStruct g_time;

TimeStruct* TmReadTime()
{
	return &g_time;
}

int TmCmosReadRegister(int reg) {
	WritePort(0x70,reg);
	return ReadPort(0x71);
}
void TmCmosWriteRegister(int reg, int data) {
	WritePort(0x70,reg);
	WritePort(0x71,data);
}

void TmGetTime (TimeStruct* pStruct) {
	do 
	{
		pStruct->statusA = TmCmosReadRegister(0x0A);
	} 
	while (pStruct->statusA & C_UPDATE_IN_PROGRESS_FLAG);
	pStruct->seconds = TmCmosReadRegister(0x00);
	pStruct->minutes = TmCmosReadRegister(0x02);
	pStruct->hours   = TmCmosReadRegister(0x04);
	pStruct->day     = TmCmosReadRegister(0x07);
	pStruct->month   = TmCmosReadRegister(0x08);
	pStruct->year    = TmCmosReadRegister(0x09);
	TimeStruct placeholder;
	do 
	{
		placeholder.seconds = pStruct->seconds; 
		placeholder.minutes = pStruct->minutes; 
		placeholder.hours   = pStruct->hours  ; 
		placeholder.day     = pStruct->day    ; 
		placeholder.month   = pStruct->month  ; 
		placeholder.year    = pStruct->year   ;
		
		do 
		{
			pStruct->statusA = TmCmosReadRegister(0x0A);
		} 
		while (pStruct->statusA & C_UPDATE_IN_PROGRESS_FLAG);
		
		pStruct->seconds = TmCmosReadRegister(0x00);
		pStruct->minutes = TmCmosReadRegister(0x02);
		pStruct->hours   = TmCmosReadRegister(0x04);
		pStruct->day     = TmCmosReadRegister(0x07);
		pStruct->month   = TmCmosReadRegister(0x08);
		pStruct->year    = TmCmosReadRegister(0x09);
		
	}
	while (!(
		placeholder.seconds == pStruct->seconds &&
		placeholder.minutes == pStruct->minutes &&
		placeholder.hours   == pStruct->hours   &&
		placeholder.day     == pStruct->day     &&
		placeholder.month   == pStruct->month   &&
		placeholder.year    == pStruct->year
	));
	
	pStruct->statusB = TmCmosReadRegister(0x0B);
	if (!(pStruct->statusB & 0x04))//BCD mode
	{
		pStruct->seconds = BCD_TO_BIN(pStruct->seconds);
		pStruct->minutes = BCD_TO_BIN(pStruct->minutes);
		pStruct->day = BCD_TO_BIN(pStruct->day);
		pStruct->month = BCD_TO_BIN(pStruct->month);
		pStruct->year = BCD_TO_BIN(pStruct->year);
		pStruct->hours= ( (pStruct->hours&0xF)+(((pStruct->hours&0x70)/16)*10))|(pStruct->hours&0x80);
	}
	
	//convert 12h to 24h
	if (!(pStruct->statusB & 0x02) && (pStruct->hours & 0x80))
		pStruct->hours = ((pStruct->hours & 0x7f)+12)%24;
	//calculate full year
	pStruct->year += 2000;
	if(pStruct->year<CURRENT_YEAR)pStruct->year+=100;
}

const char* g_monthNamesShort = 
	"Non\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0";

void TmPrintTime(TimeStruct* pStruct) {
	char hr[3],mn[3],sc[3];
	hr[0] = '0' + pStruct->hours/10;
	hr[1] = '0' + pStruct->hours%10;
	hr[2] = 0;
	mn[0] = '0' + pStruct->minutes/10;
	mn[1] = '0' + pStruct->minutes%10;
	mn[2] = 0;
	sc[0] = '0' + pStruct->seconds/10;
	sc[1] = '0' + pStruct->seconds%10;
	sc[2] = 0;
	
	LogMsg("Current time: %d %s %d  %s:%s:%s", 
		pStruct->day, &g_monthNamesShort[4*pStruct->month],
		pStruct->year,
		hr,mn,sc);
}
void TmPrintTimeFormatted(char* buffer, TimeStruct* pStruct) {
	char hr[3],mn[3],sc[3];
	hr[0] = '0' + pStruct->hours/10;
	hr[1] = '0' + pStruct->hours%10;
	hr[2] = 0;
	mn[0] = '0' + pStruct->minutes/10;
	mn[1] = '0' + pStruct->minutes%10;
	mn[2] = 0;
	sc[0] = '0' + pStruct->seconds/10;
	sc[1] = '0' + pStruct->seconds%10;
	sc[2] = 0;
	
	sprintf(
		buffer,
		"Current time: %d %s %d  %s:%s:%s", 
		pStruct->day, &g_monthNamesShort[4*pStruct->month],
		pStruct->year,
		hr,mn,sc);
}
#endif

// Run Time
#if 1
extern multiboot_info_t* g_pMultibootInfo;//main.c

int g_nRtcTicks = 0;
void GetTimeStampCounter(int* high, int* low)
{
	if (!high && !low) return; //! What's the point?
	int edx, eax;
	__asm__ volatile ("rdtsc":"=a"(eax),"=d"(edx));
	if (high) *high = edx;
	if (low ) *low  = eax;
}

int GetTickCount()
{
	return g_nRtcTicks * 1000 / RTC_TICKS_PER_SECOND;
}
int GetRawTickCount()
{
	return g_nRtcTicks;
}
#endif

// Kernel shutdown and restart
#if 1
__attribute__((noreturn))
void KeRestartSystem(void)
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = ReadPort(0x64);
    WritePort(0x64, 0xFE);
	
	// Still running.
	if (true)
	{
		// Try a triple fault instead.
		asm("mov $0, %esp\n\
			 ret");
		
		// If all else fails, declare defeat:
		KeStopSystem();
	}
}
#endif

// Random Number Generator
#if 1
// basic garbage rand():
int GetRandom()
{
	//read the tsc:
	int hi, lo;
	GetTimeStampCounter(&hi, &lo);
	//combine the high and low numbers:
	
	hi ^= lo;
	
	//then mask it out so it wont look obvious:
	hi ^= 0xe671c4b4;
	
	//then make it positive:
	hi &= 2147483647;
	
	//lastly, return.
	return hi;
}
#endif

// CPUIDFeatureBits
#if 1
extern uint32_t g_cpuidLastLeaf;
extern char g_cpuidNameEBX[];
extern char g_cpuidBrandingInfo[];
extern CPUIDFeatureBits g_cpuidFeatureBits;

const char* GetCPUType()
{
	return g_cpuidNameEBX;
}
const char* GetCPUName()
{
	return g_cpuidBrandingInfo;
}
CPUIDFeatureBits GetCPUFeatureBits()
{
	return g_cpuidFeatureBits;
}
#endif

// Time Formatting
#if 1
//note: recommend an output buffer of at least 50 chars
void FormatTime(char* output, int formatType, int seconds)
{
	switch (formatType)
	{
		case FORMAT_TYPE_FIXED: {
			//sprintf(output, "SECONDS: %05d", seconds);
			int sec = seconds % 60;
			int min = seconds / 60 % 60;
			int hrs = seconds / 3600;
			sprintf(output, "%02d:%02d:%02d", hrs, min, sec);
			break;
		}
		case FORMAT_TYPE_VAR: {
			int sec = seconds % 60;
			int min = seconds / 60 % 60;
			int hrs = seconds / 3600;
			
			char buf[50];
			if (hrs)
			{
				sprintf(buf, "%d hour%s", hrs, hrs == 1 ? "" : "s");
				strcat (output, buf);
			}
			if (min)
			{
				if (hrs)
					sprintf(buf, ", %d min%s", min, min == 1 ? "" : "s");
				else
					sprintf(buf,   "%d min%s", min, min == 1 ? "" : "s");
				strcat (output, buf);
			}
			if (sec || !seconds)
			{
				if (min)
					sprintf(buf, ", %d sec%s", sec, sec == 1 ? "" : "s");
				else
					sprintf(buf,   "%d sec%s", sec, sec == 1 ? "" : "s");
				strcat (output, buf);
			}
			
			break;
		}
	}
}
#endif

// System Information
#if 1
void KePrintMemoryMapInfo()
{
	multiboot_info_t* mbi = g_pMultibootInfo;
	int len, addr;
	len = mbi->mmap_length, addr = mbi->mmap_addr;
	
	//turn this into a virt address:
	multiboot_memory_map_t* pMemoryMap;
	
	LogMsg("mmapAddr=%x mmapLen=%x", addr, len);
	addr += 0xC0000000;
	
	for (pMemoryMap = (multiboot_memory_map_t*)addr;
		 (unsigned long) pMemoryMap < addr + mbi->mmap_length;
		 pMemoryMap = (multiboot_memory_map_t*) ((unsigned long) pMemoryMap + pMemoryMap->size + sizeof(pMemoryMap->size)))
	{
		LogMsg("S:%x A:%x%x L:%x%x T:%x", pMemoryMap->size, 
			(unsigned)(pMemoryMap->addr >> 32), (unsigned)pMemoryMap->addr,
			(unsigned)(pMemoryMap->len  >> 32), (unsigned)pMemoryMap->len,
			pMemoryMap->type
		);
	}
}
void KePrintSystemInfoAdvanced()
{
	//oldstyle:
	/*
	LogMsg("Information about the system:");
	LogMsg("CPU Type:        %s", g_cpuidNameEBX);
	LogMsg("CPU Branding:    %s", g_cpuidBrandingInfo);
	LogMsg("Feature bits:    %x", *((int*)&g_cpuidFeatureBits));
	LogMsgNoCr("x86 Family   %d ", g_cpuidFeatureBits.m_familyID);
	LogMsgNoCr("Model %d ", g_cpuidFeatureBits.m_model);
	LogMsg("Stepping %d", g_cpuidFeatureBits.m_steppingID);
	LogMsg("g_cpuidLastLeaf: %d", g_cpuidLastLeaf);*/
	
	//nativeshell style:
	LogMsg("\x01\x0BNanoShell Operating System " VersionString);
	LogMsg("\x01\x0CVersion Number: %d", VersionNumber);
	
	LogMsg("\x01\x0F-------------------------------------------------------------------------------");
	LogMsg("\x01\x09[CPU] Name: %s", GetCPUName());
	LogMsg("\x01\x09[CPU] x86 Family %d Model %d Stepping %d.  Feature bits: %d",
			g_cpuidFeatureBits.m_familyID, g_cpuidFeatureBits.m_model, g_cpuidFeatureBits.m_steppingID);
	LogMsg("\x01\x0A[RAM] PageSize: 4K. Physical pages: %d. Total usable RAM: %d Kb", GetNumPhysPages(), GetNumPhysPages()*4);
	LogMsg("\x01\x0A[VID] Screen resolution: %dx%d.  Textmode size: %dx%d characters, of type %d.", GetScreenSizeX(), GetScreenSizeY(), 
																						g_debugConsole.width, g_debugConsole.height, g_debugConsole.type);
	LogMsg("\x01\x0F");
}

void KePrintSystemInfo()
{
	//neofetch style:
	int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	LogMsg("(note, the logo you see is temporary)");
	//below are the first lines modified in 2022 :)\x01\x10  /\x01\x10     \x01\x08 \x01\x0C
	/*
	LogMsgNoCr("  \x01\x10/                   ");                                                   LogMsg("\x01\x0C OS:       \x01\x0FNanoShell Operating System");
	LogMsgNoCr(" \x01\x10/                \x01\x1F#   ");                                           LogMsg("\x01\x0C Kernel:   \x01\x0F%s (%d)", VersionString, VersionNumber);
	LogMsgNoCr("\x01\x1F#     \x01\x10.\x01\x14+\x01\x1C#####\x01\x14:\x01\x10.    \x01\x10\\  ");  LogMsg("\x01\x0C Uptime:   \x01\x0F?");
	LogMsgNoCr("    \x01\x10+\x01\x14s\x01\x1C#########\x01\x14:\x01\x10.     ");                   LogMsg("\x01\x0C CPU:      \x01\x0F%s", GetCPUName());
	LogMsgNoCr("   \x01\x10+\x01\x14s\x01\x1C###########\x01\x14:     \x01\x10|");                  LogMsg("\x01\x0C CPU type: \x01\x0F%s", GetCPUType());
	LogMsgNoCr("  \x01\x1FmMMMMMMm\x01\x14:\x01\x1C######:.   \x01\x10|");                          LogMsg("\x01\x0C Memory:   \x01\x0F%d KB / %d KB", (npp-nfpp)*4, npp*4);
	LogMsgNoCr("  \x01\x1FW\x01\x10###\x01\x1E##\x01\x1FWWWwwwwwwW#   ");                           LogMsg("\x01\x0C ");
	LogMsgNoCr("    \x01\x1F\x01\x10##\x01\x1E####\x01\x10##\x01\x1E#####    \x01\x10/");           LogMsg("\x01\x0C ");
	LogMsgNoCr("\x01\x10\\    \x01\x1E\\####\x01\x10#\x01\x1E####/    \x01\x10/ ");                 LogMsg("\x01\x0C ");
	LogMsgNoCr(" \x01\x10\\                 \x01\x10/");                                            LogMsg("\x01\x0C ");
	LogMsgNoCr("  \x01\x1F#       #       #");                                                      LogMsg("\x01\x0C ");
	LogMsgNoCr("         /\x01\x0F");                                                               LogMsg("\x01\x0C ");
	LogMsg("\x01\x0F");
	
	LogMsgNoCr("  \x01\x10/                    ");                                                   							LogMsg("\x01\x0C OS:       \x01\x0FNanoShell Operating System");
	LogMsgNoCr(" \x01\x10/                \x01\x1F#    ");                                           							LogMsg("\x01\x0C Kernel:   \x01\x0F%s (%d)", VersionString, VersionNumber);
	LogMsgNoCr("\x01\x1F#     \x01\x10.\x01\x14+\x01\x1C\x02\x02\x02\x02\x02\x01\x14:\x01\x10.    \x01\x10\\   ");  			LogMsg("\x01\x0C Uptime:   \x01\x0F?");
	LogMsgNoCr("    \x01\x10+\x01\x14s\x01\x1C\x02\x02\x02\x02\x02\x02\x02\x02\x02\x01\x14:\x01\x10.      ");					LogMsg("\x01\x0C CPU:      \x01\x0F%s", GetCPUName());
	LogMsgNoCr("   \x01\x10+\x01\x14s\x01\x1C\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x01\x14:     \x01\x10|");				LogMsg("\x01\x0C CPU type: \x01\x0F%s", GetCPUType());
	LogMsgNoCr("  \x01\x1FmMMMMMMm\x01\x14:\x01\x1C\x02\x02\x02\x02\x02\x02:.   \x01\x10|");									LogMsg("\x01\x0C Memory:   \x01\x0F%d KB / %d KB", (npp-nfpp)*4, npp*4);
	LogMsgNoCr("  \x01\x1FW\x01\x10\x02\x02\x02\x01\x1E\x02\x02\x01\x1FWWWwwwwwwW#   ");										LogMsg("\x01\x0C ");
	LogMsgNoCr("    \x01\x1F\x01\x10\x02\x02\x01\x1E\x02\x02\x02\x02\x01\x10\x02\x02\x01\x1E\x02\x02\x02\x02\x02    \x01\x10/");LogMsg("\x01\x0C ");
	LogMsgNoCr("\x01\x10\\    \x01\x1E\\\x02\x02\x02\x02\x01\x10\x02\x01\x1E\x02\x02\x02\x02/    \x01\x10/ ");					LogMsg("\x01\x0C ");
	LogMsgNoCr(" \x01\x10\\                 \x01\x10/");																		LogMsg("\x01\x0C ");
	LogMsgNoCr("  \x01\x1F#       #       #");																					LogMsg("\x01\x0C ");
	LogMsgNoCr("         \x01\x10/\x01\x0F");																					LogMsg("\x01\x0C ");
	LogMsg("\x01\x0F");*/
	
	char timingInfo[128];
	timingInfo[0] = 0;
	FormatTime(timingInfo, FORMAT_TYPE_VAR, GetTickCount() / 1000);
	LogMsg("\x01\x0E N    N       "      "\x01\x0C OS:       \x01\x0FNanoShell Operating System");
	LogMsg("\x01\x0E NN   N       "      "\x01\x0C Kernel:   \x01\x0F%s (%d)", VersionString, VersionNumber);
	LogMsg("\x01\x0E N N  N       "      "\x01\x0C Uptime:   \x01\x0F%s", timingInfo);
	LogMsg("\x01\x0E N  N N       "      "\x01\x0C CPU:      \x01\x0F%s", GetCPUName());
	LogMsg("\x01\x0E N   NN       "      "\x01\x0C CPU type: \x01\x0F%s", GetCPUType());
	LogMsg("\x01\x0E N    N\x01\x0D SSSS  \x01\x0C Memory:   \x01\x0F%d KB / %d KB", (npp-nfpp)*4, npp*4);
	LogMsg("\x01\x0D       S    S "      "\x01\x0C ");
	LogMsg("\x01\x0D       S      "      "\x01\x0C ");
	LogMsg("\x01\x0D        SSSS  "      "\x01\x0C ");
	LogMsg("\x01\x0D            S "      "\x01\x0C ");
	LogMsg("\x01\x0D       S    S "      "\x01\x0C ");
	LogMsg("\x01\x0D        SSSS  "      "\x01\x0C ");
	LogMsg("\x01\x0F");
}
#endif
