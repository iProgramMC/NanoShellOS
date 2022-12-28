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
void KeKillThreadsByConsole(Console *pConsole);

int g_ConsoleDefaultWidth = 80, g_ConsoleDefaultHeight = 25;
bool g_bConsoleInitted = false;

void TermLoadDefaultParms()
{
	if (g_bConsoleInitted) return;
	CfgGetIntValue(&g_ConsoleDefaultWidth,  "Console::Width",  80);
	CfgGetIntValue(&g_ConsoleDefaultHeight, "Console::Height", 25);
	g_bConsoleInitted = true;
}

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
			if (pConsole)
			{
				// Restore keyboard input
				if (g_focusedOnConsole == pConsole)
				{
					g_focusedOnConsole =  &g_debugConsole;
				}
				if (g_currentConsole == pConsole)
				{
					g_currentConsole =  &g_debugConsole;
				}
				
				// Kill the tasks using this console.
				
				// Make sure to kill the shell first, so that programs called by it won't wait for something to go away
				// Maybe I should add some kind of memory ownership system to the kernel heap, but I don't know...
				if (pWindow->m_pSubThread)
				{
					KeKillTask(pWindow->m_pSubThread);
					pWindow->m_pSubThread = NULL;
				}
				
				// Kill the other threads using this console.
				KeKillThreadsByConsole(pConsole);
				
				CoKill(pConsole);
				MmFree(pConsole);
				pWindow->m_data = NULL;
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
					//TODO: fix this properly by locking the entire console
					cli;
					int cursorX = pConsole->curX, cursorY = pConsole->curY;
					int lastX   = pConsole->lastX, lastY = pConsole->lastY;
					sti;
					
					CoRefreshChar(pConsole, lastX, lastY);
					
					pConsole->lastX = pConsole->curX;
					pConsole->lastY = pConsole->curY;
					
					if (pConsole->m_cursorFlashTimer > GetTickCount() && cursorX >= 0 && cursorY >= 0 && cursorX < pConsole->width && cursorY < pConsole->height)
					{
						if (pConsole->m_cursorFlashState)
						{
							VidPlotChar(
								'_',
								pConsole->offX + cursorX* pConsole->cwidth,
								pConsole->offY + cursorY * pConsole->cheight,
								g_vgaColorsToRGB[(pConsole->textBuffer[cursorY * pConsole->width + cursorX] >> 8) & 0xF],
								TRANSPARENT
							);
						}
						else
						{
							CoRefreshChar(pConsole, cursorX, cursorY);
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

void TerminalHostTask(int arg)
{
	TermLoadDefaultParms();
	int array[] = { CW_AUTOPOSITION, CW_AUTOPOSITION, g_ConsoleDefaultWidth, g_ConsoleDefaultHeight };
	
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
	
	Console* pConsole = MmAllocate(sizeof (Console));
	memcpy(pConsole, &basic_console, sizeof basic_console);
	
	pWindow->m_data  = pConsole;
	g_currentConsole = pConsole;
	pWindow->m_consoleToFocusKeyInputsTo = pConsole;
	
	CoClearScreen(pConsole);
	pConsole->curX = 0;
	pConsole->curY = 0;
	
	
	if (providedShellCmd)
	{
		char* pText = shellcmd;
		while (*pText)
			CoAddToInputQueue(g_currentConsole, *pText++);
		MmFree(shellcmd);
	}
	else if (!hookDebugConsole)
	{
		KePrintSystemVersion();
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
		
		KeUnsuspendTask(pTask);
		
		ShellInit();
	}
	
	//LogMsg("Select this window and type something.");
	
	int timeout = GetTickCount();
	while (HandleMessages (pWindow))
	{
		if (GetTickCount() > timeout)
		{
			timeout += pWindow->m_isSelected ? 10 : 20;
			
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
		}
	}
	
	//KeKillTask(pTask);
}

