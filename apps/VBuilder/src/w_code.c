/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Codename V-Builder - The code window
******************************************/
#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "w_defs.h"

enum
{
	CODE_TEXTINPUT = 1000,
};

char * g_SourceCode = NULL;

Window* g_pCodeWindow;

void VbInitCode()
{
	g_SourceCode = strdup(
		"# This is merely an example of what you can do in NanoShell Codename V-Builder!\n"
		"# Create a button with the name 'Button1', then preview the form and click the button.\n"
		"# You should now see in whatever console this program is run from, the string 'hello world'.\n"
		"\n"
		"function Test\n"
		"{\n"
		"\techo(\"hello world\");\n"
		"}\n"
		"\n"
		"function Button1_Click\n"
		"{\n"
		"\tTest();\n"
		"}\n"
		"\n"
	);
}

void VbRefreshCode(Window* pWindow)
{
	const char* rawText = TextInputGetRawText(pWindow, CODE_TEXTINPUT);
	
	size_t sl = strlen(rawText);
	
	char* src = realloc(g_SourceCode, sl + 1);
	if (!src)
	{
		// this is awkward
		LogMsg("Couldn't refresh code, try again later?");
		return;
	}
	
	g_SourceCode = src;
	
	strcpy(src, rawText);
}

void CALLBACK PrgCodeProc(Window* pWindow, int eventType, int parm1, int parm2)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			Rectangle r = { 10, 10 + TITLE_BAR_HEIGHT, DEF_CODEWIN_WID - 10, DEF_CODEWIN_HEI - 10 };
			
			AddControlEx(pWindow, CONTROL_TEXTINPUT, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, CODE_TEXTINPUT, TEXTEDIT_MULTILINE | TEXTEDIT_LINENUMS, 0);
			
			SetTextInputText(pWindow, CODE_TEXTINPUT, g_SourceCode);
			
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
