/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

      Floating point hardware module
******************************************/
#include <fpu.h>

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
