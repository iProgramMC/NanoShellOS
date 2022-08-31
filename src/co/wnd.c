/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

           Window Console module 
******************************************/

#include <main.h>
#include <console.h>
#include <video.h>
#include <window.h>

uint16_t TextModeMakeChar(uint8_t fgbg, uint8_t chr);

extern VBEData* g_vbeData;

void CoWndClearScreen(Console *this)
{
	for (int y = 0; y < this->height; y++)
		for (int x = 0; x < this->width; x++)
			CoPlotChar(this, x, y, 0);
}
void CoWndPlotChar (Console *this, int x, int y, char c)
{
	if (x < 0 || y < 0 || x >= this->width || y >= this->height) return;
	this->m_dirty = true;
	
	g_vbeData = this->m_vbeData;
	
	uint16_t chr = TextModeMakeChar (this->color, c);
	this->textBuffer [x + y * this->width] = chr;
}
void CoWndRefreshChar (Console *this, int x, int y)
{
	VBEData* backup = g_vbeData;
	g_vbeData = this->m_vbeData;
	uint16_t cd = this->textBuffer[y * this->width + x];
	
	VidPlotChar (cd & 0xFF, this->offX + x * this->cwidth, this->offY + y  * this->cheight, g_vgaColorsToRGB[(cd>>8) & 0xF], g_vgaColorsToRGB[cd >> 12]);
	g_vbeData = backup;
}
void CoWndScrollUpByOne(Console *this)
{
	if (this->pushOrWrap)
	{
		this->curX = this->curY = 0;
		return;
	}
	memcpy (this->textBuffer, &this->textBuffer[this->width], this->width * (this->height - 1) * sizeof(short));
	for (int i = 0; i < this->width; i++)
	{
		CoPlotChar (this, i, this->height - 1, 0);
	}
}
void CoWndUpdateCursor(Console* this)
{
	
}
void CoWndInit(Console *this)
{
}
void CoWndKill(Console *this)
{
	if (this->textBuffer)
	{
		MmFree(this->textBuffer);
		this->textBuffer = NULL;
	}
}
