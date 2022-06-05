/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Modals - File Picker dialog
******************************************/
#include <window.h>
#include <widget.h>
#include <vfs.h>
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

IconType CabGetIconBasedOnName(const char *pName, int pType);//btw
void FilePickerUpdate (Window *pWindow)
{
	// Get the directory path
	const char* pCwd = TextInputGetRawText(pWindow, 100001);
	if (!pCwd)
	{
		MessageBox(pWindow, "The OS really messed up", "Assert", ICON_ERROR << 16 | MB_OK);
		return;
	}
	
	ResetList (pWindow, 100002);
	
	if (strcmp (pCwd, "/")) //if can go to parent, add a button
		AddElementToList (pWindow, 100002, "..", ICON_FOLDER_PARENT);
	
	FileNode *pFolderNode = FsResolvePath (pCwd);
	
	DirEnt* pEnt = NULL;
	int i = 0;
	
	if (!FsOpenDir(pFolderNode))
	{
		MessageBox(pWindow, "Could not load directory.", "File picker", ICON_ERROR | ICON_WARNING << 16);
		return;
	}
	
	while ((pEnt = FsReadDir (pFolderNode, i)) != 0)
	{
		FileNode* pNode = FsFindDir (pFolderNode, pEnt->m_name);
		
		if (!pNode)
		{
			AddElementToList (pWindow, 100002, "<a NULL directory entry>", ICON_ERROR);
		}
		else
		{
			AddElementToList (pWindow, 100002, pNode->m_name, CabGetIconBasedOnName(pNode->m_name, pNode->m_type));
			i++;
		}
	}
}

void FilePickerCdBack (Window *pWindow)
{
	const char* pCwd = TextInputGetRawText(pWindow, 100001);
	
	int size = strlen (pCwd) + 1;
	char cwd_copy[size];
	strcpy(cwd_copy, pCwd);
	
	if (cwd_copy[1] == 0) return;//can't cd back from root
	
	char *p = strrchr (cwd_copy, '/');//get last occurrence
	
	if (pCwd == p) p++;//so that we don't cut the root / as well
	
	*p = 0;//cut it off
	
	SetTextInputText (pWindow, 100001, cwd_copy);
	FilePickerUpdate (pWindow);
	RequestRepaint   (pWindow);
}

//Null but all 0xffffffff's. Useful
#define FNULL ((void*)0xffffffff)
#define POPUP_WIDTH  (400)
#define POPUP_HEIGHT (400-18+TITLE_BAR_HEIGHT)
void CALLBACK FilePickerPopupProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND)
	{
		//Which button did we click?
		if (parm1 == 100002)
		{
			// List View
			const char* pCwd = TextInputGetRawText(pWindow, 100001);
			if (!pCwd)
			{
				MessageBox(pWindow, "The OS really messed up", "Assert", ICON_ERROR << 16 | MB_OK);
				return;
			}
			
			FileNode *pFolderNode = FsResolvePath (pCwd);
			const char* pFileName = GetElementStringFromList (pWindow, parm1, parm2);
			
			FileNode* pFileNode = FsFindDir	(pFolderNode, pFileName);
			if (strcmp (pFileName, PATH_PARENTDIR) == 0)
			{
				FilePickerCdBack(pWindow);
			}
			else if (pFileNode)
			{
				if (pFileNode->m_type & FILE_TYPE_DIRECTORY)
				{
					OnBusy(pWindow);
					
					// Is a directory.  Navigate to it.
					int size = strlen (pCwd) * 2;
					char cwd_copy[size];
					strcpy(cwd_copy, pCwd);
					
					if (cwd_copy[1] != 0)
						strcat (cwd_copy, "/");
					
					strcat (cwd_copy, pFileName);
					
					FileNode *pCurrent = FsResolvePath (cwd_copy);
					if (!pCurrent)
					{
						char buffer [256];
						sprintf (buffer, "Cannot find directory '%s'.  It may have been moved or deleted.", pFileName);
						MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
					}
					else
					{
						SetTextInputText (pWindow, 100001, cwd_copy);
						FilePickerUpdate (pWindow);
						RequestRepaint   (pWindow);
					}
					
					OnNotBusy(pWindow);
				}
				else
				{
					size_t len1 = strlen (pFileName);
					size_t len2 = strlen (pCwd);
					pWindow->m_data = MmAllocateK(len1 + len2 + 3);
					strcpy (pWindow->m_data, pCwd);
					if (strcmp (pCwd, "/"))
						strcat (pWindow->m_data, "/");
					strcat (pWindow->m_data, pFileName);
				}
			}
			else
			{
				char buffer [256];
				sprintf (buffer, "Cannot find file '%s'.  It may have been moved or deleted.\n\nTry clicking the 'Refresh' button in the top bar.", pFileName);
				MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
			}
		}
		if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
		{
			//We clicked a valid button.  Return.
			
			if (parm1 == MBID_CANCEL)
			{
				pWindow->m_data = FNULL;
				return;
			}
			const char* pText = TextInputGetRawText(pWindow, 100000);
			const char* pCwd  = TextInputGetRawText(pWindow, 100001);
			if (!pText || !pCwd)
				pWindow->m_data = FNULL;
			else
			{
				size_t len1 = strlen (pText);
				size_t len2 = strlen (pCwd);
				
				bool isAbsolutePath = pText[0] == '/';
				if (isAbsolutePath)
					len2 = 0;
				
				pWindow->m_data = MmAllocateK(len1 + len2 + 3);
				*((char*)pWindow->m_data) = 0;//so strcat works all the time
				
				if (!isAbsolutePath)
				{
					strcpy (pWindow->m_data, pCwd);
					if (strcmp (pCwd, "/"))
						strcat (pWindow->m_data, "/");
				}
				
				strcat (pWindow->m_data, pText);
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
char* FilePickerBox(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText)
{
	/*
	
	  +---------------------------------------------------------------------+
	  |                                                                     |
	  |  pPrompt text goes here                                             |
	  |                                                                     |
	  |  File path goes here                                                |
	  |                                                                     |
	  | +-----------------------------------------------------------------+ |
	  | | ..                                                              | |
	  | | Your                                                            | |
	  | | Files                                                           | |
	  | | Go                                                              | |
	  | | Right                                                           | |
	  | | Here                                                            | |
	  | |                                                                 | |
	  | |                                                                 | |
	  | |                                                                 | |
	  | |                                                                 | |
	  | |                                                                 | |
	  | |                                                                 | |
	  | |                                                                 | |
	  | +-----------------------------------------------------------------+ |
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
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, POPUP_WIDTH, POPUP_HEIGHT, FilePickerPopupProc, WF_NOCLOSE | WF_NOMINIMZ | WI_MESSGBOX);
	
	// Add the basic controls required.
	Rectangle rect;
	rect.left   = 10;
	rect.top    = 12 + TITLE_BAR_HEIGHT;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = 50;
	AddControl (pBox, CONTROL_TEXT, rect, pPrompt, 0x10000, 0, WINDOW_BACKGD_COLOR);
	
	rect.left   = 10;
	rect.top    = 12 + TITLE_BAR_HEIGHT + 20;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = 20;
	AddControl (pBox, CONTROL_TEXTINPUT, rect, NULL, 100001, 0, 0);
	SetTextInputText(pBox, 100001, "/");
	
	rect.left   = 10;
	rect.top    = 12 + TITLE_BAR_HEIGHT + 60;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = POPUP_HEIGHT - 90;
	
	//400-18+18 -  120 + 12 + 18
	
	AddControl (pBox, CONTROL_ICONVIEW, rect, NULL, 100002, 0, 0);
	
	rect.left   = 10;
	rect.top    = POPUP_HEIGHT - 90 + 20;
	rect.right  = POPUP_WIDTH - 10;
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
	
	FilePickerUpdate(pBox);
	
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
