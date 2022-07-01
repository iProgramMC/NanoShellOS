/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       SoundBlaster 16 driver module
******************************************/
#ifndef _SB_H
#define _SB_H

#include <main.h>

void SbInit();
void SbWriteData (const void *pBuffer, size_t sz);
void* SbTestGenerateSound(size_t *outSize);

#endif//_SB_H