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
		if (evt == EVENT_MAXIMIZE && (pWindow->m_flags & WF_MAXIMIZE))
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
	WindowTitleLayout(rectb, flags, pWindow->m_iconID, &bTitleHas3dShape, &titleRect, &titleGradRect, &minimBtnRect, &maximBtnRect, &closeBtnRect, &iconBtnRect);
	
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

bool g_bFirstDrag = false;

void OnUILeftClick (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	
	Window* window = ShootRayAndGetWindow(mouseX, mouseY);
	
	g_bFirstDrag = true;
	
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
			
			if (!(window->m_flags & WF_MAXIMIZE) && (t || (window->m_flags & WF_MINIMIZE)))
			{
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 1);
				WindowCheckButtons(window, EVENT_CLICKCURSOR, x, y);
			}
			else if (!(window->m_flags & WF_MINIMIZE))
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

static const int g_ResizeFlagsTable[] =
{
	0,
	CUR_RESIZE | CUR_RESIZE_MOVE_X | CUR_LOCK_Y,        // left
	CUR_RESIZE |                     CUR_LOCK_Y,        // right
	CUR_RESIZE | CUR_RESIZE_MOVE_Y | CUR_LOCK_X,        // up
	CUR_RESIZE |                     CUR_LOCK_X,        // down
	CUR_RESIZE | CUR_RESIZE_MOVE_X | CUR_RESIZE_MOVE_Y, // left  + up
	CUR_RESIZE |                     CUR_RESIZE_MOVE_Y, // right + up
	CUR_RESIZE | CUR_RESIZE_MOVE_X,                     // left  + down
	CUR_RESIZE,                                         // right + down
};

static const int g_ResizeFlagsIndices[] =
{
	// horizontal: none, left, right, left+right
	// vertical:   none, up, down, up+down
	
	0, 1, 2, 0,
	3, 5, 6, 0,
	4, 7, 8, 0,
	0, 0, 0, 0,
};

void OnUILeftClickDrag (int mouseX, int mouseY)
{
	if (!IsWindowManagerRunning()) return;
	if (!g_currentlyClickedWindow) return;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	Window* window = g_currentlyClickedWindow;
	
	if (!window->m_isBeingDragged && !(window->m_flags & WF_FROZEN))
	{
		// if we're not frozen AND we have a title to drag on
		if ((window->m_flags & WF_MINIMIZE) || !(window->m_flags & WF_NOTITLE))
		{
			//are we in the title bar region?
			Rectangle recta;
			UNUSED bool hasTitle = GetWindowTitleRect(window, &recta);
			
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
			Point mousePoint = {x, y};
			
			if (!(window->m_flags & WF_MAXIMIZE) && (RectangleContains(&recta, &mousePoint) || (window->m_flags & WF_MINIMIZE)))
			{
				window->m_isBeingDragged = true;
				
				if (g_RenderWindowContents)
				{
					HideWindow(window);
				}
				
				//change cursor:
				if (window->m_flags & WF_MINIMIZE)
				{
					Image* p = GetIconImage(window->m_iconID, 32);
					g_windowDragCursor.width    = p->width;
					g_windowDragCursor.height   = p->height;
					g_windowDragCursor.leftOffs = mouseX - window->m_fullRect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_fullRect.top;
					g_windowDragCursor.bitmap   = p->framebuffer;
					g_windowDragCursor.m_transparency = true;
					g_windowDragCursor.m_flags  = 0;
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
					g_windowDragCursor.m_flags  = window->m_resize_flags = CUR_RESIZE;
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
					g_windowDragCursor.m_flags  = 0;
					g_windowDragCursor.boundsWidth  = window->m_fullVbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_fullVbeData.m_height;
					g_windowDragCursor.mouseLockX = -1;
					g_windowDragCursor.mouseLockY = -1;
				}
				
				SetCursor (&g_windowDragCursor);
			}
			if (!(window->m_flags & WF_MINIMIZE) && !window->m_isBeingDragged)
			{
				window->m_clickedInside = true;
				WindowCheckButtons(window, EVENT_CLICKCURSOR, x, y);
				
				Rectangle margins = GetWindowMargins(window);
				int offsX = -margins.left, offsY = -margins.top;
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (offsX + x, offsY + y), 0);
			}
		}
		
		if ((window->m_flags & (WF_NOBORDER | WF_ALWRESIZ | WF_MAXIMIZE)) == WF_ALWRESIZ && g_bFirstDrag)
		{
			const int width  = GetWidth (&window->m_fullRect);
			const int height = GetHeight(&window->m_fullRect);
			const int borderSize = (window->m_flags & WF_FLATBORD) ? (1) : (BORDER_SIZE);
			
			int left, up, down, right;
			
			const int x = mouseX - window->m_fullRect.left;
			const int y = mouseY - window->m_fullRect.top;
			
			left  = x < borderSize;
			right = x >= width - borderSize;
			up    = y < borderSize;
			down  = y >= height - borderSize;
			
			int index = left | right << 1 | up << 2 | down << 3;
			
			int result = g_ResizeFlagsTable[g_ResizeFlagsIndices[index]];
			if (result != 0)
			{
				window->m_isBeingDragged = true;
				
				if (g_RenderWindowContents)
				{
					HideWindow(window);
				}
				
				window->m_resize_flags = result;
				g_windowDragCursor.width    = window->m_fullVbeData.m_width;
				g_windowDragCursor.height   = window->m_fullVbeData.m_height;
				g_windowDragCursor.leftOffs = mouseX - window->m_fullRect.left;
				g_windowDragCursor.topOffs  = mouseY - window->m_fullRect.top;
				g_windowDragCursor.bitmap   = window->m_fullVbeData.m_framebuffer32;
				g_windowDragCursor.m_transparency = false;
				g_windowDragCursor.m_flags  = window->m_resize_flags = result;
				g_windowDragCursor.boundsWidth  = window->m_fullVbeData.m_width;
				g_windowDragCursor.boundsHeight = window->m_fullVbeData.m_height;
				g_windowDragCursor.mouseLockX = mouseX;
				g_windowDragCursor.mouseLockY = mouseY;
				
				SetCursor (&g_windowDragCursor);
			}
		}
	}
	
	g_bFirstDrag = false;
	
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
	
	g_bFirstDrag = false;
	
	Window* pWindow = g_currentlyClickedWindow;
	if (pWindow->m_isBeingDragged)
	{
		if (!g_RenderWindowContents)
		{
			HideWindow(pWindow);
		}
		
		if (GetCurrentCursor()->m_flags & CUR_RESIZE)
		{
			int newWidth = GetCurrentCursor()->boundsWidth, newHeight = GetCurrentCursor()->boundsHeight;
			int newX = mouseX - g_windowDragCursor.leftOffs, newY = mouseY - g_windowDragCursor.topOffs;
			
			pWindow->m_resize_flags = 0;
			
			ResizeWindow(pWindow, newX, newY, newWidth, newHeight);
			WindowAddEventToMasterQueue(pWindow, EVENT_SHOW_WINDOW_PRIVATE, 0, 0);
			pWindow->m_isBeingDragged = false;
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
			
			pWindow->m_fullVbeData.m_dirty = true;
			pWindow->m_renderFinished = false;
			pWindow->m_isBeingDragged = false;
			
			ShowWindow(pWindow);
		}
		
		if (GetCurrentCursor() == &g_windowDragCursor)
		{
			SetCursorInternal(NULL, false);
		}
	}
	
	if (pWindow->m_flags & WF_MINIMIZE) return;
	
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
		if (!(window->m_flags & WF_MINIMIZE))
		{
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
			
			Rectangle margins = GetWindowMargins(window);
			int offsX = -margins.left, offsY = -margins.top;
			WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICK, MAKE_MOUSE_PARM (offsX + x, offsY + y), 0);
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
		if (window->m_flags & WF_MINIMIZE)
		{
			WindowRegisterEvent (window, EVENT_UNMINIMIZE, 0, 0);
		}
		else
		{
			int x = mouseX - window->m_fullRect.left;
			int y = mouseY - window->m_fullRect.top;
			
			Rectangle margins = GetWindowMargins(window);
			int offsX = -margins.left, offsY = -margins.top;
			WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICKRELEASE_PRIVATE, MAKE_MOUSE_PARM (x, y), 0);
			WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICKRELEASE, MAKE_MOUSE_PARM (offsX + x, y + offsY), 0);
		}
	}
}
