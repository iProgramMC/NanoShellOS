#include "doomgeneric.h"

uint32_t* DG_ScreenBuffer = 0;
extern uint32_t g_ebpSave, g_espSave;

bool dg_Create()
{
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	return DG_Init();
}

