/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

 Codename V-Builder - The tool box window
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "w_defs.h"

int g_selCtlType;

Window* g_pToolboxWindow;

extern Window* g_pFormDesignerWindow;

#define E(a) (1000 + (a))

int g_checkboxNums[] = {
	E(TOOL_CURSOR),
	E(TOOL_SELECT),
	E(CONTROL_TEXT),
	E(CONTROL_TEXTCENTER),
	E(CONTROL_BUTTON),
	E(CONTROL_TEXTINPUT),
	E(CONTROL_COUNT),
	E(CONTROL_CHECKBOX),
};

void VbToolkitOnSelect(int toolNum)
{
	for (int i = 0; i < (int)ARRAY_COUNT(g_checkboxNums); i++)
	{
		CheckboxSetChecked(g_pToolboxWindow, g_checkboxNums[i], false);
	}
	
	CheckboxSetChecked(g_pToolboxWindow, E(toolNum), true);
}

void CALLBACK PrgToolkitProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			//TODO: Clean up this function
			
			RECT(r, 3 + 0 * 24, 3 + 0 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(TOOL_CURSOR),        ICON_VB_CURSOR,      16);
			RECT(r, 3 + 1 * 24, 3 + 0 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(TOOL_SELECT),        ICON_VB_SELECT,      16);
			RECT(r, 3 + 0 * 24, 3 + 1 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_TEXT),       ICON_VB_TEXT,        16);
			RECT(r, 3 + 1 * 24, 3 + 1 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_TEXTCENTER), ICON_VB_TEXT_CEN,    16);
			RECT(r, 3 + 1 * 24, 3 + 2 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_TEXTINPUT),  ICON_VB_INPUT_1LINE, 16);
			RECT(r, 3 + 0 * 24, 3 + 2 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_COUNT),      ICON_VB_INPUT_MLINE, 16);
			RECT(r, 3 + 0 * 24, 3 + 3 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_BUTTON),     ICON_VB_BUTTON,      16);
			RECT(r, 3 + 1 * 24, 3 + 3 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_CHECKBOX),   ICON_VB_CHECKBOX,    16);
#undef E
			
			VbToolkitOnSelect(TOOL_CURSOR);
			
			break;
		}
		case EVENT_CHECKBOX:
		{
			// we ignore the second parameter
			VbToolkitOnSelect(parm1 - 1000);
			
			int *pType = &g_selCtlType;
			*pType = parm1 - 1000;
			
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
			if (parm1 - 1000 == TOOL_CURSOR)
				ChangeCursor (g_pFormDesignerWindow, CURSOR_DEFAULT);
			else
				ChangeCursor (g_pFormDesignerWindow, CURSOR_CROSS);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void VbCreateToolboxWindow()
{
	// The tool box window
	Window* pToolsWindow = CreateWindow ("Toolbox", 100 - D_OFFSET, 180 - D_OFFSET, DEF_TOOLBOX_WID, DEF_TOOLBOX_HEI, PrgToolkitProc, WF_NOCLOSE | WF_NOMINIMZ | WF_SYSPOPUP);
	
	if (!pToolsWindow)
	{
		exit(1);
	}
	
	SetWindowIcon(pToolsWindow, ICON_NULL);
	SetWindowData(pToolsWindow, g_pFormDesignerWindow);
	
	g_pToolboxWindow = pToolsWindow;
}



