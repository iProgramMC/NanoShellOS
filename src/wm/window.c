/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Window Object Management Module
******************************************/
#include "wi.h"

#define DIRTY_RECT_TRACK

bool g_RenderWindowContents = true;//while moving
Cursor g_windowDragCursor;

int g_NewWindowX = 10, g_NewWindowY = 10;
Window g_windows [WINDOWS_MAX];

SafeLock
g_WindowLock, 
g_ScreenLock, 
g_BufferLock, 
g_CreateLock, 
g_BackgdLock; 

bool IsLowResolutionMode()
{
	return GetScreenWidth() < 800 || GetScreenHeight() < 600;
}

Window* GetWindowFromIndex(int i)
{
	if (i >= 0x1000) i -= 0x1000;
	return &g_windows[i];
}

void UndrawWindow (Window* pWnd)
{
	RefreshRectangle(pWnd->m_rect, pWnd);
}

void HideWindowUnsafe (Window* pWindow)
{
	pWindow->m_hidden = true;
	UndrawWindow(pWindow);
}

static void ShowWindowUnsafe (Window* pWindow)
{
	pWindow->m_hidden = false;
	UpdateDepthBuffer();
	//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
	//pWindow->m_vbeData.m_dirty = true;
	//pWindow->m_renderFinished = true;
	
	// Render it to the vbeData:
	if (pWindow->m_minimized)
	{
		//TODO?
	}
	else
		VidBitBlit (
			g_vbeData,
			pWindow->m_rect.left,
			pWindow->m_rect.top,
			pWindow->m_vbeData.m_width,
			pWindow->m_vbeData.m_height,
			&pWindow->m_vbeData,
			0, 0,
			BOP_SRCCOPY
		);
}

void HideWindow (Window* pWindow)
{
	if (IsWindowManagerTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		HideWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_HIDE;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void ShowWindow (Window* pWindow)
{
	if (IsWindowManagerTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		ShowWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_SHOW;
	
	//WindowAction* ptr = 
	ActionQueueAdd(action);
	
	//while (ptr->bInProgress)
	//	KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void ResizeWindowInternal (Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	if (newPosX != -1)
	{
		if (newPosX < 0) newPosX = 0;
		if (newPosY < 0) newPosY = 0;
		if (newPosX >= GetScreenWidth ()) newPosX = GetScreenWidth ();
		if (newPosY >= GetScreenHeight()) newPosY = GetScreenHeight();
	}
	else
	{
		newPosX = pWindow->m_rect.left;
		newPosY = pWindow->m_rect.top;
	}
	if (newWidth < WINDOW_MIN_WIDTH)
		newWidth = WINDOW_MIN_WIDTH;
	if (newHeight< WINDOW_MIN_HEIGHT)
		newHeight= WINDOW_MIN_HEIGHT;
	
	//acquire the screen lock
	SLogMsg("Task who owns lock %p for now: %p", &pWindow->m_screenLock, pWindow->m_screenLock.m_task_owning_it);
	LockAcquire(&pWindow->m_screenLock);
	
	uint32_t* pNewFb = (uint32_t*)MmAllocatePhy(newWidth * newHeight * sizeof(uint32_t), ALLOCATE_BUT_DONT_WRITE_PHYS);
	if (!pNewFb)
	{
		SLogMsg("Cannot resize window to %dx%d, out of memory", newWidth, newHeight);
		LockFree(&pWindow->m_screenLock);
		return;
	}
	
	// Copy the entire framebuffer's contents from old to new.
	int oldWidth = pWindow->m_vbeData.m_width, oldHeight = pWindow->m_vbeData.m_height;
	int minWidth = newWidth, minHeight = newHeight;
	if (minWidth > oldWidth)
		minWidth = oldWidth;
	if (minHeight > oldHeight)
		minHeight = oldHeight;
	
	for (int i = 0; i < minHeight; i++)
	{
		memcpy_ints (&pNewFb[i * newWidth], &pWindow->m_vbeData.m_framebuffer32[i * oldWidth], minWidth);
	}
	
	// Free the old framebuffer.  This action should be done atomically.
	// TODO: If I ever decide to add locks to mmfree etc, then fix this so that it can't cause deadlocks!!
	
	MmFree(pWindow->m_vbeData.m_framebuffer32);
	pWindow->m_vbeData.m_framebuffer32 = pNewFb;
	pWindow->m_vbeData.m_width   = newWidth;
	pWindow->m_vbeData.m_pitch32 = newWidth;
	pWindow->m_vbeData.m_pitch16 = newWidth*2;
	pWindow->m_vbeData.m_pitch   = newWidth*4;
	pWindow->m_vbeData.m_height  = newHeight;
	
	pWindow->m_rect.left   = newPosX;
	pWindow->m_rect.top    = newPosY;
	pWindow->m_rect.right  = pWindow->m_rect.left + newWidth;
	pWindow->m_rect.bottom = pWindow->m_rect.top  + newHeight;
	
	// Mark as dirty.
	pWindow->m_vbeData.m_dirty = true;
	
	// Send window events: EVENT_SIZE, EVENT_PAINT.
	WindowAddEventToMasterQueue(pWindow, EVENT_SIZE,  MAKE_MOUSE_PARM(newWidth, newHeight), MAKE_MOUSE_PARM(oldWidth, oldHeight));
	//WindowAddEventToMasterQueue(pWindow, EVENT_PAINT, 0, 0);
	
	LockFree(&pWindow->m_screenLock);
}

static void ResizeWindowUnsafe(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	if (!pWindow->m_hidden)
	{
		pWindow->m_hidden |= WI_NOHIDDEN;
		HideWindowUnsafe (pWindow);
	}
	else
	{
		pWindow->m_hidden &= ~WI_NOHIDDEN;
	}
	
	ResizeWindowInternal (pWindow, newPosX, newPosY, newWidth, newHeight);
	
	//going to show up later by itself
	//ShowWindowUnsafe (pWindow);
}

void ResizeWindow(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	if (IsWindowManagerTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		ResizeWindowUnsafe(pWindow, newPosX, newPosY, newWidth, newHeight);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_RESIZE;
	action.rect.left   = newPosX;
	action.rect.top    = newPosY;
	action.rect.right  = newPosX + newWidth;
	action.rect.bottom = newPosY + newHeight;
	
	//WindowAction* ptr =
	ActionQueueAdd(action);
	
	//while (ptr->bInProgress)
	//	KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void SelectWindowUnsafe(Window* pWindow);

void FreeWindow(Window* pWindow)
{
	SAFE_DELETE(pWindow->m_vbeData.m_framebuffer32);
	pWindow->m_vbeData.m_version = 0;
	
	SAFE_DELETE (pWindow->m_pControlArray);
	pWindow->m_controlArrayLen = 0;
	
	SAFE_DELETE(pWindow->m_title);
	SAFE_DELETE(pWindow->m_eventQueue);
	SAFE_DELETE(pWindow->m_eventQueueParm1);
	SAFE_DELETE(pWindow->m_eventQueueParm2);
	SAFE_DELETE(pWindow->m_vbeData.m_drs);
	
	// Clear everything
	memset (pWindow, 0, sizeof (*pWindow));
}

void NukeWindowUnsafe (Window* pWindow)
{
	HideWindowUnsafe (pWindow);
	
	Window* pDraggedWnd = GetWindowFromIndex(g_currentlyClickedWindow);
	if (GetCurrentCursor() == &g_windowDragCursor  && pWindow == pDraggedWnd)
	{
		SetCursor(NULL);
	}
	
	if (g_focusedOnWindow == pWindow)
	{
		g_focusedOnWindow = NULL;
	}
	
	UserHeap *pHeapBackup = MuGetCurrentHeap();
	MuResetHeap();
	
	FreeWindow(pWindow);
	
	MuUseHeap (pHeapBackup);
	
	int et, p1, p2;
	while (WindowPopEventFromQueue(pWindow, &et, &p1, &p2));//flush queue

	// Reset the draw order
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		if (GetWindowFromIndex(g_windowDrawOrder[i]) == pWindow) //this is our window, reset the draw order
			g_windowDrawOrder[i] = -1;
	}

	// Select the (currently) frontmost window
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		if (g_windowDrawOrder[i] < 0)//doesn't exist
			continue;
		if (GetWindowFromIndex(g_windowDrawOrder[i])->m_flags & WF_SYSPOPUP) //prioritize non-system windows
			continue;

		SelectWindowUnsafe(GetWindowFromIndex(g_windowDrawOrder[i]));
		return;
	}

	// Select the (currently) frontmost window, even if it's a system popup
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		if (g_windowDrawOrder[i] < 0)//doesn't exist
			continue;

		SelectWindowUnsafe(GetWindowFromIndex(g_windowDrawOrder[i]));
		return;
	}
	
	RequestTaskbarUpdate();
}

void NukeWindow (Window* pWindow)
{
	if (IsWindowManagerTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		NukeWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_DESTROY;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void DestroyWindow (Window* pWindow)
{
	if (!IsWindowManagerRunning())
		return;
	
	if (!pWindow)
	{
		SLogMsg("Tried to destroy nullptr window?");
		return;
	}
	if (!pWindow->m_used)
		return;
	
	WindowAddEventToMasterQueue(pWindow, EVENT_DESTROY, 0, 0);
	// the task's last WindowCheckMessages call will see this and go
	// "ah yeah they want my window gone", then the WindowCallback will reply and say
	// "yeah you're good to go" and call ReadyToDestroyWindow().
}

void SelectWindowUnsafe(Window* pWindow)
{
	bool wasSelectedBefore = pWindow->m_isSelected;
	if (!wasSelectedBefore)
	{
		SetFocusedConsole (NULL);
		g_focusedOnWindow = NULL;
		for (int i = 0; i < WINDOWS_MAX; i++)
		{
			if (g_windows[i].m_used)
			{
				if (g_windows[i].m_isSelected)
				{
					g_windows[i].m_isSelected = false;
					WindowRegisterEventUnsafe(&g_windows[i], EVENT_KILLFOCUS, 0, 0);
//					WindowRegisterEventUnsafe(&g_windows[i], EVENT_PAINT, 0, 0);
					//g_windows[i].m_vbeData.m_dirty = true;
					//g_windows[i].m_renderFinished = true;
					//todo: just draw the title bar
				}
			}
		}
		
		MovePreExistingWindowToFront (pWindow - g_windows);
		pWindow->m_isSelected = true;
		UpdateDepthBuffer();
		WindowRegisterEventUnsafe(pWindow, EVENT_SETFOCUS, 0, 0);
//		WindowRegisterEventUnsafe(pWindow, EVENT_PAINT, 0, 0);
		pWindow->m_vbeData.m_dirty = true;
		pWindow->m_renderFinished = true;
		pWindow->m_vbeData.m_drs->m_bIgnoreAndDrawAll = true;
		SetFocusedConsole (pWindow->m_consoleToFocusKeyInputsTo);
		g_focusedOnWindow = pWindow;
	}
	
	RequestTaskbarUpdate();
}

void SelectWindow(Window* pWindow)
{
	if (IsWindowManagerTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		SelectWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_SELECT;
	
	//WindowAction* ptr = 
	ActionQueueAdd(action);
	
	//while (ptr->bInProgress)
	//	KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void* WmCAllocate(size_t sz)
{
	void* pMem = MmAllocate(sz);
	if (!pMem) return NULL;
	
	memset(pMem, 0, sz);
	return pMem;
}

Window* CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags)
{
	SLogMsg("Sizeof window: %d", sizeof(Window));
	
	flags &= ~WI_INTEMASK;
	
	if (!IsWindowManagerRunning())
	{
		LogMsg("WARNING: This program is a GUI program. It does not run in emergency text mode. To run this program, run the command \"w\" in the console.");
		return NULL;
	}
	
	LockAcquire (&g_CreateLock);
	
	if (xSize > GetScreenWidth())
		xSize = GetScreenWidth();
	if (ySize > GetScreenHeight())
		ySize = GetScreenHeight();
	
	if (xPos < 0 || yPos < 0)
	{
		g_NewWindowX += TITLE_BAR_HEIGHT + 4;
		g_NewWindowY += TITLE_BAR_HEIGHT + 4;
		if((g_NewWindowX + xSize + 50) >= GetScreenWidth())
			g_NewWindowX = 10;
		if((g_NewWindowY + ySize + 50) >= GetScreenHeight())
			g_NewWindowY = 10;
		xPos = g_NewWindowX;
		yPos = g_NewWindowY;
	}
	
	if (!(flags & WF_EXACTPOS))
	{
		if (xPos >= GetScreenWidth () - xSize)
			xPos  = GetScreenWidth () - xSize-1;
		if (yPos >= GetScreenHeight() - ySize)
			yPos  = GetScreenHeight() - ySize-1;
	}
	
	if (!(flags & WF_ALWRESIZ))
		flags |= WF_NOMAXIMZ;
	
	int freeArea = -1;
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (!g_windows[i].m_used)
		{
			freeArea = i; break;
		}
	}
	if (freeArea == -1) return NULL;//can't create the window.
	
	Window* pWnd = &g_windows[freeArea];
	memset (pWnd, 0, sizeof *pWnd);
	
	pWnd->m_used  = true;
	pWnd->m_title = WmCAllocate(WINDOW_TITLE_MAX);
	pWnd->m_eventQueue = WmCAllocate(EVENT_QUEUE_MAX * sizeof(short));
	pWnd->m_eventQueueParm1 = WmCAllocate(EVENT_QUEUE_MAX * sizeof(int));
	pWnd->m_eventQueueParm2 = WmCAllocate(EVENT_QUEUE_MAX * sizeof(int));
	
	if (!pWnd->m_title || !pWnd->m_eventQueue || !pWnd->m_eventQueueParm1 || !pWnd->m_eventQueueParm2)
	{
		SAFE_DELETE(pWnd->m_title);
		SAFE_DELETE(pWnd->m_eventQueue);
		SAFE_DELETE(pWnd->m_eventQueueParm1);
		SAFE_DELETE(pWnd->m_eventQueueParm2);
		
		SLogMsg("Couldn't allocate some parameters for the window!");
		
		pWnd->m_used = false;
		LockFree (&g_CreateLock);
		return NULL;
	}
	
	int strl = strlen (title) + 1;
	if (strl >= WINDOW_TITLE_MAX) strl = WINDOW_TITLE_MAX - 1;
	memcpy (pWnd->m_title, title, strl + 1);
	
	pWnd->m_renderFinished = false;
	pWnd->m_hidden         = true;//false;
	pWnd->m_isBeingDragged = false;
	pWnd->m_isSelected     = false;
	pWnd->m_minimized      = false;
	pWnd->m_maximized      = false;
	pWnd->m_clickedInside  = false;
	pWnd->m_flags          = flags;// | WF_FLATBORD;
	
	pWnd->m_EventQueueLock.m_held = false;
	pWnd->m_EventQueueLock.m_task_owning_it = NULL;
	
	pWnd->m_rect.left = xPos;
	pWnd->m_rect.top  = yPos;
	pWnd->m_rect.right  = xPos + xSize;
	pWnd->m_rect.bottom = yPos + ySize;
	pWnd->m_eventQueueSize = 0;
	pWnd->m_markedForDeletion = false;
	pWnd->m_callback = proc; 
	
	pWnd->m_lastHandledMessagesWhen          = GetTickCount();
	pWnd->m_lastSentPaintEventExternallyWhen = 0;
	pWnd->m_frequentWindowRenders            = 0;
	pWnd->m_cursorID_backup                  = 0;
	
	pWnd->m_consoleToFocusKeyInputsTo = NULL;
	
	pWnd->m_vbeData.m_version       = VBEDATA_VERSION_2;
	
	pWnd->m_vbeData.m_framebuffer32 = MmAllocatePhy (sizeof (uint32_t) * xSize * ySize, ALLOCATE_BUT_DONT_WRITE_PHYS);
	pWnd->m_vbeData.m_drs = WmCAllocate(sizeof(DsjRectSet));
	
	if (!pWnd->m_vbeData.m_framebuffer32 || !pWnd->m_vbeData.m_drs)
	{
		SAFE_DELETE(pWnd->m_vbeData.m_framebuffer32);
		SAFE_DELETE(pWnd->m_vbeData.m_drs);
		SAFE_DELETE(pWnd->m_title);
		SAFE_DELETE(pWnd->m_eventQueue);
		SAFE_DELETE(pWnd->m_eventQueueParm1);
		SAFE_DELETE(pWnd->m_eventQueueParm2);
		SLogMsg("Cannot allocate window buffer for '%s', out of memory!!!", pWnd->m_title);
		ILogMsg("Cannot allocate window buffer for '%s', out of memory!!!", pWnd->m_title);
		pWnd->m_used = false;
		LockFree (&g_CreateLock);
		return NULL;
	}
	ZeroMemory (pWnd->m_vbeData.m_framebuffer32,  sizeof (uint32_t) * xSize * ySize);
	pWnd->m_vbeData.m_width         = xSize;
	pWnd->m_vbeData.m_height        = ySize;
	pWnd->m_vbeData.m_pitch32       = xSize;
	pWnd->m_vbeData.m_bitdepth      = 2;     // 32 bit :)
	
	pWnd->m_iconID   = ICON_APPLICATION;
	
	pWnd->m_cursorID = CURSOR_DEFAULT;
	
	pWnd->m_eventQueueSize = 0;
	
	pWnd->m_screenLock.m_held = false;
	
	//give the window a starting point of 10 controls:
	pWnd->m_controlArrayLen = 10;
	size_t controlArraySize = sizeof(Control) * pWnd->m_controlArrayLen;
	pWnd->m_pControlArray   = (Control*)MmAllocateK(controlArraySize);
	
	if (!pWnd->m_pControlArray)
	{
		// The framebuffer fit, but this didn't
		ILogMsg("Couldn't allocate pControlArray for window, out of memory!");
		SAFE_DELETE(pWnd->m_vbeData.m_framebuffer32);
		SAFE_DELETE(pWnd->m_vbeData.m_drs);
		SAFE_DELETE(pWnd->m_title);
		SAFE_DELETE(pWnd->m_eventQueue);
		SAFE_DELETE(pWnd->m_eventQueueParm1);
		SAFE_DELETE(pWnd->m_eventQueueParm2);
		pWnd->m_used = false;
		LockFree (&g_CreateLock);
		return NULL;
	}
	
	memset(pWnd->m_pControlArray, 0, controlArraySize);
	
	WindowRegisterEvent(pWnd, EVENT_CREATE, 0, 0);
	WindowRegisterEvent(pWnd, EVENT_PAINT, 0, 0);
	
	LockFree (&g_CreateLock);
	
	RequestTaskbarUpdate();
	
	return pWnd;
}

// Main loop thread.
void RedrawEverything()
{
	VBEData* pBkp = g_vbeData;
	VidSetVBEData(NULL);
	UpdateDepthBuffer();
	
	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	RedrawBackground (r);
	
	//for each window, send it a EVENT_PAINT:
	for (int p = 0; p < WINDOWS_MAX; p++)
	{
		Window* pWindow = &g_windows [p];
		if (!pWindow->m_used) continue;
		
		int prm = MAKE_MOUSE_PARM(pWindow->m_rect.right - pWindow->m_rect.left, pWindow->m_rect.bottom - pWindow->m_rect.top);
		WindowAddEventToMasterQueue(pWindow, EVENT_SIZE,  prm, prm);
		WindowAddEventToMasterQueue(pWindow, EVENT_PAINT, 0,   0);
		//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
		pWindow->m_renderFinished = true;
	}
	VidSetVBEData(pBkp);
}

#ifdef OPTIMIZED_WITH_RECTANGLE_STACK
void WindowBlitTakingIntoAccountOcclusions(Rectangle e, Window* pWindow)
{
	//WmSplitRectangle
	Rectangle* pRect = NULL, *end = NULL;
	
	WmSplitRectangle(e, pWindow, &pRect, &end);
	
	for (; pRect != end; pRect++)
	{
		int eleft = pRect->left - pWindow->m_rect.left;
		int etop  = pRect->top  - pWindow->m_rect.top;
		
		//optimization
		VidBitBlit(
			g_vbeData,
			pRect->left,
			pRect->top,
			pRect->right  - pRect->left,
			pRect->bottom - pRect->top,
			&pWindow->m_vbeData,
			eleft, etop,
			BOP_SRCCOPY
		);
	}

}
#else
void WindowBlitTakingIntoAccountOcclusions(short windIndex, uint32_t* texture, int x, int x2, int y, int y2, int tw, UNUSED int th, int szx, int szy)
{
	//TODO: clean up this function!
	int oi = 0;
	if (y < 0)
	{
		oi += -y * tw;
		y = 0;
	}
	
	if (x > x2) return;
	if (y > y2) return;
	
	oi += szy * tw + szx;
	
	int sx = GetScreenWidth(), sy = GetScreenHeight();
	int pitch  = g_vbeData->m_pitch32, width  = g_vbeData->m_width;
	int offfb,                         offcp;
	for (int j = y; j != y2; j++)
	{
		int o = oi;
		if (j >= sy) break;
		offfb = j * pitch, offcp = j * width;
		if (x > 0) offfb += x, offcp += x;
		for (int i = x; i != x2; i++)
		{
			if (i < sx && i >= 0)
			{
				short n = g_windowDepthBuffer [offcp];
				if (n == windIndex)
				{
					g_framebufferCopy         [offcp] = texture[o];
					g_vbeData->m_framebuffer32[offfb] = texture[o];
				}
				offcp++;
				offfb++;
			}
			o++;
		}
		oi += tw;
	}
}
#endif

//extern void VidPlotPixelCheckCursor(unsigned x, unsigned y, unsigned color);
void RenderWindow (Window* pWindow)
{
	if (!IsWindowManagerTask())
	{
		SLogMsg("Warning: Calling RenderWindow outside of the main window task can cause data races and stuff!");
	}
	
	
	if (pWindow->m_bObscured)
	{
		SLogMsg("Window %s obscured, don't draw", pWindow->m_title);
		return;
	}
	if (pWindow->m_minimized)
	{
		// Draw as icon
		RenderIconForceSize(pWindow->m_iconID, pWindow->m_rect.left, pWindow->m_rect.top, 32);
		
		return;
	}
	
	//to avoid clashes with other threads printing, TODO use a safelock!!
#ifdef DIRTY_RECT_TRACK
	cli;
	
	DsjRectSet local_copy;
	memcpy (&local_copy, pWindow->m_vbeData.m_drs, sizeof local_copy);
	DisjointRectSetClear(pWindow->m_vbeData.m_drs);
	
	sti;
#endif
	
	//ACQUIRE_LOCK(g_screenLock);
	g_vbeData = &g_mainScreenVBEData;
	
#ifndef OPTIMIZED_WITH_RECTANGLE_STACK
	int windIndex = pWindow - g_windows;
#endif
	
	int x = pWindow->m_rect.left,  y = pWindow->m_rect.top;
	short n = GetWindowIndexInDepthBuffer (x, y);
	if (n == -1)
	{
		if (x >= 0 && y >= 0 && x < GetScreenWidth() && y < GetScreenHeight())
		{
			SLogMsg("Updating during RenderWindow()? Why?");
			UpdateDepthBuffer();
		}
	}
	
#ifdef DIRTY_RECT_TRACK
	if (!local_copy.m_bIgnoreAndDrawAll)
	{
		for (int i = 0; i < local_copy.m_rectCount; i++)
		{
			//clip all rectangles!
			Rectangle *e = &local_copy.m_rects[i];
			if (e->left < 0) e->left = 0;
			if (e->top  < 0) e->top  = 0;
			
			if (e->right >= (int)pWindow->m_vbeData.m_width)
				e->right  = (int)pWindow->m_vbeData.m_width;
			if (e->bottom >= (int)pWindow->m_vbeData.m_height)
				e->bottom  = (int)pWindow->m_vbeData.m_height;
		}
	}
#endif
	if (pWindow->m_bForemost)
	{
	#ifdef DIRTY_RECT_TRACK
		if (local_copy.m_bIgnoreAndDrawAll)
		{
	#endif
			//optimization
			VidBitBlit(
				g_vbeData,
				pWindow->m_rect.left,
				pWindow->m_rect.top,
				pWindow->m_vbeData.m_width,
				pWindow->m_vbeData.m_height,
				&pWindow->m_vbeData,
				0, 0,
				BOP_SRCCOPY
			);
	#ifdef DIRTY_RECT_TRACK
		}
		else for (int i = 0; i < local_copy.m_rectCount; i++)
		{
			Rectangle e = local_copy.m_rects[i];
			//optimization
			VidBitBlit(
				g_vbeData,
				pWindow->m_rect.left + e.left,
				pWindow->m_rect.top  + e.top,
				e.right  - e.left,
				e.bottom - e.top,
				&pWindow->m_vbeData,
				e.left, e.top,
				BOP_SRCCOPY
			);
		}
	#endif
	}
	else
	{
		int tw = pWindow->m_vbeData.m_width, th = pWindow->m_vbeData.m_height;
	#ifndef OPTIMIZED_WITH_RECTANGLE_STACK
		uint32_t *texture = pWindow->m_vbeData.m_framebuffer32;
	#endif
		
		int x2 = x + tw, y2 = y + th;
		
	#ifdef DIRTY_RECT_TRACK
		if (local_copy.m_bIgnoreAndDrawAll)
		{
	#endif

		#ifdef OPTIMIZED_WITH_RECTANGLE_STACK
			WindowBlitTakingIntoAccountOcclusions(pWindow->m_rect, pWindow);
		#else
			WindowBlitTakingIntoAccountOcclusions(windIndex, texture, x, x2, y, y2, tw, th, 0, 0);
		#endif
			
	#ifdef DIRTY_RECT_TRACK
		}
		else for (int i = 0; i < local_copy.m_rectCount; i++)
		{
			Rectangle e = local_copy.m_rects[i];
		#ifdef OPTIMIZED_WITH_RECTANGLE_STACK
			e.left   += pWindow->m_rect.left;
			e.right  += pWindow->m_rect.left;
			e.top    += pWindow->m_rect.top;
			e.bottom += pWindow->m_rect.top;
			WindowBlitTakingIntoAccountOcclusions(e, pWindow);
		#else
			WindowBlitTakingIntoAccountOcclusions(
				windIndex,
				texture,
				x + e.left,
				x + e.right,
				y + e.top,
				y + e.bottom,
				tw,
				th,
				e.left,
				e.top
			);
		#endif
		}
	#endif
	}
}
