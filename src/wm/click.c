/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Window Manager Click Module
******************************************/
#include "wi.h"

Window* g_currentlyClickedWindow = NULL;
int g_prevMouseX, g_prevMouseY;

void CloseAnyOpenMenusOutside(int posX, int posY)
{
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		Window* pWnd = &g_windows[i];
		
		if (!pWnd->m_used) continue;
		if (~pWnd->m_flags & WF_MENUITEM) continue;
		
		// this is a menu item.
		Point p = { posX, posY };
		
		if (!RectangleContains(&pWnd->m_rect, &p))
		{
			WindowAddEventToMasterQueue(pWnd, EVENT_KILLFOCUS, 0, 0);
		}
	}
}

// note: these are called from the window manager task

void OnUILeftClick (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	
	Window* window = ShootRayAndGetWindow(mouseX, mouseY);
	
	if (window)
	{
		if (!(window->m_flags & WF_FROZEN))
		{
			SelectWindow (window);
			
			g_currentlyClickedWindow = window;
			
			//are we in the title bar region? TODO
			Rectangle recta = window->m_rect;
			if (!window->m_minimized)
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
				recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.left++; recta.right--; recta.top++; recta.bottom = recta.top + TITLE_BAR_HEIGHT;
			}
			else
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
			}
			
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			Point mousePoint = {x, y};
			
			bool t = RectangleContains(&recta, &mousePoint);
			if (window->m_flags & WF_NOTITLE)
				t = false;
			
			if (!window->m_maximized && (t || window->m_minimized))
			{
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 1);
			}
			else if (!window->m_minimized)
			{
				int x = mouseX - window->m_rect.left;
				int y = mouseY - window->m_rect.top;
				
				window->m_clickedInside = true;
				
				//WindowRegisterEvent (window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
	else
	{
		g_currentlyClickedWindow = NULL;
	}
	
	CloseAnyOpenMenusOutside(mouseX, mouseY);
	
	//FREE_LOCK(g_windowLock);
}

void OnUILeftClickDrag (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	if (!g_currentlyClickedWindow) return;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	Window* window = g_currentlyClickedWindow;
	
	// if we're not frozen AND we have a title to drag on
	if (window->m_minimized || !(window->m_flags & (WF_FROZEN | WF_NOTITLE)))
	{
		if (!window->m_isBeingDragged)
		{
			//are we in the title bar region? TODO
			Rectangle recta = window->m_rect;
			if (!window->m_minimized)
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
				recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.left++; recta.right--; recta.top++; recta.bottom = recta.top + TITLE_BAR_HEIGHT;
			}
			else
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
			}
			
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			Point mousePoint = {x, y};
			
			if (!window->m_maximized && (RectangleContains(&recta, &mousePoint) || window->m_minimized))
			{
				window->m_isBeingDragged = true;
				
				if (g_RenderWindowContents)
				{
					HideWindow(window);
				}
				
				//change cursor:
				if (window->m_minimized)
				{
					Image* p = GetIconImage(window->m_iconID, 32);
					g_windowDragCursor.width    = p->width;
					g_windowDragCursor.height   = p->height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = p->framebuffer;
					g_windowDragCursor.m_transparency = true;
					g_windowDragCursor.m_resizeMode   = false;
					g_windowDragCursor.boundsWidth  = p->width;
					g_windowDragCursor.boundsHeight = p->height;
					g_windowDragCursor.mouseLockX = -1;
					g_windowDragCursor.mouseLockY = -1;
				}
				else if (g_heldAlt && (window->m_flags & WF_ALWRESIZ))
				{
					g_windowDragCursor.width    = window->m_vbeData.m_width;
					g_windowDragCursor.height   = window->m_vbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = window->m_vbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
					g_windowDragCursor.m_resizeMode   = true;
					g_windowDragCursor.boundsWidth  = window->m_vbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_vbeData.m_height;
					g_windowDragCursor.mouseLockX = mouseX;
					g_windowDragCursor.mouseLockY = mouseY;
				}
				else
				{
					g_windowDragCursor.width    = window->m_vbeData.m_width;
					g_windowDragCursor.height   = window->m_vbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = window->m_vbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
					g_windowDragCursor.m_resizeMode   = false;
					g_windowDragCursor.boundsWidth  = window->m_vbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_vbeData.m_height;
					g_windowDragCursor.mouseLockX = -1;
					g_windowDragCursor.mouseLockY = -1;
				}
				
				SetCursor (&g_windowDragCursor);
			}
			if (!window->m_minimized && !window->m_isBeingDragged)
			{
				window->m_clickedInside = true;
				
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
	
	//FREE_LOCK(g_windowLock);
}

void RenderWindow (Window* pWindow);

void OnUILeftClickRelease (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	if (!g_currentlyClickedWindow) return;
	
	mouseX = g_mouseX;
	mouseY = g_mouseY;
	
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	Window* pWindow = g_currentlyClickedWindow;
	if (pWindow->m_isBeingDragged)
	{
		if (!g_RenderWindowContents)
		{
			HideWindow(pWindow);
		}
		
		if (GetCurrentCursor()->m_resizeMode)
		{
			int newWidth = GetCurrentCursor()->boundsWidth, newHeight = GetCurrentCursor()->boundsHeight;
			
			//note that we resize the window this way here because we're running inside the wm task
			ResizeWindowInternal (pWindow, -1, -1, newWidth, newHeight);
		}
		else
		{
			Rectangle newWndRect;
			newWndRect.left   = mouseX - g_windowDragCursor.leftOffs;
			newWndRect.top    = mouseY - g_windowDragCursor.topOffs;
			if (newWndRect.top < 0)
				newWndRect.top = 0;
			newWndRect.right  = newWndRect.left + GetWidth (&pWindow->m_rect);
			newWndRect.bottom = newWndRect.top  + GetHeight(&pWindow->m_rect);
			pWindow->m_rect = newWndRect;
		}
		
		//WindowRegisterEvent(window, EVENT_PAINT, 0, 0);
		pWindow->m_vbeData.m_dirty = true;
		pWindow->m_renderFinished = false;
		pWindow->m_isBeingDragged = false;
		ShowWindow(pWindow);
		
		if (GetCurrentCursor() == &g_windowDragCursor)
		{
			SetCursorInternal(NULL, false);
		}
	}
	
	if (pWindow->m_minimized) return;
	
	int x = mouseX - pWindow->m_rect.left;
	int y = mouseY - pWindow->m_rect.top;
	
	if (pWindow->m_clickedInside)
	{
		pWindow->m_clickedInside = false;
		WindowAddEventToMasterQueue(pWindow, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (x, y), 0);
	}
	else
	{
		WindowAddEventToMasterQueue(pWindow, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (x, y), 1);
	}
	//FREE_LOCK(g_windowLock);
}

void OnUIRightClick (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	Window* window = ShootRayAndGetWindow(mouseX, mouseY);
	
	if (window)
	{
		if (!window->m_minimized)
		{
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICK, MAKE_MOUSE_PARM (x, y), 0);
		}
	}
	
	CloseAnyOpenMenusOutside(mouseX, mouseY);
}

void OnUIRightClickRelease (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	Window* window = ShootRayAndGetWindow(mouseX, mouseY);
	
	if (window)
	{
		if (window->m_minimized)
		{
			WindowRegisterEvent (window, EVENT_UNMINIMIZE, 0, 0);
		}
		else
		{
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			
			WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICKRELEASE, MAKE_MOUSE_PARM (x, y), 0);
		}
	}
}
