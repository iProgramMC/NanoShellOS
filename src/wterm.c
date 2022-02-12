/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Console Window Host module
******************************************/
#include <wterm.h>

#define DebugLogMsg  SLogMsg
extern Console *g_currentConsole, *g_focusedOnConsole, g_debugConsole;
void ShellExecuteCommand(char* p);
void CoRefreshChar (Console *this, int x, int y);
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
	#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
			if (pConsole)
			{
				pConsole->m_dirty = true;
			}
	#pragma GCC diagnostic pop
		case EVENT_UPDATE:
		{
			if (pConsole)
			{
				if (pConsole->m_dirty)
				{
					pConsole->m_dirty = false;
					//re-draw every character.
					if (pConsole->textBuffer)
					{
						VidSetFont(FONT_TAMSYN_REGULAR);//we like this font right here
						for (int j = 0; j < pConsole->height; j++)
						{
							for (int i = 0; i < pConsole->width; i++)
							{
								//CoPlotChar(pConsole, i, j, pConsole->textBuffer[i + j * pConsole->width]);
								CoRefreshChar(pConsole, i, j);
							}
						}
						VidSetFont(FONT_BASIC);//let the WM be happy
					}
					else
					{
						VidTextOut ("No console buffer associated with this.", 10, 20, 0xFFFFFF, TRANSPARENT);
					}
				}
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
	
	bool providedShellCmd = false;
	char* shellcmd = (char*)arg;
	if (shellcmd)
	{
		providedShellCmd = true;
	}
	
	Window *pWindow = CreateWindow(
		"nsterm", 
		array[0], array[1], 
		array[2] *   8 + 8 + WINDOW_RIGHT_SIDE_THICKNESS, 
		array[3] *  16 + 9 + WINDOW_RIGHT_SIDE_THICKNESS + TITLE_BAR_HEIGHT, 
		TerminalHostProc,
		0);
	if (!pWindow)
	{
		DebugLogMsg("ERROR: Could not create window for nsterm");
		return;
	}
	
	Console basic_console;
	memset (&basic_console, 0, sizeof(basic_console));
	
	int size = sizeof(uint16_t) * array[2] * array[3];
	uint16_t* pBuffer = (uint16_t*)MmAllocate(size);
	memset (pBuffer, 0, size);
	
	basic_console.type = CONSOLE_TYPE_WINDOW;
	basic_console.m_vbeData = &pWindow->m_vbeData;
	basic_console.textBuffer = pBuffer;
	basic_console.width  = array[2];
	basic_console.height = array[3];
	basic_console.offX = 4;
	basic_console.offY = 5 + TITLE_BAR_HEIGHT;
	basic_console.color = DefaultConsoleColor;//green background
	basic_console.curX = basic_console.curY = 0;
	basic_console.pushOrWrap = 0; //wrap for now
	basic_console.cwidth  = 8;
	basic_console.cheight = 16;
	basic_console.curX = 0;
	basic_console.curY = 0;
	
	pWindow->m_data = &basic_console;
	
	g_currentConsole = &basic_console;
	
	pWindow->m_consoleToFocusKeyInputsTo = &basic_console;
	
	CoClearScreen(&basic_console);
	basic_console.curX = 0;
	basic_console.curY = 0;
	if (providedShellCmd)
	{
		char* pText = shellcmd;
		while (*pText)
			CoAddToInputQueue(g_currentConsole, *pText++);
		MmFree(shellcmd);
	}
	else
		ShellExecuteCommand ("ver");
	
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
	
	//LogMsg("Select this window and type something.");
	
	ShellInit();
	int timeout = 50;
	while (HandleMessages (pWindow))
	{
		timeout--;
		if (timeout == 0)
		{
			timeout = 20;
			if (basic_console.m_dirty)
			{
				WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			}
		}
	}
	
	//KeKillTask(pTask);
}

