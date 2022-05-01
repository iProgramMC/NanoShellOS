/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

     Memory Spy fullscreen app module
******************************************/
#include <main.h>
#include <console.h>
#include <misc.h>
#include <print.h>
#include <string.h>
extern Console *g_currentConsole;
#define c g_currentConsole

static uint8_t InvertColor(uint8_t color)
{
	return (color >> 4) | (color << 4);
}

bool MmIsPageMapped(uint32_t pageAddr);

static uint32_t GoTo(bool *quit)
{
	c->curX = 0;
	c->curY = 0;
	
	LogMsgNoCr("Go Where? (hex address - type q<\x0B to cancel)");
	
	char str[9];
	str[8] = 0;
	CoGetString(str, 9);
	
	if (str[0] == 'q')
	{
		*quit = true;
		return 0xFFFFFFFF;
	}
	
	return atoihex(str);
}
static uint32_t Search(bool *bNoMatch, char* str, uint32_t start)
{
	int slen = strlen (str);
	
	//remove this row
	c->curX = 0;
	c->curY = 0;
	for (int i = 0; i < 2 * c->width; i++) LogMsgNoCr(" ");
	
	c->curX = 0;
	c->curY = 0;
	LogMsg("Progress:");
	
	int lo = 0;
	int co = (start >> 12) * c->width / 0x100000;
	for (int i = 0; i < co; i++) LogMsgNoCr("=");
	
	lo = co;
	
	int pindex = start & 0xFFF;
	
	for (int i = start >> 12; i < 0xFFFFF; i++)
	{
		if (MmIsPageMapped(i << 12))
		{
			for (int j = pindex; j < 4096 - slen; j++)
			{
				void* ptr = (void*)(i << 12 | j);
				
				if ((char*)ptr >= str && (char*)ptr <= str + slen) // we're at the string, skip it
					continue;
				
				if (memcmp (ptr, str, slen) == 0)
					return (i << 12 | j);
			}
			pindex = 0;
		}
		
		int co = i * c->width / 0x100000;
		if (lo != co)
		{
			LogMsgNoCr("=");
			lo = co;
		}
	}
	
	c->curY = 0;
	
	*bNoMatch = true;
	
	return 0xFFFFFFFF;
}

static void UpdateScreen(uint32_t address, bool groupAsInts, int numRows)
{
	c->curX = c->curY = 0;
	
	uint8_t backup = c->color;
	
	c->color = InvertColor (c->color);
	
	char str[] = "Memory Spy - 0x????????";
	sprintf(str + 15, "%x", address);
	int slen = sizeof (str) - 1;
	
	int start = (c->width - slen) / 2;
	
	for (int i = 0; i < start; i++)
		LogMsgNoCr(" ");
	LogMsgNoCr(str);
	for (int i = start+slen; i < c->width; i++)
		LogMsgNoCr(" ");
	
	
	c->curY = 0;
	
	//print the bottom bar
	c->curY = c->height - 2;
	LogMsgNoCr("[ or ]: Advance page   - or =: Advance line  G: GoTo  Z: Search String   X: Next Match");
	for (int i = c->curX; i < c->width; i++)
		LogMsgNoCr(" ");
	LogMsgNoCr("T or Y: Advance screen   R: Group as %s" "   Q: Quit  C: Refresh", groupAsInts ? "bytes" : "ints ");
	for (int i = c->curX; i < c->width-1; i++)
		LogMsgNoCr(" ");
	
	c->color = backup;
	c->curX = 0;
	c->curY = 2;
	
	// print from the start:
	for (int y = 0; y < numRows; y++)
	{
		LogMsgNoCr("%x: ", address);
		
		uint32_t address1 = address;
		
		if (MmIsPageMapped(address1))
		{
			if (groupAsInts)
			{
				for (int x = 0; x < 4; x++)
				{
					LogMsgNoCr("%x ", *((uint32_t*)address1));
					address1 += 4;
				}
			}
			else
			{
				for (int x = 0; x < 16; x++)
				{
					LogMsgNoCr("%b ", *((char*)address1));
					address1++;
				}
			}
			
			for (int x = 0; x < 16; x++)
			{
				char c = *((char*)address);
				if (c < 0x20 || c >= 0x7F) c = '.';
				
				LogMsgNoCr("%c", c);
				address++;
			}
			for (int x = c->curX; x < c->width; x++)
				LogMsgNoCr(" ");
		}
		else
		{
			if (groupAsInts)
			{
				for (int x = 0; x < 4; x++)
				{
					LogMsgNoCr("???????? ");
					address1 += 4;
				}
			}
			else
			{
				for (int x = 0; x < 16; x++)
				{
					LogMsgNoCr("?? ");
					address1++;
				}
			}
			
			for (int x = 0; x < 16; x++)
			{
				LogMsgNoCr(".");
				address++;
			}
			for (int x = c->curX; x < c->width; x++)
				LogMsgNoCr(" ");
		}
	}
}

void MemorySpy()
{
	CoClearScreen(c);
	
	uint32_t address = 0xC0000000;
	bool groupAsInts = false;
	int numRows = c->height - 4;
	UpdateScreen(address, groupAsInts, numRows);
	
	bool bRunning = true;
	
	char str[65];
	str[0] = 0;
	uint32_t lastMatch = 0;
	while (bRunning)
	{
		//read some input
		while (CoAnythingOnInputQueue(c))
		{
			while (CoAnythingOnInputQueue(c))
			{
				char chr = CoGetChar(c);
				
				/**/ if (chr == '[') address -= 4096;
				else if (chr == ']') address += 4096;
				else if (chr == 't') address -= 16 * numRows;
				else if (chr == 'y') address += 16 * numRows;
				else if (chr == '-') address -= 16;
				else if (chr == '=') address += 16;
				else if (chr == 'r') groupAsInts ^= 1;
				else if (chr == 'c') CoClearScreen(c);
				else if (chr == 'q')
				{
					bRunning = false;
					break;
				}
				else if (chr == 'g')
				{
					bool q = false;
					uint32_t a = GoTo(&q);
					if (!q)
						address = a & 0xfffffff0;
				}
				else if (chr == 'z')
				{
					bool q = false;
					
					c->curX = 0;
					c->curY = 0;
					
					LogMsg("Search for what? (type nothing to cancel):");
					
					str[64] = 0;
					CoGetString(str, 65);
					
					if (*str)
					{
						uint32_t a = Search(&q, str, 0x00000000);
						if (!q)
						{
							address = a & 0xfffffff0;
							CoClearScreen(c);
							lastMatch = a;
							SLogMsg("Last Match: %x", a);
						}
						else SLogMsg("No Match");
					}
				}
				else if (chr == 'x')
				{
					SLogMsg("X press '%s'.  Last match: %x", str, lastMatch);
					if (*str)
					{
						bool q = false;
						uint32_t a = Search(&q, str, lastMatch + 1);
						if (!q)
						{
							address = a & 0xfffffff0;
							CoClearScreen(c);
							lastMatch = a;
						}
						else SLogMsg("No Match");
					}
				}
			}
			
			UpdateScreen(address, groupAsInts, numRows);
		}
		
		
		WaitMS(16);
	}
	
	CoClearScreen(c);
	
	LogMsg("Good-bye!");
}
