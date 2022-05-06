#include <main.h>
#include <memory.h>
#ifdef EXPERIMENTAL_APICTIMER

#define CPUID_FEAT_EDX_MSR          (1 << 5)
#define CPUID_FEAT_EDX_APIC         (1 << 9)

#define IA32_APIC_BASE_MSR          0x1B
#define IA32_APIC_BASE_MSR_BSP      0x100
#define IA32_APIC_BASE_MSR_ENABLE   0x800

/**
 * Since amd64_apic_base is an array of 32-bit elements, these byte-offsets
 * need to be divided by 4 to index the array.
 * Thanks https://github.com/nilhoel1/rtemsPRU/blob/master/bsps/x86_64/amd64/include/apic.h
 */
#define APIC_OFFSET(val) (val >> 2)

#define APIC_REGISTER_APICID         APIC_OFFSET(0x20)
#define APIC_REGISTER_EOI            APIC_OFFSET(0x0B0)
#define APIC_REGISTER_SPURIOUS       APIC_OFFSET(0x0F0)
#define APIC_REGISTER_LVT_TIMER      APIC_OFFSET(0x320)
#define APIC_REGISTER_TIMER_INITCNT  APIC_OFFSET(0x380)
#define APIC_REGISTER_TIMER_CURRCNT  APIC_OFFSET(0x390)
#define APIC_REGISTER_TIMER_DIV      APIC_OFFSET(0x3E0)

#define APIC_LVT_INT_MASKED          0x10000
#define APIC_LVT_TIMER_MODE_PERIODIC 0x20000
#define APIC_SPURIOUS_ENABLE         0x00100

#define APIC_TIMER_DIVIDER           0x00004

#define PIT_CHANNEL2_TIMER           1
#define PIT_CHANNEL2_SPEAKER         2

extern uint8_t gPicMask1;
extern bool    g_bUseLapicInstead;

void     *MmMapPhysMemFast(uint32_t page);
void      MmUnmapPhysMemFast(void* pMem);
uint32_t  GetCPUFeatureBitsEdx();
void      i8259UpdateMasks();
void      SetupSoftInterrupt (int intNum, void *pIsrHandler);
void      IrqLapicTimerA();
int       KiGetPitTickCnt();

void GetCpuMsr(uint32_t msr, uint32_t* lo, uint32_t* hi)
{
	asm ("rdmsr":  "=a"(*lo), "=d"(*hi) : "c"(msr));
}
void SetCpuMsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
	asm ("wrmsr":  : "a"(lo), "d"(hi),  "c"(msr));
}
volatile uint32_t* g_pLapic = NULL;
uintptr_t GetCpuApicBase()
{
	uint32_t a,d;
	GetCpuMsr(IA32_APIC_BASE_MSR, &a, &d);
	
	return(a & 0xFFFFF000);
}
void SetCpuApicBase(uintptr_t apic)
{
	uint32_t a = (uint32_t)(apic & 0xFFFFF000) | IA32_APIC_BASE_MSR_ENABLE,
			 d = 0;
	SetCpuMsr(IA32_APIC_BASE_MSR, a, d);
}
uint32_t LapicRead(int index)
{
	return g_pLapic[index];
}
int LapicWrite(int index, int value)
{
	g_pLapic[index] = value;
	
	return LapicRead(APIC_REGISTER_APICID);
}
void ApicEoi()
{
	LapicWrite(APIC_REGISTER_EOI, 0x00);
}
#define NUM_CALIBS          10
#define PIT_CALIBRATE_TICKS 11931
__attribute__((optnone))
uint32_t ApicTimerCalibrateOnce()
{
	// Configure APIC timer in one shot mode to prep for calibration
	LapicWrite(APIC_REGISTER_LVT_TIMER,  0x20);
	LapicWrite(APIC_REGISTER_TIMER_DIV,  APIC_TIMER_DIVIDER);
	
	// Enable PIT channel 2 timer gate, and disable speaker output
	uint8_t c2val = (ReadPort(0x61) & ~PIT_CHANNEL2_SPEAKER) | PIT_CHANNEL2_TIMER;
	
	WritePort (0x61, c2val);
	
	// initialize PIT in one-shot mode on Channel 2
	WritePort (0x43, (2 << 6) | (3 << 4) | (2 << 0));//TODO: document this
	
	// Disable interrupts while calibrating
	cli;
	
	// Set PIT reload value
	uint32_t pit_ticks = PIT_CALIBRATE_TICKS;
	
	WritePort(0x42, (uint8_t)( pit_ticks       & 0xff));
	WritePort(0x42, (uint8_t)((pit_ticks >> 8) & 0xff));
	
	// Restart PIT by disabling gated input and re-enabling it
	c2val &= ~1;
	WritePort (0x43, c2val);
	c2val |= 1;
	WritePort (0x43, c2val);
	
	// Start countdown!
	LapicWrite(APIC_REGISTER_TIMER_INITCNT, 0xFFFFFFFF);
	
	uint16_t pitTicks = 0;
	while (pitTicks <= PIT_CALIBRATE_TICKS)
	{
		WritePort (0x43, 2 << 6);
		
		pitTicks  = ReadPort(0x42);
		pitTicks |= ReadPort(0x42) << 8;
	}
	
	uint32_t apicCurrent = LapicRead(APIC_REGISTER_TIMER_CURRCNT);
	
	LogMsg("PIT stopped at %d ticks", pit_ticks);
	
	// Stop APIC timer
	LapicWrite(APIC_REGISTER_LVT_TIMER, APIC_LVT_INT_MASKED);
	
	// Get counts passed
	uint32_t apicTicksPerSec = 0xFFFFFFFF - apicCurrent;
	
	LogMsg("APIC ticks passed in 1/%d of a second: %d", 1193182/PIT_CALIBRATE_TICKS,
		apicTicksPerSec);
	
	sti;
	
	// Stop the PIT
	c2val &= ~1;
	WritePort (0x43, c2val);
	
	return apicTicksPerSec;
}
void ApicTimerInitialize(uint32_t desired_freq_hz)
{
	uint32_t ticks_total = 0;
	for (int i = 0; i < NUM_CALIBS; i++)
	{
		ticks_total += ApicTimerCalibrateOnce();
	}
	
	ticks_total /= NUM_CALIBS;
	
	LogMsg("Average time: %d ticks/s", ticks_total);
	
	uint32_t reload_value = ticks_total / desired_freq_hz;
	
	LapicWrite(APIC_REGISTER_LVT_TIMER,     0x20 | APIC_LVT_TIMER_MODE_PERIODIC);
	LapicWrite(APIC_REGISTER_TIMER_DIV,     APIC_TIMER_DIVIDER);
	LapicWrite(APIC_REGISTER_TIMER_INITCNT, reload_value);
}
void ApicEnable()
{
	// Hardware enable the APIC if not enabled already
	uintptr_t p = GetCpuApicBase();
	g_pLapic = MmMapPhysMemFast (p);
	
	SetCpuApicBase(p);
	
	// Software enable the APIC by setting enable bit and the spurious IRQ vector (0xFF)
	LapicWrite(APIC_REGISTER_SPURIOUS, APIC_SPURIOUS_ENABLE | 0xFF);
	
	ApicTimerInitialize(1000);
}
void ApicTimerInit()
{
	if (!(GetCPUFeatureBitsEdx() & CPUID_FEAT_EDX_APIC))
	{
		LogMsg("CPUID does not state the existence of an APIC, I think we should not continue");
	}
	if (!(GetCPUFeatureBitsEdx() & CPUID_FEAT_EDX_MSR))
	{
		LogMsg("CPUID does not state the existence of an MSR, I think we should not continue");
	}
	
	LogMsg("Enable APIC");
	ApicEnable();
}

#endif
