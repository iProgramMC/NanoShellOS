/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Window Object Management Module
******************************************/
#include "wi.h"
#include "../mm/memoryi.h"

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

// note: The rectangle structure works as "how much to expand from X direction".
// So a margin of {3, 0, 3, 0} means you expand a rectangle by 3 to the left, and 3 to the right.

Rectangle GetMarginsWindowFlagsAndBorderSize(uint32_t flags, int borderSize)
{
	Rectangle rect = { 0, 0, 0, 0 };
	
	// if the window has a title...
	if (~flags & WF_NOTITLE)
	{
		rect.top += TITLE_BAR_HEIGHT;
	}
	
	// If the window has a flat border.
	if (flags & WF_MAXIMIZE)
	{
	}
	else if (flags & WF_FLATBORD)
	{
		rect.left  ++;
		rect.top   ++;
		rect.right ++;
		rect.bottom++;
	}
	// Otherwise, check if the window doesn't want no border.
	else if (~flags & WF_NOBORDER)
	{
		rect.left   += borderSize;
		rect.right  += borderSize;
		rect.top    += borderSize;
		rect.bottom += borderSize;
	}
	
	return rect;
}

Rectangle GetMarginsWindowFlags(uint32_t flags)
{
	return GetMarginsWindowFlagsAndBorderSize(flags, BORDER_SIZE);
}

Rectangle GetWindowMargins(Window* pWindow)
{
	return GetMarginsWindowFlagsAndBorderSize(pWindow->m_lastWindowFlags, pWindow->m_knownBorderSize);
}

Rectangle GetWindowClientRect(Window* pWindow, bool offset)
{
	Rectangle rect = pWindow->m_rect;
	
	if (!offset)
	{
		rect.right  -= rect.left;
		rect.bottom -= rect.top;
		rect.left = rect.top = 0;
	}
	
	return rect;
}

void WmOnChangedBorderParms(Window* pWindow)
{
	Rectangle ca = GetWindowClientRect(pWindow, true);
	int clientAreaWidth  = ca.right  - ca.left;
	int clientAreaHeight = ca.bottom - ca.top;
	
	SLogMsg("Client area width: %d  height: %d",clientAreaWidth,clientAreaHeight);
	
	Rectangle margins = GetMarginsWindowFlagsAndBorderSize(pWindow->m_flags, BORDER_SIZE);
	
	int newX = ca.left - margins.left, newY = ca.top - margins.top;
	int newW = clientAreaWidth + margins.left + margins.right;
	int newH = clientAreaHeight + margins.top + margins.bottom;
	
	SLogMsg("New dimensions: X:%d Y:%d W:%d H:%d",newX,newY,newW,newH);
	
	if (pWindow->m_rect.left == newX && pWindow->m_rect.top == newY && pWindow->m_rect.right == newX + newW && pWindow->m_rect.bottom == newY + newH)
		return;
	
	ResizeWindow(pWindow, newX, newY, newW, newH);
}

void WmOnChangedBorderSize()
{
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		Window* pWindow = &g_windows[i];
		
		if (!pWindow->m_used) continue;
		
		WindowAddEventToMasterQueue(pWindow, EVENT_BORDER_SIZE_UPDATE_PRIVATE, 0, 0);
	}
}

void WmRecalculateClientRect(Window* pWindow)
{
	pWindow->m_rect = pWindow->m_fullRect;
	Rectangle margins = GetWindowMargins(pWindow);
	pWindow->m_rect.left   += margins.left;
	pWindow->m_rect.top    += margins.top;
	pWindow->m_rect.right  -= margins.right;
	pWindow->m_rect.bottom -= margins.bottom;
}

int AddTimer(Window* pWindow, int frequency, int event)
{
	KeVerifyInterruptsEnabled;
	
	cli;
	if (pWindow->m_timer_count >= C_MAX_WIN_TIMER)
	{
		sti;
		return -1;
	}
	
	// find a free spot
	int freeSpot = 0;
	for (int i = 0; i < C_MAX_WIN_TIMER; i++)
	{
		if (pWindow->m_timers[i].m_used == false)
		{
			freeSpot = i;
			break;
		}
	}
	
	WindowTimer* pTimer  = &pWindow->m_timers[freeSpot];
	pTimer->m_used       = true;
	pTimer->m_frequency  = frequency;
	pTimer->m_nextTickAt = 0;
	pTimer->m_firedEvent = event;
	
	pWindow->m_timer_count++;
	sti;
	
	return pTimer - pWindow->m_timers;
}

void DisarmTimer(Window* pWindow, int timerID)
{
	// timerID out of bounds guard
	if (timerID < 0 || timerID >= C_MAX_WIN_TIMER) return;
	
	KeVerifyInterruptsEnabled;
	
	cli;
	
	pWindow->m_timers[timerID].m_used = false;
	pWindow->m_timer_count--;
	
	sti;
}

void ChangeTimer(Window* pWindow, int timerID, int newFrequency, int newEvent)
{
	// timerID out of bounds guard
	if (timerID < 0 || timerID >= C_MAX_WIN_TIMER) return;
	
	KeVerifyInterruptsEnabled;
	
	cli;
	
	if (newFrequency != -1)
		pWindow->m_timers[timerID].m_frequency  = newFrequency;
	
	if (newEvent != -1)
		pWindow->m_timers[timerID].m_firedEvent = newEvent;
	
	sti;
}

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
	RefreshRectangle(pWnd->m_fullRect, pWnd);
}

void HideWindowUnsafe (Window* pWindow)
{
	pWindow->m_hidden = true;
	UndrawWindow(pWindow);
}

static void ShowWindowUnsafe (Window* pWindow)
{
	pWindow->m_hidden = false;
	
	// Render it to the vbeData:
	if (pWindow->m_flags & WF_MINIMIZE)
	{
		//TODO?
	}
	else
	{
		// TODO: fix this will leave garbage that should be occluded out. Fix this by adding a RefreshRectangle which refreshes things above this window.
		RefreshRectangle(pWindow->m_fullRect, NULL);
	}
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
		newPosX = pWindow->m_fullRect.left;
		newPosY = pWindow->m_fullRect.top;
	}
	if (newWidth < WINDOW_MIN_WIDTH)
		newWidth = WINDOW_MIN_WIDTH;
	if (newHeight< WINDOW_MIN_HEIGHT)
		newHeight= WINDOW_MIN_HEIGHT;
	
	HideWindow(pWindow);
	
	uint32_t* pNewFb = (uint32_t*)MmAllocatePhy(newWidth * newHeight * sizeof(uint32_t), ALLOCATE_BUT_DONT_WRITE_PHYS);
	if (!pNewFb)
	{
		SLogMsg("Cannot resize window to %dx%d, out of memory", newWidth, newHeight);
		return;
	}
	
	// Copy the entire framebuffer's contents from old to new.
	int oldWidth = pWindow->m_fullVbeData.m_width, oldHeight = pWindow->m_fullVbeData.m_height;
	int minWidth = newWidth, minHeight = newHeight;
	if (minWidth > oldWidth)
		minWidth = oldWidth;
	if (minHeight > oldHeight)
		minHeight = oldHeight;
	
	for (int i = 0; i < minHeight; i++)
	{
		memcpy_ints (&pNewFb[i * newWidth], &pWindow->m_fullVbeData.m_framebuffer32[i * oldWidth], minWidth);
		
		if (newWidth > minWidth)
			memset_ints(&pNewFb[i * newWidth + minWidth], 0, newWidth - minWidth);
	}
	
	for (int i = minHeight; i < newHeight; i++)
	{
		memset_ints(&pNewFb[i * newWidth], 0x80, newWidth);
	}
	
	// Free the old framebuffer.  This action should be done atomically.
	// TODO: If I ever decide to add locks to mmfree etc, then fix this so that it can't cause deadlocks!!
	
	MmFree(pWindow->m_fullVbeData.m_framebuffer32);
	pWindow->m_fullVbeData.m_framebuffer32 = pNewFb;
	pWindow->m_fullVbeData.m_width   = newWidth;
	pWindow->m_fullVbeData.m_pitch32 = newWidth;
	pWindow->m_fullVbeData.m_pitch16 = newWidth*2;
	pWindow->m_fullVbeData.m_pitch   = newWidth*4;
	pWindow->m_fullVbeData.m_height  = newHeight;
	
	Rectangle oldMarg = GetWindowMargins(pWindow);
	pWindow->m_knownBorderSize = BORDER_SIZE;
	pWindow->m_lastWindowFlags = pWindow->m_flags;
	Rectangle margins = GetWindowMargins(pWindow);
	
	pWindow->m_vbeData.m_framebuffer32 = &pNewFb[newWidth * margins.top + margins.left];
	pWindow->m_vbeData.m_width    = newWidth  - margins.left - margins.right;
	pWindow->m_vbeData.m_height   = newHeight - margins.top  - margins.bottom;
	pWindow->m_vbeData.m_pitch32  = newWidth;
	pWindow->m_vbeData.m_pitch16  = newWidth * 2;
	pWindow->m_vbeData.m_pitch    = newWidth * 4;
	pWindow->m_vbeData.m_offsetX  = margins.left;
	pWindow->m_vbeData.m_offsetY  = margins.top;
	
	pWindow->m_fullRect.left   = newPosX;
	pWindow->m_fullRect.top    = newPosY;
	pWindow->m_fullRect.right  = pWindow->m_fullRect.left + newWidth;
	pWindow->m_fullRect.bottom = pWindow->m_fullRect.top  + newHeight;
	
	WmRecalculateClientRect(pWindow);
	
	// Mark as dirty.
	pWindow->m_fullVbeData.m_dirty = true;
	
	ShowWindow(pWindow);
	
	// Send window events: EVENT_SIZE, EVENT_PAINT.
	WindowAddEventToMasterQueue(pWindow, EVENT_SIZE,
		MAKE_MOUSE_PARM(newWidth - margins.left - margins.right, newHeight - margins.left - margins.right),
		MAKE_MOUSE_PARM(oldWidth - oldMarg.left - oldMarg.right, oldHeight - oldMarg.left - oldMarg.right)
	);
}

void ResizeWindow(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	// request a resize from the window itself.
	WindowAddEventToMasterQueue(pWindow, EVENT_REQUEST_RESIZE_PRIVATE, MAKE_MOUSE_PARM(newPosX, newPosY), MAKE_MOUSE_PARM(newWidth, newHeight));
}

void SelectWindowUnsafe(Window* pWindow);

void FreeWindow(Window* pWindow)
{
	SAFE_DELETE(pWindow->m_fullVbeData.m_framebuffer32);
	pWindow->m_fullVbeData.m_version = 0;
	pWindow->m_vbeData.m_version = 0;
	
	SAFE_DELETE (pWindow->m_pControlArray);
	pWindow->m_controlArrayLen = 0;
	
	SAFE_DELETE(pWindow->m_title);
	SAFE_DELETE(pWindow->m_eventQueue);
	SAFE_DELETE(pWindow->m_eventQueueParm1);
	SAFE_DELETE(pWindow->m_eventQueueParm2);
	SAFE_DELETE(pWindow->m_fullVbeData.m_drs);
	SAFE_DELETE(pWindow->m_inputBuffer);
	
	// Clear everything
	memset (pWindow, 0, sizeof (*pWindow));
}

void NukeWindowUnsafe (Window* pWindow)
{
	HideWindowUnsafe (pWindow);
	
	RemoveWindowFromDrawOrder(pWindow - g_windows);
	
	Window* pDraggedWnd = g_currentlyClickedWindow;
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
	if (!pWindow->m_used) return;
	
	bool bNeverSelect = (pWindow->m_flags & WI_NEVERSEL);
	
	bool wasSelectedBefore = pWindow->m_isSelected;
	
	if (!wasSelectedBefore)
	{
		if (!bNeverSelect)
		{
			SetFocusedConsole (NULL);
			g_focusedOnWindow = NULL;
		}
		
		for (int i = 0; i < WINDOWS_MAX && !bNeverSelect; i++)
		{
			if (g_windows[i].m_used)
			{
				if (g_windows[i].m_isSelected)
				{
					g_windows[i].m_isSelected = false;
					WindowRegisterEventUnsafe(&g_windows[i], EVENT_KILLFOCUS, 0, 0);
				}
			}
		}
		
		MovePreExistingWindowToFront (pWindow - g_windows);
		
		if (!bNeverSelect)
		{
			pWindow->m_isSelected = true;
			WindowRegisterEventUnsafe(pWindow, EVENT_SETFOCUS, 0, 0);
			pWindow->m_fullVbeData.m_dirty = true;
			pWindow->m_renderFinished = true;
			pWindow->m_fullVbeData.m_drs->m_bIgnoreAndDrawAll = true;
			SetFocusedConsole (pWindow->m_consoleToFocusKeyInputsTo);
			g_focusedOnWindow = pWindow;
		}
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

void* WmCAllocateIntsDis(size_t sz)
{
	void* pMem = MhAllocate(sz, NULL);
	if (!pMem) return NULL;
	
	memset(pMem, 0, sz);
	return pMem;
}

extern int g_TaskbarHeight;

Window* CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags)
{
	flags &= ~WI_INTEMASK;
	
	// TODO: For now.
	//flags &= ~(WF_NOTITLE | WF_NOBORDER | WF_FLATBORD);
	
	if (!IsWindowManagerRunning())
	{
		LogMsg("WARNING: This program is a GUI program. It does not run in emergency text mode. To run this program, run the command \"w\" in the console.");
		return NULL;
	}
	
	LockAcquire (&g_CreateLock);
	
	Rectangle margins = GetMarginsWindowFlags(flags);
	
	xSize += margins.left + margins.right;
	ySize += margins.top  + margins.bottom;
	
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
	
	xPos  -= margins.left;
	yPos  -= margins.top;
	
	if (!(flags & WF_EXACTPOS))
	{
		if (xPos >= GetScreenWidth () - xSize)
			xPos  = GetScreenWidth () - xSize-1;
		if (yPos >= GetScreenHeight() - ySize)
			yPos  = GetScreenHeight() - ySize-1;
	}
	
	if (!(flags & WF_ALWRESIZ))
		flags |= WF_NOMAXIMZ;
	
	int clientXSize = xSize - margins.left - margins.right, clientYSize = ySize - margins.top - margins.bottom;
	
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
	
	cli;
	pWnd->m_used  = true;
	pWnd->m_title = WmCAllocateIntsDis(WINDOW_TITLE_MAX);
	pWnd->m_eventQueue = WmCAllocateIntsDis(EVENT_QUEUE_MAX * sizeof(short));
	pWnd->m_eventQueueParm1 = WmCAllocateIntsDis(EVENT_QUEUE_MAX * sizeof(int));
	pWnd->m_eventQueueParm2 = WmCAllocateIntsDis(EVENT_QUEUE_MAX * sizeof(int));
	pWnd->m_inputBuffer     = WmCAllocateIntsDis(WIN_KB_BUF_SIZE);
	sti;
	
	if (!pWnd->m_title || !pWnd->m_eventQueue || !pWnd->m_eventQueueParm1 || !pWnd->m_eventQueueParm2 || !pWnd->m_inputBuffer)
	{
		SAFE_DELETE(pWnd->m_title);
		SAFE_DELETE(pWnd->m_eventQueue);
		SAFE_DELETE(pWnd->m_eventQueueParm1);
		SAFE_DELETE(pWnd->m_eventQueueParm2);
		SAFE_DELETE(pWnd->m_inputBuffer);
		
		SLogMsg("Couldn't allocate some parameters for the window!");
		
		pWnd->m_used = false;
		LockFree (&g_CreateLock);
		return NULL;
	}
	
	int strl = strlen (title) + 1;
	if (strl >= WINDOW_TITLE_MAX) strl = WINDOW_TITLE_MAX - 1;
	memcpy (pWnd->m_title, title, strl + 1);
	
	pWnd->m_pOwnerThread   = KeGetRunningTask();
	pWnd->m_renderFinished = false;
	pWnd->m_hidden         = true;//false;
	pWnd->m_isBeingDragged = false;
	pWnd->m_isSelected     = false;
	pWnd->m_clickedInside  = false;
	pWnd->m_flags          = pWnd->m_lastWindowFlags = flags;// | WF_FLATBORD;
	
	pWnd->m_EventQueueLock.m_held = false;
	pWnd->m_EventQueueLock.m_task_owning_it = NULL;
	
	pWnd->m_fullRect.left = xPos;
	pWnd->m_fullRect.top  = yPos;
	pWnd->m_fullRect.right  = xPos + xSize;
	pWnd->m_fullRect.bottom = yPos + ySize;
	
	if (pWnd->m_flags & WF_MAXIMIZE)
	{
		pWnd->m_rectBackup = pWnd->m_fullRect;
		
		pWnd->m_fullRect.left   = 0;
		pWnd->m_fullRect.top    = g_TaskbarHeight;
		pWnd->m_fullRect.right  = GetScreenWidth();
		pWnd->m_fullRect.bottom = GetScreenHeight() - g_TaskbarHeight;
		
		xSize = GetWidth (&pWnd->m_fullRect);
		ySize = GetHeight(&pWnd->m_fullRect);
		xPos = pWnd->m_fullRect.left;
		yPos = pWnd->m_fullRect.top;
		
		clientXSize = xSize - margins.left - margins.right;
		clientYSize = ySize - margins.top - margins.bottom;
		
		if (!(pWnd->m_flags & WF_FLATBORD))
			pWnd->m_flags |= WF_FLBRDFRC | WF_FLATBORD;
	}
	
	pWnd->m_eventQueueSize = 0;
	pWnd->m_markedForDeletion = false;
	pWnd->m_callback = proc; 
	
	pWnd->m_lastHandledMessagesWhen          = GetTickCount();
	pWnd->m_lastSentPaintEventExternallyWhen = 0;
	pWnd->m_frequentWindowRenders            = 0;
	pWnd->m_cursorID_backup                  = 0;
	
	pWnd->m_consoleToFocusKeyInputsTo = NULL;
	
	pWnd->m_fullVbeData.m_version       = VBEDATA_VERSION_3;
	
	pWnd->m_fullVbeData.m_framebuffer32 = MmAllocatePhy (sizeof (uint32_t) * xSize * ySize, ALLOCATE_BUT_DONT_WRITE_PHYS);
	pWnd->m_fullVbeData.m_drs = WmCAllocate(sizeof(DsjRectSet));
	
	pWnd->m_knownBorderSize = BORDER_SIZE;
	
	if (!pWnd->m_fullVbeData.m_framebuffer32 || !pWnd->m_fullVbeData.m_drs)
	{
		SAFE_DELETE(pWnd->m_fullVbeData.m_framebuffer32);
		SAFE_DELETE(pWnd->m_fullVbeData.m_drs);
		SAFE_DELETE(pWnd->m_title);
		SAFE_DELETE(pWnd->m_eventQueue);
		SAFE_DELETE(pWnd->m_eventQueueParm1);
		SAFE_DELETE(pWnd->m_eventQueueParm2);
		SAFE_DELETE(pWnd->m_inputBuffer);
		SLogMsg("Cannot allocate window buffer for '%s', out of memory!!!", pWnd->m_title);
		ILogMsg("Cannot allocate window buffer for '%s', out of memory!!!", pWnd->m_title);
		pWnd->m_used = false;
		LockFree (&g_CreateLock);
		return NULL;
	}
	
	pWnd->m_fullVbeData.m_offsetX = 0;
	pWnd->m_fullVbeData.m_offsetY = 0;
	
	ZeroMemory (pWnd->m_fullVbeData.m_framebuffer32,  sizeof (uint32_t) * xSize * ySize);
	pWnd->m_fullVbeData.m_width    = xSize;
	pWnd->m_fullVbeData.m_height   = ySize;
	pWnd->m_fullVbeData.m_pitch32  = xSize;
	pWnd->m_fullVbeData.m_pitch16  = xSize * 2;
	pWnd->m_fullVbeData.m_pitch    = xSize * 4;
	pWnd->m_fullVbeData.m_bitdepth = 2;     // 32 bit :)
	
	// set up the client vbeData
	pWnd->m_vbeData.m_framebuffer32 = &pWnd->m_fullVbeData.m_framebuffer32[xSize * margins.top + margins.left];
	pWnd->m_vbeData.m_width    = clientXSize;
	pWnd->m_vbeData.m_height   = clientYSize;
	pWnd->m_vbeData.m_pitch32  = xSize;
	pWnd->m_vbeData.m_pitch16  = xSize * 2;
	pWnd->m_vbeData.m_pitch    = xSize * 4;
	pWnd->m_vbeData.m_bitdepth = 2;
	pWnd->m_vbeData.m_drs      = pWnd->m_fullVbeData.m_drs;
	pWnd->m_vbeData.m_offsetX  = margins.left;
	pWnd->m_vbeData.m_offsetY  = margins.top;
	pWnd->m_vbeData.m_version  = VBEDATA_VERSION_3;
	
	pWnd->m_iconID   = ICON_APPLICATION;
	
	pWnd->m_cursorID = CURSOR_DEFAULT;
	
	pWnd->m_eventQueueSize = 0;
	
	pWnd->m_screenLock.m_held = false;
	
	WmRecalculateClientRect(pWnd);
	
	//give the window a starting point of 10 controls:
	pWnd->m_controlArrayLen = 10;
	size_t controlArraySize = sizeof(Control) * pWnd->m_controlArrayLen;
	pWnd->m_pControlArray   = (Control*)MmAllocateK(controlArraySize);
	
	if (!pWnd->m_pControlArray)
	{
		// The framebuffer fit, but this didn't
		ILogMsg("Couldn't allocate pControlArray for window, out of memory!");
		SAFE_DELETE(pWnd->m_fullVbeData.m_framebuffer32);
		SAFE_DELETE(pWnd->m_fullVbeData.m_drs);
		SAFE_DELETE(pWnd->m_title);
		SAFE_DELETE(pWnd->m_eventQueue);
		SAFE_DELETE(pWnd->m_eventQueueParm1);
		SAFE_DELETE(pWnd->m_eventQueueParm2);
		SAFE_DELETE(pWnd->m_inputBuffer);
		pWnd->m_used = false;
		LockFree (&g_CreateLock);
		return NULL;
	}
	
	memset(pWnd->m_pControlArray, 0, controlArraySize);
	AddWindowToDrawOrder (freeArea);
	
	pWnd->m_lastHandledMessagesWhen = GetTickCount();
	
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
	
	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	RedrawBackground (r);
	
	//for each window, send it a EVENT_PAINT:
	for (int p = 0; p < WINDOWS_MAX; p++)
	{
		Window* pWindow = &g_windows [p];
		if (!pWindow->m_used) continue;
		
		int prm = MAKE_MOUSE_PARM(pWindow->m_fullRect.right - pWindow->m_fullRect.left, pWindow->m_fullRect.bottom - pWindow->m_fullRect.top);
		WindowAddEventToMasterQueue(pWindow, EVENT_SIZE,  prm, prm);
		WindowAddEventToMasterQueue(pWindow, EVENT_PAINT, 0,   0);
		//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
		pWindow->m_renderFinished = true;
	}
	VidSetVBEData(pBkp);
}

void WindowBlitTakingIntoAccountOcclusions(Rectangle e, Window* pWindow)
{
	//WmSplitRectangle
	Rectangle* pRect = NULL, *end = NULL;
	
	WmSplitRectangle(e, pWindow, &pRect, &end);
	
	for (; pRect != end; pRect++)
	{
		int eleft = pRect->left - pWindow->m_fullRect.left;
		int etop  = pRect->top  - pWindow->m_fullRect.top;
		
		//optimization
		VidBitBlit(
			g_vbeData,
			pRect->left,
			pRect->top,
			pRect->right  - pRect->left,
			pRect->bottom - pRect->top,
			&pWindow->m_fullVbeData,
			eleft, etop,
			BOP_SRCCOPY
		);
	}
}

//extern void VidPlotPixelCheckCursor(unsigned x, unsigned y, unsigned color);
void RenderWindow (Window* pWindow)
{
	if (!IsWindowManagerTask())
	{
		SLogMsg("Warning: Calling RenderWindow outside of the main window task can cause data races and stuff!");
		SLogMsg("Return Address: %p, Task: %p", __builtin_return_address(0), KeGetRunningTask());
	}
	
	if (pWindow->m_flags & WF_MINIMIZE)
	{
		// Draw as icon
		RenderIconForceSize(pWindow->m_iconID, pWindow->m_fullRect.left, pWindow->m_fullRect.top, 32);
		
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
	
#ifdef DIRTY_RECT_TRACK
	if (!local_copy.m_bIgnoreAndDrawAll)
	{
		for (int i = 0; i < local_copy.m_rectCount; i++)
		{
			//clip all rectangles!
			Rectangle *e = &local_copy.m_rects[i];
			if (e->left < 0) e->left = 0;
			if (e->top  < 0) e->top  = 0;
			
			if (e->right >= (int)pWindow->m_fullVbeData.m_width)
				e->right  = (int)pWindow->m_fullVbeData.m_width;
			if (e->bottom >= (int)pWindow->m_fullVbeData.m_height)
				e->bottom  = (int)pWindow->m_fullVbeData.m_height;
		}
	}
#endif

#ifdef DIRTY_RECT_TRACK
	if (local_copy.m_bIgnoreAndDrawAll)
	{
#endif
		WindowBlitTakingIntoAccountOcclusions(pWindow->m_fullRect, pWindow);
#ifdef DIRTY_RECT_TRACK
	}
	else for (int i = 0; i < local_copy.m_rectCount; i++)
	{
		Rectangle e = local_copy.m_rects[i];
		e.left   += pWindow->m_fullRect.left;
		e.right  += pWindow->m_fullRect.left;
		e.top    += pWindow->m_fullRect.top;
		e.bottom += pWindow->m_fullRect.top;
		WindowBlitTakingIntoAccountOcclusions(e, pWindow);
	}
#endif
}
