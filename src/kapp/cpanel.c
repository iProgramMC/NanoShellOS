/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Control panel Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <vfs.h>
#include <elf.h>


enum {
	CONTPNL_SYSTEM = 0x10,
	CONTPNL_NOTEPAD,
	CONTPNL_PAINT,
	CONTPNL_CABINET,
	CONTPNL_TEXTBOX1,
	
	
	CONTPNL_LABEL1 = 0xE0,
	CONTPNL_LABEL2,
	CONTPNL_LABEL3,
	CONTPNL_LABEL4,
	CONTPNL_LABEL5,
	CONTPNL_ICON1 = 0xF0,
	CONTPNL_ICON2,
	CONTPNL_ICON3,
	CONTPNL_ICON4,
	CONTPNL_ICON5,
	
	CONTPNL_MENUBAR  = 0xFE,
	CONTPNL_SHUTDOWN = 0xFF,
};

void CALLBACK ControlProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
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
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, CONTPNL_MENUBAR, 0, 0);
			
			// Add some testing elements to the menubar.  A comboID of zero means you're adding to the root.
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 0, 1, "Settings");
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 0, 2, "Help");
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 1, 3, "Exit");
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 2, 4, "About Control Panel...");
			
			break;
		}
		case EVENT_PAINT: {
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
			if (parm1 == CONTPNL_MENUBAR)
			{
				switch (parm2)
				{
					case 3:
						DestroyWindow(pWindow);
						break;
					case 4:
						LaunchVersion();
						break;
				}
			}
			LogMsg("Unknown command event.  Parm1: %d Parm2: %d", parm1, parm2);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void ControlEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = 400, wh = 260;
	int wx = 150, wy = 150;
	
	Window* pWindow = CreateWindow ("Control Panel", wx, wy, ww, wh, ControlProgramProc, 0);//WF_NOCLOSE);
	pWindow->m_iconID = ICON_FOLDER_SETTINGS;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
