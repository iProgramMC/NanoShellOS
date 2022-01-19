/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

       VGA Video module header file
******************************************/
#ifndef _VGA_H
#define _VGA_H

extern void WriteFont8px  (const unsigned char* buf); // KERNEL.ASM!!
extern void WriteFont16px (const unsigned char* buf); // KERNEL.ASM!!
void SwitchMode(bool _80x50);

#endif//_VGA_H