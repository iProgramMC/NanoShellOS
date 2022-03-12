#include <nsstandard.h>

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
};

typedef struct NotepadData
{
	bool m_untitled;
	bool m_ackChanges;
	char m_filename[PATH_MAX + 2];
}
NotepadData;

#define NOTEPDATA(Window) ((NotepadData*)(Window->m_data))

void NotepadUpdateTitle(Window* pWindow)
{
	char buffer[WINDOW_TITLE_MAX];
	sprintf(
		buffer,
		"%s%s - NotepadUser",
		TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW) ? "*" : "",
		NOTEPDATA(pWindow)->m_untitled ? "untitled" : NOTEPDATA(pWindow)->m_filename
	);
	//strcpy (pWindow->m_title, buffer);
	
	SetWindowTitle(pWindow, buffer);
	
	//RequestRepaintNew(pWindow);
}

void NotepadOpenFile (Window* pWindow, const char*pFileName)
{
	//int fd = FiOpen (pFileName, O_RDONLY);
	//FILE* pFile = fopen (pFileName, "r");
	int fd = open (pFileName, O_RDONLY);
	
	if (fd < 0)
	{
		//no file
		char buff[1024];
		sprintf(buff, "Cannot find the file '%s'.\n\nWould you like to create a new file?", pFileName);
		if (MessageBox(pWindow, buff, "NotepadUser", MB_YESNO | ICON_WARNING << 16) == MBID_YES)
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
		int fileSize = tellsz (fd);//FiTellSize (fd);
		
		// allocate a buffer
		char* buffer = malloc(fileSize + 1);
		buffer[fileSize] = 0;
		
		//FiRead (fd, buffer, fileSize);
		//fread (buffer, 1, fileSize, pFile);
		read (fd, buffer, fileSize);
		
		// close the file
		//fclose (pFile);
		close (fd);
		
		// load the file into the box
		SetTextInputText(pWindow, NOTEP_TEXTVIEW, buffer);
		
		TextInputClearDirtyFlag(pWindow, NOTEP_TEXTVIEW);
		
		// internally this function copies the text anyway, so free it here:
		free (buffer);
		
		
		strcpy(NOTEPDATA(pWindow)->m_filename, pFileName);
		NOTEPDATA(pWindow)->m_untitled   = false;
		NOTEPDATA(pWindow)->m_ackChanges = false;
			
		NotepadUpdateTitle(pWindow);
	}
}

void NotepadOnSave(UNUSED Window* pWindow)
{
	//This function requests a save
	//SLogMsg("SAVE REQUEST!");
	if (NOTEPDATA(pWindow)->m_untitled)
	{
		// Request a name to save to
		char* data = InputBox(pWindow, "Type in a file path to save to.", "NotepadUser", NULL);
		if (!data) return;//No input
		
		if (strlen (data) > sizeof (NOTEPDATA(pWindow)->m_filename)-5)
		{
			return;//don't bof
		}
		
		strcpy (NOTEPDATA(pWindow)->m_filename, data);
		NOTEPDATA(pWindow)->m_untitled = false;
			
		NotepadUpdateTitle(pWindow);
	}
	
	// Write to the file
	//int fd = FiOpen(NOTEPDATA(pWindow)->m_filename, O_WRONLY | O_CREAT);
	//FILE* pf = fopen (NOTEPDATA(pWindow)->m_filename, "w");
	int fd = open (NOTEPDATA(pWindow)->m_filename, O_WRONLY | O_CREAT);
	if (fd < 0)
	{
		char buffer[1024];
		sprintf(buffer, "Could not save to %s, try saving to another directory.", NOTEPDATA(pWindow)->m_filename);
		MessageBox(pWindow, buffer, "NotepadUser", ICON_WARNING << 16 | MB_OK);
		NOTEPDATA(pWindow)->m_untitled = true;
		return;
	}
	
	const char* p = TextInputGetRawText(pWindow, NOTEP_TEXTVIEW);
	if (!p)
	{
		MessageBox(pWindow, "Could not save file.  The text input control is gone!?", "NotepadUser", ICON_ERROR << 16 | MB_OK);
		return;
	}
	//FiWrite(fd, (void*)p, strlen (p));
	//FiClose(fd);
	//fwrite(p, 1, strlen(p), pf);
	write (fd, p, strlen (p));
	//fclose(pf);
	close (fd);
	
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
			
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             36
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ NOTEP_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ NOTEP_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			AddControlEx (pWindow, CONTROL_TEXTINPUT, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, NOTEP_TEXTVIEW, 1 | 2, 0);
			
			// Add some basic controls
			/*RECT(r, PADDING_AROUND_LISTVIEW, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING - 30, 50, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "New", NOTEP_BTNNEW, 0, 0);
			RECT(r, PADDING_AROUND_LISTVIEW + 55, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING - 30, 50, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Open", NOTEP_BTNOPEN, 0, 0);
			RECT(r, PADDING_AROUND_LISTVIEW +110, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING - 30, 50, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Save", NOTEP_BTNSAVE, 0, 0);*/
			
			RECT(r, 0, 0, 0, 0);
			AddControl(pWindow, CONTROL_MENUBAR, r, NULL, NOTEP_MENUBAR, 0, 0);
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_ROOT, NOTEP_MENUBAR_FILE, "File");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_ROOT, NOTEP_MENUBAR_HELP, "Help");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNNEW,       "New file");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNOPEN,      "Open...");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNSAVE,      "Save");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNSAVEAS,    "Save as...");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_FILE, NOTEP_BTNEXIT,      "Exit");
			AddMenuBarItem(pWindow, NOTEP_MENUBAR, NOTEP_MENUBAR_HELP, NOTEP_BTNABOUT,     "About Notepad");
			
			//NotepadOpenFile (pWindow, "/hello2.txt");
			
			SetTextInputText (pWindow, NOTEP_TEXTVIEW, "");
			
			pWindow->m_data = malloc (sizeof (NotepadData));
			NOTEPDATA(pWindow)->m_untitled = true;
			NOTEPDATA(pWindow)->m_filename[0] = 0;
			
			if (pOldData)
			{
				//If we have a parameter
				NotepadOpenFile(pWindow, (const char*)pOldData);
				free((void*)pOldData);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == NOTEP_MENUBAR)
			{
				switch (parm2)
				{
					case NOTEP_BTNABOUT:
					{
						ShellAbout("NotepadUser", ICON_NOTEPAD);
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
							int result = MessageBox (pWindow, buffer, "NotepadUser", MB_YESNOCANCEL | ICON_TEXT_FILE << 16);
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
							int result = MessageBox (pWindow, buffer, "NotepadUser", MB_YESNOCANCEL | ICON_TEXT_FILE << 16);
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
						char* data = InputBox(pWindow, "Type in a file path to open.", "NotepadUser", NULL);
						if (!data) break;
						
						NotepadOpenFile(pWindow, data);
						free(data);
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
				int result = MessageBox (pWindow, buffer, "NotepadUser", MB_YESNOCANCEL | ICON_TEXT_FILE << 16);
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
				free(pWindow->m_data);
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

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	Window *pWindow = CreateWindow ("NotepadUser", CW_AUTOPOSITION, CW_AUTOPOSITION, NOTEP_WIDTH, NOTEP_HEIGHT, BigTextWndProc, WF_ALWRESIZ);
	
	if (!pWindow) {
		LogMsg("Could not create window.");
		return 1;
	}
	
	pWindow->m_iconID = ICON_NOTEPAD;
	pWindow->m_data   = NULL;//TODO
	
	while (HandleMessages (pWindow));
	
	return 0;
}
