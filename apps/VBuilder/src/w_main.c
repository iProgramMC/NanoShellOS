/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Codename V-Builder - The main window
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "w_defs.h"

extern DesignerControl* g_controlsFirst, *g_controlsLast;
extern Window* g_pFormDesignerWindow;
extern Window* g_pPreviewWindow;

DesignerControl* g_pEditedControl;

Window* g_pMainWindow;

void VbSetEditedControl2(DesignerControl* pControl)
{
	if (pControl)
	{
		SetTextInputText(g_pMainWindow, CO_EDITED_CTL, pControl->m_name);
	}
	else
	{
		SetTextInputText(g_pMainWindow, CO_EDITED_CTL, "");
	}
	
	CallWindowCallbackAndControls(g_pMainWindow, EVENT_PAINT, 0, 0);
}

void VbSetEditedControl(DesignerControl* pControl)
{
	RegisterEvent(g_pMainWindow, EVENT_USER, (int)pControl, 0);
}

#define SELECT_CTL_WIDTH  (150)
#define SELECT_CTL_HEIGHT (200 + TITLE_BAR_HEIGHT)

void CALLBACK PrgVbSelectCtlProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			
			RECT(r, 10, TITLE_BAR_HEIGHT + 10, SELECT_CTL_WIDTH - 20, SELECT_CTL_HEIGHT - 50 - TITLE_BAR_HEIGHT);
			
			AddControl(pWindow, CONTROL_LISTVIEW, r, NULL, 1000, 0, 0);
			
			RECT(r, SELECT_CTL_WIDTH - 80, SELECT_CTL_HEIGHT - 30, 70, 20);
			
			AddControl(pWindow, CONTROL_BUTTON, r, "OK", 1001, 0, 0);
			
			// Add whatever's in the list
			for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
			{
				AddElementToList(pWindow, 1000, C->m_name, ICON_FORWARD);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == 1001)
			{
				// Get the element.
				DesignerControl* element = NULL;
				
				int index = GetSelectedIndexList(pWindow, 1000);
				const char* str = GetElementStringFromList(pWindow, 1000, index);
				
				if (str)
				{
					for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
					{
						if (strcmp(C->m_name, str) == 0)
						{
							element = C;
							break;
						}
					}
					
					*((DesignerControl**)GetWindowData(pWindow)) = element;
				}
				
				DestroyWindow(pWindow);
			}
			break;
		}
		default:
		{
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
		}
	}
}

void VbSelectControlDialog()
{
	Rectangle mwr;
	GetWindowRect(g_pMainWindow, &mwr);
	
	DesignerControl* ctler = NULL;
	
	PopupWindowEx(g_pMainWindow, "Select Control", mwr.left + 50, mwr.top + 50, SELECT_CTL_WIDTH, SELECT_CTL_HEIGHT, PrgVbSelectCtlProc, WF_SYSPOPUP | WF_NOMINIMZ, &ctler);
	
	for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
		C->m_sele = false;
	
	if (ctler)
	{
		ctler->m_sele = true;
	}
	
	VbSetEditedControl2(ctler);
	
	RegisterEvent(g_pFormDesignerWindow, EVENT_PAINT, 0, 0);
}

void VbCodeWindow()
{
	LogMsg("Code window!");
}

void CALLBACK PrgVbMainWndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_USER:
		{
			VbSetEditedControl2((DesignerControl*)parm1);
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r = {0,0,0,0};
			
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, CO_MENU_BAR, 0, 0);
			
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 1, "File");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 2, "Edit");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 3, "View");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 4, "Window");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 5, "Help");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 5, 6, "About Codename V-Builder...");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 3, 7, "Preview...");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 3, 8, "");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 3, 9, "Exit");
			
			//#define DEF_VBUILD_HEI (TITLE_BAR_HEIGHT*2 + 6 + TEXT_BOX_HEIGHT + 6)
			int thingY = TITLE_BAR_HEIGHT * 2 + 6;
			
			RECT(r, 10, thingY, 120, TEXT_BOX_HEIGHT);
			
			AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, CO_EDITED_CTL, TEXTEDIT_READONLY, 0);
			
			RECT(r, 132, thingY, TEXT_BOX_HEIGHT - 1, TEXT_BOX_HEIGHT - 1);
			
			AddControl(pWindow, CONTROL_BUTTON_ICON, r, NULL, CO_EDITED_CHOOSE, ICON_BROWSE_SMALL, 16);
			
			RECT(r, 132 + TEXT_BOX_HEIGHT * 1, thingY, TEXT_BOX_HEIGHT - 1, TEXT_BOX_HEIGHT - 1);
			AddControl(pWindow, CONTROL_BUTTON_ICON, r, NULL, CO_COMPILE, ICON_PLAY_SMALL, 16);
			
			RECT(r, 132 + TEXT_BOX_HEIGHT * 2, thingY, TEXT_BOX_HEIGHT - 1, TEXT_BOX_HEIGHT - 1);
			AddControl(pWindow, CONTROL_BUTTON_ICON, r, NULL, CO_STOP, ICON_STOP_SMALL, 16);
			
			RECT(r, 132 + TEXT_BOX_HEIGHT * 3, thingY, TEXT_BOX_HEIGHT - 1, TEXT_BOX_HEIGHT - 1);
			AddControl(pWindow, CONTROL_BUTTON_ICON, r, NULL, CO_SHOW_CODE, ICON_FILE_WRITE, 16);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 != CO_MENU_BAR)
			{
				switch (parm1)
				{
					case CO_EDITED_CHOOSE:
						VbSelectControlDialog();
						break;
					case CO_COMPILE:
						VbPreviewWindow();
						break;
					case CO_SHOW_CODE:
						VbCodeWindow();
						break;
					case CO_STOP:
						if (!g_pPreviewWindow)
							MessageBox(pWindow, "The preview is not running.", "Codename V-Builder", MB_OK);
						else
							RegisterEvent(g_pPreviewWindow, EVENT_DESTROY, 0, 0);
						break;
				}
				
				break;
			}
			
			switch (parm2)
			{
				case 9:
					DefaultWindowProc(pWindow, EVENT_DESTROY, 0, 0);
					break;
				case 6:
					ShellAbout("Codename V-Builder", ICON_DLGEDIT);
					break;
				case 7:
					VbPreviewWindow();
					break;
			}
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void VbCreateMainWindow()
{
	Window* pMainWindow = CreateWindow ("Codename V-Builder", 100, 100, DEF_VBUILD_WID, DEF_VBUILD_HEI, PrgVbMainWndProc, WF_ALWRESIZ);
	
	if (!pMainWindow) exit(1);
	
	SetWindowIcon(pMainWindow, ICON_DLGEDIT);
	g_pMainWindow = pMainWindow;
}
