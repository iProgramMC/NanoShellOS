/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Window Manager Tooltip Module
******************************************/
#include "wi.h"

#define LOCK_FPS
int g_WmLockMS = 16;
#define LOCK_MS g_WmLockMS
//#define LAG_DEBUG

bool RefreshMouse(void);
void RenderCursor(void);

int g_DefaultCursorID = CURSOR_DEFAULT;

int g_FPS, g_FPSThisSecond, g_FPSLastCounted;
Rectangle g_TaskbarMargins;

void QueryWindows(WindowQuery* table, size_t tableSize, size_t* numWindows)
{
	LockAcquire(&g_CreateLock);
	
	cli;
	*numWindows = 0;
	
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (!g_windows[i].m_used) continue;
		
		Window* pWindow = &g_windows[i];
		WindowQuery* pQuery = &table[(*numWindows)++];
		pQuery->windowID = i;
		pQuery->flags    = pWindow->m_flags;
		pQuery->iconID   = pWindow->m_iconID;
		strncpy(pQuery->titleOut, pWindow->m_title, pQuery->titleOutSize);
		pQuery->titleOut[pQuery->titleOutSize - 1] = 0;
		
		if (*numWindows == tableSize) break;
	}
	
	sti;
	
	LockFree(&g_CreateLock);
}

void UpdateFPSCounter()
{
	if (g_FPSLastCounted + 1000 <= GetTickCount())
	{
		g_FPS = g_FPSThisSecond;
		g_FPSThisSecond = 0;
		g_FPSLastCounted = GetTickCount();
	}
	g_FPSThisSecond++;
}

int GetWindowManagerFPS()
{
	return g_FPS;
}

Task *g_pWindowMgrTask;
bool IsWindowManagerTask ()
{
	return (KeGetRunningTask() == g_pWindowMgrTask);
}

bool g_windowManagerRunning = false;
bool g_heldAlt = false;

bool IsWindowManagerRunning()
{
	return g_windowManagerRunning;
}

void SetupWindowManager()
{
	LogMsg("Please wait...");
	
	g_DefaultCursorID = CURSOR_WAIT;
	
	memset (&g_windows, 0, sizeof (g_windows));
	
	g_debugConsole.curY = g_debugConsole.height / 2;
	g_clickQueueSize = 0;
	ResetWindowDrawOrder();
	WmCreateRectangleStack();
	//CoClearScreen (&g_debugConsole);
	g_debugConsole.curX = g_debugConsole.curY = 0;
	g_debugConsole.pushOrWrap = 1;
	
	g_windowManagerRunning = true;
	
	g_pShutdownMessage = NULL;
	
	g_shutdownSentDestroySignals = false;
	g_shutdownWaiting			 = false;
	
	LoadDefaultThemingParms ();
	//VidFillScreen(BACKGROUND_COLOR);
	SetDefaultBackground ();
	
	//redraw background?
	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	RedrawBackground (r);
	
	CfgGetIntValue(&g_WmLockMS, "Desktop::UpdateMS", 16);
	
	//VidSetFont(FONT_BASIC);
	//VidSetFont(FONT_TAMSYN_BOLD);
	//VidSetFont(FONT_TAMSYN_REGULAR);
	//VidSetFont(FONT_FAMISANS);
	VidSetFont(FONT_GLCD);
	//VidSetFont(FONT_BIGTEST);
	
	//LogMsg("\n\n\n");
	
	WindowCallInit ();
	
	//test:
#if !THREADING_ENABLED
	LogMsgNoCr("Huh!?? This shouldn't be on");
	LauncherEntry(0);
#else
	int errorCode = 0;
	Task* pTask;
	
	//create the taskbar task.
	errorCode = 0;
	pTask = KeStartTask(TaskbarEntry, 0, &errorCode);
	KeUnsuspendTask(pTask);
	KeDetachTask(pTask);
	DebugLogMsg("Created taskbar task. pointer returned:%x, errorcode:%x", pTask, errorCode);
	
#endif
}

void HandleKeypressOnWindow(unsigned char key)
{
	if (key == KEY_ALT)
	{
		g_heldAlt = true;
	}
	else if (key == (KEY_ALT | SCANCODE_RELEASE))
	{
		g_heldAlt = false;
		KillAltTab();
	}
	else if (key == KEY_TAB && g_heldAlt)
	{
		OnPressAltTabOnce();
	}
}

void WmOnTaskDied(Task *pTask)
{
	if (!g_windowManagerRunning) return;
	
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (g_windows[i].m_used)
		{
			if (g_windows[i].m_pSubThread   == pTask)
				g_windows[i].m_pSubThread   =  NULL;
		}
	}
}

void WmOnTaskCrashed(Task *pTask)
{
	if (!g_windowManagerRunning) return;
	
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (!g_windows[i].m_used) continue;
		
		if (g_windows[i].m_pOwnerThread != pTask) continue;
		
		// if there's a sub thread, kill that too
		if (g_windows[i].m_pSubThread)
		{
			KeKillTask(g_windows[i].m_pSubThread);
			g_windows[i].m_pSubThread = NULL;
		}
		
		// take over it ourselves.
		WmTakeOverWindow(&g_windows[i]);
		
		// send it an event: Die
		WindowRegisterEvent(&g_windows[i], EVENT_DESTROY, 0, 0);
	}
}

bool WidgetNone_OnEvent(Control* pCtl, int eventType, int parm1, int parm2, Window* pWindow);

// Takes over a window. Simple enough, this will make the window inert (i.e. the default
// steps will be taken for every event.) Good if you want to destroy it later.
void WmTakeOverWindow(Window* pWindow)
{
	cli;
	
	pWindow->m_bWindowManagerUpdated = true;
	pWindow->m_callback              = DefaultWindowProc;
	pWindow->m_screenLock.m_held     = false;
	
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		// kinda hacky, but it's to help support custom controls.
		Control* pCtl = &pWindow->m_pControlArray[i];
		
		uint32_t addr = (uint32_t)pCtl->OnEvent;
		
		// if this isn't part of the kernel...
		if (addr < KERNEL_BASE_ADDRESS)
			pCtl->OnEvent = WidgetNone_OnEvent;
	}
	
	sti;
}

void WmTimerTick(Window* pWindow)
{
	WindowTimer timers   [C_MAX_WIN_TIMER];
	int         tickTimes[C_MAX_WIN_TIMER] = { 0 };
	
	// get the current tick count
	int tickCount = GetTickCount();
	
	// take a snapshot
	cli;
	
	for (int i = 0; i < C_MAX_WIN_TIMER; i++)
	{
		if (pWindow->m_timers[i].m_used      == 0) continue;
		if (pWindow->m_timers[i].m_frequency == 0) continue; // timer is disarmed
		
		if (!pWindow->m_timers[i].m_nextTickAt)
		{
			pWindow->m_timers[i].m_nextTickAt = tickCount;
		}
		
		while (pWindow->m_timers[i].m_nextTickAt <= tickCount)
		{
			pWindow->m_timers[i].m_nextTickAt += pWindow->m_timers[i].m_frequency;
			tickTimes[i]++;
		}
		
		timers[i] = pWindow->m_timers[i];
	}
	sti;
	
	// look through each of the timers
	
	for (int i = 0; i < C_MAX_WIN_TIMER; i++)
	{
		while (tickTimes[i] > 0)
		{
			tickTimes[i]--;
			WindowAddEventToMasterQueue(pWindow, timers[i].m_firedEvent, i, 0);
		}
	}
}

static const int g_ResizeCursorTable[] =
{
	CURSOR_DEFAULT, CURSOR_SIZE_NS, CURSOR_SIZE_WE, CURSOR_SIZE_NESW, CURSOR_SIZE_NWSE, CURSOR_SIZE_ALL
};

static const int g_ResizeCursorIndices[] =
{
	// horizontal: none, left, right, left+right
	// vertical:   none, up, down, up+down
	
	0, 2, 2, 0,
	1, 4, 3, 0,
	1, 3, 4, 0,
	0, 0, 0, 0,
};

Cursor* GetWindowCursor(Window* pWindow, int x, int y)
{
	int width  = GetWidth (&pWindow->m_fullRect);
	int height = GetHeight(&pWindow->m_fullRect);
	
	int cursorID = pWindow->m_cursorID;
	
	if ((pWindow->m_flags & (WF_NOBORDER | WF_ALWRESIZ | WF_MAXIMIZE)) == WF_ALWRESIZ)
	{
		int borderSize = (pWindow->m_flags & WF_FLATBORD) ? (1) : (BORDER_SIZE);
		
		int left, up, down, right;
		
		left  = x < borderSize;
		right = x >= width - borderSize;
		up    = y < borderSize;
		down  = y >= height - borderSize;
		
		int index = left | right << 1 | up << 2 | down << 3;
		
		int result = g_ResizeCursorTable[g_ResizeCursorIndices[index]];
		
		if (result != CURSOR_DEFAULT)
			cursorID = result;
	}
	
	return GetCursorBasedOnID(cursorID, pWindow);
}

void WindowCheckButtons(Window* pWindow, int eventType, int x, int y);

int g_oldMouseX = -1, g_oldMouseY = -1;
void WindowManagerTask(__attribute__((unused)) int useless_argument)
{
	if (g_windowManagerRunning)
	{
		LogMsg("Cannot start up window manager again.");
		return;
	}
	
	if (!IsMouseAvailable())
	{
		LogMsg("No PS/2 mouse has been found. At this stage of development, the window system is not interactable without a mouse. Please restart the system with a PS/2 mouse by typing 'rb' in the prompt.");
		return;
	}
	
	SetupWindowManager();
	
	g_pWindowMgrTask = KeGetRunningTask ();
	
	int timeout = 10;
	#define UPDATE_TIMEOUT 50
	int UpdateTimeout = UPDATE_TIMEOUT;
	
	while (true)
	{
		int tick_count_start = GetTickCount ();
		
		bool handled = false;
		UpdateFPSCounter();
		CrashReporterCheck();
		bool updated = false;
		
		KeUnsuspendTasksWaitingForWM();
		
		LockAcquire(ActionQueueGetSafeLock());
		
		while (!ActionQueueEmpty())
		{
			WindowAction *pFront = ActionQueueGetFront();
			
			//SLogMsg("Executing action %d on window %x", pFront->nActionType, pFront->pWindow);
			
			switch (pFront->nActionType)
			{
				case WACT_UPDATEALL:
					//WmOnChangedBorderSize();
					break;
				case WACT_DESTROY:
					NukeWindow(pFront->pWindow);
					break;
				case WACT_HIDE:
					HideWindow(pFront->pWindow);
					break;
				case WACT_SHOW:
					ShowWindow(pFront->pWindow);
					break;
				case WACT_SELECT:
					SelectWindow(pFront->pWindow);
					break;
				case WACT_UNDRAW_RECT:
					RefreshRectangle(pFront->rect, pFront->pWindow);
					break;
			}
			
			pFront->bInProgress = false;
			ActionQueuePop();
		}
		
		LockFree(ActionQueueGetSafeLock());
		
		for (int p = 0; p < WINDOWS_MAX; p++)
		{
			Window* pWindow = &g_windows [p];
			if (!pWindow->m_used) continue;
			
			if (UpdateTimeout == 0 || updated)
			{
				UpdateTimeout = UPDATE_TIMEOUT;
				updated = true;
			}
			
			WmTimerTick(pWindow);
			
			if (pWindow->m_isSelected || (pWindow->m_flags & WF_SYSPOPUP))
			{
				//Also send an EVENT_MOVECURSOR
				int posX = g_mouseX - pWindow->m_fullRect.left;
				int posY = g_mouseY - pWindow->m_fullRect.top;
				if (g_oldMouseX - pWindow->m_fullRect.left != posX || g_oldMouseY - pWindow->m_fullRect.top != posY)
				{
					if (posX < 0) posX = 0;
					if (posY < 0) posY = 0;
					if (posX >= (int)pWindow->m_fullVbeData.m_width)  posX = (int)pWindow->m_fullVbeData.m_width  - 1;
					if (posY >= (int)pWindow->m_fullVbeData.m_height) posY = (int)pWindow->m_fullVbeData.m_height - 1;
					
					Rectangle margins = GetWindowMargins(pWindow);
					int offsX = -margins.left, offsY = -margins.top;
					if (g_GlowOnHover)
					{
						WindowCheckButtons(pWindow, EVENT_MOVECURSOR, posX, posY);
						WindowAddEventToMasterQueue(pWindow, EVENT_MOVECURSOR, MAKE_MOUSE_PARM(offsX + posX, offsY + posY), 0);
					}
					else if (posX >= 0 && posY >= 0 && posX < (int)pWindow->m_fullVbeData.m_width && posY < (int)pWindow->m_fullVbeData.m_height)
					{
						WindowAddEventToMasterQueue(pWindow, EVENT_MOVECURSOR, MAKE_MOUSE_PARM(offsX + posX, offsY + posY), 0);
					}
				}
			}
			
			if (pWindow->m_bWindowManagerUpdated)
			{
				HandleMessages(pWindow);
			}
			
		#if !THREADING_ENABLED
			if (pWindow == g_pShutdownMessage)
				if (!HandleMessages (pWindow))
				{
					KeStopSystem();
					continue;
				}
		#endif
			if (!pWindow->m_hidden)
			{
				if (pWindow->m_renderFinished && !g_BackgdLock.m_held)
				{
					pWindow->m_renderFinished = false;
					
					//ACQUIRE_LOCK(g_backgdLock);
					RenderWindow(pWindow);
					
					//FREE_LOCK(g_backgdLock);
					
					Point p = { g_mouseX, g_mouseY };
					if (RectangleContains(&pWindow->m_fullRect, &p))
						RenderCursor();
					
					if (RectangleOverlap(&pWindow->m_fullRect, &g_tooltip.m_rect))
						TooltipDraw();
				}
			}
			
			if (pWindow->m_markedForDeletion)
			{
				//turn it off, because DestroyWindow sends an event here, 
				//and we don't want it to stack overflow. Stack overflows
				//go pretty ugly in this OS, so we need to be careful.
				pWindow->m_markedForDeletion = false;
				DestroyWindow (pWindow);
			}
		}
		
		g_oldMouseX = g_mouseX;
		g_oldMouseY = g_mouseY;
		
		UpdateTimeout--;
		
		RunOneEffectFrame ();
		
		Cursor* pDefaultCursor = GetCursorBasedOnID(g_DefaultCursorID, NULL);
		
		// Get the window we're over:
		Window* pWindowOver = ShootRayAndGetWindow(g_mouseX, g_mouseY);
		
		if (pWindowOver)
		{
			Cursor* pCur = GetWindowCursor(pWindowOver, g_mouseX - pWindowOver->m_fullRect.left, g_mouseY - pWindowOver->m_fullRect.top);
			if (g_currentCursor != &g_windowDragCursor && g_currentCursor != pCur)
			{
				SetCursor(pCur);
			}
		}
		else if (g_currentCursor != &g_windowDragCursor && g_currentCursor != pDefaultCursor)
		{
			SetCursor(pDefaultCursor);
		}
		
		if (!handled)
		{
			unsigned char key = KbGetKeyFromRawBuffer();
			HandleKeypressOnWindow(key);
		}
		UpdateAltTabWindow();
		
		LockAcquire (&g_ClickQueueLock);
		
		if (RefreshMouse())
			TooltipDismiss();
		
		for (int i = 0; i < g_clickQueueSize; i++)
		{
			switch (g_clickQueue[i].clickType)
			{
				case CLICK_LEFT:   OnUILeftClick        (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_LEFTD:  OnUILeftClickDrag    (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_LEFTR:  OnUILeftClickRelease (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_RIGHT:  OnUIRightClick       (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_RIGHTR: OnUIRightClickRelease(g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
			}
		}
		g_clickQueueSize = 0;
		LockFree (&g_ClickQueueLock);
		
		timeout--;
		
		if (g_shutdownRequest && !g_shutdownProcessing)
		{
			int errorCode = 0;
			Task *pTask = KeStartTask(ShutdownProcessing, 0, &errorCode);
			KeUnsuspendTask(pTask);
			KeDetachTask(pTask);
			g_shutdownProcessing = true;
			if (pTask == NULL)
			{
				ILogMsg("Cannot spawn shutdown processing task!  That's... weird.");
				g_shutdownRequest = false;
			}
		}
		if (!g_shutdownProcessing)
		{
			if (g_shutdownDoneAll)
			{
				g_shutdownDoneAll    = false;
				WindowManagerOnShutdown();
			}
		}
		
		int tick_count_end = GetTickCount();
		
		//how many ms did this take? add 1ms just to be safe
		int ms_dur = tick_count_end - tick_count_start + 1;
		
		//how many ms are left of a 60 hz refresh?
		int ms_left = LOCK_MS - ms_dur;
		
	#ifdef LOCK_FPS
		if (ms_left >= 0)
			WaitMS (ms_left);
		
		//if ms_left < 0, means we're lagging behind
	#ifdef LAG_DEBUG
		else
			SLogMsg("Lagging behind! This cycle of the window manager took %d ms", ms_dur);
	#endif
	#endif
	}
	
	WmFreeRectangleStack();
	g_debugConsole.pushOrWrap = 0;
	VidSetFont (FONT_TAMSYN_REGULAR);
}
