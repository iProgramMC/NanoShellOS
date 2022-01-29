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
	Task* pTask = KeStartTask(SystemMonitorEntry, 0, &errorCode);
	DebugLogMsg("Created System window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchIconTest()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	DebugLogMsg("Created icon test window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchTextShell()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(TerminalHostTask, 0, &errorCode);
	//Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	DebugLogMsg("Created Text Shell window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchPaint()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(PrgPaintTask, 0, &errorCode);
	DebugLogMsg("Created Paint window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchControlPanel()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(ControlEntry, 0, &errorCode);
	DebugLogMsg("Created Control Panel window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchNotepad()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(BigTextEntry, 0, &errorCode);
	DebugLogMsg("Created Notepad window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
}
void LaunchCabinet()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(CabinetEntry, 0, &errorCode);
	DebugLogMsg("Created Cabinet window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
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
	
	LAUNCHER_LISTVIEW = 0x10,
	LAUNCHER_MENUBAR  = 0xFE,
	LAUNCHER_LABEL1   = 0xFF,
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
			
			RECT(r, START_X, TITLE_BAR_HEIGHT * 2 + 15, 200, 20);
			AddControl (pWindow, CONTROL_TEXT, r, "Welcome to NanoShell!", LAUNCHER_LABEL1, 0, TRANSPARENT);
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (TITLE_BAR_HEIGHT + TITLE_BAR_HEIGHT + 15)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 270 - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			AddControl(pWindow, CONTROL_ICONVIEW, r, NULL, LAUNCHER_LISTVIEW, 0, 0);
			
			// Add list items:
			ResetList(pWindow, LAUNCHER_LISTVIEW);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "System Monitor",    ICON_RESMON);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "File cabinet",      ICON_CABINET);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "Icon demo",         ICON_INFO);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "Text Editor",       ICON_NOTES);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "Scribble!",         ICON_DRAW);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "Command Shell",     ICON_COMMAND);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "Control Panel",     ICON_FOLDER_SETTINGS);
			AddElementToList(pWindow, LAUNCHER_LISTVIEW, "Shutdown Computer", ICON_COMPUTER_SHUTDOWN);
			
			break;
		}
		case EVENT_COMMAND: {
			switch (parm1)
			{
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
				case LAUNCHER_LISTVIEW:
				{
					switch (parm2)
					{
						case 0:
							LaunchSystem();
							break;
						case 1:
							LaunchCabinet();
							break;
						case 2:
							LaunchIconTest();
							break;
						case 3:
							LaunchNotepad();
							break;
						case 4:
							LaunchPaint();
							break;
						case 5:
							LaunchTextShell();
							break;
						case 6:
							LaunchControlPanel();
							break;
						case 7:
							ConfirmShutdown(pWindow);
							break;
					}
					break;
				}
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
	pWindow->m_iconID = ICON_HOME;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
