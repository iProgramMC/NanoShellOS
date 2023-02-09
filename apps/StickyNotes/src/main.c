/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
         Sticky Notes application

             Main source file
******************************************/

#include <nsstandard.h>
#include <note.h>

Window* g_pMainWindow = NULL;

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 320

NOTE g_notes[64];
int  g_noteCount = 0;

static void OpenNote(int noteNum)
{
	if (noteNum < 0) return;
	
	NoteOpen (&g_notes[noteNum]);
}
static void RefreshList()
{
	// add the loaded items
	ResetList (g_pMainWindow, M_LIST_VIEW);
	
	for (int i = 0; i < g_noteCount; i++)
	{
		AddElementToList (g_pMainWindow, M_LIST_VIEW, g_notes[i].m_title, ICON_NOTE_YELLOW + i % 4);//TODO
	}
	
	//add an "Add Note" item
	AddElementToList (g_pMainWindow, M_LIST_VIEW, "Add New Note...", ICON_PLUS);//TODO: + icon
}
static int AddNote()
{
	if (g_noteCount >= (int)ARRAY_COUNT(g_notes))
	{
		char buffer[512];
		sprintf (buffer, "You cannot create more than %d notes. This will be changed in the future.", ARRAY_COUNT(g_notes));
		MessageBox(g_pMainWindow, buffer, "Sticky Notes", MB_OK | ICON_WARNING << 16);
		
		return -1;
	}
	
	char* pText = InputBox(g_pMainWindow, "What will be the name of the new note?", "Sticky Notes", "Untitled");
	if (!pText)
		return -1;
	
	int noteNum = g_noteCount++;
	NOTE* pNote = &g_notes[noteNum];
	
	pNote->m_pText = NULL;
	
	if (*pText)
		strcpy(pNote->m_title, pText);
	else
		strcpy(pNote->m_title, "Untitled note");
	
	MmKernelFree (pText);
	
	pNote->m_pWindow = NULL;
	
	return noteNum;
}
void CALLBACK MainWindowProc (Window* pWindow, int msg, int parm1, int parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             8
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TOP_PADDING, 
				/*X Size */ WINDOW_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ WINDOW_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TOP_PADDING
			);
			
			AddControlEx (pWindow, CONTROL_ICONVIEW, 
				ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM,
				r, NULL, M_LIST_VIEW, 0, 0);
			
			RefreshList();
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == M_LIST_VIEW)
			{
				if (parm2 == g_noteCount)//add new note
				{
					OpenNote(AddNote());
					RefreshList();
				}
				else OpenNote (parm2);
			}
			break;
		}
		default:
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
	}
}

void LoadData (const char *pPath)
{
	
}
void SaveData (const char *pPath)
{
	
}

void CreateMainWindow()
{
	g_pMainWindow = CreateWindow ("Sticky Notes", CW_AUTOPOSITION, CW_AUTOPOSITION, WINDOW_WIDTH, WINDOW_HEIGHT, MainWindowProc, WF_ALWRESIZ);
	
	if (!g_pMainWindow) {
		LogMsg("Could not create window.");
		return;
	}
	
	SetWindowIcon (g_pMainWindow, ICON_STICKY_NOTES);	
}

void OpenMainMenu()
{
	if (!g_pMainWindow)
		CreateMainWindow();
}

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	CreateMainWindow ();
	if (!g_pMainWindow) return 1;
	
	//NB: A registry-like thing would be extremely convenient
	const char *pPath = "/Fat0/note_data.ntd"; //for testing
	
	LoadData(pPath);
	
	while (true)
	{
		bool bAnyWindowActive = false;
		
		if (g_pMainWindow)
		{
			bAnyWindowActive = true;
			if (!HandleMessages(g_pMainWindow))
				g_pMainWindow = NULL; // close the main window
		}
		
		// handle other windows
		for (int i = 0; i < g_noteCount; i++)
		{
			if (g_notes[i].m_pWindow)
			{
				bAnyWindowActive = true;
				
				if (!HandleMessages(g_notes[i].m_pWindow))
					g_notes[i].m_pWindow = NULL;
			}
		}
		
		if (!bAnyWindowActive) break;
	}
	
	SaveData(pPath);
	
	return 0;
}
