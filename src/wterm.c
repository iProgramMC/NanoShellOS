/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Console Window Host module
******************************************/
#include <wterm.h>
#include <icon.h>
#include <wbuiltin.h>
#include <misc.h>
#include <process.h>
#include <vfs.h>

int g_TerminalFont = FONT_TAMSYN_SMALL_REGULAR;

#define DebugLogMsg  SLogMsg
extern Console *g_currentConsole, *g_focusedOnConsole, g_debugConsole, g_debugSerialConsole;
extern uint32_t g_vgaColorsToRGB[];

void ShellExecuteCommand(char* p);
void CoRefreshChar (Console *this, int x, int y);
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);
void KeKillThreadsByConsole(Console *pConsole);

char KbMapAtCodeToChar(char kc);

void CALLBACK TerminalHostProc (UNUSED Window* pWindow, UNUSED int messageType, UNUSED int parm1, UNUSED int parm2)
{
	Console* pConsole = (Console*)pWindow->m_data;
	switch (messageType)
	{
		case EVENT_CLICKCURSOR:
		{
			//CLogMsgNoCr(pConsole, "Clicked! ");
			break;
		}
		case EVENT_KEYRAW:
		{
			unsigned char key_code = (unsigned char) parm1 & 0x7F;
			
			if (key_code != 0xE0 && (~parm1 & 0x80))
			{
				char chr = KbMapAtCodeToChar((char)key_code);
				
				SLogMsg("WriteChr: %b", chr);
				
				if (chr != 0)
					FiWrite(FD_STDIN, &chr, 1);
			}
			
			break;
		}
		case EVENT_CLOSE:
		case EVENT_DESTROY:
		{
			CoKill(pConsole);
			if (pConsole->textBuffer)
			{
				MmFree(pConsole->textBuffer);
				pConsole->textBuffer = NULL;
			}
			
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			break;
		}
		case EVENT_PAINT:
		{
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
		}
		case EVENT_UPDATE:
		{
			if (pConsole)
			{
				// This will read from the stdin
				char oneByte;
				uint32_t read = FiRead(FD_STDOUT, &oneByte, 1);
				uint32_t chances = 2;
				
				if (read > 0)
				{
					char buffer[2048];
					CoPrintChar(pConsole, oneByte);
					while ((read = FiRead(FD_STDOUT, &buffer, sizeof buffer)) > 0 && chances > 0)
					{
						for (uint32_t i = 0; i < read; i++)
							CoPrintChar(pConsole, buffer[i]);
						chances--;
					}
				}
				
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
					sti;
					
					if (pConsole->m_cursorFlashTimer > GetTickCount() && cursorX > 0 && cursorY > 0 && cursorX <= pConsole->width && cursorY <= pConsole->height)
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

//! NOTE: arg is a pointer to an array of 4 ints.

void TerminalHostTask(int arg)
{
	// Setup a pipe duplex.
	int fds[2];
	int errCode = FiCreatePipe("TerminalHost", fds, O_NONBLOCK);
	if (errCode < 0)
	{
		SLogMsg("TerminalHost could not create a terminal instance. %s", GetErrNoString(errCode));
		return;
	}
	
	// The application will read (stdin) from fd[0] and write (stdout) to fd[1].
	// The terminal will handle the incoming data from fd[0] (written to fd[1] by the application),
	// and pass on keypresses and stuff through fd[1] (it will be read from fd[0] on the application side)
	ASSERT(fds[0] == FD_STDIN && fds[1] == FD_STDOUT);
	
	// Duplicate this handle for stderr. TODO
	//FiDuplicateHandle(fds[1]);
	
	int array[] = { CW_AUTOPOSITION, CW_AUTOPOSITION, 80, 25 };
	
	bool providedShellCmd = false;
	
	char* shellcmd = (char*)arg;
	
	if (shellcmd) providedShellCmd = true;
	
	VidSetFont(g_TerminalFont);//we like this font right here
	int charWidth = GetCharWidth('W'), charHeite = GetLineHeight();
	VidSetFont(SYSTEM_FONT);
	Window *pWindow = CreateWindow(
		"NanoShell Terminal", 
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
	
	// Create a text buffer.
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
	
	Console* pConsole = &basic_console;
	
	//note: this is a stack variable, but it should be fine, since the window procedure is
	//only run within the stack trace of this function.
	pWindow->m_data = pConsole;
	
	
	//g_currentConsole = pConsole;
	//pWindow->m_consoleToFocusKeyInputsTo = pConsole;
	
	CoClearScreen(pConsole);
	pConsole->curX = 0;
	pConsole->curY = 0;
	
	if (providedShellCmd)
	{
		char* pText = shellcmd;
		
		// Write the shellcmd as an argument, and then a return.
		FiWrite(FD_STDIN, pText, strlen(pText));
		FiWrite(FD_STDIN, "\n", 1);
		
		MmFree(shellcmd);
	}
	else
	{
		KePrintSystemVersion();
	}
	
	// reset the g_cwd for the current task
	ShellInit();
	
	int confusion = 0;
	Task* pTask = KeStartTask(ShellRun, (int)(&basic_console),  &confusion);
	
	if (!pTask)
	{
		ILogMsg("ERROR: Could not spawn task for nsterm (returned error code %x)", confusion);
		
		DestroyWindow(pWindow);
		while (HandleMessages(pWindow));
		
		MmFree(basic_console.textBuffer);
		
		FiClose(FD_STDOUT);
		FiClose(FD_STDIN);
		
		return;
	}
	
	ExAddTaskToProcess(ExGetRunningProc(), pTask);
	
	KeUnsuspendTask(pTask);
	
	int timeout = GetTickCount();
	while (HandleMessages (pWindow))
	{
		if (GetTickCount() > timeout)
		{
			// Update slower if the window isn't selected.
			timeout += pWindow->m_isSelected ? 10 : 20;
			
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
		}
	}
	
	// Kill ourselves.
	ExKillProcess(ExGetRunningProc());
}

int TerminalHostStart(int arg)
{
	// Create a new process. This will contain the TerminalHostTask.
	int errCode = 0;
	Process* pProc = ExCreateProcess(TerminalHostTask, arg, "TerminalHost", 0, &errCode, NULL);
	if (!pProc)
	{
		SLogMsg("ERROR in TerminalHostStart: %d", errCode);
		return -1;
	}
	
	return 0;
}

