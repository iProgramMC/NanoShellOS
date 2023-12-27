/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

         Modal Window Util Module
******************************************/
#include "wi.h"

void PopupWindowEx(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags, void* data)
{
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		wasSelectedBefore = pWindow->m_isSelected;
		if (wasSelectedBefore)
		{
			pWindow->m_isSelected = false;
			WmPaintWindowTitle(pWindow);
		}
	}
	
	VBEData* pBackup = g_vbeData;
	
	VidSetVBEData(NULL);
	// Freeze the current window.
	int old_flags = 0;
	WindowProc pProc;
	if (pWindow)
	{
		pProc = pWindow->m_callback;
		old_flags = pWindow->m_flags;
		//pWindow->m_callback = MessageBoxWindowLightCallback;
		pWindow->m_flags |= WF_FROZEN;//Do not respond to user attempts to move/other
	}
	
	Window* pSubWindow = CreateWindow(newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags);
	if (pSubWindow)
	{
		pSubWindow->m_data = data;
		while (HandleMessages(pSubWindow));
	}
	
	if (pWindow)
	{
		pWindow->m_callback = pProc;
		pWindow->m_flags    = old_flags;
	}
	g_vbeData = pBackup;
	
	//NB: No null dereference, because if pWindow is null, wasSelectedBefore would be false anyway
	if (wasSelectedBefore)
	{
		//pWindow->m_isSelected = true;
		SelectWindow (pWindow);
		WmPaintWindowTitle (pWindow);
	}
}

void PopupWindow(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags)
{
	PopupWindowEx(pWindow, newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags, NULL);
}
