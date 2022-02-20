/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

        Console module header file
******************************************/
#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <video.h>

//#define DefaultConsoleColor 0x1F
#define DefaultConsoleColor 0x0F

enum ConsoleType {
	CONSOLE_TYPE_NONE, // uninitialized
	CONSOLE_TYPE_TEXT, // always full screen
	CONSOLE_TYPE_FRAMEBUFFER, // can either be the entire screen or just a portion of it. TODO
	CONSOLE_TYPE_SERIAL, // just plain old serial
	CONSOLE_TYPE_E9HACK, // Port E9 hack - qemu and bochs support this.
	CONSOLE_TYPE_WINDOW,
};

#define KB_BUF_SIZE 512

typedef struct ConsoleStruct {
	int  type; // ConsoleType enum
	int  width, height; // width and height
	uint16_t *textBuffer; // unused in fb mode
	uint16_t color; // colors
	int  curX, curY; // cursor X and Y positions
	bool pushOrWrap;// check if we should push whole screen up, or clear&wrap
	VBEData* m_vbeData;//vbe data to switch to when drawing, ONLY APPLIES TO CONSOLE_TYPE_WINDOW!!
	int  offX, offY;
	int  font;
	int  cwidth, cheight;
	bool m_dirty;
	char m_inputBuffer[KB_BUF_SIZE];
	int  m_inputBufferBeg, m_inputBufferEnd;
	int  m_cursorFlashTimer, m_cursorFlashState;
} Console;

extern Console g_debugConsole; // for LogMsg
extern Console g_debugSerialConsole; // for SLogMsg
extern uint32_t g_vgaColorsToRGB[];

void CoPlotChar (Console *this, int x, int y, char c);
void CoScrollUpByOne (Console *this);
void CoClearScreen (Console *this);
void CoPrintChar (Console* this, char c);
void CoPrintString (Console* this, const char *c);
void CoInitAsText (Console* this);
void CoInitAsGraphics (Console* this);
void CoInitAsSerial (Console* this);
void CoInitAsE9Hack (Console *this);
void CoAddToInputQueue     (Console *this, char input);
bool CoAnythingOnInputQueue(Console* this);
char CoReadFromInputQueue  (Console* this);
void CLogMsg (Console *this, const char* fmt, ...);
void CLogMsgNoCr (Console *this, const char* fmt, ...);
void LogMsg (const char* fmt, ...);
void LogMsgNoCr (const char* fmt, ...);
void SLogMsg (const char* fmt, ...);
void SLogMsgNoCr (const char* fmt, ...);
void LogHexDumpData (void* pData, int size);
void ResetConsole();
void SetConsole(Console* pConsole);

// on current console:

bool CoInputBufferEmpty();
char CoGetChar   ();
void CoGetString (char* buffer, int buffer_size);

#endif//_CONSOLE_H