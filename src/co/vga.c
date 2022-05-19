/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

       VGA text mode console module 
******************************************/

#include <main.h>
#include <console.h>
#include <video.h>
#include <window.h>

extern uint16_t* g_pBufferBase; // = 0xB8000
extern int g_textWidth, g_textHeight;
uint16_t TextModeMakeChar(uint8_t fgbg, uint8_t chr);

void CoVgaClearScreen(Console *this)
{
	uint16_t emptyChar = TextModeMakeChar(this->color, '\0');
	for (int i = 0; i < this->width * this->height; i++)
		this->textBuffer[i] = emptyChar;
}
void CoVgaPlotChar (Console *this, int x, int y, char c)
{
	if (x < 0 || y < 0 || x >= this->width || y >= this->height) return;
	this->m_dirty = true;
	uint16_t chara = TextModeMakeChar (this->color, c);
	this->textBuffer [x + y * this->width] = chara;
}
void CoVgaRefreshChar (UNUSED Console *this, UNUSED int x, UNUSED int y)
{
	
}
void CoVgaScrollUpByOne(Console *this)
{
	if (this->pushOrWrap)
	{
		this->curX = this->curY = 0;
		return;
	}
	
	memcpy (this->textBuffer, &this->textBuffer[this->width], this->width * (this->height - 1) * sizeof(short));
	for (int i=0; i<this->width; i++)
	{
		CoPlotChar (this, i, this->height - 1, 0);
	}
}
void CoVgaUpdateCursor(Console* this)
{
	uint16_t cursorLocation = this->curY * this->width + this->curX;
	WritePort(0x3d4, 14);
	WritePort(0x3d5, cursorLocation >> 8);
	WritePort(0x3d4, 15);
	WritePort(0x3d5, cursorLocation);
}
void CoVgaInit(Console *this)
{
	this->curX = this->curY = 0;
	this->width  = g_textWidth;
	this->height = g_textHeight;
	this->textBuffer = g_pBufferBase;
	this->pushOrWrap = 0;//push
	
	CoVgaClearScreen(this);
}

