/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Modals - Message Box dialog
******************************************/
#include <window.h>
#include <widget.h>
#include <icon.h>

extern SafeLock
g_WindowLock, 
g_ScreenLock, 
g_BufferLock, 
g_CreateLock, 
g_BackgdLock;
extern VBEData* g_vbeData, g_mainScreenVBEData;
extern void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow);
extern void SelectWindow(Window* pWindow);

void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2)
{
	DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

void CALLBACK MessageBoxCallback (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND)
	{
		//Which button did we click?
		if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
		{
			//We clicked a valid button.  Return.
			pWindow->m_data = (void*)parm1;
		}
	}
	else
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

//TODO FIXME: Moving this anywhere but here causes a strange bug where moving this out
//causes attempting to switch to the back window to freeze the whole system for whatever reason.
//The code itself has NO cli's nor does it ever call cli, so why the mouse freezes I don't know.
//It's also worth noting that once you switch to another window clicking the back window no longer freezes.
//This does NOT happen when the code sits right here.

//I think this is a deadlock, HOWEVER, I don't know why the position of the code matters there at all.

//TODO: You can now safely move this out (I think), as I've totally revamped the way that mouse events send to
//parent windows.

int MessageBox (Window* pWindow, const char* pText, const char* pCaption, uint32_t style)
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
			PaintWindowBorderNoBackgroundOverpaint (pWindow);
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
	
	int szX, szY;
	
	char* test = MmAllocateK(strlen(pText) * 2);
	int buttonStyle = style & 0x7;
	if (!test)
	{
		SLogMsg("Cannot allocate text buffer, returning default");
		SLogMsg("%s", pText);
		SLogMsg("Msgbox parms: %x", style);
		
		switch (buttonStyle)
		{
			case MB_OK: return MBID_OK;
			case MB_OKCANCEL:
			case MB_RETRYCANCEL:
			case MB_CANCELTRYCONTINUE: 
			case MB_YESNOCANCEL: return MBID_CANCEL;
			case MB_YESNO: return MBID_NO;
			case MB_ABORTRETRYIGNORE: return MBID_IGNORE;
		}
	}
	WrapText(test, pText, GetScreenWidth() * 2 / 3);
	
	// Measure the pText text.
	VidTextOutInternal (test, 0, 0, 0, 0, true, &szX, &szY);
	
	szY += 12;
	
	int  iconID = style >> 16;
	bool iconAvailable = iconID != ICON_NULL;
	
	if (iconAvailable)
		if (szY < 50)
			szY = 50;
	
	int buttonWidth  = 70;
	int buttonWidthG = 76;
	int buttonHeight = 20;
	
	// We now have the text's size in szX and szY.  Get the window size.
	int wSzX = szX + 
			40 + //X padding on both sides
			10 + //Gap between icon and text.
			32 * iconAvailable + //Icon's size.
			5 +
			WINDOW_RIGHT_SIDE_THICKNESS;//End.
	int wSzY = szY + 
			20 + //Y padding on both sides
			buttonHeight + //Button's size.
			TITLE_BAR_HEIGHT +
			5 + 
			WINDOW_RIGHT_SIDE_THICKNESS;
	
	int wPosX = (GetScreenSizeX() - wSzX) / 2,
		wPosY = (GetScreenSizeY() - wSzY) / 2;
	
	// Spawn a new window.
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, wSzX, wSzY, MessageBoxCallback, WF_NOCLOSE | WF_NOMINIMZ | WI_MESSGBOX);
	if (!pBox)
	{
		MmFreeK(test);
		SLogMsg("Cannot show window, returning default");
		SLogMsg("%s", pText);
		SLogMsg("Msgbox parms: %x", style);
		
		switch (buttonStyle)
		{
			case MB_OK: return MBID_OK;
			case MB_OKCANCEL:
			case MB_RETRYCANCEL:
			case MB_CANCELTRYCONTINUE: 
			case MB_YESNOCANCEL: return MBID_CANCEL;
			case MB_YESNO: return MBID_NO;
			case MB_ABORTRETRYIGNORE: return MBID_IGNORE;
		}
	}
	
	wSzX = pBox->m_rect.right - pBox->m_rect.left;
	
	// Add the basic controls required.
	Rectangle rect;
	rect.left   = 20 + iconAvailable*32 + 10;
	rect.top    = TITLE_BAR_HEIGHT + 2;
	rect.right  = wSzX - 20;
	rect.bottom = wSzY - buttonHeight - 20;
	AddControl (pBox, CONTROL_TEXTHUGE, rect, NULL, 0x10000, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED);
	SetHugeLabelText(pBox, 0x10000, test);
	
	MmFreeK(test);
	
	if (iconAvailable)
	{
		rect.left = 20;
		rect.top  = TITLE_BAR_HEIGHT + 2 + (szY - 32) / 2;
		rect.right = rect.left + 32;
		rect.bottom= rect.top  + 32;
		AddControl (pBox, CONTROL_ICON, rect, NULL, 0x10001, iconID, 0);
	}
	
	switch (buttonStyle)
	{
		case MB_OK:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "OK", MBID_OK, 0, 0);
			SetDisabledControl(pBox, MBID_OK, true);
			break;
		}
		case MB_RESTART:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Restart", MBID_OK, 0, 0);
			break;
		}
		case MB_YESNOCANCEL:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "No", MBID_NO, 0, 0);
			rect.right -= buttonWidthG;
			rect.left  -= buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Yes", MBID_YES, 0, 0);
			rect.right += 2 * buttonWidthG;
			rect.left  += 2 * buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			break;
		}
		case MB_ABORTRETRYIGNORE:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Retry", MBID_RETRY, 0, 0);
			rect.right -= buttonWidthG;
			rect.left  -= buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Abort", MBID_ABORT, 0, 0);
			rect.right += 2 * buttonWidthG;
			rect.left  += 2 * buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Ignore", MBID_IGNORE, 0, 0);
			break;
		}
		case MB_CANCELTRYCONTINUE:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Try again", MBID_TRY_AGAIN, 0, 0);
			rect.right -= buttonWidthG;
			rect.left  -= buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			rect.right += 2 * buttonWidthG;
			rect.left  += 2 * buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Continue", MBID_CONTINUE, 0, 0);
			break;
		}
		case MB_YESNO:
		{
			rect.left = (wSzX - buttonWidthG * 2) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Yes", MBID_YES, 0, 0);
			rect.right += buttonWidthG;
			rect.left  += buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "No", MBID_NO, 0, 0);
			break;
		}
		case MB_OKCANCEL:
		{
			rect.left = (wSzX - buttonWidthG * 2) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "OK", MBID_OK, 0, 0);
			rect.right += buttonWidthG;
			rect.left  += buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			break;
		}
		case MB_RETRYCANCEL:
		{
			rect.left = (wSzX - buttonWidthG * 2) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Retry", MBID_RETRY, 0, 0);
			rect.right += buttonWidthG;
			rect.left  += buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			break;
		}
	}
	
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
		PaintWindowBorderNoBackgroundOverpaint (pWindow);
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
