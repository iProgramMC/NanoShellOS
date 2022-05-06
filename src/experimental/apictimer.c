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
__attribute__((optnone))
void ApicCalibrate()
{
	// Tell APIC to use divider 16
	LapicWrite(APIC_REGISTER_TIMER_DIV, 0x3);
	
	// Prepare the PIT to sleep for 50 ms
	int tc = KiGetPitTickCnt() + 50;
	
	// Set APIC init counter to -1
	LapicWrite(APIC_REGISTER_TIMER_INITCNT, 0xFFFFFFFF);
	
	// Perform PIT-supported sleep.
	while (KiGetPitTickCnt() < tc)
	{
		__asm__ volatile ("hlt" ::: "memory");
	}
	
	uint32_t old_config = LapicRead(APIC_REGISTER_LVT_TIMER);
	
	// Stop the APIC timer
	LapicWrite(APIC_REGISTER_LVT_TIMER, old_config | APIC_LVT_INT_MASKED);
	
	uint32_t ticks_per_50ms = 0xFFFFFFFF - LapicRead(APIC_REGISTER_TIMER_CURRCNT);
	
	LogMsg("Ticks per 50 ms: %d", ticks_per_50ms);
	
	// Mask IRQ 0
	gPicMask1 |= 0x01;
	i8259UpdateMasks();
	
	g_bUseLapicInstead = true;
	
	// Start timer as periodic on IRQ 0, divider 16, with the tickNum we counted:
	LapicWrite(APIC_REGISTER_LVT_TIMER,  0x20 | APIC_LVT_TIMER_MODE_PERIODIC);
	LapicWrite(APIC_REGISTER_TIMER_DIV,  0x3);
	LapicWrite(APIC_REGISTER_LVT_TIMER,  ticks_per_50ms / 5); // 10 millis instead. Used 50 to make it maybe more accurate
	
	// Acknowledge any interrupts that you need to acknowledge
	g_pLapic[APIC_REGISTER_EOI] = 0;
	
	// Re-start the APIC timer
	LapicWrite(APIC_REGISTER_LVT_TIMER, old_config & ~APIC_LVT_INT_MASKED);
}
void ApicEnable()
{
	// Hardware enable the LAPIC if not enabled already
	uintptr_t p = GetCpuApicBase();
	g_pLapic = MmMapPhysMemFast (p);
	
	SetCpuApicBase(p);
	
	// set the spurious interrupt to 0x7
	LapicWrite (APIC_REGISTER_SPURIOUS, 0x100 | 0x07);
	
	ApicCalibrate();
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
