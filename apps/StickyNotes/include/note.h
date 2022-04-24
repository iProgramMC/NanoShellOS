// NanoShell Operating System
// Sticky Notes application
// (C) 2022 iProgramInCpp

#ifndef _NOTE_H
#define _NOTE_H

#include <nsstandard.h>

typedef struct
{
	char  m_title[256];
	char* m_pText;
	
	Window* m_pWindow;
}
NOTE;

enum
{
	//M_ = main, N_ = note
	M_NULL,
	M_LIST_VIEW,
	M_MENU_BAR,
	
	N_MENU_BAR,//note menu bar
	N_TEXT_VIEW,
};

void NoteOpen (NOTE* pNote);
void NoteClose(NOTE* pNote, bool alsoRemWnd);

#endif//_NOTE_H

