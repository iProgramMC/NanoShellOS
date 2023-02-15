/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Window Manager Cursor Module
******************************************/
#include <process.h>
#include "wi.h"

#define C_MAX_CURSOR_SLOTS (1024)
#define C_BUILTIN_CURSOR_COUNT (CURSOR_CUSTOM + 1)

// Built-in cursors.
Cursor* const g_CursorLUT[] =
{
	&g_defaultCursor,
	&g_waitCursor,
	&g_iBeamCursor,
	&g_crossCursor,
	&g_pencilCursor,
	&g_resizeNSCursor,
	&g_resizeWECursor,
	&g_resizeNWSECursor,
	&g_resizeNESWCursor,
	&g_resizeAllCursor,
};

typedef struct
{
	bool     m_bUsed;
	Cursor   m_cursor;
	Process* m_ownedBy;
}
CursorSlot;

CursorSlot g_CursorSlots[C_MAX_CURSOR_SLOTS];

static void ImageToCursor(Cursor* pCur, Image* pImg, int xOff, int yOff, uint32_t* pBuf)
{
	// Duplicate the frame buffer.
	int nPixels = pImg->width * pImg->height;
	bool bTransparent = false;
	
	for (int i = 0; i < nPixels; i++)
	{
		pBuf[i] = pImg->framebuffer[i];
		if (pBuf[i] == TRANSPARENT)
			bTransparent = true;
	}
	
	memset(pCur, 0, sizeof *pCur);
	pCur->bitmap   = pBuf;
	pCur->width    = pImg->width;
	pCur->height   = pImg->height;
	pCur->leftOffs = xOff;
	pCur->topOffs  = yOff;
	pCur->m_transparency = bTransparent;
	pCur->mouseLockX = pCur->mouseLockY = -1;
}

int UploadCursor(Image * pImage, int xOff, int yOff)
{
	KeVerifyInterruptsEnabled;
	
	int nPixels = pImage->width * pImage->height;
	uint32_t *pBuf = MmAllocate(nPixels * sizeof(uint32_t));
	if (!pBuf)
		return -1;
	
	cli;
	
	// locate a free slot.
	int freeSlot = -1;
	for (int i = 0; i < C_MAX_CURSOR_SLOTS; i++)
	{
		if (!g_CursorSlots[i].m_bUsed)
		{
			freeSlot = i;
			break;
		}
	}
	
	if (freeSlot < 0)
	{
		sti;
		return -1;
	}
	
	g_CursorSlots[freeSlot].m_bUsed   = true;
	g_CursorSlots[freeSlot].m_ownedBy = ExGetRunningProc();
	ImageToCursor(&g_CursorSlots[freeSlot].m_cursor, pImage, xOff, yOff, pBuf);
	
	sti;
	
	return freeSlot + C_BUILTIN_CURSOR_COUNT;
}

void ReleaseCursor(int cursorID)
{
	KeVerifyInterruptsEnabled;
	
	if (cursorID < C_BUILTIN_CURSOR_COUNT) return;
	cursorID -= C_BUILTIN_CURSOR_COUNT;
	
	uint32_t* pBuffer;
	
	cli;
	pBuffer = (uint32_t*)g_CursorSlots[cursorID].m_cursor.bitmap;
	g_CursorSlots[cursorID].m_cursor.bitmap = NULL;
	g_CursorSlots[cursorID].m_ownedBy = NULL;
	g_CursorSlots[cursorID].m_bUsed = false;
	sti;
	
	if (pBuffer)
		MmFree(pBuffer);
}

void ReleaseCursorsBy(Process* pProc)
{
	uint32_t* s_cursor_buffers[C_MAX_CURSOR_SLOTS];
	int sp = 0;
	cli;
	for (int i = 0; i < C_MAX_CURSOR_SLOTS; i++)
	{
		if (g_CursorSlots[i].m_ownedBy == pProc)
		{
			s_cursor_buffers[sp++] = (uint32_t*)g_CursorSlots[i].m_cursor.bitmap;
			g_CursorSlots[i].m_ownedBy = NULL;
			g_CursorSlots[i].m_bUsed   = false;
		}
	}
	sti;
	
	for (int i = 0; i < sp; i++)
	{
		if (s_cursor_buffers[i])
			MmFree(s_cursor_buffers[i]);
	}
}

Cursor* GetCursorBasedOnID(int m_cursorID, Window *pWindow)
{
	if (m_cursorID == CURSOR_CUSTOM)
	{
		if (pWindow)
			return &pWindow->m_customCursor;
		else
			return &g_defaultCursor;
	}
	
	if (m_cursorID < CURSOR_DEFAULT) return &g_defaultCursor;
	if (m_cursorID >= C_BUILTIN_CURSOR_COUNT)
	{
		m_cursorID -= C_BUILTIN_CURSOR_COUNT;
		if (m_cursorID >= C_MAX_CURSOR_SLOTS) return &g_defaultCursor;
		
		CursorSlot* pSlot = &g_CursorSlots[m_cursorID];
		if (!pSlot->m_bUsed) return &g_defaultCursor;
		
		return &pSlot->m_cursor;
	}
	
	return g_CursorLUT[m_cursorID];
}

void ChangeCursor (Window* pWindow, int cursorID)
{
	pWindow->m_cursorID = cursorID;
}


