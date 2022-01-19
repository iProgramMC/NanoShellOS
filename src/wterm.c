/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Console Window Host module
******************************************/
#include <wterm.h>

#define DebugLogMsg  SLogMsg
extern Console *g_currentConsole;
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
		{
			// Kill the subordinate task.
			
			if (pWindow->m_pSubThread)
				KeKillTask(pWindow->m_pSubThread);//kill that first
			pWindow->m_pSubThread = NULL;
			
			LogMsg("Sub task killed! Exitting...");
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			break;
		}
		case EVENT_UPDATE:
		case EVENT_PAINT:
		{
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
			break;
		}
		case EVENT_DESTROY:
			if (pConsole->textBuffer)
			{
				MmFree(pConsole->textBuffer);
				pConsole->textBuffer = NULL;
			}
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}
//! NOTE: arg is a pointer to an array of 4 ints.
void TerminalHostTask(int arg)
{
	int* array = (int*)arg;
	int arrayDefault[] = { 100, 100, 80, 25 };
	if (!array)
		array = arrayDefault;
	Window *pWindow = CreateWindow(
		"nsterm", 
		array[0], array[1], 
		array[2] * 6 + 8 + WINDOW_RIGHT_SIDE_THICKNESS, 
		array[3] * 8 + 9 + WINDOW_RIGHT_SIDE_THICKNESS + TITLE_BAR_HEIGHT, 
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
	basic_console.color = 0x1F;//green background
	basic_console.curX = basic_console.curY = 0;
	basic_console.pushOrWrap = 0; //wrap for now
	basic_console.cwidth = 6;
	basic_console.cheight = 8;
	basic_console.curX = 0;
	basic_console.curY = 0;
	
	pWindow->m_data = &basic_console;
	
	g_currentConsole = &basic_console;
	
	pWindow->m_consoleToFocusKeyInputsTo = &basic_console;
	
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
	
	CoClearScreen(&basic_console);
	ShellExecuteCommand ("ver");
	//LogMsg("Select this window and type something.");
	
	ShellInit();
	while (HandleMessages (pWindow))
	{
		if (basic_console.m_dirty)
		{
			basic_console.m_dirty = false;
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
		}
	}
	
	//KeKillTask(pTask);
}

