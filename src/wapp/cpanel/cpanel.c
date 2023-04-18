/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Control panel Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <wtheme.h>
#include <vfs.h>
#include <elf.h>
#include <keyboard.h>

void CplMouse   (Window* pWindow);
void CplDisplay (Window* pWindow);
void CplDesktop (Window* pWindow);
void CplTaskbar (Window* pWindow);
void CplTerminal(Window* pWindow);
void CplKeyboard(Window* pWindow);


enum {
	CONTPNL_LISTVIEW = 0x10,
	CONTPNL_MENUBAR  = 0xFE,
};

int state=0;
void KbSetLedStatus(uint8_t status);

void PopupWindow(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags);
void CALLBACK Cpl$WndProc (Window* pWindow, int messageType, int parm1, int parm2)
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
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (5)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 260 - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			AddControlEx(pWindow, CONTROL_ICONVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, CONTPNL_LISTVIEW, 0, 0);
			
			// Add list items:
			ResetList(pWindow, CONTPNL_LISTVIEW);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Display",             ICON_ADAPTER);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Keyboard",            ICON_KEYBOARD);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Mouse",               ICON_MOUSE);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Desktop",             ICON_DESKTOP);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Terminal settings",   ICON_COMMAND);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Launcher",            ICON_HOME);
			/*
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Environment Paths",   ICON_DIRECTIONS);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Permissions",         ICON_RESTRICTED);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Serial Port",         ICON_SERIAL);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Download over Serial",ICON_BILLBOARD);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Date and Time",       ICON_CLOCK);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Password Lock",       ICON_LOCK);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "App Memory Limit",    ICON_RESMON);
			*/
			
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
			else if (parm1 == CONTPNL_LISTVIEW)
			{
				switch (parm2)
				{
					case 0:
						CplDisplay(pWindow);
						break;
					case 1:
						CplKeyboard(pWindow);
						break;
					case 2:
						CplMouse(pWindow);
						break;
					case 3:
						CplDesktop(pWindow);
						break;
					case 4:
						CplTerminal(pWindow);
						break;
					case 5:
						CplTaskbar(pWindow);
						break;
					default:
						MessageBox(pWindow, "Not Implemented!", "Control Panel", MB_OK | ICON_WARNING << 16);
						break;
				}
			}
			else
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
	
	Window* pWindow = CreateWindow ("Control Panel", wx, wy, ww, wh, Cpl$WndProc, WF_ALWRESIZ);//WF_NOCLOSE);
	pWindow->m_iconID = ICON_FOLDER_SETTINGS;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	while (HandleMessages (pWindow));
}
