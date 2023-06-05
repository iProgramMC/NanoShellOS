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

enum
{
	FP_FILE_TEXT   = 100000,
	FP_CWD_TEXT,
	FP_DIR_LISTING,
	FP_PROMPT_TEXT,
};

extern SafeLock
g_CreateLock, 
g_BackgdLock;
extern VBEData* g_vbeData, g_mainScreenVBEData;
extern void WmPaintWindowTitle(Window* pWindow);
extern void SelectWindow(Window* pWindow);
extern void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2);

IconType CabGetIconBasedOnName(const char *pName, int pType);//btw

void FilePickerChangeDirectory(Window* pWindow, const char* cwd, bool bTakeToRootOnFailure);

void FilePickerUpdate (Window *pWindow)
{
	// Get the directory path
	const char* pCwd = TextInputGetRawText(pWindow, FP_CWD_TEXT);
	if (!pCwd)
	{
		MessageBox(pWindow, "The OS really messed up", "Assert", ICON_ERROR << 16 | MB_OK);
		return;
	}
	
	bool bIsRootDir = (strcmp(pCwd, "/") == 0);
	
	ResetList (pWindow, FP_DIR_LISTING);
	
	if (!bIsRootDir) //if can go to parent, add a button
		AddElementToList (pWindow, FP_DIR_LISTING, "..", ICON_FOLDER_PARENT);
	
	int dd = FiOpenDir(pCwd);
	if (dd < 0)
	{
		const char* errorMsg = "Could not load directory. Taking you back to root.";
		
		if (bIsRootDir)
			errorMsg = "Could not load root directory, uh oh!";
		
		MessageBox(pWindow, errorMsg, "File picker", MB_OK | ICON_WARNING << 16);
		
		if (!bIsRootDir)
		{
			FilePickerChangeDirectory(pWindow, "/", false);
		}
		
		return;
	}
	
	DirEnt ent, *pEnt = &ent;
	memset(&ent, 0, sizeof ent);
	
	int err = 0;
	
	while ((err = FiReadDir(pEnt, dd)) == 0)
	{
		int iconFlags = 0;
		int icon = CabGetIconBasedOnName(pEnt->m_name, pEnt->m_type);
		if (pEnt->m_type == FILE_TYPE_SYMBOLIC_LINK)
		{
			iconFlags = ICON_SHORTCUT_FLAG;
			
			StatResult sr;
			int stat = FiStat(pEnt->m_name, &sr);
			if (stat < 0)
				icon = ICON_CHAIN_BROKEN;
			else
				icon = CabGetIconBasedOnName(pEnt->m_name, sr.m_type);
		}
		
		AddElementToList(pWindow, FP_DIR_LISTING, pEnt->m_name, iconFlags | icon);
	}
	
	if (err < 0)
	{
		SLogMsg("Couldn't fully read directory: %s", GetErrNoString(err));
	}
	
	FiCloseDir(dd);
}

void FilePickerCdBack (Window *pWindow)
{
	FilePickerChangeDirectory(pWindow, "..", true);
}

//Null but all 0xffffffff's. Useful
#define FNULL ((void*)0xffffffff)
#define POPUP_WIDTH  (400)
#define POPUP_HEIGHT (400)

void FilePickerChangeDirectory(Window* pWindow, const char* cwd, bool bTakeToRootOnFailure)
{
	if (strlen(cwd) >= PATH_MAX - 2)
	{
		MessageBox(pWindow, "Cannot navigate to directory.\n\nPath name would be too long.", "Error", ICON_ERROR << 16 | MB_OK);
		return;
	}
	
	if (FiChangeDir(cwd) < 0)
	{
		char buffer [256];
		sprintf (buffer, "Cannot find directory '%s'.  It may have been moved or deleted.\n\n%s", cwd,
			bTakeToRootOnFailure ? "Changing back to the root directory." : "Try another file or directory.");
		MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
		
		if (bTakeToRootOnFailure)
			FilePickerChangeDirectory(pWindow, "/", false);
		
		return;
	}
	
	SetTextInputText (pWindow, FP_CWD_TEXT, FiGetCwd());
	FilePickerUpdate (pWindow);
	RequestRepaint   (pWindow);
}

void CALLBACK FilePickerPopupProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_KEYPRESS:
		{
			if ((char)parm1 == '\r' || (char)parm1 == '\n')
			{
				// if the CWD item is focused:
				if (IsControlFocused(pWindow, FP_CWD_TEXT))
				{
					// update
					FilePickerUpdate(pWindow);
					RequestRepaint  (pWindow);
				}
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			// Which button did we click?
			if (parm1 == FP_DIR_LISTING)
			{
				// List View
				const char* pCwd = TextInputGetRawText(pWindow, FP_CWD_TEXT);
				if (!pCwd)
				{
					MessageBox(pWindow, "The OS really messed up", "Assert", ICON_ERROR << 16 | MB_OK);
					return;
				}
				
				int dd = FiOpenDir(pCwd);
				if (dd < 0)
				{
					FilePickerChangeDirectory(pWindow, "/", false);
					return;
				}
				
				const char* pFileName = GetElementStringFromList (pWindow, parm1, parm2);
				if (!pFileName)
				{
					SLogMsg("pFileName is NULL!");
					FiCloseDir(dd);
					return;
				}
				
				OnBusy(pWindow);
				if (strcmp (pFileName, PATH_PARENTDIR) == 0)
				{
					FilePickerCdBack(pWindow);
				}
				else
				{
					StatResult sr;
					int res = FiStatAt(dd, pFileName, &sr);
					
					if (res >= 0)
					{
						if (sr.m_type & FILE_TYPE_DIRECTORY)
						{
							FilePickerChangeDirectory(pWindow, pFileName, true);
						}
						else if (sr.m_type & FILE_TYPE_SYMBOLIC_LINK)
						{
						//	FilePickerChangeDirectory(pWindow, pFileName, true);
							if (FiChangeDir(pFileName) < 0)
							{
								goto _regular_file;
							}
							else
							{
								FilePickerChangeDirectory(pWindow, ".", true);
							}
						}
						else
						{
						_regular_file:;
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
						OnNotBusy(pWindow);
						char buffer [512];
						snprintf(buffer, sizeof buffer, "Cannot access file '%s'.  It may have been moved or deleted.\n\n%s\n\nTry clicking the 'Refresh' button in the top bar.", GetErrNoString(res), pFileName);
						MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
					}
				}
				
				OnNotBusy(pWindow);
				
				FiCloseDir(dd);
			}
			if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
			{
				//We clicked a valid button.  Return.
				
				if (parm1 == MBID_CANCEL)
				{
					pWindow->m_data = FNULL;
					return;
				}
				const char* pText = TextInputGetRawText(pWindow, FP_FILE_TEXT);
				const char* pCwd  = FiGetCwd();
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
			break;
		}
		case EVENT_CREATE:
		{
			pWindow->m_fullVbeData.m_dirty = 1;
			DefaultWindowProc (pWindow, messageType, parm1, parm2);
			break;
		}
		default:
			DefaultWindowProc (pWindow, messageType, parm1, parm2);
			break;
	}
}

extern char g_cwd[];

// Pops up a text box requesting an input string, and returns a MmAllocate'd
// region of memory with the text inside.  Make sure to free the result,
// if it's non-null.
//
// Returns NULL if the user cancels.
char* FilePickerBoxEx(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText, const char* pFilePath)
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
	
	char cwd[PATH_MAX + 2];
	strcpy(cwd, FiGetCwd());
	
	if (FiChangeDir(pFilePath) < 0)
	{
		FiChangeDir("/");
	}
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		wasSelectedBefore = pWindow->m_isSelected;
		if (wasSelectedBefore)
		{
			pWindow->m_isSelected = false;
			WmPaintWindowTitle (pWindow);
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
	rect.top    = 12;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = 50;
	AddControl (pBox, CONTROL_TEXT, rect, pPrompt, FP_PROMPT_TEXT, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
	
	rect.left   = 10;
	rect.top    = 12 + 20;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = 20;
	AddControl (pBox, CONTROL_TEXTINPUT, rect, NULL, FP_CWD_TEXT, 0, 0);
	SetTextInputText(pBox, FP_CWD_TEXT, FiGetCwd());
	
	rect.left   = 10;
	rect.top    = 12 + 60;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = POPUP_HEIGHT - 90;
	
	//400-18+18 -  120 + 12 + 18
	
	AddControl (pBox, CONTROL_ICONVIEW, rect, NULL, FP_DIR_LISTING, 0, 0);
	
	rect.left   = 10;
	rect.top    = POPUP_HEIGHT - 90 + 20;
	rect.right  = POPUP_WIDTH - 10;
	rect.bottom = 20;
	AddControl (pBox, CONTROL_TEXTINPUT, rect, NULL, FP_FILE_TEXT, 0, 0);
	if (pDefaultText)
		SetTextInputText(pBox, FP_FILE_TEXT, pDefaultText);
	
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
		WmPaintWindowTitle (pWindow);
	}
	
	// just change it back anyway.. If anything fails, this acts as if the directory was deleted while we were still there
	strcpy(g_cwd, cwd);
	
	return dataReturned;
}

char* FilePickerBox(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText)
{
	return FilePickerBoxEx(pWindow, pPrompt, pCaption, pDefaultText, "/");
}
