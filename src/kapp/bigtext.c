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
	NOTHING,
	NOTEP_TEXTVIEW,
	NOTEP_BTNNEW,
	NOTEP_BTNOPEN,
	NOTEP_BTNSAVE,
};

typedef struct NotepadData
{
	bool m_untitled;
	char m_filename[PATH_MAX + 2];
}
NotepadData;

#define NOTEPDATA(Window) ((NotepadData*)(Window->m_data))

void NotepadOpenFile (Window* pWindow, const char*pFileName)
{
	//SetTextInputText(pWindow, int comboID, pText)
	int fd = FiOpen (pFileName, O_RDONLY);
	if (fd < 0)
	{
		//no file
		char buff[1024];
		sprintf(buff, "Can't open '%s'!", pFileName);
		MessageBox(pWindow, buff, "NotepadOpenFile", MB_OK | ICON_ERROR << 16);
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
	}
}

void NotepadOnSave(UNUSED Window* pWindow)
{
	//This function requests a save
	SLogMsg("SAVE REQUEST!");
}

#define NOTEP_WIDTH  800
#define NOTEP_HEIGHT 600

void CALLBACK BigTextWndProc (Window* pWindow, int msg, int parm1, int parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             36
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ NOTEP_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ NOTEP_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			AddControl (pWindow, CONTROL_TEXTINPUT, r, NULL, NOTEP_TEXTVIEW, 1 | 2, 0);
			
			// Add some basic controls
			RECT(r, PADDING_AROUND_LISTVIEW, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING - 30, 50, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "New", NOTEP_BTNNEW, 0, 0);
			RECT(r, PADDING_AROUND_LISTVIEW + 55, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING - 30, 50, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Open", NOTEP_BTNOPEN, 0, 0);
			RECT(r, PADDING_AROUND_LISTVIEW +110, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING - 30, 50, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Save", NOTEP_BTNSAVE, 0, 0);
			
			//NotepadOpenFile (pWindow, "/hello2.txt");
			SetTextInputText (pWindow, NOTEP_TEXTVIEW, "");
			
			pWindow->m_data = MmAllocate (sizeof (NotepadData));
			NOTEPDATA(pWindow)->m_untitled = true;
			NOTEPDATA(pWindow)->m_filename[0] = 0;
			
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
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
					char* data = InputBox(pWindow, "Type in a file path to open.", "Notepad", NULL);
					if (!data) break;
					SLogMsg("Properly got fed data: '%s'", data);
					NotepadOpenFile(pWindow, data);
					MmFree(data);
					RequestRepaint(pWindow);
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
			if (pWindow->m_data)
			{
				MmFree(pWindow->m_data);
				pWindow->m_data = NULL;
			}
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
		}
		case EVENT_PAINT:
			
			break;
		default:
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
	}
}

void BigTextEntry (UNUSED int arg)
{
	Window *pWindow = CreateWindow ("Notepad", 50, 50, NOTEP_WIDTH, NOTEP_HEIGHT, BigTextWndProc, 0);
	
	if (!pWindow) {
		LogMsg("Could not create window.");
		return;
	}
	
	while (HandleMessages (pWindow));
}