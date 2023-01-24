/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Codename V-Builder - The code window
******************************************/
#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "w_defs.h"


char * g_SourceCode = NULL;

Window* g_pCodeWindow;

void VbInitCode()
{
	g_SourceCode = strdup("function Test\n{\n\techo(\"hello world\");\n}\n\nfunction Button1_Click\n{\n\tTest();\n}\n\n");
}

void VbRefreshCode(Window* pWindow)
{
	
}

void CALLBACK PrgCodeProc(Window* pWindow, int eventType, int parm1, int parm2)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			Rectangle r = { 10, 10 + TITLE_BAR_HEIGHT, DEF_CODEWIN_WID - 10, DEF_CODEWIN_HEI - 10 };
			
			AddControlEx(pWindow, CONTROL_TEXTINPUT, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, 1000, TEXTEDIT_MULTILINE | TEXTEDIT_LINENUMS, 0);
			
			SetTextInputText(pWindow, 1000, g_SourceCode);
			
			break;
		}
		case EVENT_SETFOCUS:
		case EVENT_KILLFOCUS:
		{
			VbRefreshCode(pWindow);
			DefaultWindowProc(pWindow, eventType, parm1, parm2); // TODO: maybe call this by default?
			break;
		}
		default:
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			break;
	}
}

void VbCreateCodeWindow()
{
	if (g_pCodeWindow)
	{
		return;
	}
	
	Window * pWindow = CreateWindow("Code Window", 300, 180+60-18+TITLE_BAR_HEIGHT, DEF_CODEWIN_WID, DEF_CODEWIN_HEI, PrgCodeProc, WF_ALWRESIZ);
	
	if (!pWindow)
	{
		LogMsg("Could not create code window.");
		return;
	}
	
	g_pCodeWindow = pWindow;
}
