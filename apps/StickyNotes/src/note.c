// NanoShell Operating System
// Sticky Notes application
// (C) 2022 iProgramInCpp

#include <note.h>

#define NOTE_WIDTH 240
#define NOTE_HEIGHT 180 + TITLE_BAR_HEIGHT

void OpenMainMenu();

void NoteClose(NOTE* pNote, bool alsoRemWnd)
{
	Window *pWindow = pNote->m_pWindow;
	if (pWindow)
	{
		const char *text = TextInputGetRawText (pWindow, N_TEXT_VIEW);
		
		int len = strlen(text);
		pNote->m_pText = malloc(len + 1);//for safe storage
		memcpy (pNote->m_pText, text, len + 1);
		
		//this won't allow CloseNote to recurse onto itself
		pNote->m_pWindow = NULL;
		
		if (alsoRemWnd)
		{
			DestroyWindow (pWindow);
			while (HandleMessages(pWindow));
		}
	}
}

#define EVENT_MAIN_MENU_THING 0x1000//there will never be 4096 events.
void CALLBACK NoteWindowProc (Window* pWindow, int msg, int parm1, int parm2)
{
	NOTE* pNote = (NOTE*)pWindow->m_data;
	if (!pNote) return;
	
	switch (msg)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_TEXTVIEW 2
			#define TOP_PADDING             2
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_TEXTVIEW, 
				/*Y Coord*/ PADDING_AROUND_TEXTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ NOTE_WIDTH - PADDING_AROUND_TEXTVIEW * 2, 
				/*Y Size */ NOTE_HEIGHT- PADDING_AROUND_TEXTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			AddControlEx (pWindow, CONTROL_TEXTINPUT, 
				ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM,
				r, NULL, N_TEXT_VIEW, TEXTEDIT_MULTILINE, 0);
			
			if (pNote->m_pText)
			{
				SetTextInputText(pWindow, N_TEXT_VIEW, pNote->m_pText);
				free(pNote->m_pText);
				pNote->m_pText = NULL;
			}
			
			// Add a button right next to the close button
			r.right  = pWindow->m_vbeData.m_width - 3 - 1;
			r.left   = r.right - 18 + 2;
			r.top    = 2;
			r.bottom = r.top + TITLE_BAR_HEIGHT - 4;
			
			r.left  -= 18;
			r.right -= 18;
			
			AddControlEx (pWindow, CONTROL_BUTTON_EVENT, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT, r, "\x13", N_MAINMENU_BTN, EVENT_MAIN_MENU_THING, 0);
			
			break;
		}
		
		case EVENT_MAIN_MENU_THING:
		{
			//open main menu
			OpenMainMenu ();
			
			break;
		}
		
		case EVENT_DESTROY :
		{
			//register the note as some text
			NoteClose(pNote, false);
			
			break;
		}
		
		default : DefaultWindowProc( pWindow, msg, parm1, parm2 );
			break;
	}
}

void NoteOpen (NOTE* pNote)
{
	//create a window for it:
	if (pNote->m_pWindow) 
		//already opened
		//TODO: select that window
		return;
	
	pNote->m_pWindow = CreateWindow (pNote->m_title, CW_AUTOPOSITION, CW_AUTOPOSITION, NOTE_WIDTH, NOTE_HEIGHT, NoteWindowProc, WF_FLATBORD | WF_ALWRESIZ | WF_NOMINIMZ | WF_NOMAXIMZ);
	
	if (!pNote->m_pWindow)
		return;
	
	pNote->m_pWindow->m_data = pNote;
}


