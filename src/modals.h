// Modal dialog box code.
#if 1

//Forward declaration
void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow);

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
	bool wnLock = g_windowLock, scLock = g_screenLock, eqLock = false;
	if  (wnLock) FREE_LOCK (g_windowLock);
	if  (scLock) FREE_LOCK (g_screenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_eventQueueLock;
		if (eqLock) FREE_LOCK (pWindow->m_eventQueueLock);
	
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
	
	char* test = MmAllocateK(strlen(pText)+5);
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
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, wSzX, wSzY, MessageBoxCallback, WF_NOCLOSE | WF_NOMINIMZ);
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
		if (eqLock) ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	}
	if (wnLock) ACQUIRE_LOCK (g_windowLock);
	if (scLock) ACQUIRE_LOCK (g_screenLock);
	return dataReturned;
}

//Null but all 0xffffffff's. Useful
#define FNULL ((void*)0xffffffff)
#define POPUP_WIDTH  400
#define POPUP_HEIGHT 120
void CALLBACK InputPopupProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND)
	{
		//Which button did we click?
		if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
		{
			//We clicked a valid button.  Return.
			
			if (parm1 == MBID_CANCEL)
			{
				pWindow->m_data = FNULL;
				return;
			}
			const char* pText = TextInputGetRawText(pWindow, 100000);
			if (!pText)
				pWindow->m_data = FNULL;
			else
				pWindow->m_data = strdup(pText);
		}
	}
	else if (messageType == EVENT_CREATE)
	{
		pWindow->m_vbeData.m_dirty = 1;
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
	}
	else if (messageType == EVENT_PAINT || messageType == EVENT_SETFOCUS || messageType == EVENT_KILLFOCUS ||
			 messageType == EVENT_CLICKCURSOR || messageType == EVENT_RELEASECURSOR)
	{
		pWindow->m_vbeData.m_dirty = 1;
		pWindow->m_renderFinished  = 1;
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
	}
	else
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

// Pops up a text box requesting an input string, and returns a MmAllocate'd
// region of memory with the text inside.  Make sure to free the result,
// if it's non-null.
//
// Returns NULL if the user cancels.
char* InputBox(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText)
{
	/*
	
	  +---------------------------------------------------------------------+
	  |                                                                     |
	  |  pPrompt text goes here                                             |
	  |                                                                     |
	  | +-----------------------------------------------------------------+ |
	  | | Your text will go here.                                         | |
	  | +-----------------------------------------------------------------+ |
	  |                                                                     |
	  |            +----------+                  +----------+               |
	  |            |    OK    |                  |  Cancel  |               |
	  |            +----------+                  +----------+               |
	  |                                                                     |
	  +---------------------------------------------------------------------+
	
	*/
	
	
	// Free the locks that have been acquired.
	bool wnLock = g_windowLock, scLock = g_screenLock, eqLock = false;
	if  (wnLock) FREE_LOCK (g_windowLock);
	if  (scLock) FREE_LOCK (g_screenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_eventQueueLock;
		if (eqLock) FREE_LOCK (pWindow->m_eventQueueLock);
	
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
	
	int wPosX = (GetScreenWidth()  - POPUP_WIDTH)  / 2;
	int wPosY = (GetScreenHeight() - POPUP_HEIGHT) / 2;
	// Spawn a new window.
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, POPUP_WIDTH, POPUP_HEIGHT, InputPopupProc, WF_NOCLOSE | WF_NOMINIMZ);
	
	// Add the basic controls required.
	Rectangle rect;
	rect.left   = 10;
	rect.top    = 12 + TITLE_BAR_HEIGHT;
	rect.right  = POPUP_WIDTH - 20;
	rect.bottom = 50;
	AddControl (pBox, CONTROL_TEXT, rect, pPrompt, 0x10000, 0, WINDOW_BACKGD_COLOR);
	
	rect.left   = 10;
	rect.top    = 12 + TITLE_BAR_HEIGHT + 20;
	rect.right  = POPUP_WIDTH - 20;
	rect.bottom = 20;
	AddControl (pBox, CONTROL_TEXTINPUT, rect, NULL, 100000, 0, 0);
	if (pDefaultText)
		SetTextInputText(pBox, 100000, pDefaultText);
	
	RECT(rect, (POPUP_WIDTH - 250)/2, POPUP_HEIGHT - 30, 100, 20);
	AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
	RECT(rect, (POPUP_WIDTH - 250)/2+150, POPUP_HEIGHT - 30, 100, 20);
	AddControl (pBox, CONTROL_BUTTON, rect, "OK", MBID_OK, 0, 0);
	
	pBox->m_data   = NULL;
	pBox->m_iconID = ICON_NULL;
	
	// Handle messages for this modal dialog window.
	while (HandleMessages(pBox))
	{
		if (pBox->m_data)
		{
			break;//we're done.
		}
	}
	
	char* dataReturned = (char*)pBox->m_data;
	
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
		if (eqLock) ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	}
	if (wnLock) ACQUIRE_LOCK (g_windowLock);
	if (scLock) ACQUIRE_LOCK (g_screenLock);
	if (dataReturned == FNULL) dataReturned = NULL;
	return dataReturned;
}

//TODO FIXME: Why does this freeze the OS when clicking on the main controlpanel window
//when I put this in kapp/cpanel.c??

//No matter, because you just advanced to a generic function!

void PopupWindowEx(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags, void* data)
{
	// Free the locks that have been acquired.
	bool wnLock = g_windowLock, scLock = g_screenLock, eqLock = false;
	if  (wnLock) FREE_LOCK (g_windowLock);
	if  (scLock) FREE_LOCK (g_screenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_eventQueueLock;
		if (eqLock) FREE_LOCK (pWindow->m_eventQueueLock);
	
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
		PaintWindowBorderNoBackgroundOverpaint (pWindow);
	}
	
	// Re-acquire the locks that have been freed before.
	if (pWindow)
	{
		if (eqLock) ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	}
	if (wnLock) ACQUIRE_LOCK (g_windowLock);
	if (scLock) ACQUIRE_LOCK (g_screenLock);
}

void PopupWindow(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags)
{
	PopupWindowEx(pWindow, newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags, NULL);
}

const uint32_t g_DefaultPickColors[] = {
	0x000000,0x00007F,0x007F00,0x007F7F,0x7F0000,0x7F007F,0x7F7F00,0x7F7F7F,
	0xa0a0a0,0x0000FF,0x00FF00,0x00FFFF,0xFF0000,0xFF00FF,0xFFFF00,0xFFFFFF,
};

const char* g_DefaultPickColorsText[] = {
	"Black",  "D-Blue", "Green", "Turq", "D-Red", "Purple",  "D-Yellow", "Grey",
	"D-Grey", "Blue",   "Lime",  "Cyan", "Red",   "Magenta", "Yellow",   "White",
};

STATIC_ASSERT(ARRAY_COUNT(g_DefaultPickColors) == ARRAY_COUNT(g_DefaultPickColorsText), "Change the other array too if adding colors");

void CALLBACK ColorPopupProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND)
	{
		//Which button did we click?
		if (parm1 >= 900 && parm1 < 900+(int)ARRAY_COUNT(g_DefaultPickColors))
		{
			SetScrollBarPos( pWindow, 100, ( g_DefaultPickColors[ parm1 - 900 ] >> 16 ) & 0xff );
			SetScrollBarPos( pWindow, 101, ( g_DefaultPickColors[ parm1 - 900 ] >>  8 ) & 0xff );
			SetScrollBarPos( pWindow, 102, ( g_DefaultPickColors[ parm1 - 900 ] >>  0 ) & 0xff );
		
			SetIcon( pWindow, 2000, g_DefaultPickColors[ parm1 - 900 ] | 0x01000000 );
			
			RequestRepaint( pWindow );
		}
		else if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
		{
			//We clicked a valid button.  Return.
			
			if (parm1 == MBID_CANCEL)
			{
				pWindow->m_data = (void*)TRANSPARENT;
				return;
			}
			
			int rd = GetScrollBarPos(pWindow, 100), gd = GetScrollBarPos(pWindow, 101), bd = GetScrollBarPos(pWindow, 102);
			uint32_t color = 0xF0000000 | rd<<16 | gd<<8 | bd;
			
			pWindow->m_data = (void*)color;
		}
	}
	else if (messageType == EVENT_CLICKCURSOR || messageType == EVENT_RELEASECURSOR)
	{
		int rd = GetScrollBarPos(pWindow, 100), gd = GetScrollBarPos(pWindow, 101), bd = GetScrollBarPos(pWindow, 102);
		uint32_t color = 0x01000000 | rd<<16 | gd<<8 | bd;
		SetIcon(pWindow, 2000, color);
		
		RequestRepaint( pWindow );
	}
	else if (messageType == EVENT_CREATE)
	{
		pWindow->m_vbeData.m_dirty = 1;
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
	}
	else if (messageType == EVENT_PAINT || messageType == EVENT_SETFOCUS || messageType == EVENT_KILLFOCUS ||
			 messageType == EVENT_RELEASECURSOR)
	{
		pWindow->m_vbeData.m_dirty = 1;
		pWindow->m_renderFinished  = 1;
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
	}
	else
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

#define COLOR_POPUP_WIDTH  500
#define COLOR_POPUP_HEIGHT 260
// Returns TRANSPARENT if the window is cancelled.
uint32_t ColorInputBox(Window* pWindow, const char* pPrompt, const char* pCaption)
{
	/*
	
	  +---------------------------------------------------------------------+
	  |                                                                     |
	  | pPrompt text goes here                                              |
	  |                                                                     |
	  | +-----------------------------------------------------------------+ |
	  | | R | < |                                                     | > | |
	  | +-----------------------------------------------------------------+ |
	  | | G | < |                                                     | > | |
	  | +-----------------------------------------------------------------+ |
	  | | B | < |                                                     | > | |
	  | +-----------------------------------------------------------------+ |
	  |                                                                     |
	  | Or choose one of the default colors:                                |
	  |                                                                     |
	  | [     ] [     ] [     ] [     ] [     ] ...                         |
	  |                                                                     |
	  |                                                                     |
	  |            +----------+                  +----------+               |
	  |            |    OK    |                  |  Cancel  |               |
	  |            +----------+                  +----------+               |
	  |                                                                     |
	  +---------------------------------------------------------------------+
	
	*/
	
	
	// Free the locks that have been acquired.
	bool wnLock = g_windowLock, scLock = g_screenLock, eqLock = false;
	if  (wnLock) FREE_LOCK (g_windowLock);
	if  (scLock) FREE_LOCK (g_screenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_eventQueueLock;
		if (eqLock) FREE_LOCK (pWindow->m_eventQueueLock);
	
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
	
	int wPosX = (GetScreenWidth()  - COLOR_POPUP_WIDTH)  / 2;
	int wPosY = (GetScreenHeight() - COLOR_POPUP_HEIGHT) / 2;
	// Spawn a new window.
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, COLOR_POPUP_WIDTH, COLOR_POPUP_HEIGHT, ColorPopupProc, WF_NOCLOSE | WF_NOMINIMZ);
	
	// Add the basic controls required.
	int y = TITLE_BAR_HEIGHT + 10;
	
	Rectangle r;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-10, 25);
	AddControl(pBox, CONTROL_TEXT, r, pPrompt, 203, 0, WINDOW_BACKGD_COLOR);
	y += 20;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-96-10, 15);
	AddControl (pBox, CONTROL_TEXT, r, "Red", 200, 0, WINDOW_BACKGD_COLOR);
	RECT(r, 96, y, COLOR_POPUP_WIDTH-96-10, 1);
	AddControl (pBox, CONTROL_HSCROLLBAR, r, NULL, 100, 0<<16|256, 1<<16|0);
	y += 32;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-96-10, 15);
	AddControl (pBox, CONTROL_TEXT, r, "Green", 201, 0, WINDOW_BACKGD_COLOR);
	RECT(r, 96, y, COLOR_POPUP_WIDTH-96-10, 1);
	AddControl (pBox, CONTROL_HSCROLLBAR, r, NULL, 101, 0<<16|256, 1<<16|0);
	y += 32;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-96-10, 15);
	AddControl (pBox, CONTROL_TEXT, r, "Blue", 202, 0, WINDOW_BACKGD_COLOR);
	RECT(r, 96, y, COLOR_POPUP_WIDTH-96-10, 1);
	AddControl (pBox, CONTROL_HSCROLLBAR, r, NULL, 102, 0<<16|256, 1<<16|0);
	
	y += 20;
	RECT(r, (COLOR_POPUP_WIDTH-100)/2, y, 100, 15);
	AddControl (pBox, CONTROL_TEXTCENTER, r, "Color Preview", 2000, 0x01000000, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED);
	
	y += 20;
	RECT(r, 10, y, COLOR_POPUP_WIDTH-10, 25);
	AddControl(pBox, CONTROL_TEXT, r, "Or choose one of the default colors:", 203, 0, WINDOW_BACKGD_COLOR);
	y += 10;
	
	RECT(r, (COLOR_POPUP_WIDTH - 250)/2, COLOR_POPUP_HEIGHT - 30, 100, 20);
	AddControl (pBox, CONTROL_BUTTON, r, "Cancel", MBID_CANCEL, 0, 0);
	RECT(r, (COLOR_POPUP_WIDTH - 250)/2+150, COLOR_POPUP_HEIGHT - 30, 100, 20);
	AddControl (pBox, CONTROL_BUTTON, r, "OK", MBID_OK, 0, 0);
	
	int bwidth = (COLOR_POPUP_WIDTH-10)/8, bheight = 25;
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			RECT(r, 10 + j * bwidth, y + i * bheight, bwidth-5, bheight-5);
			AddControl(pBox, CONTROL_BUTTON_COLORED, r, g_DefaultPickColorsText[i<<3|j], 900+(i<<3|j), (1-i)*0xffffff, g_DefaultPickColors[i<<3|j]);
		}
	}
	
	pBox->m_data   = NULL;
	pBox->m_iconID = ICON_NULL;
	
	// Handle messages for this modal dialog window.
	while (HandleMessages(pBox))
	{
		if (pBox->m_data)
		{
			break;//we're done.
		}
	}
	
	uint32_t dataReturned = (uint32_t)pBox->m_data;
	
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
		if (eqLock) ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	}
	if (wnLock) ACQUIRE_LOCK (g_windowLock);
	if (scLock) ACQUIRE_LOCK (g_screenLock);
	return dataReturned;
}


#endif
