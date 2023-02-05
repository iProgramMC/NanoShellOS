/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

  Window Modals - Shutdown Picker dialog
******************************************/
#include <window.h>
#include <widget.h>
#include <wbuiltin.h>
#include <icon.h>

extern SafeLock
g_WindowLock, 
g_ScreenLock, 
g_BufferLock, 
g_CreateLock, 
g_BackgdLock;
extern VBEData* g_vbeData, g_mainScreenVBEData;
extern void WmRepaintBorder(Window* pWindow);
extern void SelectWindow(Window* pWindow);
extern void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2);

void CALLBACK ShutdownBoxCallback (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND || messageType == EVENT_CHECKBOX)
	{
		//Which button did we click?
		switch (parm1)
		{
			case 105:
				//We clicked a valid button.  Return.
				/**/ if (CheckboxGetChecked (pWindow, 102))
					pWindow->m_data = (void*)MBID_ABORT;
				else if (CheckboxGetChecked (pWindow, 103))
					pWindow->m_data = (void*)MBID_RETRY;
				else if (CheckboxGetChecked (pWindow, 104))
					pWindow->m_data = (void*)MBID_IGNORE;
				break;
			case 102:
			case 103:
			case 104:
				CheckboxSetChecked(pWindow, 102, parm1 == 102);
				CheckboxSetChecked(pWindow, 103, parm1 == 103);
				CheckboxSetChecked(pWindow, 104, parm1 == 104);
				CallWindowCallbackAndControls (pWindow, EVENT_PAINT, 0, 0);
				break;
		}
	}
	else
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

// MBID_ABORT = Shut down, MBID_RETRY = Restart, MBID_IGNORE = Do nothing
int ShutdownBox (Window* pWindow)
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
			WmRepaintBorder (pWindow);
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
		pWindow->m_callback = MessageBoxWindowLightCallback;
		pWindow->m_flags |= WF_FROZEN;//Do not respond to user attempts to move/other
	}
	
	int wSzX = 320, wSzY = 170 + TITLE_BAR_HEIGHT - 18;
	
	int wPosX = (GetScreenWidth () - wSzX) / 2;
	int wPosY = (GetScreenHeight() - wSzY) / 2;
	
	// Spawn a new window.
	Window* pBox = CreateWindow ("Shutdown Computer", wPosX, wPosY, wSzX, wSzY, ShutdownBoxCallback, WF_NOCLOSE | WF_NOMINIMZ | WI_MESSGBOX);
	if (!pBox)
	{
		SLogMsg("Cannot show window, returning default");
		
		return MBID_IGNORE;
	}
	
	Rectangle rect;
	RECT (rect, 20, 10 + TITLE_BAR_HEIGHT, 32, 32);
	AddControl (pBox, CONTROL_ICON, rect, NULL, 100, ICON_EXIT, 0);
	RECT (rect, 80, 20 + TITLE_BAR_HEIGHT, 200, 32);
	AddControl (pBox, CONTROL_TEXT, rect, "You want to:", 101, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
	RECT (rect, 80, 45 + TITLE_BAR_HEIGHT, 200, 22);
	AddControl (pBox, CONTROL_CHECKBOX, rect, "Shutdown", 102, 0, 0);
	RECT (rect, 80, 70 + TITLE_BAR_HEIGHT, 200, 22);
	AddControl (pBox, CONTROL_CHECKBOX, rect, "Shutdown and Restart", 103, 0, 0);
	RECT (rect, 80, 95 + TITLE_BAR_HEIGHT, 200, 22);
	AddControl (pBox, CONTROL_CHECKBOX, rect, "Continue to NanoShell", 104, 0, 0);
	RECT (rect, (wSzX - 50) / 2, 120 + TITLE_BAR_HEIGHT, 50, 20);
	AddControl (pBox, CONTROL_BUTTON, rect, "OK", 105, 0, 0);
	
	CheckboxSetChecked(pBox, 102, true);
	
	pBox->m_iconID = ICON_NULL;
	
	// Handle messages for this modal dialog window.
	while (HandleMessages(pBox))
	{
		if (pBox->m_data)
		{
			break;//we're done.
		}
		//hlt;
		KeTaskDone();
	}
	
	int dataReturned = (int)pBox->m_data;
	
	DestroyWindow(pBox);
	while (HandleMessages(pBox));
	
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
		WmRepaintBorder (pWindow);
	}
	
	// Re-acquire the locks that have been freed before.
	if (pWindow)
	{
		if (eqLock) LockAcquire (&pWindow->m_EventQueueLock);
	}
	if (wnLock) LockAcquire (&g_WindowLock);
	if (scLock) LockAcquire (&g_ScreenLock);
	return dataReturned;
}
