/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

   Window Manager Message Handle Module
******************************************/
#include "wi.h"

static bool IsEventPrivate(int eventType)
{
	return eventType >= EVENT_PRIVATE_START && eventType < EVENT_USER;
}

void OnWindowHung(Window *pWindow)
{
	if (!pWindow->m_used)
		return;
	
	//don't hang twice
	if (pWindow->m_privFlags & WPF_HUNGWIND)
		return;
	
	SLogMsg("Window with address %x (title: %s) is being marked as hung...", pWindow, pWindow->m_title);
	
	pWindow->m_privFlags |= WPF_HUNGWIND;
	if (!(pWindow->m_flags & WF_FROZEN))
		pWindow->m_privFlags |= WPF_FROZENRM;
	
	// draw the window title bar and say that it's not responding
	if (!(pWindow->m_flags & WF_NOBORDER))
	{
		WmPaintWindowTitle(pWindow);
		pWindow->m_renderFinished = true;
	}
	
	pWindow->m_cursorID_backup = pWindow->m_cursorID;
	pWindow->m_cursorID = CURSOR_WAIT;
}

//This is what you should use in most cases.
void WindowRegisterEvent (Window* pWindow, short eventType, int parm1, int parm2)
{
	WindowAddEventToMasterQueue(pWindow, eventType, parm1, parm2);
}

void RequestRepaint (Window* pWindow)
{
	WindowAddEventToMasterQueue(pWindow, EVENT_PAINT, 0, 0);
}

void RequestRepaintNew (Window* pWindow)
{
	WmPaintWindowBorder(pWindow);
	WmPaintWindowTitle(pWindow);
	CallWindowCallbackAndControls  (pWindow, EVENT_PAINT, 0, 0);
}

bool IsEventDestinedForControlsToo(int type)
{
	switch (type)
	{
		case EVENT_CREATE:
		case EVENT_MOVE:
		case EVENT_ACTIVATE:
		case EVENT_UPDATE:
		case EVENT_CLOSE:
		case EVENT_MINIMIZE:
		case EVENT_UNMINIMIZE:
		case EVENT_UPDATE2:
		case EVENT_CLICK_CHAR:
		case EVENT_MAXIMIZE:
		case EVENT_UNMAXIMIZE:
		case EVENT_CHECKBOX:
		case EVENT_COMBOSELCHANGED:
		case EVENT_MAX:
			return false;
	}
	return true;
}

bool IsEventDestinedForInvisibleCtlsToo(int type)
{
	switch (type)
	{
		case EVENT_DESTROY:
		case EVENT_MOVE:
		case EVENT_SIZE:
		case EVENT_CTLUPDATEVISIBLE:
			return true;
	}
	return false;
}

void CallControlCallback(Window* pWindow, int comboID, int eventType, int parm1, int parm2)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control* p = &pWindow->m_pControlArray[i];
		if (!p->m_active || p->m_comboID != comboID) continue;
		
		if (p->m_bVisible || IsEventDestinedForInvisibleCtlsToo(eventType))
			p->OnEvent(p, eventType, parm1, parm2, pWindow);
		return;
	}
}

void ControlProcessEvent (Window* pWindow, int eventType, int parm1, int parm2)
{
	// Go backwards, because some controls might spawn other controls
	// They may want to be checked AFTER their children controls, so
	// we just go backwards.
	
	for (int i = pWindow->m_controlArrayLen - 1; i != -1; i--)
	{
		if (!pWindow->m_pControlArray[i].m_active) continue;
		
		Control* p = &pWindow->m_pControlArray[i];
		if (!p->OnEvent) continue;
		
		if (p->m_bVisible || IsEventDestinedForInvisibleCtlsToo(eventType))
		{
			if (p->OnEvent(p, eventType, parm1, parm2, pWindow))
				return;
		}
		
		if (eventType == EVENT_CREATE)
		{
			// Let the control adjust itself
			p->m_triedRect = p->m_rect;
		}
	}
}

//ugly hax to make calling window callback not need to preserve edi, esi, ebx
//this was not an issue with no optimization but is now
int __attribute__((noinline)) CallWindowCallback(Window* pWindow, int eq, int eqp1, int eqp2)
{
	pWindow->m_callback(pWindow, eq, eqp1, eqp2);
	return eq * eqp1 * eqp2;
}

int __attribute__((noinline)) CallWindowCallbackAndControls(Window* pWindow, int eq, int eqp1, int eqp2)
{
	if ((eq != EVENT_CLICKCURSOR && eq != EVENT_RELEASECURSOR) || eqp2 != 1)
	{
		VidSetVBEData (&pWindow->m_vbeData);
		pWindow->m_callback(pWindow, eq, eqp1, eqp2);
		VidSetVBEData (&pWindow->m_vbeData);
	}
	
	if (IsEventDestinedForControlsToo(eq))
	{
		VidSetVBEData (&pWindow->m_vbeData);
		ControlProcessEvent(pWindow, eq, eqp1, eqp2);
		VidSetVBEData (&pWindow->m_vbeData);
	}
	
	return eq * eqp1 * eqp2;
}

int someValue = 0;

void UpdateControlsBasedOnAnchoringModes(Window* pWindow, int oldSizeParm, int newSizeParm)
{
	//SLogMsg("TODO!");
	int oldSizeX = GET_X_PARM(oldSizeParm), oldSizeY = GET_Y_PARM(oldSizeParm);
	int newSizeX = GET_X_PARM(newSizeParm), newSizeY = GET_Y_PARM(newSizeParm);
	
	int sizeDifX = oldSizeX - newSizeX;
	int sizeDifY = oldSizeY - newSizeY;
	
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_active)
		{
			Control *pControl = &pWindow->m_pControlArray[i];
			
			if (pControl->m_anchorMode & ANCHOR_LEFT_TO_RIGHT)
				pControl->m_triedRect.left   += sizeDifX;
			if (pControl->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				pControl->m_triedRect.right  += sizeDifX;
			if (pControl->m_anchorMode & ANCHOR_TOP_TO_BOTTOM)
				pControl->m_triedRect.top    += sizeDifY;
			if (pControl->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				pControl->m_triedRect.bottom += sizeDifY;
			
			pControl->m_rect = pControl->m_triedRect;
			if (pControl->m_rect.right  - pControl->m_rect.left < 10)
				pControl->m_rect.right  = pControl->m_rect.left + 10;
			if (pControl->m_rect.bottom - pControl->m_rect.top  < 10)
				pControl->m_rect.bottom = pControl->m_rect.top  + 10;
		}
	}
}

static bool OnProcessOneEvent(Window* pWindow, int eventType, int parm1, int parm2);

static void PreProcessEvent(Window* pWindow, int eventType, UNUSED int parm1, UNUSED int parm2)
{
	switch (eventType)
	{
		case EVENT_MINIMIZE:
		{
			Rectangle old_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
			
			HideWindow (pWindow);
			if (~pWindow->m_flags & WF_MINIMIZE)
			{
				pWindow->m_flags      |= WF_MINIMIZE;
				pWindow->m_rectBackup  = pWindow->m_rect;
				
				pWindow->m_rect.left += (pWindow->m_rect.right  - pWindow->m_rect.left - 32) / 2;
				pWindow->m_rect.top  += (pWindow->m_rect.bottom - pWindow->m_rect.top  - 32) / 2;
				pWindow->m_rect.right  = pWindow->m_rect.left + 32;
				pWindow->m_rect.bottom = pWindow->m_rect.top  + 32;
			}
			
			Rectangle new_title_rect = pWindow->m_rect;
			
			//if a taskbar is running:
			new_title_rect = pWindow->m_taskbarRect;
			
			CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
			break;
		}
		case EVENT_UNMINIMIZE:
		{
			Rectangle old_title_rect = pWindow->m_rect;
			//if a taskbar is running:
			old_title_rect = pWindow->m_taskbarRect;
			
			HideWindow (pWindow);
			
			pWindow->m_flags &= ~WF_MINIMIZE;
			pWindow->m_rect = pWindow->m_rectBackup;
			
			ShowWindow (pWindow);
			
			WmPaintWindowBackgd(pWindow);
			WmPaintWindowBorder(pWindow);
			WmPaintWindowTitle(pWindow);
			
			OnProcessOneEvent(pWindow, EVENT_PAINT, 0, 0);
			
			pWindow->m_renderFinished = true;
			
			Rectangle new_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
			
			CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
			break;
		}
		case EVENT_SIZE:
		{
			DirtyRectInvalidateAll();
			
			WmPaintWindowBackgd(pWindow);
			WmPaintWindowBorder(pWindow);
			WmPaintWindowTitle(pWindow);
			
			// Update controls based on their anchoring modes.
			UpdateControlsBasedOnAnchoringModes (pWindow, parm1, parm2);
			
			break;
		}
		case EVENT_REPAINT_PRIVATE:
		{
			WmPaintWindowBackgd(pWindow);
			WmPaintWindowBorder(pWindow);
			WmPaintWindowTitle(pWindow);
			DirtyRectInvalidateAll();
			
			break;
		}
		case EVENT_MAXIMIZE:
		{
			Rectangle old_title_rect;
			
			if (!GetWindowTitleRect(pWindow, &old_title_rect))
			{
				Rectangle newRect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
				old_title_rect = newRect;
			}
			else
			{
				old_title_rect.left   += pWindow->m_fullRect.left;
				old_title_rect.top    += pWindow->m_fullRect.top;
				old_title_rect.right  += pWindow->m_fullRect.left;
				old_title_rect.bottom += pWindow->m_fullRect.top;
			}
			
			if (~pWindow->m_flags & WF_MAXIMIZE)
				pWindow->m_rectBackup = pWindow->m_fullRect;
			
			pWindow->m_flags |= WF_MAXIMIZE;
			
			if (!(pWindow->m_flags & WF_FLATBORD))
			{
				pWindow->m_flags |= WF_FLATBORD;
				pWindow->m_privFlags |= WPF_FLBRDFRC;
			}
			
			ResizeWindow(
				pWindow,
				g_TaskbarMargins.left,
				g_TaskbarMargins.top,
				GetScreenWidth() - g_TaskbarMargins.left - g_TaskbarMargins.right,
				GetScreenHeight() - g_TaskbarMargins.top - g_TaskbarMargins.bottom
			);
			
			pWindow->m_renderFinished = true;
			
			Rectangle new_title_rect = {
				g_TaskbarMargins.left,
				g_TaskbarMargins.top,
				GetScreenWidth() - g_TaskbarMargins.right - 1,
				g_TaskbarMargins.top + TITLE_BAR_HEIGHT - 1
			};
			
			CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
			break;
		}
		case EVENT_UNMAXIMIZE:
		{
			Rectangle old_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
			
			Rectangle new_title_rect = { pWindow->m_rectBackup.left + 3, pWindow->m_rectBackup.top + 3, pWindow->m_rectBackup.right - 3, pWindow->m_rectBackup.top + 3 + TITLE_BAR_HEIGHT };
			
			if (pWindow->m_flags & WF_MAXIMIZE)
			{
				ResizeWindow(pWindow, pWindow->m_rectBackup.left, pWindow->m_rectBackup.top, pWindow->m_rectBackup.right - pWindow->m_rectBackup.left, pWindow->m_rectBackup.bottom - pWindow->m_rectBackup.top);
			}
			
			pWindow->m_flags &= ~WF_MAXIMIZE;
			
			if (pWindow->m_privFlags & WPF_FLBRDFRC)
			{
				pWindow->m_flags &= ~WF_FLATBORD;
				pWindow->m_privFlags &= ~WPF_FLBRDFRC;
			}
			
			pWindow->m_renderFinished = true;
			
			CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
			break;
		}
		case EVENT_SMARTSNAP:
		{
			// if the window is maximized, unmaximize it and deflect
			if (pWindow->m_flags & WF_MAXIMIZE)
			{
				WindowAddEventToMasterQueue(pWindow, EVENT_UNMAXIMIZE, 0, 0);
				WindowAddEventToMasterQueue(pWindow, EVENT_SMARTSNAP,  parm1, 0);
			}
			else if (pWindow->m_flags & WF_MINIMIZE)
			{
				WindowAddEventToMasterQueue(pWindow, EVENT_UNMINIMIZE, 0, 0);
				WindowAddEventToMasterQueue(pWindow, EVENT_SMARTSNAP,  parm1, 0);
			}
			else
			{
				int x = -1, y, w, h;
				
				int screenW = GetScreenWidth()  - g_TaskbarMargins.left - g_TaskbarMargins.right;
				int screenH = GetScreenHeight() - g_TaskbarMargins.top - g_TaskbarMargins.bottom;
				
				switch (parm1)
				{
					case 0: // UP
						x = g_TaskbarMargins.left;
						y = g_TaskbarMargins.top;
						w = screenW;
						h = screenH / 2;
						break;
					case 1: // DOWN
						x = g_TaskbarMargins.left;
						w = screenW;
						y = h = screenH / 2;
						break;
					case 2: // LEFT
						x = g_TaskbarMargins.left;
						y = g_TaskbarMargins.top;
						w = screenW / 2;
						h = screenH;
						break;
					case 3: // RIGHT
						y = g_TaskbarMargins.top;
						x = w = screenW / 2;
						h = screenH;
						break;
					case 4: // UP LEFT
						x = g_TaskbarMargins.left;
						y = g_TaskbarMargins.top;
						w = screenW / 2;
						h = screenH / 2;
						break;
					case 5: // UP RIGHT
						y = g_TaskbarMargins.top;
						x = w = screenW / 2;
						h = screenH / 2;
					case 6: // DOWN LEFT
						x = g_TaskbarMargins.left;
						w = screenW / 2;
						y = h = screenH / 2;
						break;
					case 7: // DOWN RIGHT
						x = w = screenW / 2;
						y = h = screenH / 2;
						break;
				}
				
				if (x != -1)
				{
					ResizeWindow(pWindow, x, y, w, h);
				}
			}
			break;
		}
		case EVENT_CREATE:
		{
			VidSetVBEData (&pWindow->m_vbeData);
			
			WmPaintWindowBackgd(pWindow);
			WmPaintWindowBorder(pWindow);
			WmPaintWindowTitle(pWindow);
			
			DefaultWindowProc (pWindow, EVENT_CREATE, 0, 0);
			break;
		}
	}
}

static bool OnProcessOneEvent(Window* pWindow, int eventType, int parm1, int parm2)
{
	//SLogMsg("Window \"%s\": Event %d", pWindow->m_title, eventType);
	
	pWindow->m_lastHandledMessagesWhen = GetTickCount();
	//setup paint stuff so the window can only paint in their little box
	
	VBEData* pBackup = VidSetVBEData (&pWindow->m_vbeData);
	
	VidSetFont(SYSTEM_FONT);
	pWindow->m_fullVbeData.m_dirty = 0;
	//pWindow->m_renderFinished = false;
	
	if (pWindow->m_privFlags & WPF_HUNGWIND)
	{
		//SLogMsg("Window %s no longer frozen!", pWindow->m_title);
		// Window no longer frozen
		pWindow->m_privFlags &= ~WPF_HUNGWIND;
		
		if (pWindow->m_privFlags & WPF_FROZENRM)
			pWindow->m_flags &= ~WF_FROZEN;
		
		// Un-hang the window
		WmPaintWindowTitle(pWindow);
		
		pWindow->m_cursorID = pWindow->m_cursorID_backup;
	}
	
	// Perform operations before calling the window's event handler function
	PreProcessEvent(pWindow, eventType, parm1, parm2);
	
	// Perform the actual call to the user application here.
	VidSetVBEData (&pWindow->m_vbeData);
	
	if (IsEventPrivate(eventType))
	{
		DefaultWindowProc(pWindow, eventType, parm1, parm2);
	}
	else
	{
		someValue = CallWindowCallbackAndControls(pWindow, eventType, parm1, parm2);
	}
	
	// Perform operations after calling into the user application.
	
	if (~pWindow->m_flags & WF_MINIMIZE)
	{
		if (pWindow->m_vbeData.m_dirty)
		{
			pWindow->m_renderFinished = true;
		}
	}
	else
	{
		pWindow->m_renderFinished = true;
	}
	
	//if the contents of this window have been modified, redraw them:
	//if (pWindow->m_vbeData.m_dirty && !pWindow->m_hidden)
	//	RenderWindow(pWindow);
	
	if (eventType == EVENT_SIZE || eventType == EVENT_REPAINT_PRIVATE)
	{
		OnProcessOneEvent(pWindow, EVENT_PAINT, 0, 0);
		
		pWindow->m_renderFinished = true;
		
		if (!pWindow->m_hidden)
			ShowWindow (pWindow);
	}
	else if (eventType == EVENT_CREATE)
	{
		// If the window isn't minimized
		if (~pWindow->m_flags & WF_MINIMIZE)
		{
			ShowWindow(pWindow);
			
			if (~pWindow->m_flags & WF_NOIFOCUS)
				SelectWindow(pWindow);
		}
	}
	else if (eventType == EVENT_DESTROY)
	{
		KeTaskDone();
		
		NukeWindow(pWindow);
		
		// Reset to main screen data.
		VidSetVBEData (pBackup);
		
		return false;
	}
	else if (eventType == EVENT_SET_WINDOW_TITLE_PRIVATE)
	{
		// free whatever we have in parm1
		MmFree((void*)parm1);
	}
	
	// Reset to main screen data.
	VidSetVBEData (pBackup);
	
	return true;
}

void WmWaitForEvent(Window* pWindow);

bool HandleMessages(Window* pWindow)
{
	int tickCountStart = GetTickCount();
	
	if (!IsWindowManagerRunning())
		return false;
	
	if (!pWindow->m_used)
	{
		SLogMsg("Sir, this window is gone");
		return false;
	}
	
	pWindow->m_lastHandledMessagesWhen = GetTickCount();
	
	bool have_handled_events = false;
	
	// If the window manager was being asked to redraw the window, but it did not
	// Withdraw the statement, so you can draw another frame
	//bool bkp = pWindow->m_renderFinished;
	
	//pWindow->m_renderFinished = false;
	
	// While we have events in the master queue...
	int et = 0, p1 = 0, p2 = 0;
	while (WindowPopEventFromQueue(pWindow, &et, &p1, &p2))
	{
		have_handled_events = true;
		if (!OnProcessOneEvent(pWindow, et, p1, p2))
			return false;
	}
	
	// Keyboard events are handled separately, in games you may miss input otherwise...
	while (WinAnythingOnInputQueue(pWindow))
	{
		have_handled_events = true;
		
		unsigned char out = WinReadFromInputQueue(pWindow);
		
		OnProcessOneEvent(pWindow, EVENT_KEYRAW, out, 0);
		
		// if the key was just pressed:
		if ((out & 0x80) == 0)
		{
			// convert it to a standard char
			char sensible = KbMapAtCodeToChar (out & 0x7F);
			
			if (sensible)
				OnProcessOneEvent(pWindow, EVENT_KEYPRESS, sensible, 0);
		}
	}
	
	int tickCountEnd = GetTickCount();
	
	bool bIsNotWM = !IsWindowManagerTask();
	if (!have_handled_events)
	{
		// suspend until the window manager has updated itself.
		// if this IS the window manager handling events for us, we'd basically be waiting forever, so don't
		if (bIsNotWM)
		{
			if (pWindow->m_flags & WF_NOWAITWM)
			{
				WaitMS(1);// ayy
			}
			else if (ExGetRunningProc())
			{
				if (ExGetRunningProc()->nWindows > 1)
				{
					WaitUntilWMUpdate();
				}
				else
				{
					WaitObject(pWindow);
				}
			}
			else
			{
				// note: kernel tasks typically only have one window running..
				WaitObject(pWindow);
			}
		}
	}
	
	// if this is the window manager, let it handle everything else first
	if (!bIsNotWM)
		KeTaskDone(); // give it a good halt
	return true;
}

void ResizeWindowInternal(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight);
void DirtyRectLogger (int x, int y, int width, int height);

void DefaultWindowProc (Window* pWindow, int messageType, UNUSED int parm1, UNUSED int parm2)
{
	if (!IsWindowManagerRunning())
		return;
	
	switch (messageType)
	{
		case EVENT_BGREPAINT:
		{
			// By default, this draws the window's background color. This can be changed by overloading the event.
			Rectangle rect = { GET_X_PARM(parm1), GET_Y_PARM(parm1), GET_X_PARM(parm2), GET_Y_PARM(parm2) };
			VidFillRect(WINDOW_BACKGD_COLOR, rect.left, rect.top, rect.right - 1, rect.bottom - 1);
			break;
		}
		case EVENT_REQUEST_RESIZE_PRIVATE:
		{
			int x = GET_X_PARM(parm1);
			int y = GET_Y_PARM(parm1);
			int w = GET_X_PARM(parm2);
			int h = GET_Y_PARM(parm2);
			
			ResizeWindowInternal(pWindow, x, y, w, h);
			//ShowWindow(pWindow);
			
			break;
		}
		case EVENT_HIDE_WINDOW_PRIVATE:
		{
			HideWindow(pWindow);
			break;
		}
		case EVENT_SHOW_WINDOW_PRIVATE:
		{
			ShowWindow(pWindow);
			break;
		}
		case EVENT_BORDER_SIZE_UPDATE_PRIVATE:
		{
			WmOnChangedBorderParms(pWindow);
			break;
		}
		case EVENT_CREATE:
		{
			if (pWindow->m_privFlags & WPF_INITGOOD) break;
			
			pWindow->m_privFlags |= WPF_INITGOOD;
			
			//AddTimer(pWindow, g_TickSpeed, EVENT_TICK);
			
			break;
		}
		
		case EVENT_CTLREPAINT:
		{
			CallControlCallback(pWindow, parm1, EVENT_PAINT, 0, 0);
			break;
		}
		
		case EVENT_PAINT:
			//nope, user should handle this themselves
			//Actually EVENT_PAINT just requests a paint event,
			//so just mark this as dirty
			pWindow->m_vbeData.m_dirty = 1;
			break;
			
		case EVENT_SETFOCUS:
		case EVENT_KILLFOCUS:
		case EVENT_REPAINT_TITLE_PRIVATE:
		{
			WmPaintWindowTitle(pWindow);
			pWindow->m_vbeData.m_dirty = true;
			break;
		}
			
		case EVENT_CLOSE:
			DestroyWindow(pWindow);
			break;
			
		case EVENT_DESTROY:
			//NukeWindow(pWindow);//exits
			break;
			
		case EVENT_RIGHTCLICKRELEASE_PRIVATE:
		{
			Rectangle recta;
			if (!GetWindowTitleRect(pWindow, &recta)) break;
			Point mousePoint = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (!(pWindow->m_flags & WF_NOTITLE) && RectangleContains(&recta, &mousePoint))
			{
		case EVENT_SHOW_MENU_PRIVATE:
				OnRightClickShowMenu(pWindow, parm1);
			}
			
			break;
		}
		case EVENT_COMMAND_PRIVATE:
		{
			if (parm1 == WINDOW_ACTION_MENU_ORIG_CID  &&  !pWindow->m_bWindowManagerUpdated)
			{
				int eventType = EVENT_NULL, parm1 = 0;
				switch (parm2)
				{
					// basic window actions.
					case CID_RESTORE:  eventType = EVENT_UNMAXIMIZE; break;
					case CID_MINIMIZE: eventType = EVENT_MINIMIZE;   break;
					case CID_MAXIMIZE: eventType = EVENT_MAXIMIZE;   break;
					case CID_CLOSE:    eventType = EVENT_CLOSE;      break;
					
					// SmartSnap stuff
					case CID_SMARTSNAP_0: case CID_SMARTSNAP_1: case CID_SMARTSNAP_2: case CID_SMARTSNAP_3:
					case CID_SMARTSNAP_4: case CID_SMARTSNAP_5: case CID_SMARTSNAP_6: case CID_SMARTSNAP_7:
						eventType = EVENT_SMARTSNAP, parm1 = parm2 - CID_SMARTSNAP_0;
						break;
					
					// default:
					default: SLogMsg("Unknown menu action %d", parm2);
				}
				
				if (eventType != EVENT_NULL)
					WindowAddEventToMasterQueue(pWindow, eventType, parm1, 0);
			}
			break;
		}
		case EVENT_SET_WINDOW_TITLE_PRIVATE:
		{
			char* chr = (char*)parm1;
			size_t sz = strlen(chr);
			if (sz < WINDOW_TITLE_MAX - 1)
				sz = WINDOW_TITLE_MAX - 1;
			memcpy(pWindow->m_title, chr, sz);
			pWindow->m_title[sz] = 0;
			
			//paint the window border:
			WmPaintWindowTitle(pWindow);
			break;
		}
		default:
			break;
	}
}
