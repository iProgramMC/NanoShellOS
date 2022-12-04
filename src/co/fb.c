/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

      VBE framebuffer console module 
******************************************/

#include <main.h>
#include <console.h>
#include <video.h>
#include <string.h>
#include "../mm/memoryi.h"

#define USE_SHIFT_SCREEN 1

uint16_t TextModeMakeChar(uint8_t fgbg, uint8_t chr);

extern uint32_t g_vgaColorsToRGB [];
extern bool g_uses8by16Font;
extern VBEData* g_vbeData, g_mainScreenVBEData;

void CoVbeClearScreen(Console *this)
{
	VidFillScreen (g_vgaColorsToRGB[this->color >> 4]);
	memset_shorts(this->textBuffer, TextModeMakeChar(this->color, 0), this->width * this->height);
}

void CoVbePlotChar (Console *this, int x, int y, char c)
{
	if (x < 0 || y < 0 || x >= this->width || y >= this->height) return;
	this->m_dirty = true;
	
	uint16_t chr = TextModeMakeChar (this->color, c);
	this->textBuffer [x + y * this->width] = chr;
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	
	VidPlotChar (c, this->offX + (x << 3), this->offY + (y << (3 + (g_uses8by16Font))), g_vgaColorsToRGB[this->color & 0xF], g_vgaColorsToRGB[this->color >> 4]);
	g_vbeData = backup;
}

void CoVbeRefreshChar (Console *this, int x, int y)
{
	uint16_t cd = this->textBuffer[y * this->width + x];
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	
	VidPlotChar(cd & 0xFF, this->offX + (x << 3), this->offY + (y << (3 + (g_uses8by16Font))), g_vgaColorsToRGB[(cd >> 8) & 0xF], g_vgaColorsToRGB[cd >> 12]);
	g_vbeData = backup;
}

void CoVbeScrollUpByOne(Console *this)
{
	if (this->pushOrWrap)
	{
		CoClearScreen(this);
		this->curX = this->curY = 0;
	}
	else
	{
		//NOTE: This is actually slower than the VidShiftScreen method on qemu... probably because I have to re-render a lot.
	#if USE_SHIFT_SCREEN
		VidShiftScreen(1 << (3 + g_uses8by16Font));
	#else
		for (int y = 0; y < this->height - 1; y++)
		{
			for (int x = 0; x < this->width; x++)
			{
				//if there's any difference...
				uint16_t* dst = &this->textBuffer[x + (y + 0) * this->width];
				uint16_t* src = &this->textBuffer[x + (y + 1) * this->width];
				
				if (*dst != *src)
				{
					*dst = *src;
					CoVbeRefreshChar(this, x, y);
				}
			}
		}
	#endif
		
		for (int x = 0; x < this->width; x++)
		{
			CoVbePlotChar (this, x, this->height - 1, '\0');
		}
	}
}

void CoVbeUpdateCursor(UNUSED Console* this)
{
	
}

void CoVbeInit(Console *this)
{
	this->curX = this->curY = 0;
	this->width = GetScreenSizeX() / 8;
	this->height = GetScreenSizeY() / (g_uses8by16Font ? 16 : 8);
	this->color = DefaultConsoleColor;//default
	this->pushOrWrap = 0;//push
	
	// setup a text buffer
	bool bIntsDisabled = KeCheckInterruptsDisabled();
	if (!bIntsDisabled)
		cli;
	
	this->textBuffer = MhAllocate(sizeof(short) * this->width * this->height, NULL);
	if (!this->textBuffer)
	{
		VidTextOut("ERROR: Could not initialize fullscreen console. :^(", 0, 0, 0xFFFFFF, 0x000000);
		KeStopSystem();
	}
	
	if (!bIntsDisabled)
		sti;
	
	CoVbeClearScreen (this);
}

void CoVbeKill(Console* this)
{
	if (this->textBuffer)
	{
		MmFree(this->textBuffer);
		this->textBuffer = NULL;
	}
}
