/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       SoundBlaster 16 driver module
******************************************/
#ifndef _SB_H
#define _SB_H

#include <main.h>

void SbInit();
void SbSoundWave (uint8_t index, uint8_t wave);
void SbSoundVolume (uint8_t index, uint8_t v);
void SbSoundVolumeMaster (uint8_t v);
void SbSoundNote (uint8_t index, uint8_t octave, uint8_t note);

#endif//_SB_H