/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

      VBE framebuffer console module 
******************************************/

#include <main.h>
#include <console.h>
#include <video.h>
#include <window.h>

extern uint32_t g_vgaColorsToRGB [];
extern bool g_uses8by16Font;
extern VBEData* g_vbeData, g_mainScreenVBEData;

void CoVbeClearScreen(Console *this)
{
	VidFillScreen (g_vgaColorsToRGB[this->color >> 4]);
}
void CoVbePlotChar (Console *this, int x, int y, char c)
{
	if (x < 0 || y < 0 || x >= this->width || y >= this->height) return;
	this->m_dirty = true;
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	this->offX = this->offY = 0;
	VidPlotChar (c, this->offX + (x << 3), this->offY + (y << (3 + (g_uses8by16Font))), g_vgaColorsToRGB[this->color & 0xF], g_vgaColorsToRGB[this->color >> 4]);
	g_vbeData = backup;
}
void CoVbeRefreshChar (UNUSED Console *this, UNUSED int x, UNUSED int y)
{
	
}
void CoVbeScrollUpByOne(UNUSED Console *this)
{
	if (this->pushOrWrap)
	{
		CoClearScreen(this);
		this->curX = this->curY = 0;
	}
	else
	{
		int htChar = 1 << (3 + g_uses8by16Font);
		VidShiftScreen (htChar);
		VidFillRect (g_vgaColorsToRGB[this->color >> 4], 0, (this->height - 1) * htChar, GetScreenSizeX() - 1, GetScreenSizeY() - 1);
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
	CoVbeClearScreen (this);
}

