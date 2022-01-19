/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

Floating point hardware module header file
******************************************/
#ifndef _FPU_H
#define _FPU_H

#include <main.h>

void KiFpuInit();

#define M_E  2.71828182845904523536f
#define M_PI 3.14159265358979323846f

int round(float f);
int floor(float f);

//TODO: these functions are slightly broken atm
//float roundf(float f);
//float floorf(float f);

#endif//_FPU_H