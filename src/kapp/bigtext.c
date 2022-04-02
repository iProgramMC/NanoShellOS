/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        BigText Application module
******************************************/


#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>

enum
{
	NOTEP_MENUBAR_ROOT,//=0, Start by adding to comboid 0
	NOTEP_TEXTVIEW,
	NOTEP_MENUBAR,
	NOTEP_MENUBAR_FILE,
	NOTEP_MENUBAR_EDIT,
	NOTEP_MENUBAR_VIEW,
	NOTEP_MENUBAR_HELP,
	NOTEP_BTNNEW,
	NOTEP_BTNOPEN,
	NOTEP_BTNSAVE,
	NOTEP_BTNSAVEAS,
	NOTEP_BTNEXIT,
	NOTEP_BTNABOUT,
	NOTEP_BTNSYNHL,
	NOTEP_BTNLNNUM,
	NOTEP_BTNFONTP,
};

typedef struct NotepadData
{
	bool m_untitled;
	bool m_ackChanges;
	char m_filename[PATH_MAX + 2];
	int  m_editor_mode;
	int  m_current_font;
}
NotepadData;

#define NOTEPDATA(Window) ((NotepadData*)(Window->m_data))

void NotepadUpdateTitle(Window* pWindow)
{
	char buffer[WINDOW_TITLE_MAX];
	sprintf(
		buffer,
		"%s%s - Notepad",
		TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW) ? "*" : "",
		NOTEPDATA(pWindow)->m_untitled ? "untitled" : NOTEPDATA(pWindow)->m_filename
	);
	strcpy (pWindow->m_title, buffer);
	
	RequestRepaintNew(pWindow);
}

void NotepadOpenFile (Window* pWindow, const char*pFileName)
{
	int fd = FiOpen (pFileName, O_RDONLY);
	if (fd < 0)
	{
		//no file
		char buff[1024];
		sprintf(buff, "Cannot find the file '%s'.\n\nWould you like to create a new file?", pFileName);
		if (MessageBox(pWindow, buff, "Notepad", MB_YESNO | ICON_WARNING << 16) == MBID_YES)
		{
			SetTextInputText(pWindow, NOTEP_TEXTVIEW, "");
			NOTEPDATA(pWindow)->m_untitled = true;
			NOTEPDATA(pWindow)->m_filename[0] = 0;
			TextInputClearDirtyFlag(pWindow, NOTEP_TEXTVIEW);
			NotepadUpdateTitle(pWindow);
		}
		return;
	}
	else
	{
		// get the size
		int fileSize = FiTellSize (fd);
		
		// allocate a buffer
		char* buffer = MmAllocate (fileSize + 1);
		buffer[fileSize] = 0;
		FiRead (fd, buffer, fileSize);
		
		// close the file
		FiClose (fd);
		
		// load the file into the box
		SetTextInputText(pWindow, NOTEP_TEXTVIEW, buffer);
		
		TextInputClearDirtyFlag(pWindow, NOTEP_TEXTVIEW);
		
		// internally this function copies the text anyway, so free it here:
		MmFree (buffer);
		
		
		strcpy(NOTEPDATA(pWindow)->m_filename, pFileName);
		NOTEPDATA(pWindow)->m_untitled   = false;
		NOTEPDATA(pWindow)->m_ackChanges = false;
			
		NotepadUpdateTitle(pWindow);
	}
}

void NotepadOnSave(UNUSED Window* pWindow)
{
	//This function requests a save
	SLogMsg("SAVE REQUEST!");
	if (NOTEPDATA(pWindow)->m_untitled)
	{
		// Request a name to save to
		char* data = FilePickerBox(pWindow, "Type in a file path to save to.", "Notepad", NULL);
		if (!data) return;//No input
		SLogMsg("Properly got fed data: '%s'", data);
		if (strlen (data) > sizeof (NOTEPDATA(pWindow)->m_filename)-5)
		{
			return;//don't bof
		}
		
		strcpy (NOTEPDATA(pWindow)->m_filename, data);
		NOTEPDATA(pWindow)->m_untitled = false;
			
		NotepadUpdateTitle(pWindow);
	}
	
	// Write to the file
	int fd = FiOpen(NOTEPDATA(pWindow)->m_filename, O_WRONLY | O_CREAT);
	if (fd < 0)
	{
		char buffer[1024];
		sprintf(buffer, "Could not save to %s, try saving to another directory.", NOTEPDATA(pWindow)->m_filename);
		MessageBox(pWindow, buffer, "Notepad", ICON_WARNING << 16 | MB_OK);
		NOTEPDATA(pWindow)->m_untitled = true;
		return;
	}
	
	const char* p = TextInputGetRawText(pWindow, NOTEP_TEXTVIEW);
	if (!p)
	{
		MessageBox(pWindow, "Could not save file.  The text input control is gone!?", "Notepad", ICON_ERROR << 16 | MB_OK);
		return;
	}
	FiWrite(fd, (void*)p, strlen (p));
	FiClose(fd);
	
	NOTEPDATA(pWindow)->m_ackChanges = false;
	TextInputClearDirtyFlag(pWindow, NOTEP_TEXTVIEW);
	NotepadUpdateTitle(pWindow);
}

#define NOTEP_WIDTH  500
#define NOTEP_HEIGHT 400

void CALLBACK BigTextWndProc (Window* pWindow, int msg, int parm1, int parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			void *pOldData = pWindow->m_data;
			Rectangle r;
			// Add a list view control.
			pWindow->m_data = MmAllocate (sizeof (NotepadData));
			
			#define PADDING_AROUND_TEXTVIEW 4
			#define TOP_PADDING             TITLE_BAR_HEIGHT
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_TEXTVIEW, 
				/*Y Coord*/ PADDING_AROUND_TEXTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ NOTEP_WIDTH - PADDING_AROUND_TEXTVIEW * 2, 
				/*Y Size */ NOTEP_HEIGHT- PADDING_AROUND_TEXTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			NOTEPDATA(pWindow)->m_editor_mode  = TEXTEDIT_MULTILINE;
			NOTEPDATA(pWindow)->m_current_font = FONT_TAMSYN_MED_REGULAR;
			
			AddControlEx (
				pWindow,
				CONTROL_TEXTINPUT,
				ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM,
				r,
				NULL,
				NOTEP_TEXTVIEW,
				NOTEPDATA(pWindow)->m_editor_mode,
				NOTEPDATA(pWindow)->m_current_font
			);
			
			RECT(r, 0, 0, 0, 0);
			AddControl(pWindow, CONTROL_MENUBAR, r, NULL, NOTEP_MENUBAR, 0, 0);
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_ROOT, NOTEP_MENUBAR_FILE, "File");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_ROOT, NOTEP_MENUBAR_VIEW, "View");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_ROOT, NOTEP_MENUBAR_HELP, "Help");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNNEW,       "New file");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNOPEN,      "Open...");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNSAVE,      "Save");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNSAVEAS,    "Save as...");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNEXIT,      "Exit");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_HELP, NOTEP_BTNABOUT,     "About Notepad");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_VIEW, NOTEP_BTNSYNHL,     "Syntax Highlighting");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_VIEW, NOTEP_BTNLNNUM,     "Line numbers");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_VIEW, NOTEP_BTNFONTP,     "Font...");
			
			SetTextInputText (pWindow, NOTEP_TEXTVIEW, "");
			
			NOTEPDATA(pWindow)->m_untitled = true;
			NOTEPDATA(pWindow)->m_filename[0] = 0;
			
			if (pOldData)
			{
				//If we have a parameter
				NotepadOpenFile(pWindow, (const char*)pOldData);
				MmFree((void*)pOldData);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == NOTEP_MENUBAR)
			{
				switch (parm2)
				{
					case NOTEP_BTNSYNHL:
					{
						NOTEPDATA(pWindow)->m_editor_mode ^= TEXTEDIT_SYNTHILT;
						TextInputSetMode (pWindow, NOTEP_TEXTVIEW, NOTEPDATA(pWindow)->m_editor_mode);
						RequestRepaintNew (pWindow);
						break;
					}
					case NOTEP_BTNLNNUM:
					{
						NOTEPDATA(pWindow)->m_editor_mode ^= TEXTEDIT_LINENUMS;
						TextInputSetMode (pWindow, NOTEP_TEXTVIEW, NOTEPDATA(pWindow)->m_editor_mode);
						RequestRepaintNew (pWindow);
						break;
					}
					case NOTEP_BTNABOUT:
					{
						ShellAbout("Notepad", ICON_NOTEPAD);
						break;
					}
					case NOTEP_BTNEXIT:
					{
						BigTextWndProc (pWindow, EVENT_CLOSE, 0, 1);
						break;
					}
					case NOTEP_BTNNEW:
					{
						// Create a new document:
						if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
						{
							//The document has been changed, ask user if they'd like to save first
							char buffer[1024];
							sprintf(
								buffer,
								"You haven't saved your changes to \"%s\".\n\nWould you like to save them before creating a new document?",
								NOTEPDATA(pWindow)->m_untitled ? "Untitled" : NOTEPDATA(pWindow)->m_filename
							);
							int result = MessageBox (pWindow, buffer, "Notepad", MB_YESNOCANCEL | ICON_TEXT_FILE << 16);
							if (result == MBID_CANCEL)
							{
								break;//Do nothing
							}
							else if (result == MBID_YES)
							{
								NotepadOnSave(pWindow);
							}
						}
						//Now that we've saved (or not), create the new document.
						TextInputClearDirtyFlag(pWindow, NOTEP_TEXTVIEW);
						SetTextInputText (pWindow, NOTEP_TEXTVIEW, "");
						NOTEPDATA(pWindow)->m_untitled = true;
						NOTEPDATA(pWindow)->m_filename[0] = 0;
						RequestRepaint(pWindow);
						break;
					}
					case NOTEP_BTNOPEN:
					{
						// Create a new document:
						if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
						{
							//The document has been changed, ask user if they'd like to save first
							char buffer[1024];
							sprintf(
								buffer,
								"You haven't saved your changes to \"%s\".\n\nWould you like to save them before opening another document?",
								NOTEPDATA(pWindow)->m_untitled ? "Untitled" : NOTEPDATA(pWindow)->m_filename
							);
							int result = MessageBox (pWindow, buffer, "Notepad", MB_YESNOCANCEL | ICON_TEXT_FILE << 16);
							if (result == MBID_CANCEL)
							{
								break;//Do nothing
							}
							else if (result == MBID_YES)
							{
								NotepadOnSave(pWindow);
							}
						}
						//Now that we've saved (or not), open the new document.
						char* data = FilePickerBox(pWindow, "Type in a file path to open.", "Notepad", NULL);
						if (!data) break;
						SLogMsg("Properly got fed data: '%s'", data);
						NotepadOpenFile(pWindow, data);
						MmFree(data);
						RequestRepaint(pWindow);
						break;
					}
					case NOTEP_BTNSAVEAS:
					{
						//Now that we've saved (or not), open the new document.
						NOTEPDATA(pWindow)->m_untitled = true;
						NotepadOnSave(pWindow);
						RequestRepaint(pWindow);
						break;
					}
					case NOTEP_BTNSAVE:
					{
						// Create a new document:
						if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
						{
							//The document has been changed, save this
							NotepadOnSave(pWindow);
						}
						break;
					}
				}
			}
			break;
		}
		case EVENT_CLOSE:
		{
			if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
			{
				char buffer[1024];
				sprintf(
					buffer,
					"You haven't saved your changes to \"%s\".\n\nWould you like to save them before closing?",
					NOTEPDATA(pWindow)->m_untitled ? "Untitled" : NOTEPDATA(pWindow)->m_filename
				);
				int result = MessageBox (pWindow, buffer, "Notepad", MB_YESNOCANCEL | ICON_TEXT_FILE << 16);
				if (result == MBID_CANCEL) break;//Don't close.
				if (result == MBID_YES)
					NotepadOnSave(pWindow);
			}
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
		}
		case EVENT_DESTROY:
		{
			if (pWindow->m_data)
			{
				MmFree(pWindow->m_data);
				pWindow->m_data = NULL;
			}
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
		}
		case EVENT_KEYRAW:
		{
			if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
			{
				if (!NOTEPDATA(pWindow)->m_ackChanges)
				{
					NOTEPDATA(pWindow)->m_ackChanges = true;
					NotepadUpdateTitle(pWindow);
				}
			}
			break;
		}
		case EVENT_PAINT:
			
			break;
		default:
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
	}
}

void BigTextEntry (int arg)
{
	Window *pWindow = CreateWindow ("Notepad", CW_AUTOPOSITION, CW_AUTOPOSITION, NOTEP_WIDTH, NOTEP_HEIGHT, BigTextWndProc, WF_ALWRESIZ);
	
	if (!pWindow) {
		LogMsg("Could not create window.");
		return;
	}
	
	pWindow->m_iconID = ICON_NOTEPAD;
	pWindow->m_data   = (void*)arg;
	
	while (HandleMessages (pWindow));
}
