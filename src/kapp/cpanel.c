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
	CONTPNL_LISTVIEW = 0x10,
	CONTPNL_MENUBAR  = 0xFE,
};
extern VBEData g_mainScreenVBEData;
extern bool    g_ps2MouseAvail;
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
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (TITLE_BAR_HEIGHT + 5)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 260 - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			AddControl(pWindow, CONTROL_ICONVIEW, r, NULL, CONTPNL_LISTVIEW, 0, 0);
			
			// Add list items:
			ResetList(pWindow, CONTPNL_LISTVIEW);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Display",             ICON_ADAPTER);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Keyboard",            ICON_KEYBOARD);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Launcher",            ICON_HOME);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Environment Paths",   ICON_DIRECTIONS);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Permissions",         ICON_RESTRICTED);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Serial Port",         ICON_SERIAL);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Mouse",               ICON_HAND);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Download over Serial",ICON_BILLBOARD);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Date and Time",       ICON_CLOCK);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Password Lock",       ICON_LOCK);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Terminal settings",   ICON_COMMAND);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Desktop",             ICON_DESKTOP);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "App Memory Limit",    ICON_RESMON);
			
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
					//NOTE: These are just mock for now.
					case 0:
					{
						char buff[2048];
						sprintf (buff, 
							"Display: %s\n"
							"Driver Name: %s\n\n"
							"Screen Size: %d x %d\n\n"
							"Framebuffer map address: 0x%X",
							
							"Generic VESA VBE-capable device",
							"NanoShell Basic VBE Display Driver",
							GetScreenWidth(), GetScreenHeight(),
							g_mainScreenVBEData.m_framebuffer32
						);
						MessageBox(pWindow, buff, "Display adapter info", MB_OK | ICON_ADAPTER << 16);
						break;
					}
					case 1:
					{
						char buff[2048];
						sprintf (buff, 
							"Keyboard: %s\n"
							"Driver Name: %s",
							
							"Generic 101/102 Key PS/2 Keyboard HID device",
							"NanoShell Basic PS/2 Keyboard Driver",
							GetScreenWidth(), GetScreenHeight()
						);
						MessageBox(pWindow, buff, "Keyboard info", MB_OK | ICON_KEYBOARD << 16);
						break;
					}
					case 6:
					{
						char buff[2048];
						sprintf (buff, 
							"Mouse: %s\n"
							"Driver Name: %s",
							
							//TODO: fill in info such as 'does it support intellimouse?'
							g_ps2MouseAvail ? "Generic PS/2 Mouse HID device" : "Fake Mouse Driver controlled by keyboard",
							"NanoShell Basic Mouse Driver",
							GetScreenWidth(), GetScreenHeight()
						);
						MessageBox(pWindow, buff, "Mouse info", MB_OK | ICON_HAND << 16);
						break;
					}
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
