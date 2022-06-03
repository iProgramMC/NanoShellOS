/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Window Modals - Text Input dialog
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
extern void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow);
extern void SelectWindow(Window* pWindow);
extern void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2);

//Null but all 0xffffffff's. Useful
#define FNULL ((void*)0xffffffff)
#define POPUP_WIDTH  (400)
#define POPUP_HEIGHT (120-18+TITLE_BAR_HEIGHT)
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
			{
				size_t len = strlen (pText);
				pWindow->m_data = MmAllocateK(len + 1);
				strcpy (pWindow->m_data, pText);
			}
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
	
	char* dataReturned = NULL;
	if (pBox->m_data != FNULL)
	{
		char*  data1 = (char*)pBox->m_data;
		size_t leng1 = strlen (data1) + 1;
		
		dataReturned = MmAllocate (leng1);//allocate it on the user heap
		memcpy (dataReturned, data1, leng1);
		MmFreeK(data1);
	}
	
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
	if (dataReturned == FNULL) dataReturned = NULL;
	return dataReturned;
}
