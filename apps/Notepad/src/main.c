/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
            Notepad application

             Main source file
******************************************/
#include <nsstandard.h>

#define COOLBAR_BUTTON_HEIGHT (TITLE_BAR_HEIGHT - 6 + 8)

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
	
	//coolbar actions
	CB$NEW,
	CB$OPEN,
	CB$SAVE,
	CB$FIND,
	CB$CUT,
	CB$COPY,
	CB$PASTE,
	CB$UNDO,
	CB$REDO,
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

NotepadData g_NotepadData;

#define NOTEPDATA(Window) (&g_NotepadData)

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
	int fd = open (pFileName, O_RDONLY);
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
		int fileSize = tellsz (fd);
		
		// allocate a buffer
		char* buffer = malloc (fileSize + 1);
		buffer[fileSize] = 0;
		read (fd, buffer, fileSize);
		
		// close the file
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
	if (NOTEPDATA(pWindow)->m_untitled)
	{
		// Request a name to save to
		char* data = FilePickerBox(pWindow, "Type in a file path to save to.", "Notepad", NULL);
		if (!data) return;//No input
		
		if (strlen (data) > sizeof (NOTEPDATA(pWindow)->m_filename)-5)
		{
			MmKernelFree(data);
			return;//don't bof
		}
		
		// does the file exist?
		int fd = open(data, O_RDONLY);
		if (fd >= 0)
		{
			// yeah, popup a dialog asking user if they want to overwrite the file
			char buffer[1024];
			sprintf(buffer, "The file '%s' already exists.\n\nWould you like to replace it?", data);
			if (MessageBox(pWindow, buffer, "Notepad", ICON_WARNING << 16 | MB_YESNO) == MBID_NO)
			{
				close(fd);
				MmKernelFree(data);
				return;
			}
		}
		
		close(fd);
		strcpy (NOTEPDATA(pWindow)->m_filename, data);
		NOTEPDATA(pWindow)->m_untitled = false;
		MmKernelFree(data);
		NotepadUpdateTitle(pWindow);
	}
	
	// Write to the file
	int fd = open(NOTEPDATA(pWindow)->m_filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		char buffer[1024];
		sprintf(buffer, "Could not save to %s, try saving to another directory.  %s", NOTEPDATA(pWindow)->m_filename, ErrNoStr(fd));
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
	write(fd, (void*)p, strlen (p));
	close(fd);
	NOTEPDATA(pWindow)->m_ackChanges = false;
	TextInputClearDirtyFlag(pWindow, NOTEP_TEXTVIEW);
	NotepadUpdateTitle(pWindow);
}

#define NOTEP_WIDTH  500
#define NOTEP_HEIGHT 400

void NotepadNotImplemented(Window* pWindow)
{
	MessageBox(pWindow, "Not implemented!  Check back later or something", "Notepad", MB_OK | ICON_INFO << 16);
}
void NotepadOnActionNew(Window* pWindow)
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
			return;//Do nothing
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
}
void NotepadOnActionOpen(Window* pWindow)
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
			return;//Do nothing
		}
		else if (result == MBID_YES)
		{
			NotepadOnSave(pWindow);
		}
	}
	//Now that we've saved (or not), open the new document.
	char* data = FilePickerBox(pWindow, "Type in a file path to open.", "Notepad", NULL);
	if (!data) return;
	
	NotepadOpenFile(pWindow, data);
	MmKernelFree(data);
	RequestRepaint(pWindow);
}
void NotepadOnActionSave(Window* pWindow)
{
	// Create a new document:
	if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
	{
		//The document has been changed, save this
		NotepadOnSave(pWindow);
	}
}
void NotepadOnActionSaveAs(Window* pWindow)
{
	//Now that we've saved (or not), open the new document.
	NOTEPDATA(pWindow)->m_untitled = true;
	NotepadOnSave(pWindow);
	RequestRepaint(pWindow);
}
void NotepadOnActionSyntaxHighlight(Window* pWindow)
{
	NOTEPDATA(pWindow)->m_editor_mode ^= TEXTEDIT_SYNTHILT;
	TextInputSetMode (pWindow, NOTEP_TEXTVIEW, NOTEPDATA(pWindow)->m_editor_mode);
	RequestRepaintNew (pWindow);
}
void NotepadOnActionLineNum(Window* pWindow)
{
	NOTEPDATA(pWindow)->m_editor_mode ^= TEXTEDIT_LINENUMS;
	TextInputSetMode (pWindow, NOTEP_TEXTVIEW, NOTEPDATA(pWindow)->m_editor_mode);
	RequestRepaintNew (pWindow);
}
void NotepadOnActionFind(Window* pWindow)
{
	NotepadNotImplemented(pWindow);
}
void NotepadOnActionCut(Window* pWindow)
{
	NotepadNotImplemented(pWindow);
}
void NotepadOnActionCopy(Window* pWindow)
{
	NotepadNotImplemented(pWindow);
}
void NotepadOnActionPaste(Window* pWindow)
{
	NotepadNotImplemented(pWindow);
}
void NotepadOnActionUndo(Window* pWindow)
{
	NotepadNotImplemented(pWindow);
}
void NotepadOnActionRedo(Window* pWindow)
{
	NotepadNotImplemented(pWindow);
}

const char* g_Argument1 = NULL;

void CALLBACK BigTextWndProc (Window* pWindow, int msg, long parm1, long parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			
			#define PADDING_AROUND_TEXTVIEW 8
			#define TOP_PADDING             TITLE_BAR_HEIGHT + COOLBAR_BUTTON_HEIGHT + 5
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_TEXTVIEW, 
				/*Y Coord*/ PADDING_AROUND_TEXTVIEW + TOP_PADDING, 
				/*X Size */ NOTEP_WIDTH - PADDING_AROUND_TEXTVIEW * 2, 
				/*Y Size */ NOTEP_HEIGHT- PADDING_AROUND_TEXTVIEW * 2 - (TOP_PADDING)
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
			
			// Add the cool bar widgets
			int i = 0;
			int button_icons[] = {
				ICON_FILE, ICON_ACTION_OPEN, ICON_ACTION_SAVE, // TODO: specific 'new' icon
				-1,
				ICON_UNDO, ICON_REDO,
				-1,
				ICON_FILE_SEARCH,
				-1,
				ICON_COPY, ICON_PASTE,
			};
			int button_actions[] = {
				CB$NEW, CB$OPEN, CB$SAVE,
				-1,
				CB$UNDO, CB$REDO,
				-1,
				CB$FIND,
				-1,
				CB$COPY, CB$PASTE,
			};
			int x_pos = PADDING_AROUND_TEXTVIEW;
			for (i = 0; i < (int)ARRAY_COUNT(button_icons); i++)
			{
				if (button_icons[i] == 0)
					continue; // none
				if (button_icons[i] == -1)
				{
					RECT(r, x_pos, PADDING_AROUND_TEXTVIEW + TITLE_BAR_HEIGHT, 5, COOLBAR_BUTTON_HEIGHT);
					//add a simple vertical line
					AddControl(pWindow, CONTROL_SIMPLE_VLINE, r, NULL, 0, 0, 0);
					x_pos += 5;
				}
				else
				{
					RECT(r, x_pos, PADDING_AROUND_TEXTVIEW + TITLE_BAR_HEIGHT, COOLBAR_BUTTON_HEIGHT, COOLBAR_BUTTON_HEIGHT);
					AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, button_actions[i], button_icons[i], COOLBAR_BUTTON_HEIGHT > 36 ? 32 : 16);
					
					x_pos += (COOLBAR_BUTTON_HEIGHT + 2);
				}
			}
			
			
			SetTextInputText (pWindow, NOTEP_TEXTVIEW, "");
			
			NOTEPDATA(pWindow)->m_untitled = true;
			NOTEPDATA(pWindow)->m_filename[0] = 0;
			
			if (g_Argument1)
			{
				//If we have a parameter (fed from argv)
				NotepadOpenFile(pWindow, (const char*)g_Argument1);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
				case CB$NEW:
					NotepadOnActionNew(pWindow);
					break;
				case CB$OPEN:
					NotepadOnActionOpen(pWindow);
					break;
				case CB$SAVE:
					NotepadOnActionSaveAs(pWindow);
					break;
				case CB$FIND:
					NotepadOnActionFind(pWindow);
					break;
				case CB$CUT:
					NotepadOnActionCut(pWindow);
					break;
				case CB$COPY:
					NotepadOnActionCopy(pWindow);
					break;
				case CB$PASTE:
					NotepadOnActionPaste(pWindow);
					break;
				case CB$UNDO:
					NotepadOnActionUndo(pWindow);
					break;
				case CB$REDO:
					NotepadOnActionRedo(pWindow);
					break;
				case NOTEP_MENUBAR:
				{
					switch (parm2)
					{
						case NOTEP_BTNSYNHL:
						{
							NotepadOnActionSyntaxHighlight(pWindow);
							break;
						}
						case NOTEP_BTNLNNUM:
						{
							NotepadOnActionLineNum(pWindow);
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
							NotepadOnActionNew(pWindow);
							break;
						}
						case NOTEP_BTNOPEN:
						{
							NotepadOnActionOpen(pWindow);
							break;
						}
						case NOTEP_BTNSAVEAS:
						{
							NotepadOnActionSaveAs(pWindow);
							break;
						}
						case NOTEP_BTNSAVE:
						{
							NotepadOnActionSave(pWindow);
							break;
						}
					}
					break;
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
			if (TextInputQueryDirtyFlag(pWindow, NOTEP_TEXTVIEW))
			{
				char buffer[1024];
				sprintf(
					buffer,
					"You haven't saved your changes to \"%s\".\n\nWould you like to save them before closing?",
					NOTEPDATA(pWindow)->m_untitled ? "Untitled" : NOTEPDATA(pWindow)->m_filename
				);
				int result = MessageBox (pWindow, buffer, "Notepad", MB_YESNO | ICON_TEXT_FILE << 16);
				if (result == MBID_YES)
					NotepadOnSave(pWindow);
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

int main(int argc, char** argv)
{
	Window *pWindow = CreateWindow ("NotepadUser", CW_AUTOPOSITION, CW_AUTOPOSITION, NOTEP_WIDTH, NOTEP_HEIGHT, BigTextWndProc, WF_ALWRESIZ);
	
	if (!pWindow) {
		LogMsg("Could not create window.");
		return 1;
	}
	
	if (argc > 1) {
		g_Argument1 = argv[1];
	}
	
	SetWindowIcon (pWindow, ICON_NOTEPAD);
	
	while (HandleMessages (pWindow));
	
	return 0;
}
