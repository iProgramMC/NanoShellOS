/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

      Floating point hardware module
******************************************/
#include <fpu.h>

void KiFpuInit()
{
	asm("clts");
	
	size_t cr0, cr4;
	asm("mov %%cr0, %0" : "=r"(cr0));
	
	if (cr0 & (1 << 2))
	{
		LogMsg("Warning: no FPU hardware detected!");
	}
	
	cr0 &= ~(1<<2);//Turn off the EM bit
	cr0 |=  (1<<1);//Turn on the MP bit
	asm("mov %0, %%cr0" :: "r"(cr0));
	asm("mov %%cr4, %0" : "=r"(cr4));
	
	//Enable OSFXSR and OSXMMEXCPT
	cr4 |= 3 << 9;
	//Enable OSXSAVE
	//cr4 |= 1 << 18;
	
	asm("mov %0, %%cr4" :: "r"(cr4));
	
	asm("fninit");
}

extern int Round(float f);

double FltAbs (double x)
{
	return x < 0.0 ? -x : x;
}

double FltMod (double x, double m)
{
	double result;
	asm (
		"1:\n\t"
		"fnstsw %%ax\n\t"
		"sahf\n\t"
		"jp 1b"
		: "=t"(result) : "0"(x), "u"(m) : "ax", "cc"
	);
	return result;
}

double FltSin (double x)
{
	double result;
	asm ("fsin" : "=t"(result) : "0"(x));
	return result;
}
double FltCos (double x)
{
	return FltSin(x + M_PI / 2.0);
}
