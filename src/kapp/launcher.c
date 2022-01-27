/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Launcher Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <vfs.h>
#include <elf.h>

void LaunchVersion()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(VersionProgramTask, 0, &errorCode);
	DebugLogMsg("Created version window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchSystem()
{
	int errorCode = 0;
	//Task* pTask = KeStartTask(VersionProgramTask, 0, &errorCode);
	Task* pTask = KeStartTask(SystemMonitorEntry, 0, &errorCode);
	DebugLogMsg("Created System window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchNotepad()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(TerminalHostTask, 0, &errorCode);
	//Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	DebugLogMsg("Created Notepad window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchPaint()
{
	int errorCode = 0;
	//Task* pTask = KeStartTask(PrgPaintTask, 0, &errorCode);
	//Task* pTask = KeStartTask(ControlEntry, 0, &errorCode);
	Task* pTask = KeStartTask(BigTextEntry, 0, &errorCode);
	DebugLogMsg("Created Paint window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
extern FileNode *g_pCwdNode;
void ExecuteSomeElfFile(UNUSED int argument)
{
	FileNode* pNode = g_pCwdNode;
	FileNode* pFile = FsFindDir(pNode, "win.nse");
	if (!pFile)
		LogMsg("No such file or directory");
	else
	{
		KeTaskAssignTag(KeGetRunningTask(), "win.nse");
		int length = pFile->m_length;
		char* pData = (char*)MmAllocate(length + 1);
		pData[length] = 0;
		
		FsRead(pFile, 0, length, pData);
		
		ElfExecute(pData, length);
		
		MmFree(pData);
	}
}
void LaunchCabinet(UNUSED Window* pWindow)
{
	/*if (MessageBox (pWindow, "Would you like to launch 'win.nse'?", "Hey!", MB_YESNO | ICON_EXECUTE_FILE << 16) == MBID_YES)
	{
		int errorCode = 0;
		Task* pTask = KeStartTask(ExecuteSomeElfFile, 0, &errorCode);
		DebugLogMsg("Created ELF TASK. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	}*/
	/*if (MessageBox (pWindow, "Would you like to launch Cabinet?", "Home Menu", MB_YESNO | ICON_CABINET << 16) == MBID_YES)
	{
		int errorCode = 0;
		Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
		DebugLogMsg("Created Cabinet window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	}*/
	int errorCode = 0;
	Task* pTask = KeStartTask(CabinetEntry, 0, &errorCode);
	DebugLogMsg("Created list view test window", pTask, errorCode);
}
void WindowManagerShutdown();
void ConfirmShutdown(Window* pWindow)
{
	if (MessageBox (pWindow, "This will end your NanoShell session.", "Shut Down", MB_OKCANCEL | ICON_COMPUTER_SHUTDOWN << 16) == MBID_OK)
	{
		WindowManagerShutdown ();
	}
}

enum {
	LAUNCHER_SYSTEM = 0x10,
	LAUNCHER_NOTEPAD,
	LAUNCHER_PAINT,
	LAUNCHER_CABINET,
	LAUNCHER_TEXTBOX1,
	
	
	LAUNCHER_LABEL1 = 0xE0,
	LAUNCHER_LABEL2,
	LAUNCHER_LABEL3,
	LAUNCHER_LABEL4,
	LAUNCHER_LABEL5,
	LAUNCHER_ICON1 = 0xF0,
	LAUNCHER_ICON2,
	LAUNCHER_ICON3,
	LAUNCHER_ICON4,
	LAUNCHER_ICON5,
	
	LAUNCHER_MENUBAR  = 0xFE,
	LAUNCHER_SHUTDOWN = 0xFF,
};

void CALLBACK LauncherProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE: {
			#define START_X 20
			#define STEXT_X 60
			#define START_Y 40
			#define DIST_ITEMS 36
			// Add a label welcoming the user to NanoShell.
			Rectangle r;
			
			RECT(r, 0, 0, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, LAUNCHER_MENUBAR, 0, 0);
			
			// Add some testing elements to the menubar.  A comboID of zero means you're adding to the root.
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0, 1, "Help");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1, 2, "About Launcher...");
			/*
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0, 2, "Menu");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0, 3, "Bar");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0, 4, "Help");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1, 5, "About");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1, 6, "More");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 6, 7, "Hi!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 6, 8, "Hello!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1, 9, "Hello!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1,10, "TEST!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1,11, "Hello!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 2,20, "AAAAAAAAA!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 3,21, "BBBBBBBBB!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 3,22, "CCCCCCCCC!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 4,23, "DDDDDDDDD!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 4,24, "EEEEEEEEE!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 4,25, "About!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 2,26, "TEST1!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,26,27, "TEST2!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,26,28, "TEST3!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,28,29, "TEST2!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,28,30, "TEST3!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,29,31, "TEST2!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,29,32, "TEST3!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,32,34, "TEST2!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,32,33, "TEST3!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,33,35, "TEST3!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1,36, "TEST1!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1,37, "TEST2!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1,38, "TEST3!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0,60, "Stands Alone");*/
			
			RECT(r, START_X, 30, 200, 20);
			AddControl (pWindow, CONTROL_TEXT, r, "Welcome to NanoShell!", LAUNCHER_LABEL1, 0, TRANSPARENT);
			
			// Add the system icon.
			RECT(r, START_X, START_Y+0*DIST_ITEMS, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, LAUNCHER_ICON1, ICON_BOMB, 0);
			
			RECT(r, STEXT_X, START_Y+0*DIST_ITEMS, 200, 32);
			AddControl(pWindow, CONTROL_CLICKLABEL, r, "System Monitor", LAUNCHER_SYSTEM, 0, 0);
			
			// Add the file cabinet icon.
			RECT(r, START_X, START_Y+1*DIST_ITEMS, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, LAUNCHER_ICON2, ICON_CABINET, 0);
			
			RECT(r, STEXT_X, START_Y+1*DIST_ITEMS, 200, 32);
			AddControl(pWindow, CONTROL_CLICKLABEL, r, "File cabinet", LAUNCHER_CABINET, 0, 0);
			
			// Add the notepad icon.
			RECT(r, START_X, START_Y+2*DIST_ITEMS, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, LAUNCHER_ICON3, ICON_KEYBOARD2, 0);
			
			RECT(r, STEXT_X, START_Y+2*DIST_ITEMS, 200, 32);
			AddControl(pWindow, CONTROL_CLICKLABEL, r, "Text shell", LAUNCHER_NOTEPAD, 0, 0);
			
			// Add the paint icon.
			RECT(r, START_X, START_Y+3*DIST_ITEMS, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, LAUNCHER_ICON4, ICON_ERROR, 0);
			
			RECT(r, STEXT_X, START_Y+3*DIST_ITEMS, 200, 32);
			AddControl(pWindow, CONTROL_CLICKLABEL, r, "Control panel", LAUNCHER_PAINT, 0, 0);
			
			// Add the shutdown icon.
			RECT(r, START_X, START_Y+5*DIST_ITEMS, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, LAUNCHER_ICON5, ICON_COMPUTER_SHUTDOWN, 0);
			
			RECT(r, STEXT_X, START_Y+5*DIST_ITEMS, 200, 32);
			AddControl(pWindow, CONTROL_CLICKLABEL, r, "Shutdown", LAUNCHER_SHUTDOWN, 0, 0);
			
			// Add a testing textbox.
			RECT(r, 200, 50, 300, 15);
			
			//parms after rectangle: default text, comboID for getting the text from the textbox, max characters
			//AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, LAUNCHER_TEXTBOX1, 128, 0);
			
			//DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			break;
		}
		case EVENT_PAINT: {
			/*char test[100];
			sprintf(test, "Hi!  Memory usage: %d KB / %d KB", (npp-nfpp)*4, npp*4);
			VidFillRect (0xFF00FF, 10, 40, 100, 120);
			VidTextOut (test, 10, 30, 0, TRANSPARENT);*/
			VidFillRect(0xFFFFFF,
						3, 4 + TITLE_BAR_HEIGHT, 
						GetScreenSizeX() - WINDOW_RIGHT_SIDE_THICKNESS - 4, 
						GetScreenSizeY() - WINDOW_RIGHT_SIDE_THICKNESS - 4);
			VidDrawRect(0x7F7F7F,
						3, 4 + TITLE_BAR_HEIGHT, 
						GetScreenSizeX() - WINDOW_RIGHT_SIDE_THICKNESS - 4, 
						GetScreenSizeY() - WINDOW_RIGHT_SIDE_THICKNESS - 4);
			break;
		}
		case EVENT_COMMAND: {
			switch (parm1)
			{
				case LAUNCHER_SYSTEM:
					LaunchSystem();
					break;
				case LAUNCHER_NOTEPAD:
					LaunchNotepad();
					break;
				case LAUNCHER_PAINT:
					LaunchPaint();
					break;
				case LAUNCHER_CABINET:
					LaunchCabinet(pWindow);
					break;
				case LAUNCHER_SHUTDOWN:
					ConfirmShutdown(pWindow);
					break;
				case LAUNCHER_MENUBAR:
				{
					switch (parm2)
					{
						case 2: 
							LaunchVersion ();
							break;
					}
					break;
				}
				/*{
					//The only button:
					int randomX = GetRandom() % 320;
					int randomY = GetRandom() % 240;
					int randomColor = GetRandom();
					VidTextOut("*click*", randomX, randomY, randomColor, TRANSPARENT);
					break;
				}*/
				default:
					LogMsg("Unknown command.  Parm1: %d Parm2: %d", parm1, parm2);
					break;
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void LauncherEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = 400, wh = 270, sw = GetScreenSizeX(), sh = GetScreenSizeY();
	int wx = (sw - ww) / 2, wy = (sh - wh) / 2;
	
	Window* pWindow = CreateWindow ("Home", wx, wy, ww, wh, LauncherProgramProc, 0);//WF_NOCLOSE);
	
	if (!pWindow)
		DebugLogMsg("Hey, the main launcher window couldn't be created");
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
