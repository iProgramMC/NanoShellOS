/*****************************************
		NanoShell Operating System
		(C)2021-2022 iProgramInCpp

              Console module
******************************************/
#include <stdint.h>
#include <main.h>
#include <string.h>
#include <print.h>
#include <console.h>
#include <video.h>

uint16_t TextModeMakeChar(uint8_t fgbg, uint8_t chr) {
	uint8_t comb = fgbg;
	uint16_t comb2 = comb << 8 | chr;
	return comb2;
}

uint32_t g_vgaColorsToRGB[] = {
	0x00000000,
	0x000000AA,
	0x0000AA00,
	0x0000AAAA,
	0x00AA0000,
	0x00AA00AA,
	0x00AAAA00,
	0x00AAAAAA,
	0x00555555,
	0x005555FF,
	0x0055FF55,
	0x0055FFFF,
	0x00FF5555,
	0x00FF55FF,
	0x00FFFF55,
	0x00FFFFFF,
};

extern bool g_uses8by16Font;
Console g_debugConsole; // for LogMsg
Console g_debugSerialConsole; // for SLogMsg

Console* g_currentConsole = &g_debugConsole;

uint16_t* g_pBufferBase = (uint16_t*)(KERNEL_MEM_START + 0xB8000);
int g_textWidth = 80, g_textHeight = 25;

void SetConsole(Console* pConsole)
{
	g_currentConsole = pConsole;
}

void ResetConsole()
{
	g_currentConsole = &g_debugConsole;
}

Console* GetCurrentConsole()
{
	return g_currentConsole;
}

void CoNopKill(UNUSED Console *this)
{
}

void CoNopSetWndTitle(UNUSED Console *this, UNUSED const char* unused)
{
	
}

void CoColorFlip (Console *this);

// The mess that goes into generalizing the console implementation to support any console device ever invented. Ever.
#if 1
	
	// Function forward definitions
	#if 1
		void CoE9xClearScreen  (Console *this);
		void CoFilClearScreen  (Console *this);
		void CoSerClearScreen  (Console *this);
		void CoVbeClearScreen  (Console *this);
		void CoVgaClearScreen  (Console *this);
		void CoWndClearScreen  (Console *this);
		void CoE9xInit         (Console *this);
		void CoFilInit         (Console *this);
		void CoSerInit         (Console *this);
		void CoVbeInit         (Console *this);
		void CoVgaInit         (Console *this);
		void CoWndInit         (Console *this);
		void CoNopKill         (Console *this);
		void CoFilKill         (Console *this);
		void CoVbeKill         (Console *this);
		void CoWndKill         (Console *this);
		void CoE9xPlotChar     (Console *this, int,  int,  char);
		void CoFilPlotChar     (Console *this, int,  int,  char);
		void CoSerPlotChar     (Console *this, int,  int,  char);
		void CoVbePlotChar     (Console *this, int,  int,  char);
		void CoVgaPlotChar     (Console *this, int,  int,  char);
		void CoWndPlotChar     (Console *this, int,  int,  char);
		bool CoE9xPrintCharInt (Console *this, char, bool);
		bool CoFilPrintCharInt (Console *this, char, bool);
		bool CoSerPrintCharInt (Console *this, char, bool);
		bool CoVisComPrnChrInt (Console *this, char, bool);//common for visual consoles
		void CoE9xRefreshChar  (Console *this, int,  int);
		void CoFilRefreshChar  (Console *this, int,  int);
		void CoSerRefreshChar  (Console *this, int,  int);
		void CoVbeRefreshChar  (Console *this, int,  int);
		void CoVgaRefreshChar  (Console *this, int,  int);
		void CoWndRefreshChar  (Console *this, int,  int);
		void CoE9xUpdateCursor (Console *this);
		void CoFilUpdateCursor (Console *this);
		void CoSerUpdateCursor (Console *this);
		void CoVbeUpdateCursor (Console *this);
		void CoVgaUpdateCursor (Console *this);
		void CoWndUpdateCursor (Console *this);
		void CoE9xScrollUpByOne(Console *this);
		void CoFilScrollUpByOne(Console *this);
		void CoSerScrollUpByOne(Console *this);
		void CoVbeScrollUpByOne(Console *this);
		void CoVgaScrollUpByOne(Console *this);
		void CoWndScrollUpByOne(Console *this);
		void CoNopSetWndTitle  (Console *this, const char*);
		void CoWndSetWndTitle  (Console *this, const char*);
	#endif
	
	// Type definitions
	#if 1
		typedef void (*tCoClearScreen)  (Console *this);
		typedef void (*tCoInit)         (Console *this);
		typedef void (*tCoKill)         (Console *this);
		typedef void (*tCoPlotChar)     (Console *this, int,  int,  char);
		typedef bool (*tCoPrintCharInt) (Console *this, char, bool);
		typedef void (*tCoRefreshChar)  (Console *this, int,  int);
		typedef void (*tCoUpdateCursor) (Console *this);
		typedef void (*tCoScrollUpByOne)(Console *this);
		typedef void (*tCoSetWndTitle)  (Console *this, const char*);
	#endif
	
	// Array definitions
	#if 1
		static tCoClearScreen g_clear_screen[] = {
			NULL,
			CoVgaClearScreen,
			CoVbeClearScreen,
			CoSerClearScreen,
			CoE9xClearScreen,
			CoWndClearScreen,
			CoFilClearScreen,
		};
		static tCoInit g_init[] = {
			NULL,
			CoVgaInit,
			CoVbeInit,
			CoSerInit,
			CoE9xInit,
			CoWndInit,
			CoFilInit,
		};
		static tCoKill g_kill[] = {
			NULL,
			CoNopKill,
			CoVbeKill,
			CoNopKill,
			CoNopKill,
			CoWndKill,
			CoFilKill,
		};
		static tCoPlotChar g_plot_char[] = {
			NULL,
			CoVgaPlotChar,
			CoVbePlotChar,
			CoSerPlotChar,
			CoE9xPlotChar,
			CoWndPlotChar,
			CoFilPlotChar,
		};
		static tCoPrintCharInt g_print_char_int[] = {
			NULL,
			CoVisComPrnChrInt,
			CoVisComPrnChrInt,
			CoSerPrintCharInt,
			CoE9xPrintCharInt,
			CoVisComPrnChrInt,
			CoFilPrintCharInt,
		};
		static tCoRefreshChar g_refresh_char[] = {
			NULL,
			CoVgaRefreshChar,
			CoVbeRefreshChar,
			CoSerRefreshChar,
			CoE9xRefreshChar,
			CoWndRefreshChar,
			CoFilRefreshChar,
		};
		static tCoUpdateCursor g_update_cursor[] = {
			NULL,
			CoVgaUpdateCursor,
			CoVbeUpdateCursor,
			CoSerUpdateCursor,
			CoE9xUpdateCursor,
			CoWndUpdateCursor,
			CoFilUpdateCursor,
		};
		static tCoUpdateCursor g_scroll_up_by_one[] = {
			NULL,
			CoVgaScrollUpByOne,
			CoVbeScrollUpByOne,
			CoSerScrollUpByOne,
			CoE9xScrollUpByOne,
			CoWndScrollUpByOne,
			CoFilScrollUpByOne,
		};
		static tCoSetWndTitle g_set_window_title[] = {
			NULL,
			CoNopSetWndTitle,
			CoNopSetWndTitle,
			CoNopSetWndTitle,
			CoNopSetWndTitle,
			CoWndSetWndTitle,
			CoNopSetWndTitle,
		};
	#endif
	
	// The functions themselves.
	void CoClearScreen(Console *this)
	{
		g_clear_screen[this->type](this);
	}
	void CoInit(Console *this)
	{
		g_init[this->type](this);
	}
	void CoKill(Console *this)
	{
		g_kill[this->type](this);
	}
	void CoInitAsText(Console *this)
	{
		this->type = CONSOLE_TYPE_TEXT;
		CoInit(this);
	}
	void CoInitAsE9Hack(Console *this)
	{
		this->type = CONSOLE_TYPE_E9HACK;
		CoInit(this);
	}
	void CoInitAsGraphics(Console *this)
	{
		this->type = CONSOLE_TYPE_FRAMEBUFFER;
		CoInit(this);
	}
	void CoMoveCursor(Console* this)
	{
		g_update_cursor[this->type](this);
	}
	void CoPlotChar (Console *this, int x, int y, char c)
	{
		if (this->m_ansiAttributes & ANSI_FLAG_NEGATIVE)
			CoColorFlip (this);
		g_plot_char[this->type](this,x,y,c);
		if (this->m_ansiAttributes & ANSI_FLAG_NEGATIVE)
			CoColorFlip (this);
	}
	void CoRefreshChar (Console *this, int x, int y)
	{
		g_refresh_char[this->type](this,x,y);
	}
	void CoScrollUpByOne(Console *this)
	{
		g_scroll_up_by_one[this->type](this);
	}
	bool CoPrintCharInternal (Console* this, char c, bool bDontUpdateCursor)
	{
		return g_print_char_int[this->type](this,c,bDontUpdateCursor);
	}
	void CoSetWindowTitle(Console* this, const char* str)
	{
		return g_set_window_title[this->type](this, str);
	}
#endif

int g_ansiToVGAColors[] = {
	// 0-7 - these are octal characters
	000, 004, 002, 006, 001, 005, 003, 007,
	010, 014, 012, 016, 011, 015, 013, 017,
};

void CoColorFlip (Console *this)
{
	this->color = (this->color >> 4) | ((this->color << 4) & 0xF0);
}

void CoVisParseSGR(Console *this, char *contents)
{
	//use the betterStrTok method this time
	TokenState state;
	state.m_bInitted = 0;
	char *token = Tokenize (&state, contents, ";");
	if (!token) return;
	
	do
	{
		int n = 0;
		if (*token)
			n = atoi (token);
		
		// Set foreground colors
		/**/ if (n >= ANSI_ATTR_SETFGCOLORS && n <= ANSI_ATTR_SETFGCOLORS + 7)
		{
			if (!(this->m_ansiAttributes & ANSI_FLAG_FG_COLOR))
			{
				this->m_ansiAttributes |= ANSI_FLAG_FG_COLOR;
				this->m_ansiFgColorBackup = this->color;
			}
			this->color = (this->color & 0xF0) | g_ansiToVGAColors[n - ANSI_ATTR_SETFGCOLORS];
		}
		// Set background colors
		else if (n >= ANSI_ATTR_SETBGCOLORS && n <= ANSI_ATTR_SETBGCOLORS + 7)
		{
			if (!(this->m_ansiAttributes & ANSI_FLAG_BG_COLOR))
			{
				this->m_ansiAttributes |= ANSI_FLAG_BG_COLOR;
				this->m_ansiBgColorBackup = this->color;
			}
			this->color = (this->color & 0x0F) | g_ansiToVGAColors[n - ANSI_ATTR_SETBGCOLORS] << 4;
		}
		// Set bright foreground colors
		else if (n >= ANSI_ATTR_SETFGCOLRLS && n <= ANSI_ATTR_SETFGCOLRLS + 7)
		{
			if (!(this->m_ansiAttributes & ANSI_FLAG_FG_COLOR))
			{
				this->m_ansiAttributes |= ANSI_FLAG_FG_COLOR;
				this->m_ansiFgColorBackup = this->color;
			}
			this->color = (this->color & 0xF0) | g_ansiToVGAColors[n - ANSI_ATTR_SETFGCOLRLS + 8];
		}
		// Set bright background colors
		else if (n >= ANSI_ATTR_SETBGCOLRLS && n <= ANSI_ATTR_SETBGCOLRLS + 7)
		{
			if (!(this->m_ansiAttributes & ANSI_FLAG_BG_COLOR))
			{
				this->m_ansiAttributes |= ANSI_FLAG_BG_COLOR;
				this->m_ansiBgColorBackup = this->color;
			}
			this->color = (this->color & 0x0F) | g_ansiToVGAColors[n - ANSI_ATTR_SETBGCOLRLS + 8] << 4;
			if (this->type == CONSOLE_TYPE_TEXT) // Text mode?
			{
				this->color &= 0x7F; // disable the BLINK bit
			}
		}
		else switch (n)
		{
			case ANSI_ATTR_INVISIBLE:
				this->m_ansiAttributes |= ANSI_FLAG_INVISIBLE;
				break;
			case ANSI_ATTR_NOINVISIBLE:
				this->m_ansiAttributes &=~ANSI_FLAG_INVISIBLE;
				break;
			case ANSI_ATTR_NEGATIVE:
				this->m_ansiAttributes |= ANSI_FLAG_NEGATIVE;
				break;
			case ANSI_ATTR_SETBGCOLORD:
				if (this->m_ansiAttributes & ANSI_FLAG_BG_COLOR) 
				{
					this->color = (this->color & 0x0F) | (this->m_ansiBgColorBackup & 0xF0);
					this->m_ansiAttributes &= ~ANSI_FLAG_BG_COLOR;
				}
				break;
			case ANSI_ATTR_SETFGCOLORD:
				if (this->m_ansiAttributes & ANSI_FLAG_FG_COLOR) 
				{
					this->color = (this->color & 0xF0) | (this->m_ansiFgColorBackup & 0x0F);
					this->m_ansiAttributes &= ~ANSI_FLAG_FG_COLOR;
				}
				break;
			case ANSI_ATTR_NONEGATIVE:
				this->m_ansiAttributes &=~ANSI_FLAG_NEGATIVE;
				break;
			case ANSI_ATTR_OFF:
				if (this->m_ansiAttributes & ANSI_FLAG_FG_COLOR) this->color = (this->color & 0xF0) | (this->m_ansiFgColorBackup & 0x0F);
				if (this->m_ansiAttributes & ANSI_FLAG_BG_COLOR) this->color = (this->color & 0x0F) | (this->m_ansiBgColorBackup & 0xF0);
				this->m_ansiAttributes = 0;
				break;
			// WORK: Add more features here
		}
		
		token = Tokenize (&state, NULL, ";");
	}
	while (token);
}
void CoVisComOnAnsiEscCode(Console *this)
{
	//SLogMsg("CoVisComOnAnsiEscCode: dumping ANSI escape code as hex:");
	char *pEscCode = this->m_ansiEscCode;
	char *pLastChar = NULL;
	while (*pEscCode)
	{
		//SLogMsgNoCr("%b ",*pEscCode);
		pLastChar = pEscCode;
		pEscCode++;
	}
	//SLogMsg(".");
	
	char firstChar = this->m_ansiEscCode[0];
	char lastChar  = *pLastChar;
	
	char* contentsAfter = &this->m_ansiEscCode[1];
	*pLastChar = '\0';
	
	switch (firstChar)
	{
		// Xterm Specific
		case ']': {
			switch (lastChar)
			{
				case '\a': // Bell - Set Window Title
				{
					char str1[16], str2[256];
					str1[0] = 0;
					str2[0] = 0;
					char *headwrite = str1, *headend = str1 + sizeof str1;
					char *headread  = contentsAfter, *headreadend = pLastChar;
					while (*headread && headread != headreadend)
					{
						if (*headread == ';')
						{
							headwrite = str2, headend = str2 + sizeof str2;
						}
						else if (headwrite + 1 < headend)
						{
							*(headwrite++) = *headread;
							*(headwrite)   = '\0';
						}
						headread++;
					}
					
					// TODO: It's actually line;column. Fix this.
					
					int mode = 2;
					if (*str1)
						mode = atoi (str1);
					
					// if the mode isn't just "set icon name to <string>"
					if (mode != 1)
					{
						CoSetWindowTitle(this, str2);
					}
					
					break;
				}
			}
			break;
		}
		// CSI
		case '[': {
			switch (lastChar)
			{
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				{
					int cx = this->curX, cy = this->curY;
					
					int pos = 1;
					
					if (*contentsAfter)
						pos = atoi (contentsAfter);
					
					switch (lastChar)
					{
						case 'A': cy -= pos; break;
						case 'B': cy += pos; break;
						case 'D': cx -= pos; break;
						case 'C': cx += pos; break;
					}
					
					if (cx < 0) cx = 0;
					if (cy < 0) cy = 0;
					if (cx >= this->width)  cx = this->width  - 1;
					if (cy >= this->height) cy = this->height - 1;
					
					this->curX = cx, this->curY = cy;
					
					break;
				}
				case 'm':
				{
					// parse SGR
					CoVisParseSGR(this, contentsAfter);
					break;
				}
				case 'H': // Go to absolute position
				case 'f':
				{
					char str1[64], str2[64];
					str1[0] = 0;
					str2[0] = 0;
					char *headwrite = str1, *headend = str1 + sizeof str1;
					char *headread  = contentsAfter, *headreadend = pLastChar;
					while (*headread && headread != headreadend)
					{
						if (*headread == ';')
						{
							headwrite = str2, headend = str2 + sizeof str2;
						}
						else if (headwrite + 1 < headend)
						{
							*(headwrite++) = *headread;
							*(headwrite)   = '\0';
						}
						headread++;
					}
					
					// TODO: It's actually line;column. Fix this.
					
					int x1 = 1, y1 = 1;
					if (*str1)
						x1 = atoi (str1);
					if (*str2)
						y1 = atoi (str2);
					
					if (x1 < 1) x1 = 1;
					if (y1 < 1) y1 = 1;
					if (x1 >= this->width ) x1 = this->width;
					if (y1 >= this->height) y1 = this->height;
					
					this->curX = x1 - 1;
					this->curY = y1 - 1;
					
					break;
				}
				case 'G': // Go to absolute position horizontally
				{
					int pos = 1;
					
					if (*contentsAfter)
						pos = atoi (contentsAfter);
					
					if (pos < 0)
						pos = 0;
					if (pos >= this->width)
						pos  = this->width;
					this->curX = pos;
					break;
				}
				case 'T': // Scroll down
				{
					int pos = 1;
					
					if (*contentsAfter)
						pos = atoi (contentsAfter);
					
					while (pos--)
						CoScrollUpByOne(this);
					break;
				}
				case 'K': // Erase in Line
				{
					int pos = 0;
					
					if (*contentsAfter)
						pos = atoi (contentsAfter);
					
					if (pos == 2)
					{
						for (int i = 0; i < this->width; i++)
							CoPlotChar (this, i, this->curY, ' ');
					}
					else if (pos == 1)
					{
						for (int i = 0; i <= this->curX; i++)
							CoPlotChar (this, i, this->curY, ' ');
					}
					else
					{
						for (int i = this->curX; i < this->width; i++)
							CoPlotChar (this, i, this->curY, ' ');
					}
					break;
				}
				case 'J': // Erase in Display
				{
					int pos = 0;
					
					if (*contentsAfter)
						pos = atoi (contentsAfter);
					
					switch (pos)
					{
						case 0: // clear from cursor to end of screen
							for (int i = this->curX; i < this->width; i++) CoPlotChar(this, i, this->curY, ' ');
							break;
						case 1: // clear from beginning of the screen to the cursor
							for (int i = 0; i <= this->curX; i++) CoPlotChar(this, i, this->curY, ' ');
							break;
						case 2: // clear the display
						case 3:
							CoClearScreen(this);
							//NB: Only DOS ANSI.SYS moves the cursor to the upper left. So you must also send a '\e[1;1H'
							break;
					}
					
					break;
				}
				case 'P': // NanoShell specific : Permanentize a color scheme.
				{
					int pos = 0;
					
					if (*contentsAfter)
						pos = atoi (contentsAfter);
					
					if (pos == 1337)
					{
						this->m_ansiAttributes &= ~(ANSI_FLAG_BG_COLOR | ANSI_FLAG_FG_COLOR);
					}
					
					break;
				}
			}
			break;
		}
	}
}
//returns a bool, if it's a true, we need to skip the next character.
bool CoVisComPrnChrInt (Console* this, char c, bool bDontUpdateCursor)
{
	if (!this->m_usingAnsiEscCode)
	{
		switch (c)
		{
			case '\e':
			{
				// ANSI escape code.
				this->m_usingAnsiEscCode = true;
				this->m_ansiEscCode[0] = 0;
				break;
			}
			case '\b':
				if (--this->curX < 0) {
					this->curX = this->width - 1;
					if (--this->curY < 0) this->curY = 0;
				}
				break;
			case '\x7F':
				if (--this->curX < 0) {
					this->curX = this->width - 1;
					if (--this->curY < 0) this->curY = 0;
				}
				CoPlotChar(this, this->curX, this->curY, 0);
				break;
			case '\r': 
				this->curX = 0;
				break;
			case '\n': 
				this->curX = 0;
				this->curY++;
				while (this->curY >= this->height) {
					CoScrollUpByOne(this);
					this->curY--;
				}
				break;
			case '\t': 
				this->curX = (this->curX + 4) & ~3;
				if (this->curX >= this->width) {
					this->curX = 0;
					this->curY++;
				}
				while (this->curY >= this->height) {
					CoScrollUpByOne(this);
					this->curY--;
				}
				break;
			
			default: {
				CoPlotChar(this, this->curX, this->curY, c);
				// advance cursor
				if (++this->curX >= this->width) {
					this->curX = 0;
					this->curY++;
				}
				while (this->curY >= this->height) {
					CoScrollUpByOne(this);
					this->curY--;
				}
				break;
			}
		}
	}
	else
	{
		bool bFirstChar = this->m_ansiEscCode[0] == 0;
		
		// append to the ANSI escape code
		char t[2];
		t[1] = 0;
		t[0] = c;
		strcat (this->m_ansiEscCode, t);
		
		if (this->m_ansiEscCode[sizeof(this->m_ansiEscCode) - 2] != 0) // Uh oh
		{
			this->m_usingAnsiEscCode = false;
			CoVisComOnAnsiEscCode(this);
		}
		
		// if this is the end:
		if (!bFirstChar)
		{
			bool condition = false;
			if (this->m_ansiEscCode[0] == ']')
			{
				condition = strchr("\a", c) != NULL;
			}
			else
			{
				condition = strchr("@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\a", c) != NULL;
			}
			
			if (condition)
			{
				this->m_usingAnsiEscCode = false;
				CoVisComOnAnsiEscCode(this);
			}
		}
		
		return false;
	}
	if (!bDontUpdateCursor) CoMoveCursor(this);
	return false;
}

void CoPrintChar (Console* this, char c)
{
	CoPrintCharInternal(this, c, true);
}

void CoPrintString (Console* this, const char *c)
{
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	while (*c)
	{
		// if we need to advance 2 characters instead of 1:
		if (CoPrintCharInternal(this, *c, false))
			c++;
		// advance the one we would've anyways
		c++;
	}
	CoMoveCursor(this);
}

// Input:
void CoAddToInputQueue (Console* this, char input)
{
	if (!input) return;
	
	this->m_inputBuffer[this->m_inputBufferEnd++] = input;
	
	this->m_inputBufferEnd %= KB_BUF_SIZE;
}

bool CoAnythingOnInputQueue (Console* this)
{
	return this->m_inputBufferBeg != this->m_inputBufferEnd;
}

char CoReadFromInputQueue (Console* this)
{
	if (CoAnythingOnInputQueue(this))
	{
		char k = this->m_inputBuffer[this->m_inputBufferBeg++];
		
		this->m_inputBufferBeg %= KB_BUF_SIZE;
		
		return k;
	}
	else return 0;
}
