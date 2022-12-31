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

int g_FPS, g_FPSThisSecond, g_FPSLastCounted;
int g_TaskbarHeight = 0;

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
	
	memset (&g_windows, 0, sizeof (g_windows));
	
	g_debugConsole.curY = g_debugConsole.height / 2;
	g_clickQueueSize = 0;
	// load background?
	InitWindowDepthBuffer();
	//CoClearScreen (&g_debugConsole);
	g_debugConsole.curX = g_debugConsole.curY = 0;
	g_debugConsole.pushOrWrap = 1;
	
	g_windowManagerRunning = true;
	
	g_pShutdownMessage = NULL;
	
	g_shutdownSentDestroySignals = false;
	g_shutdownWaiting			 = false;
	
	UpdateDepthBuffer();
	
	LoadDefaultThemingParms ();
	//VidFillScreen(BACKGROUND_COLOR);
	SetDefaultBackground ();
	
	//redraw background?
	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	RedrawBackground (r);
	
	//CreateTestWindows();
	UpdateDepthBuffer();
	
	CfgGetIntValue(&g_WmLockMS, "Desktop::UpdateMS", 16);
	
	//VidSetFont(FONT_BASIC);
	//VidSetFont(FONT_TAMSYN_BOLD);
	//VidSetFont(FONT_TAMSYN_REGULAR);
	//VidSetFont(FONT_FAMISANS);
	VidSetFont(FONT_GLCD);
	//VidSetFont(FONT_BIGTEST);
	
	LogMsg("\n\n\n");
	
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
			if (g_windows[i].m_pOwnerThread == pTask)
				g_windows[i].m_pOwnerThread =  NULL;
			if (g_windows[i].m_pSubThread   == pTask)
				g_windows[i].m_pSubThread   =  NULL;
		}
	}
}

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
				case WACT_RESIZE:
					ResizeWindow(pFront->pWindow, pFront->rect.left, pFront->rect.top, GetWidth(&pFront->rect), GetHeight(&pFront->rect));
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
			
			if (pWindow->m_isSelected || (pWindow->m_flags & WF_SYSPOPUP))
			{
				//Also send an EVENT_MOVECURSOR
				int posX = g_mouseX - pWindow->m_rect.left;
				int posY = g_mouseY - pWindow->m_rect.top;
				if (g_oldMouseX - pWindow->m_rect.left != posX || g_oldMouseY - pWindow->m_rect.top != posY)
				{
					if (posX < 0) posX = 0;
					if (posY < 0) posY = 0;
					if (posX >= (int)pWindow->m_vbeData.m_width)  posX = (int)pWindow->m_vbeData.m_width  - 1;
					if (posY >= (int)pWindow->m_vbeData.m_height) posY = (int)pWindow->m_vbeData.m_height - 1;
					
					if (g_GlowOnHover)
					{
						WindowAddEventToMasterQueue(pWindow, EVENT_MOVECURSOR, MAKE_MOUSE_PARM(posX, posY), 0);
					}
					else if (posX >= 0 && posY >= 0 && posX < (int)pWindow->m_vbeData.m_width && posY < (int)pWindow->m_vbeData.m_height)
					{
						WindowAddEventToMasterQueue(pWindow, EVENT_MOVECURSOR, MAKE_MOUSE_PARM(posX, posY), 0);
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
					if (RectangleContains(&pWindow->m_rect, &p))
						RenderCursor();
					
					if (RectangleOverlap(&pWindow->m_rect, &g_tooltip.m_rect))
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
		
		// Get the window we're over:
		short windowOver = GetWindowIndexInDepthBuffer (g_mouseX, g_mouseY);
		
		if (windowOver >= 0)
		{
			Window* pWindow = &g_windows [windowOver];
			if (g_currentCursor != &g_windowDragCursor && g_currentCursor != GetCursorBasedOnID(pWindow->m_cursorID, pWindow))
				SetCursor(GetCursorBasedOnID(pWindow->m_cursorID, pWindow));
		}
		else if (g_currentCursor != &g_windowDragCursor && g_currentCursor != &g_defaultCursor)
			SetCursor(&g_defaultCursor);
		
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
	KillWindowDepthBuffer();
	g_debugConsole.pushOrWrap = 0;
	VidSetFont (FONT_TAMSYN_REGULAR);
}
