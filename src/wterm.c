/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Console Window Host module
******************************************/
#include <wterm.h>
#include <icon.h>
#include <wbuiltin.h>
#include <misc.h>

int g_TerminalFont = FONT_TAMSYN_SMALL_REGULAR;

#define DebugLogMsg  SLogMsg
extern Console *g_currentConsole, *g_focusedOnConsole, g_debugConsole, g_debugSerialConsole;
extern uint32_t g_vgaColorsToRGB[];
void ShellExecuteCommand(char* p);
void CoRefreshChar (Console *this, int x, int y);
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);
void CALLBACK TerminalHostProc (UNUSED Window* pWindow, UNUSED int messageType, UNUSED int parm1, UNUSED int parm2)
{
	Console* pConsole = (Console*)pWindow->m_data;
	switch (messageType)
	{
		case EVENT_CLICKCURSOR:
			//CLogMsgNoCr(pConsole, "Clicked! ");
			break;
		case EVENT_KEYPRESS:
		{
			//CoPrintChar(pConsole, (char)parm1);
			break;
		}
		case EVENT_CLOSE:
		case EVENT_DESTROY:
		{
			// Restore keyboard input
			if (g_focusedOnConsole == pConsole)
			{
				g_focusedOnConsole =  &g_debugConsole;
				g_currentConsole   =  &g_debugConsole;
			}
			
			// Kill the subordinate task.
			if (pWindow->m_pSubThread)
			{
				KeKillTask(pWindow->m_pSubThread);//kill that first
			}
			pWindow->m_pSubThread = NULL;
			
			if (pConsole->textBuffer)
			{
				MmFree(pConsole->textBuffer);
				pConsole->textBuffer = NULL;
			}
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			break;
		}
		case EVENT_PAINT:
			if (pConsole)
			{
				pConsole->m_dirty = true;
				
				// Draw the rectangle around the console window.
				Rectangle r;
				
				VidSetFont(g_TerminalFont);//we like this font right here
				int charWidth = GetCharWidth('W'), charHeite = GetLineHeight();
				VidSetFont(SYSTEM_FONT);
				RECT (r, 3, 3 + TITLE_BAR_HEIGHT, pConsole->width * charWidth + 3, pConsole->height * charHeite + 4);
				
				RenderButtonShapeSmallInsideOut (r, 0xBFBFBF, 0x808080, TRANSPARENT);
			}
		case EVENT_UPDATE:
		{
			if (pConsole)
			{
				VidSetFont(pConsole->font);//we like this font right here
				if (pConsole->m_dirty)
				{
					pConsole->m_dirty = false;
					//re-draw every character.
					if (pConsole->textBuffer)
					{
						for (int j = 0; j < pConsole->height; j++)
						{
							for (int i = 0; i < pConsole->width; i++)
							{
								//CoPlotChar(pConsole, i, j, pConsole->textBuffer[i + j * pConsole->width]);
								CoRefreshChar(pConsole, i, j);
							}
						}
					}
					else
					{
						VidTextOut ("No console buffer associated with this.", 10, 20, 0xFFFFFF, TRANSPARENT);
					}
				}
				
				//Cursor flashing shall only occur if the window is selected to save on CPU time
				if (pWindow->m_isSelected)
				{
					if (pConsole->m_cursorFlashTimer > GetTickCount())
					{
						if (pConsole->m_cursorFlashState)
						{
							VidPlotChar(
								'_',
								pConsole->offX + pConsole->curX * pConsole->cwidth,
								pConsole->offY + pConsole->curY * pConsole->cheight,
								g_vgaColorsToRGB[(pConsole->textBuffer[pConsole->curY * pConsole->width + pConsole->curX] >> 8) & 0xF],
								TRANSPARENT
							);
						}
						else
						{
							CoRefreshChar(pConsole, pConsole->curX, pConsole->curY);
						}
					}
					else
					{
						pConsole->m_cursorFlashTimer = GetTickCount() + 500;
						pConsole->m_cursorFlashState ^= 1;
					}
				}
				VidSetFont(SYSTEM_FONT);//let the WM be happy
			}
			else
			{
				VidTextOut ("No console associated with this.", 10, 20, 0xFFFFFF, TRANSPARENT);
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}
extern void ShellInit(void);
//! NOTE: arg is a pointer to an array of 4 ints.
void TerminalHostTask(int arg)
{
	int array[] = { 100, 100, 80, 25 };
	
	bool providedShellCmd = false, hookDebugConsole = false;
	char* shellcmd = (char*)arg;
	if (shellcmd)
	{
		providedShellCmd = true;
		if (strcmp (shellcmd, "--HookDebugConsole") == 0)
		{
			hookDebugConsole = true;
			
			providedShellCmd = false;
			MmFree(shellcmd);
			shellcmd = NULL;
		}
	}
	
	VidSetFont(g_TerminalFont);//we like this font right here
	int charWidth = GetCharWidth('W'), charHeite = GetLineHeight();
	VidSetFont(SYSTEM_FONT);
	Window *pWindow = CreateWindow(
		hookDebugConsole ? "NanoShell debug console" : "NanoShell Terminal", 
		array[0], array[1], 
		array[2] *  charWidth + 6 + 4 + WINDOW_RIGHT_SIDE_THICKNESS, 
		array[3] *  charHeite + 6 + 4 + WINDOW_RIGHT_SIDE_THICKNESS + TITLE_BAR_HEIGHT, 
		TerminalHostProc,
		0);
	if (!pWindow)
	{
		DebugLogMsg("ERROR: Could not create window for nsterm");
		return;
	}
	
	Console basic_console;
	if (hookDebugConsole)
	{
		pWindow->m_iconID = ICON_BOMB_SPIKEY;
		pWindow->m_data = &g_debugSerialConsole;
		g_currentConsole = &g_debugSerialConsole;
		pWindow->m_consoleToFocusKeyInputsTo = &g_debugSerialConsole;
		
		// Change the debug console to point to us instead.
		int size = sizeof(uint16_t) * array[2] * array[3];
		uint16_t* pBuffer = (uint16_t*)MmAllocate(size);
		memset (pBuffer, 0, size);
		
		g_debugSerialConsole.type = CONSOLE_TYPE_WINDOW;
		g_debugSerialConsole.m_vbeData = &pWindow->m_vbeData;
		g_debugSerialConsole.textBuffer = pBuffer;
		g_debugSerialConsole.width  = array[2];
		g_debugSerialConsole.height = array[3];
		g_debugSerialConsole.font = g_TerminalFont;
		g_debugSerialConsole.offX = 3 + 2;
		g_debugSerialConsole.offY = 3 + 2 + TITLE_BAR_HEIGHT;
		g_debugSerialConsole.color = DefaultConsoleColor;//green background
		g_debugSerialConsole.curX = basic_console.curY = 0;
		g_debugSerialConsole.pushOrWrap = 0; //wrap for now
		g_debugSerialConsole.cwidth  = charWidth;
		g_debugSerialConsole.cheight = charHeite;
		g_debugSerialConsole.curX = 0;
		g_debugSerialConsole.curY = 0;
		g_debugSerialConsole.m_cursorFlashTimer = 0;
	}
	else
	{
		pWindow->m_iconID = ICON_COMMAND;
		memset (&basic_console, 0, sizeof(basic_console));
		
		int size = sizeof(uint16_t) * array[2] * array[3];
		uint16_t* pBuffer = (uint16_t*)MmAllocate(size);
		memset (pBuffer, 0, size);
		
		basic_console.type = CONSOLE_TYPE_WINDOW;
		basic_console.m_vbeData = &pWindow->m_vbeData;
		basic_console.textBuffer = pBuffer;
		basic_console.width  = array[2];
		basic_console.height = array[3];
		basic_console.font = g_TerminalFont;
		basic_console.offX = 3 + 2;
		basic_console.offY = 3 + 2 + TITLE_BAR_HEIGHT;
		basic_console.color = DefaultConsoleColor;//green background
		basic_console.curX = basic_console.curY = 0;
		basic_console.pushOrWrap = 0; //wrap for now
		basic_console.cwidth  = charWidth;
		basic_console.cheight = charHeite;
		basic_console.curX = 0;
		basic_console.curY = 0;
		basic_console.m_cursorFlashTimer = 0;
		
		pWindow->m_data = &basic_console;
		g_currentConsole = &basic_console;
		pWindow->m_consoleToFocusKeyInputsTo = &basic_console;
		
		CoClearScreen(&basic_console);
		basic_console.curX = 0;
		basic_console.curY = 0;
	}
	
	
	if (providedShellCmd)
	{
		char* pText = shellcmd;
		while (*pText)
			CoAddToInputQueue(g_currentConsole, *pText++);
		MmFree(shellcmd);
	}
	else if (!hookDebugConsole)
	{
		ShellExecuteCommand ("ver");
	}
	else
	{
		LogMsg("NanoShell debug console.  Do not close, or else any E9-prints will fail and crash the system");
	}
	
	if (!hookDebugConsole)
	{
		int confusion = 0;
		Task* pTask = KeStartTask(ShellRun, (int)(&basic_console),  &confusion);
		
		if (!pTask)
		{
			DebugLogMsg("ERROR: Could not spawn task for nsterm (returned error code %x)", confusion);
			//DestroyWindow(pWindow);
			//ReadyToDestroyWindow(pWindow);
			return;
		}
		
		pWindow->m_pSubThread = pTask;
		
		ShellInit();
	}
	
	//LogMsg("Select this window and type something.");
	
	int timeout = 50;
	while (HandleMessages (pWindow))
	{
		timeout--;
		if (timeout == 0)
		{
			timeout = 10;
			if (pWindow->m_isSelected || basic_console.m_dirty)
			{
				WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			}
		}
	}
	
	//KeKillTask(pTask);
}

