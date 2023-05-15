/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

     Memory Spy fullscreen app module
******************************************/
#include <main.h>
#include <console.h>
#include <time.h>
#include <print.h>
#include <string.h>
#include <ansi.h>

//#define c g_currentConsole
//extern Console *g_currentConsole;

bool MmIsPageMapped(uint32_t pageAddr);

static uint32_t GoTo(bool *quit)
{
	LogMsgNoCr(ANSI_GO_TO(1,1) "Go Where? (hex address - type q<\x0B to cancel)");
	
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
	LogMsg(ANSI_GO_TO(1,1) ANSI_CLEAR_LINE_TO_END "Please wait...");
	
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
	}
	
	LogMsgNoCr(ANSI_GO_TO(1,1));
	
	*bNoMatch = true;
	
	return 0xFFFFFFFF;
}

static void UpdateScreen(uint32_t address, bool groupAsInts, int numRows)
{
	LogMsgNoCr(ANSI_GO_TO(1,1) ANSI_INVERT);
	
	char str[] = "Memory Spy - 0x????????";
	sprintf(str + 15, "%x", address);
	
	LogMsgNoCr("%s" ANSI_CLEAR_LINE_TO_END, str);
	
	//print the bottom bar - This uses a really high number to go to the real bottom
	LogMsgNoCr(ANSI_GO_TO(1, 999) ANSI_GO_UP(1));
	LogMsgNoCr("[ or ]: Advance page   - or =: Advance line  G: GoTo  Z: Search String   X: Next Match" ANSI_CLEAR_LINE_TO_END);
	LogMsgNoCr(ANSI_GO_TO(1, 999));
	LogMsgNoCr("T or Y: Advance screen   R: Group as %s" "   Q: Quit  C: Refresh" ANSI_CLEAR_LINE_TO_END, groupAsInts ? "bytes" : "ints ");
	
	LogMsgNoCr(ANSI_UNINVERT ANSI_GO_TO(1,3));
	
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
			
			LogMsg("");
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
			
			LogMsg("");
		}
	}
}

void MemorySpy()
{
	LogMsgNoCr(ANSI_CLEAR_SCREEN ANSI_GO_TO(1,1));
	
	uint32_t address = 0xC0000000;
	bool groupAsInts = false;
	int numRows = 20; // TODO
	UpdateScreen(address, groupAsInts, numRows);
	
	bool bRunning = true;
	
	char str[65];
	str[0] = 0;
	uint32_t lastMatch = 0;
	while (bRunning)
	{
		//read some input
		while (!CoInputBufferEmpty())
		{
			while (!CoInputBufferEmpty())
			{
				char chr = CoGetChar();
				
				/**/ if (chr == '[') address -= 4096;
				else if (chr == ']') address += 4096;
				else if (chr == 't') address -= 16 * numRows;
				else if (chr == 'y') address += 16 * numRows;
				else if (chr == '-') address -= 16;
				else if (chr == '=') address += 16;
				else if (chr == 'r') groupAsInts ^= 1;
				else if (chr == 'c') LogMsgNoCr(ANSI_CLEAR_SCREEN ANSI_GO_TO(1,1));
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
					
					LogMsg(ANSI_GO_TO(1,1) "Search for what? (type nothing to cancel):");
					
					str[64] = 0;
					CoGetString(str, 65);
					
					if (*str)
					{
						uint32_t a = Search(&q, str, 0x00000000);
						if (!q)
						{
							address = a & 0xfffffff0;
							LogMsgNoCr(ANSI_CLEAR_SCREEN ANSI_GO_TO(1,1));
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
							LogMsgNoCr(ANSI_CLEAR_SCREEN ANSI_GO_TO(1,1));
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
	
	LogMsg(ANSI_CLEAR_SCREEN ANSI_GO_TO(1,1) "Goodbye!");
}
