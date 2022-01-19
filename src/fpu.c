/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

      Floating point hardware module
******************************************/
#include <fpu.h>

float test(float pi)
{
	return pi * 2;	
}

void KiFpuInit()
{
	asm("clts");
	
	size_t cr0, cr4;
	asm("mov %%cr0, %0" : "=r"(cr0));
	
	if (cr0 & (1 << 2))
	{
		LogMsg("Warning: no FPU hardware detected!");
	}
	
	cr0 &= ~(1<<2);
	cr0 |=  (1<<1);
	asm("mov %0, %%cr0" :: "r"(cr0));
	asm("mov %%cr4, %0" : "=r"(cr4));
	
	//some SSE shit and fast FPU save&restore
	//TODO: maybe not necessary?
	cr4 |= 3 << 9;
	
	asm("mov %0, %%cr4" :: "r"(cr4));
	
	asm("fninit");
}

extern int Round(float f);

void FpuTest()
{
	LogMsg("test: %d", test(M_PI));
	LogMsg("rounded: %d", round(3.6f));
	LogMsg("floored: %d", floor(3.6f));
	//LogMsg("roundedf: %d", roundf(3.6f));
	//LogMsg("flooredf: %d", floorf(3.6f));
}

