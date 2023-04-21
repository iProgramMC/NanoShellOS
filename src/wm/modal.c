/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

         Modal Window Util Module
******************************************/
#include "wi.h"

//Forward declarations
void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2);

void PopupWindowEx(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags, void* data)
{
	// Free the locks that have been acquired.
	bool wnLock = g_WindowLock.m_held, scLock = g_ScreenLock.m_held, eqLock = false;
	if  (wnLock) LockFree (&g_WindowLock);
	if  (scLock) LockFree (&g_ScreenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_EventQueueLock.m_held;
		if (eqLock) LockFree (&pWindow->m_EventQueueLock);
	
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
		while (HandleMessages(pSubWindow))
		{
			KeTaskDone();
		}
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
	
	// Re-acquire the locks that have been freed before.
	if (pWindow)
	{
		if (eqLock) LockAcquire (&pWindow->m_EventQueueLock);
	}
	if (wnLock) LockAcquire (&g_WindowLock);
	if (scLock) LockAcquire (&g_ScreenLock);
}

void PopupWindow(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags)
{
	PopupWindowEx(pWindow, newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags, NULL);
}
