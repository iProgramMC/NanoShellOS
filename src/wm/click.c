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
		
		if (!RectangleContains(&pWnd->m_fullRect, &p))
		{
			WindowAddEventToMasterQueue(pWnd, EVENT_KILLFOCUS, 0, 0);
		}
	}
}

enum eTitleBarButton
{
	E_MINIMIZE,
	E_MAXIMIZE,
	E_CLOSE,
	E_ICON,
};

uint32_t s_HoveredMask[] = { WPF_MINIMIZEHOVERED, WPF_MAXIMIZEHOVERED, WPF_CLOSEBTNHOVERED, WPF_ICONBUTNHOVERED };
uint32_t s_ClickedMask[] = { WPF_MINIMIZECLICKED, WPF_MAXIMIZECLICKED, WPF_CLOSEBTNCLICKED, WPF_ICONBUTNCLICKED };
uint32_t s_ClickEvent [] = { EVENT_MINIMIZE,      EVENT_MAXIMIZE,      EVENT_CLOSE,         EVENT_SHOW_MENU_PRIVATE };

void WindowCheckButton(Window* pWindow, int eventType, int x, int y, int btnIndex, Rectangle rect)
{
	uint32_t hoveredMask = s_HoveredMask[btnIndex], clickedMask = s_ClickedMask[btnIndex];
	uint32_t oldFlags = pWindow->m_privFlags;
	
	Point pt = { x, y };
	rect.right  -= 2;
	rect.bottom -= 2;
	bool rectContains = RectangleContains(&rect, &pt);
	rect.right  += 2;
	rect.bottom += 2;
	if (!rectContains)
	{
		pWindow->m_privFlags &= ~(hoveredMask | clickedMask);
	}
	else if (eventType == EVENT_MOVECURSOR)
	{
		pWindow->m_privFlags |= hoveredMask;
	}
	else if (eventType == EVENT_RELEASECURSOR)
	{
		pWindow->m_privFlags &= ~(hoveredMask | clickedMask);
		
		int evt = s_ClickEvent[btnIndex], parm1 = 0;
		if (evt == EVENT_MAXIMIZE && pWindow->m_maximized)
			evt =  EVENT_UNMAXIMIZE;
		if (evt == EVENT_SHOW_MENU_PRIVATE)
			parm1 = MAKE_MOUSE_PARM(rect.left, rect.bottom);
		
		WindowAddEventToMasterQueue(pWindow, evt, parm1, 0);
	}
	else if (eventType == EVENT_CLICKCURSOR)
	{
		pWindow->m_privFlags |=  clickedMask;
	}
	
	if (pWindow->m_privFlags != oldFlags)
	{
		WindowAddEventToMasterQueue(pWindow, EVENT_REPAINT_BORDER_PRIVATE, 0, 0);
	}
}

void WindowTitleLayout(
	Rectangle windowRect,
	uint32_t flags,
	uint32_t iconID,
	uint32_t maximized,
	bool      *bTitleHas3dShape,
	Rectangle *pTitleRect,
	Rectangle *pTitleGradientRect,
	Rectangle *pMinimButtonRect,
	Rectangle *pMaximButtonRect,
	Rectangle *pCloseButtonRect,
	Rectangle *pIconButtonRect
);

void WindowCheckButtons(Window* pWindow, int eventType, int x, int y)
{
	uint32_t flags = pWindow->m_flags;
	if (flags & WF_NOTITLE) return;
	
	Rectangle rectb = pWindow->m_fullRect;
	rectb.right  -= rectb.left;
	rectb.bottom -= rectb.top;
	rectb.top     = rectb.left = 0;
	
	Rectangle titleRect, titleGradRect, minimBtnRect, maximBtnRect, closeBtnRect, iconBtnRect;
	bool bTitleHas3dShape;
	WindowTitleLayout(rectb, flags, pWindow->m_iconID, pWindow->m_maximized, &bTitleHas3dShape, &titleRect, &titleGradRect, &minimBtnRect, &maximBtnRect, &closeBtnRect, &iconBtnRect);
	
	if (pWindow->m_iconID != ICON_NULL)
	{
		WindowCheckButton(pWindow, eventType, x, y, E_ICON, iconBtnRect);
	}
	
	if (~flags & WF_NOMINIMZ)
		WindowCheckButton(pWindow, eventType, x, y, E_MINIMIZE, minimBtnRect);
	
	if (!(flags & WF_NOMAXIMZ) && (flags & WF_ALWRESIZ))
		WindowCheckButton(pWindow, eventType, x, y, E_MAXIMIZE, maximBtnRect);
	
	if (~flags & WF_NOCLOSE)
		WindowCheckButton(pWindow, eventType, x, y, E_CLOSE, closeBtnRect);
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
			
			//are we in the title bar region?
			Rectangle recta;
			bool hasTitle = GetWindowTitleRect(window, &recta);
			
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
			Point mousePoint = {x, y};
			
			bool t = hasTitle ? RectangleContains(&recta, &mousePoint) : false;
			
			Rectangle margins = GetWindowMargins(window);
			int offsX = -margins.left, offsY = -margins.top;
			
			if (!window->m_maximized && (t || window->m_minimized))
			{
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 1);
				WindowCheckButtons(window, EVENT_CLICKCURSOR, x, y);
			}
			else if (!window->m_minimized)
			{
				int x = mouseX - window->m_fullRect.left;
				int y = mouseY - window->m_fullRect.top;
				
				window->m_clickedInside = true;
				
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 0);
				WindowCheckButtons(window, EVENT_CLICKCURSOR, x, y);
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
			//are we in the title bar region?
			Rectangle recta;
			UNUSED bool hasTitle = GetWindowTitleRect(window, &recta);
			
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
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
					g_windowDragCursor.leftOffs = mouseX - window->m_fullRect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_fullRect.top;
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
					g_windowDragCursor.width    = window->m_fullVbeData.m_width;
					g_windowDragCursor.height   = window->m_fullVbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_fullRect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_fullRect.top;
					g_windowDragCursor.bitmap   = window->m_fullVbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
					g_windowDragCursor.m_resizeMode   = true;
					g_windowDragCursor.boundsWidth  = window->m_fullVbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_fullVbeData.m_height;
					g_windowDragCursor.mouseLockX = mouseX;
					g_windowDragCursor.mouseLockY = mouseY;
				}
				else
				{
					g_windowDragCursor.width    = window->m_fullVbeData.m_width;
					g_windowDragCursor.height   = window->m_fullVbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_fullRect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_fullRect.top;
					g_windowDragCursor.bitmap   = window->m_fullVbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
					g_windowDragCursor.m_resizeMode   = false;
					g_windowDragCursor.boundsWidth  = window->m_fullVbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_fullVbeData.m_height;
					g_windowDragCursor.mouseLockX = -1;
					g_windowDragCursor.mouseLockY = -1;
				}
				
				SetCursor (&g_windowDragCursor);
			}
			if (!window->m_minimized && !window->m_isBeingDragged)
			{
				window->m_clickedInside = true;
				WindowCheckButtons(window, EVENT_CLICKCURSOR, x, y);
				
				Rectangle margins = GetWindowMargins(window);
				int offsX = -margins.left, offsY = -margins.top;
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 0);
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
			//ResizeWindowInternal (pWindow, -1, -1, newWidth, newHeight);
			ResizeWindow(pWindow, -1, -1, newWidth, newHeight);
		}
		else
		{
			Rectangle newWndRect;
			newWndRect.left   = mouseX - g_windowDragCursor.leftOffs;
			newWndRect.top    = mouseY - g_windowDragCursor.topOffs;
			if (newWndRect.top < 0)
				newWndRect.top = 0;
			newWndRect.right  = newWndRect.left + GetWidth (&pWindow->m_fullRect);
			newWndRect.bottom = newWndRect.top  + GetHeight(&pWindow->m_fullRect);
			pWindow->m_fullRect = newWndRect;
			WmRecalculateClientRect(pWindow);
		}
		
		//WindowRegisterEvent(window, EVENT_PAINT, 0, 0);
		pWindow->m_fullVbeData.m_dirty = true;
		pWindow->m_renderFinished = false;
		pWindow->m_isBeingDragged = false;
		ShowWindow(pWindow);
		
		if (GetCurrentCursor() == &g_windowDragCursor)
		{
			SetCursorInternal(NULL, false);
		}
	}
	
	if (pWindow->m_minimized) return;
	
	int x = mouseX - pWindow->m_fullRect.left;
	int y = mouseY - pWindow->m_fullRect.top;
	
	Rectangle margins = GetWindowMargins(pWindow);
	int offsX = -margins.left, offsY = -margins.top;
	if (pWindow->m_clickedInside)
	{
		pWindow->m_clickedInside = false;
		WindowCheckButtons(pWindow, EVENT_RELEASECURSOR, x, y);
		WindowAddEventToMasterQueue(pWindow, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 0);
	}
	else
	{
		WindowCheckButtons(pWindow, EVENT_RELEASECURSOR, x, y);
		WindowAddEventToMasterQueue(pWindow, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 1);
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
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
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
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
			
			WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICKRELEASE, MAKE_MOUSE_PARM (x, y), 0);
		}
	}
}
