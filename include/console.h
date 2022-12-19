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
	CONSOLE_TYPE_FILE,
};

#define KB_BUF_SIZE 512

enum {
	ANSI_ATTR_OFF         = 0,
	ANSI_ATTR_BOLD        = 1,
	ANSI_ATTR_DIM         = 2,
	ANSI_ATTR_ITALIC      = 3,
	ANSI_ATTR_UNDERLINE   = 4,
	ANSI_ATTR_BLINK       = 5, // only supported by the VGA text mode driver?
	ANSI_ATTR_BLINK_FAST  = 6,
	ANSI_ATTR_NEGATIVE    = 7,
	ANSI_ATTR_INVISIBLE   = 8,
	ANSI_ATTR_STRIKETHRU  = 9,
	ANSI_ATTR_PRIMFONT    = 10,
	ANSI_ATTR_ALTFONTST   = 11, // Alternative font start
	ANSI_ATTR_DOUBLEUNDER = 21,
	ANSI_ATTR_NOBOLD      = 22, // Normal intensity - no bold or faint
	ANSI_ATTR_NOITALIC    = 23, // Normal slant - no italic
	ANSI_ATTR_NOUNDERLINE = 24,
	ANSI_ATTR_NOBLINK     = 25,
	ANSI_ATTR_PROPORTSP   = 26,
	ANSI_ATTR_NONEGATIVE  = 27,
	ANSI_ATTR_NOINVISIBLE = 28,
	ANSI_ATTR_NOSTRIKETHR = 29,
	ANSI_ATTR_SETFGCOLORS = 30, // Set foreground color (start, until 37)
	ANSI_ATTR_SETFGCOLORR = 38, // Set foreground color as (5;n) or (2;r;g;b). Won't support?
	ANSI_ATTR_SETFGCOLORD = 39, // Set foreground color as DefaultConsoleColor
	ANSI_ATTR_SETBGCOLORS = 40, // Set background color (start, until 47)
	ANSI_ATTR_SETBGCOLORR = 48, // Set background color as (5;n) or (2;r;g;b). Won't support?
	ANSI_ATTR_SETBGCOLORD = 49, // Set foreground color as DefaultConsoleColor
	ANSI_ATTR_NOPROPORTSP = 50,
	ANSI_ATTR_FRAMED      = 51,
	ANSI_ATTR_CIRCLED     = 52,
	ANSI_ATTR_OVERLINED   = 53,
	ANSI_ATTR_NOFRAMECIRC = 54, // No frame or encircle
	ANSI_ATTR_UNDERLINECL = 58, // Set underline color as (5;n) or (2;r;g;b)
	ANSI_ATTR_SETFGCOLRLS = 90, // Set foreground bright color (start, until 97)
	ANSI_ATTR_SETBGCOLRLS = 100,// Set background bright color (start, until 107)
	
};

// ANSI attribute flags
enum
{
	ANSI_FLAG_NEGATIVE  = (1 << 0),
	ANSI_FLAG_INVISIBLE = (1 << 1),
	ANSI_FLAG_BOLD      = (1 << 2),
	ANSI_FLAG_BLINK     = (1 << 3),
	ANSI_FLAG_DIM       = (1 << 4),
	ANSI_FLAG_ITALIC    = (1 << 5),
	ANSI_FLAG_STRIKETHR = (1 << 6),
	ANSI_FLAG_FG_COLOR  = (1 << 7),
	ANSI_FLAG_BG_COLOR  = (1 << 8),
	//WORK: Add more here
};

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
	int  m_scrollY, m_actualHeight;//for wterm
	char m_ansiEscCode[64];
	bool m_usingAnsiEscCode;
	uint32_t m_ansiAttributes;
	uint8_t  m_ansiBgColorBackup, m_ansiFgColorBackup;
	int  lastX, lastY; // for wterm
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
void CoKill(Console *this);
void CoAddToInputQueue     (Console *this, char input);
bool CoAnythingOnInputQueue(Console* this);
char CoReadFromInputQueue  (Console* this);
void LogHexDumpData (void* pData, int size);
void ResetConsole();
void SetConsole(Console* pConsole);

// on current console:

bool CoInputBufferEmpty();
char CoGetChar   ();
void CoGetString (char* buffer, int buffer_size);

void CoKickOff(); // main.c calls this

// The LogMsg family

// CLogMsg - Log a message on any console object.
void CLogMsg (Console *this, const char* fmt, ...);
void CLogMsgNoCr (Console *this, const char* fmt, ...);

// LogMsg - Log a message to the current console. Must have interrupts enabled.
void LogMsg (const char* fmt, ...);
void LogMsgNoCr (const char* fmt, ...);

// LogMsg - Log a message to the screen. Can have interrupts disabled.
void ILogMsg (const char* fmt, ...);
void ILogMsgNoCr (const char* fmt, ...);

// SLogMsg - Log a message to the debug console (0xE9 port). Can have interrupts disabled.
void SLogMsg (const char* fmt, ...);
void SLogMsgNoCr (const char* fmt, ...);

#endif//_CONSOLE_H