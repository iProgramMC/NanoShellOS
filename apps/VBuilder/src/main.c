/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

  Codename V-Builder - The form designer
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "s_all.h"
#include "w_defs.h"

extern Window* g_pToolboxWindow;
extern Window* g_pFormDesignerWindow;
extern Window* g_pMainWindow;
extern Window* g_pPreviewWindow;

void KillWindow(Window* pWindow)
{
	DestroyWindow(pWindow);
	while (HandleMessages(pWindow));
}

int NsMain(UNUSED int argc, UNUSED char** argv)
{
	// The form builder window
	VbCreateMainWindow();
	VbCreateFormDesignerWindow();
	VbCreateToolboxWindow();
	
	SetWindowData(g_pMainWindow,  g_pFormDesignerWindow);
	
	// event loop:
	while (true)
	{
		bool b1 = HandleMessages (g_pMainWindow);
		bool b2 = HandleMessages (g_pFormDesignerWindow);
		bool b3 = HandleMessages (g_pToolboxWindow);
		bool b4 = false;
		
		if (g_pPreviewWindow)
			b4 = HandleMessages (g_pPreviewWindow);
		
		if (!b4)
			g_pPreviewWindow = NULL;
		
		if (!b1) g_pMainWindow         = NULL;
		if (!b2) g_pFormDesignerWindow = NULL;
		if (!b3) g_pToolboxWindow      = NULL;
		
		if (!b1 || !b2 || !b3)
		{
			if (g_pMainWindow)
			{
				KillWindow(g_pMainWindow);
				g_pMainWindow = NULL;
			}
			if (g_pFormDesignerWindow)
			{
				KillWindow(g_pFormDesignerWindow);
				g_pFormDesignerWindow = NULL;
			}
			if (g_pToolboxWindow)
			{
				KillWindow(g_pToolboxWindow);
				g_pToolboxWindow = NULL;
			}
			if (g_pPreviewWindow)
			{
				KillWindow(g_pPreviewWindow);
				g_pPreviewWindow = NULL;
			}
			break;
		}
	}
	
	return 0;
}
