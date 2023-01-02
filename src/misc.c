/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

           Misc - timing module
******************************************/
#include <multiboot.h>
#include <main.h>
#include <misc.h>
#include <config.h>
#include <memory.h>
#include <video.h>
#include <print.h>
#include <string.h>
#include <vga.h>
#include <storabs.h>
#include <task.h>
#include <debug.h>

#define CURRENT_YEAR 2022

// Real Time
#if 1
TimeStruct g_time;

TimeStruct* TmReadTime()
{
	return &g_time;
}

int TmCmosReadRegister(int reg)
{
	WritePort(0x70,reg);
	return ReadPort(0x71);
}
void TmCmosWriteRegister(int reg, int data)
{
	WritePort(0x70,reg);
	WritePort(0x71,data);
}

// I don't remember exactly where I copied this from
int TmGetWeekDay(TimeStruct* pStruct)
{
	int day = pStruct->day, month = pStruct->month, year = pStruct->year;
	
    int wday = (day  + ((153 * (month + 12 * ((14 - month) / 12) - 3) + 2) / 5)
				+ (365 * (year + 4800 - ((14 - month) / 12)))
				+ ((year + 4800 - ((14 - month) / 12)) / 4)
				- ((year + 4800 - ((14 - month) / 12)) / 100)
				+ ((year + 4800 - ((14 - month) / 12)) / 400)
				- 32045
      ) % 7;
	
    return (wday + 1) % 7; // so that Sunday is day 0
}

void TmGetTime (TimeStruct* pStruct)
{
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
		pStruct->day     = BCD_TO_BIN(pStruct->day);
		pStruct->month   = BCD_TO_BIN(pStruct->month);
		pStruct->year    = BCD_TO_BIN(pStruct->year);
		pStruct->hours   = ((pStruct->hours & 0xF) + (((pStruct->hours & 0x70) / 16) * 10)) | (pStruct->hours & 0x80);
	}
	
	// convert 12h to 24h
	if (!(pStruct->statusB & 0x02) && (pStruct->hours & 0x80))
		pStruct->hours = ((pStruct->hours & 0x7f) + 12) % 24;
	
	// calculate full year
	pStruct->year += 2000;
	if (pStruct->year < CURRENT_YEAR)
		pStruct->year += 100;
	
	// calculate the weekday parameter. The RTC's weekday may not be entirely reliable.
	pStruct->weekday = TmGetWeekDay(pStruct);
}

const char* g_monthNamesShort = 
	"Non\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0";

void TmPrintTime(TimeStruct* pStruct)
{
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

void TmPrintTimeFormatted(char* buffer, TimeStruct* pStruct)
{
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
void GetTimeStampCounter(uint32_t* high, uint32_t* low)
{
	if (!high && !low) return; //! What's the point?
	int edx, eax;
	__asm__ volatile ("rdtsc":"=a"(eax),"=d"(edx));
	if (high) *high = edx;
	if (low ) *low  = eax;
}

uint64_t ReadTSC()
{
	uint32_t hi, lo;
	GetTimeStampCounter(&hi, &lo);
	return ((uint64_t)hi << 32LL) | (uint64_t)(lo);
}

void KeOnShutDownSaveData()
{
	SLogMsg("Flushing ALL cache units before rebooting!");
	StFlushAllCaches();
}

//not accurate at all, use GetTickCount() instead.
int GetRtcBasedTickCount()
{
	return g_nRtcTicks * 1000 / RTC_TICKS_PER_SECOND;
}
int GetRawTickCount()
{
	return g_nRtcTicks;
}

// This function tries to approximate unix time the best that it can.
// This does not take into account leap seconds. Despite that, it matches the time
// functions on my system (used gmtime, matches exactly with a call to time(NULL))
static int s_daysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int GetEpochTime()
{
	TimeStruct ts = *TmReadTime();
	int year = ts.year - 1970;
	int days = 365 * year + (ts.day - 1);
	
	// (almost) every 4 years since 1972 are leap years. Count them in
	// This should work again. (year - 2 + 4) / 4 = (year - 2) / 4 + 1.
	int leapYears = ((year - 1) + 2) / 4;
	
	days += leapYears;
	
	// based on the month, determine the day
	for (int month = 1; month < ts.month; month++)
	{
		days += s_daysPerMonth[month];
		if (month == 2 && ts.year % 4 == 0) // February -- leap year
			days++;
	}
	
	return days * 86400 + ts.hours * 3600 + ts.minutes * 60 + ts.seconds;
}

void GetHumanTimeFromEpoch(int utime, TimeStruct* pOut)
{
	// Separate the amount of days from the rest of the time.
	int days = utime / 86400, secs = utime % 86400;

	// Turn the secs into hours, minutes and seconds. This is trivial.
	pOut->hours = secs / 3600;
	pOut->minutes = secs / 60 % 60;
	pOut->seconds = secs % 60;

	// Get the year number.
	int year = 1970;
	while (days >= 365 + (year % 4 == 0))
	{
		days -= 365 + (year % 4 == 0);
		year++;
	}

	// Get the month number.
	int month = 1;
	while (days >= s_daysPerMonth[month])
	{
		days -= s_daysPerMonth[month];
		if (month == 2 && year % 4 == 0) days--;
		month++;
	}

	pOut->day   = days + 1;
	pOut->month = month;
	pOut->year  = year;
}

extern uint64_t g_tscOneSecondAgo, g_tscTwoSecondsAgo;
extern int g_nSeconds;
int gEarliestTickCount;
bool g_bRtcInitialized = false;

// Only call this if you have interrupts disabled!!!
int GetTickCountUnsafe()
{
	uint64_t tscNow     = ReadTSC();
	uint64_t difference = tscNow - g_tscOneSecondAgo;             // the difference
	uint64_t tscPerSec  = g_tscOneSecondAgo - g_tscTwoSecondsAgo; // a full complete second has passed
	int nSeconds = g_nSeconds;
	
	//SLogMsg("TSC per second: %q   Difference: %q   Tsc Now: %q", tscPerSec, difference, tscNow);
	
	int ms_count = 0;
	
	if (tscPerSec)
		ms_count = difference * 1000 / tscPerSec;
	
	int newTickCount = nSeconds * 1000 + ms_count;
	if (gEarliestTickCount < newTickCount)
		gEarliestTickCount = newTickCount;
	return gEarliestTickCount;
}
int GetTickCount()
{
	if (!g_bRtcInitialized)
		return 0;
	
	KeVerifyInterruptsEnabled;
	cli;
	int tc = GetTickCountUnsafe();
	sti;
	
	return tc;
}

uint64_t gEarliestUsecCount;

uint64_t GetUsecCountUnsafe()
{
	uint64_t tscNow     = ReadTSC();
	uint64_t difference = tscNow - g_tscOneSecondAgo;             // the difference
	uint64_t tscPerSec  = g_tscOneSecondAgo - g_tscTwoSecondsAgo; // a full complete second has passed
	uint64_t nSeconds = g_nSeconds;
	
	//SLogMsg("TSC per second: %q   Difference: %q   Tsc Now: %q", tscPerSec, difference, tscNow);
	
	uint64_t us_count = 0;
	
	if (tscPerSec)
		us_count = difference * 1000000 / tscPerSec;// I hope this works :)
	
	uint64_t newUsecCount = nSeconds * 1000000ULL + us_count;
	if (gEarliestUsecCount < newUsecCount)
		gEarliestUsecCount = newUsecCount;
	return gEarliestUsecCount;
}

void KiTimingWait()
{
	// wait 1 second for system to initialize, so that we can wait and stuff
	while (GetTickCount() < 1000)
	{
		asm("hlt":::"memory");
	}
}
uint64_t GetUsecCount()
{
	if (!g_bRtcInitialized)
		return 0;
	
	KeVerifyInterruptsEnabled;
	cli;
	uint64_t tc = GetUsecCountUnsafe();
	sti;
	return tc;
}
#endif

// Kernel shutdown and restart
#if 1
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
#endif

// Random Number Generator
#if 1
int random_seed_thing[] = {
	-1244442060, -1485266709, 234119953, 
	-201730870, -1913591016, -1591339077, 
	2041649642, -886342341, 711555730, 
	40192431, 1517024340, -576781024, 
	-891284503, 1587790946, 1566147323, 
	2010441970, 1320486880, 66264233, 
	737611192, -1119832783, -865761768, 
	-420460857, 1682411333, -477284012, 
	-1116287873, -1211736837, 963092786,
	1730979436, 1505625485, 873340739, 
	1627747955, 1683315894, -1257054734, 
	1161594573, -1427794708, 648449375, 
	1030832345, -1089406083, 1545559989, 
	1407925421, -905406156, -1280911754, 
	-2001308758, 227385715, 520740534, 
	-1282033083, 1284654531, 1238828013, 
	148302191, -1656360560, -1139028545, 
	-704504193, 1367565640, -1213605929, 
	289692537, 2057687908, 684752482, 
	1301104665, 1933084912, 1134552268, 
	-1865611277, -757445071, -827902808, 
	-439546422, 1978388539, -997231600, 
	-1912768246, -922198660, 467538317, 
	-395047754, 146974967, 225791951, 
	-1521041638, 784853764, -586969394, 
	-465017921, 1258064203, 1500716775, 
	-250934705, -1117364667, -293202430, 
	-595719623, 376260241, -535874599, 
	-1939247052, 839033960, 1069014255, 
	-282157071, -22946408, -1178535916, 
	-537160644, -105327143, -316195415, 
	-571571219, 353753037, 1463803685, 
	1161010901, -404201378, 391199413, 
	405473472, 742889183, 452611239, 
	-1790628764, -1572764945, 917344354, 
	-1045176342, 1083902565, 1454280688, 
	-1995648000, 1037588263, 2145497403, 
	-1112202532, -1856081951, 938534213, 
	-1691760356, 2109781738, 1820109821, 
	1988028837, -2101599530, -926145485, 
	1852560250, 1917576092, -1377736232, 
	216878403, -1405586386, -544290439, 
	-382424683, -1477587186, -1488023165, 
	-1360589093, 25592369, 1695536505, 
	1821713859, -690132140, 1967963236, 
	1833534930, 74127397, -1987912089, 
	-586108586, -868646236, 1034085250, 
	527296915, 1954505506, 2083589286, 
	1608417972, 1461209721, 1669506543, 
	-140128717, 2089251682, -476684220, 
	1361481586, -543753628, 1954572638, 
	-1308070260, 1671930579, -922485963, 
	2047578920, -456758938, 1635678287, 
	-92864401, -457923115, 1647288373, 
	-852311725, 1513449752, 1503502780, 
	-98945231, -1537511900, -79182046, 
	635382674, 1144074041, -1850919743, 
	-2087056151, -1780458811, -582321540, 
	-1488230638, 2032974544, -1888292665, 
	205171085, 1986540608, -1362867570, 
	-358549401, -432582257, 2083465592, 
	440402956, 1274505014, -212283123, 
	865327044, -2051882447, -1521574964, 
	219506962, 2117666163, -45436631, 
	1947688981, -1094014751, -1712545352, 
	17866106, -1716024722, 1073364778, 
	625435084, -974600754, 505280162, 
	397970076, 643236003, 1046854828, 
	-944971290, -1297255861, -683254942, 
	721663995, 323535860, 1313747580, 
	-1802955617, -537520271, 1933763889, 
	1463929564, 268181342, -999074919, 
	-1399420127, 523583817, -844122414, 
	-1128805346, -1243791916, 593274684, 
	-82140258, -130641455, 2026672404, 
	841707801, -1597038831, 1654379730, 
	-1514952243, 400358679, 673293266, 
	839530185, 1371387967, 1469106392, 
	1646045314, 2112649705, -1727683438, 
	-555809424, 205081272, 748992652, 
	858137072, 1873953998, -884024544, 
	1336521232, -1480354168, 1238715379, 
	2009642630, 284719651, 1292482073, 
	1424572383, 971818364, -2069392610, 
	-1551855738, -1381069134, 1521291482, 
	-195336867, 
}, random_seed_thing2;
// basic garbage rand():
int GetRandom()
{
	//read the tsc:
	uint32_t hi, lo;
	GetTimeStampCounter(&hi, &lo);
	//combine the high and low numbers:
	
	hi ^= lo;
	
	//then mask it out so it wont look obvious:
	hi ^= 0xe671c4b4;
	
	//more masking
	hi ^= random_seed_thing[random_seed_thing2++];
	if (random_seed_thing2 >= (int)ARRAY_COUNT(random_seed_thing))
		random_seed_thing2 = 0;
	
	//then make it positive:
	hi &= 2147483647;
	
	//lastly, return.
	return (int) hi;
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
	LogMsg("\e[96mNanoShell Operating System " VersionString);
	LogMsg("\e[91mVersion Number: %d", VersionNumber);
	
	LogMsg("\e[97m-------------------------------------------------------------------------------");
	LogMsg("\e[94m[CPU] Name: %s", GetCPUName());
	LogMsg("\e[94m[CPU] x86 Family %d Model %d Stepping %d.  Feature bits: %d",
			g_cpuidFeatureBits.m_familyID, g_cpuidFeatureBits.m_model, g_cpuidFeatureBits.m_steppingID);
	LogMsg("\e[92m[RAM] PageSize: 4K. Physical pages: %d. Total usable RAM: %d Kb", MpGetNumAvailablePages(), MpGetNumAvailablePages()*4);
	LogMsg("\e[92m[VID] Screen resolution: %dx%d.  Textmode size: %dx%d characters, of type %d.", GetScreenSizeX(), GetScreenSizeY(), 
																						g_debugConsole.width, g_debugConsole.height, g_debugConsole.type);
	LogMsg("\e[97m");
}

void KePrintSystemInfo()
{
	//neofetch style:
	int npp = MpGetNumAvailablePages(), nfpp = MpGetNumFreePages();
	
	char timingInfo[128];
	timingInfo[0] = 0;
	FormatTime(timingInfo, FORMAT_TYPE_VAR, GetTickCount() / 1000);
	LogMsg("\e[93m N    N");
	LogMsg("\e[93m NN   N");
	LogMsg("\e[93m N N  N");
	LogMsg("\e[93m N  N N");
	LogMsg("\e[93m N   NN");
	LogMsg("\e[93m N    N\e[95m SSSS");
	LogMsg("\e[95m       S    S");
	LogMsg("\e[95m       S     ");
	LogMsg("\e[95m        SSSS ");
	LogMsg("\e[95m            S");
	LogMsg("\e[95m       S    S");
	LogMsg("\e[95m        SSSS ");
	
	//go up a bunch
	LogMsgNoCr("\e[A\e[A\e[A\e[A\e[A\e[A\e[A\e[A\e[A\e[A\e[A\e[A");
	
	LogMsg("\e[14G\e[91m OS:       \e[97mNanoShell Operating System");
	LogMsg("\e[14G\e[91m Kernel:   \e[97m%s (%d)", VersionString, VersionNumber);
	LogMsg("\e[14G\e[91m Uptime:   \e[97m%s", timingInfo);
	LogMsg("\e[14G\e[91m CPU:      \e[97m%s", GetCPUName());
	LogMsg("\e[14G\e[91m CPU type: \e[97m%s", GetCPUType());
	LogMsg("\e[14G\e[91m Memory:   \e[97m%d KB / %d KB", (npp-nfpp)*4, npp*4);
	LogMsg("\e[14G\e[91m ");
	LogMsg("\e[14G\e[91m ");
	LogMsg("\e[14G\e[91m ");
	LogMsg("\e[14G\e[91m ");
	LogMsg("\e[14G\e[91m ");
	LogMsg("\e[14G\e[91m ");
	LogMsg("\e[0m");
}
#endif

// Mini clock
#if 1
const int g_mini_clock_cosa[]={    0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,   0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995,-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105 };
const int g_mini_clock_sina[]={-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105,   0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,    0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995 };

__attribute__((always_inline))
static inline void RenderThumbClockHand(int deg, int len, int cenX, int cenY, unsigned color)
{
	int begPointX = cenX,                                         begPointY = cenY;
	int endPointX = cenX + (g_mini_clock_cosa[deg] * len / 1000), endPointY = cenY + (g_mini_clock_sina[deg] * len / 1000);
	VidDrawLine (color, begPointX, begPointY, endPointX, endPointY);
}
void RenderThumbClock(int x, int y, int size)//=32
{
	if (size == 16) return;
	//render simple clock:
	TimeStruct* time = TmReadTime();
	
	int centerX = x + size/2, centerY = y + size/2;
	int diameter = size;
	int handMaxLength = (2 * diameter / 5);
	
	RenderThumbClockHand(time->hours % 12 * 5 + time->minutes / 12, 4 * handMaxLength / 9, centerX, centerY, 0xFF0000);
	RenderThumbClockHand(time->minutes,                             6 * handMaxLength / 9, centerX, centerY, 0x000000);
	RenderThumbClockHand(time->seconds,                             8 * handMaxLength / 9, centerX, centerY, 0x000000);
}

#endif

// Formerly in main.c
#if 1

multiboot_info_t *g_pMultibootInfo;

char g_cmdline [1024];

void KePrintSystemVersion()
{
	LogMsg("NanoShell (TM), December 2022 - " VersionString);
	LogMsg("[%d Kb System Memory, %d Kb Usable Memory]", g_pMultibootInfo->mem_upper, MpGetNumAvailablePages() * 4);
	LogMsg("Built on: %s %s", __DATE__, __TIME__);
}
void MbSetup (uint32_t check, uint32_t mbaddr)
{
	if (check != 0x2badb002)
	{
		ILogMsg("NanoShell has not booted from a Multiboot-compatible bootloader.  A bootloader such as GRUB is required to run NanoShell.");
		KeStopSystem();
	}
	
	// Read the multiboot data:
	multiboot_info_t *mbi = (multiboot_info_t *)(mbaddr + KERNEL_BASE_ADDRESS);
	g_pMultibootInfo = mbi;
}
multiboot_info_t* KiGetMultibootInfo()
{
	return g_pMultibootInfo;
}
void MbReadCmdLine ()
{
	uint32_t cmdlineaddr = g_pMultibootInfo->cmdline;
	if (!(g_pMultibootInfo->flags & MULTIBOOT_INFO_CMDLINE))
	{
		strcpy (g_cmdline, "None!");
	}
	else if (cmdlineaddr < 0x800000)
	{
		strcpy (g_cmdline, ((char*)0xC0000000 + cmdlineaddr));
	}
	else
	{
		strcpy (g_cmdline, "None!");
	}
}
void MbCheckMem()
{
	if (g_pMultibootInfo->mem_upper < 15360)
	{
		SwitchMode(0);
		CoInitAsText(&g_debugConsole);
		ILogMsg("NanoShell has not found enough extended memory.	16Mb of extended "
		        "memory is\nrequired to run NanoShell.    You may need to upgrade "
		        "your computer.");
		KeStopSystem();
	}
}
void MbCheckCmdLine()
{
	if (strcmp (g_cmdline, "None!") == 0 || g_cmdline[0] == 0)
	{
		ILogMsg("NanoShell cannot boot, because either:");
		ILogMsg("- no cmdline was passed");
		ILogMsg("- cmdline's address was %x%s", g_pMultibootInfo->cmdline, g_pMultibootInfo->cmdline >= 0x100000 ? " (was bigger than 8 MB)" : "");
		KeStopSystem();
	}
}
bool KiEmergencyMode()
{
	bool textMode = true;
	ConfigEntry *pEntry = CfgGetEntry ("emergency");
	if (pEntry)
	{
		if (strcmp(pEntry->value, "yes") == 0)
		{
			ILogMsg("Using emergency text mode");
		}
		else
		{
			textMode = false;
		}
	}
	else
	{
		ILogMsg("No 'emergency' config key found, using text mode");
	}
	return textMode;
}
void KiLaunch (TaskedFunction func)
{
	int err_code = 0;
	//TODO: spawn a process instead
	Task* pTask = KeStartTaskD(func, 0, &err_code, __FILE__, "Init", __LINE__);
	
	if (!pTask)
		KeBugCheck(BC_EX_INIT_NOT_SPAWNABLE, NULL);
	
	KeTaskAssignTag(pTask, "System");
	
	// And take off!
	KeUnsuspendTask(pTask);
}
#endif
