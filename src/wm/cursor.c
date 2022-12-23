/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Window Manager Cursor Module
******************************************/
#include "wi.h"

Cursor* const g_CursorLUT[] =
{
	&g_defaultCursor,
	&g_waitCursor,
	&g_iBeamCursor,
	&g_crossCursor,
	&g_pencilCursor,
};

Cursor* GetCursorBasedOnID(int m_cursorID, Window *pWindow)
{
	if (m_cursorID == CURSOR_CUSTOM)
	{
		if (pWindow)
			return &pWindow->m_customCursor;
		else
			return &g_defaultCursor;
	}
	
	if (m_cursorID < CURSOR_DEFAULT || m_cursorID >= CURSOR_COUNT) return &g_defaultCursor;
	
	return g_CursorLUT[m_cursorID];
}

void ChangeCursor (Window* pWindow, int cursorID)
{
	pWindow->m_cursorID = cursorID;
}


