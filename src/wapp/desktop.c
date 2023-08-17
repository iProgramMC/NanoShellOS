/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Icon Test Application module
******************************************/

#include <wbuiltin.h>
#include <wmenu.h>
#include <image.h>
#include "../wm/wi.h"

#define EVENT_UPDATE_TASKBAR_POS  (EVENT_USER + 3) // keep this in sync with taskbar.c!!

#define ICON_WIDTH  (80)
#define ICON_HEIGHT (60)

#define DOUBLE_CLICK_TIMING (200) // 50 ms. Ideally, this would be a theming property or something

typedef struct DesktopIcon
{
	struct DesktopIcon *m_pNext, *m_pPrev;
	
	int m_icon;
	
	int m_x, m_y;
	int m_offsetX, m_offsetY;
	
	// a big amount of text, but this is fine
	char m_text[2000];
	
	char m_shown_text[64]; // a wrapped version of the text
	
	bool m_bEverDrawn;
	Rectangle m_DrawnIconRect;
	Rectangle m_DrawnTextRect;
	
	bool m_bSelected;
	
	int m_LastClicked;
}
DesktopIcon;

// going to allow only one instance for now. This application is for testing, anyways.
bool g_bDesktopRunning = false;
bool g_bDraggingAnIcon = false;
bool g_bWasClickingBefore = false;
int g_DraggingCursor = -1;
DesktopIcon* g_pFirstIcon;         // the linked lisst
DesktopIcon* g_pHeldIcon = NULL;   // the icon that's currently being clicked/dragged
Image g_BlankImage = { 0, 0, NULL };
Window* g_pDesktopWindow = NULL;
Point g_StartedClickAt;

int g_IconX, g_IconY;

Window* GetDesktopWindow()
{
	return g_pDesktopWindow;
}

SAI int DtMax(int x, int y)
{
	return x > y ? x : y;
}

SAI int DtMin(int x, int y)
{
	return x < y ? x : y;
}

SAI int DtAbs(int x)
{
	return x < 0 ? -x : x;
}

void DtAddIconToList(DesktopIcon* pIcon)
{
	if (g_pFirstIcon)
	{
		g_pFirstIcon->m_pPrev = pIcon;
		pIcon->m_pNext = g_pFirstIcon;
	}
	g_pFirstIcon = pIcon;
}

void DtRemoveIconFromList(DesktopIcon* pIcon)
{
	if (pIcon->m_pNext)
		pIcon->m_pNext->m_pPrev = pIcon->m_pPrev;
	
	if (pIcon->m_pPrev)
		pIcon->m_pPrev->m_pNext = pIcon->m_pNext;
	
	if (pIcon == g_pFirstIcon)
		g_pFirstIcon = pIcon->m_pNext;
	
	if (pIcon == g_pHeldIcon)
		g_pHeldIcon = NULL;
	
	MmFree(pIcon);
}

void DtAdvanceIconCoords()
{
	g_IconX += ICON_WIDTH;
	if (g_IconX > 400)
	{
		g_IconX -= 400;
		g_IconY += ICON_HEIGHT;
		if (g_IconY >= 300)
			g_IconY = 0;
	}
}

extern SafeLock g_BackgdLock;

void RedrawBackground2(Rectangle rect);
void DtRedrawBackground(Rectangle rect)
{
	//VidFillRectangle(WINDOW_BACKGD_COLOR, rect);
	
	// offset it by the current window's position, to keep consistency
	Rectangle windowRect = GetWindowClientRect(g_pDesktopWindow, true);
	
	LockAcquire(&g_BackgdLock);
	RedrawBackground2(rect);
	LockFree(&g_BackgdLock);
}

void DtAddIcon(int icon, const char* text)
{
	DesktopIcon* pIcon = MmAllocate(sizeof(DesktopIcon));
	memset(pIcon, 0, sizeof *pIcon);
	
	pIcon->m_icon = icon;
	pIcon->m_x = g_IconX;
	pIcon->m_y = g_IconY;
	
	strncpy(pIcon->m_text, text, sizeof pIcon->m_text - 1);
	pIcon->m_text[sizeof pIcon->m_text - 1] = 0;
	
	char buffer[5000];
	WrapText(buffer, pIcon->m_text, ICON_WIDTH - 8);
	
	strncpy(pIcon->m_shown_text, buffer, sizeof pIcon->m_shown_text - 1);
	pIcon->m_shown_text[sizeof pIcon->m_shown_text - 1] = 0;
	
	strcpy (pIcon->m_shown_text + sizeof pIcon->m_shown_text - 4, "...");
	
	DtAdvanceIconCoords();
	DtAddIconToList(pIcon);
}

Rectangle DtIconGetTotalBounds(DesktopIcon* pIcon)
{
	Rectangle x;
	
	x.left   = DtMin(pIcon->m_DrawnIconRect.left,   pIcon->m_DrawnTextRect.left);
	x.top    = DtMin(pIcon->m_DrawnIconRect.top,    pIcon->m_DrawnTextRect.top);
	x.right  = DtMax(pIcon->m_DrawnIconRect.right,  pIcon->m_DrawnTextRect.right);
	x.bottom = DtMax(pIcon->m_DrawnIconRect.bottom, pIcon->m_DrawnTextRect.bottom);
	
	return x;
}

void DtDrawIcon(DesktopIcon* pIcon)
{
	pIcon->m_bEverDrawn = true;
	
	// draw the icon
	int iconX = pIcon->m_x + (ICON_WIDTH - 32) / 2, iconY = pIcon->m_y + 3;
	RenderIconForceSize(pIcon->m_icon, iconX, iconY, 32);
	
	Rectangle iconRect = { iconX, iconY, iconX + 32, iconY + 32 };	
	pIcon->m_DrawnIconRect = iconRect;
	
	
	int width = 0, height = 0;
	VidTextOutInternal(pIcon->m_shown_text, 0, 0, 0, 0, true, &width, &height);
	
	width += 2;
	height += 2;
	
	Rectangle textRect = { pIcon->m_x + (ICON_WIDTH - width) / 2, pIcon->m_y + 40, 0, 0 };
	textRect.right  = textRect.left + width;
	textRect.bottom = textRect.top  + height;
	
	uint32_t bg = TRANSPARENT, fg = WINDOW_TEXT_COLOR;
	
	if (pIcon->m_bSelected)
	{
		bg = SELECTED_ITEM_COLOR;
		fg = SELECTED_TEXT_COLOR;
	}
	
	if (bg == TRANSPARENT)
		DtRedrawBackground(textRect);
	else
		VidFillRect(bg, textRect.left, textRect.top, textRect.right - 1, textRect.bottom - 1);
	
	textRect.top++;
	VidDrawText(pIcon->m_shown_text, textRect, TEXTSTYLE_HCENTERED, fg, TRANSPARENT);
	textRect.top--;
	
	pIcon->m_DrawnTextRect = textRect;
}

DesktopIcon* DtGetIconByPosition(Point pt)
{
	for (DesktopIcon* pIcon = g_pFirstIcon; pIcon; pIcon = pIcon->m_pNext)
	{
		if (RectangleContains(&pIcon->m_DrawnIconRect, &pt) ||
			RectangleContains(&pIcon->m_DrawnTextRect, &pt))
			
			return pIcon;
	}
	
	return NULL;
}

void DtUndrawIcon(DesktopIcon* pIcon, bool bRedrawOtherIcons)
{
	if (!pIcon->m_bEverDrawn) return;
	
	// draw the background over the two rectangles
	DtRedrawBackground(pIcon->m_DrawnIconRect);
	DtRedrawBackground(pIcon->m_DrawnTextRect);
	
	if (!bRedrawOtherIcons) return;
	
	Rectangle totalBounds = DtIconGetTotalBounds(pIcon);
	
	VidSetClipRect(&totalBounds);
	
	for (DesktopIcon* pOtherIcon = g_pFirstIcon; pOtherIcon; pOtherIcon = pOtherIcon->m_pNext)
	{
		if (pOtherIcon == pIcon) continue;
		
		Rectangle otherTotalBounds = DtIconGetTotalBounds(pOtherIcon);
		if (!RectangleOverlap(&otherTotalBounds, &totalBounds)) continue;
		
		DtDrawIcon(pOtherIcon);
	}
	
	VidSetClipRect(NULL);
}

void DtRedrawIcon(DesktopIcon* pIcon)
{
	DtUndrawIcon(pIcon, true);
	DtDrawIcon(pIcon);
}

void DtDrawAllIcons()
{
	for (DesktopIcon* pIcon = g_pFirstIcon; pIcon; pIcon = pIcon->m_pNext)
	{
		DtDrawIcon(pIcon);
	}
}

void DtRedrawAllIcons()
{
	for (DesktopIcon* pIcon = g_pFirstIcon; pIcon; pIcon = pIcon->m_pNext)
	{
		DtUndrawIcon(pIcon, false);
	}
	DtDrawAllIcons();
}

void DtOnClick(Point pt)
{
	DesktopIcon* pIcon = DtGetIconByPosition(pt);
	
	for (DesktopIcon* pCIcon = g_pFirstIcon; pCIcon; pCIcon = pCIcon->m_pNext)
	{
		if (!pCIcon->m_bSelected) continue;
		if (pIcon == pCIcon) continue;
		
		pCIcon->m_bSelected = false;
		DtDrawIcon(pCIcon);
	}
	
	if (pIcon)
	{
		g_pHeldIcon = pIcon;
		if (!pIcon->m_bSelected)
		{
			pIcon->m_bSelected = true;
			DtDrawIcon(pIcon);
		}
	}
	
	g_StartedClickAt = pt;
}

void DtOnDrag(Point pt)
{
	DesktopIcon* pIcon = g_pHeldIcon;
	if (!pIcon) return;
	if (g_bDraggingAnIcon) return;
	
	// check if we moved enough. helps people who accidentally click and drag a small amount
	if (DtAbs(g_StartedClickAt.x - pt.x) <= 5 &&
		DtAbs(g_StartedClickAt.y - pt.y) <= 5)
		return;
	
	// hide the icon
	DtUndrawIcon(pIcon, true);
	
	// start dragging it. This involves...
	g_bDraggingAnIcon = true;
	
	// 1. setting the proper offsets
	pIcon->m_offsetX = pt.x - pIcon->m_x;
	pIcon->m_offsetY = pt.y - pIcon->m_y;
	
	int iconX = pt.x, iconY = pt.y;
	if (pIcon->m_bEverDrawn)
	{
		iconX = pIcon->m_DrawnIconRect.left;
		iconY = pIcon->m_DrawnIconRect.top;
	}
	
	Image* pImage = GetIconImage(pIcon->m_icon, 32);
	if (!pImage)
		pImage = &g_BlankImage;
	
	if (g_DraggingCursor >= 0)
	{
		ReleaseCursor(g_DraggingCursor);
		g_DraggingCursor = -1;
	}
	
	g_DraggingCursor = UploadCursor(pImage, pt.x - iconX, pt.y - iconY);
	
	if (g_DraggingCursor >= 0)
		ChangeCursor(g_pDesktopWindow, g_DraggingCursor);
}

void DtOnRelease(Point pt)
{
	DesktopIcon* pIcon = g_pHeldIcon;
	if (!pIcon) return;
	if (!g_bDraggingAnIcon)
	{
		// try double click
		if (pIcon->m_LastClicked + DOUBLE_CLICK_TIMING > GetTickCount())
		{
			WindowAddEventToMasterQueue(g_pDesktopWindow, EVENT_USER, (int)pIcon, 0);
		}
		
		pIcon->m_LastClicked = GetTickCount();
		return;
	}
	
	// hide the icon
	DtUndrawIcon(pIcon, true);
	
	// change its position.
	pIcon->m_x = pt.x - pIcon->m_offsetX;
	pIcon->m_y = pt.y - pIcon->m_offsetY;
	
	Rectangle rect = GetWindowClientRect(g_pDesktopWindow, true);
	int ww = rect.right - rect.left, wh = rect.bottom - rect.top;
	
	if (pIcon->m_x > ww - ICON_WIDTH)
		pIcon->m_x = ww - ICON_WIDTH;
	if (pIcon->m_y > wh - ICON_HEIGHT)
		pIcon->m_y = wh - ICON_HEIGHT;
	if (pIcon->m_x < 0) pIcon->m_x = 0;
	if (pIcon->m_y < 0) pIcon->m_y = 0;
	
	// draw the icon there
	DtDrawIcon(pIcon);
	
	// turn off some things
	g_bDraggingAnIcon = false;
	
	// set the cursor to default
	ChangeCursor(g_pDesktopWindow, CURSOR_DEFAULT);
}

void CALLBACK DesktopProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			DtAddIcon(ICON_COMPUTER, "My Computer");
			DtAddIcon(ICON_FILES,    "My Files");
			DtAddIcon(ICON_SHUTDOWN, "Shut Down");
			DtAddIcon(ICON_SHUTDOWN, "Really long text to test out the wrapping of the name");
			DtAddIcon(ICON_SHUTDOWN, "Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! Really long text to test out the wrapping of the name and the ellipses! ");
			
			DtDrawAllIcons();			
			break;
		}
		case EVENT_USER:
		{
			DesktopIcon* pIcon = (DesktopIcon*)parm1;
			
			char buffer[4096];
			snprintf(buffer, sizeof buffer, "You clicked '%s'!", pIcon->m_text);
			MessageBox(pWindow, buffer, "Icon test", MB_OK | pIcon->m_icon << 16);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			if (g_bWasClickingBefore)
				DtOnDrag(pt);
			else
				DtOnClick(pt);
			
			g_bWasClickingBefore = true;
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			DtOnRelease(pt);
			g_bWasClickingBefore = false;
			g_pHeldIcon = NULL;
			break;
		}
		case EVENT_PAINT:
		{
			DtRedrawAllIcons();
			break;
		}
		case EVENT_COMMAND:
		{
			break;
		}
		case EVENT_UPDATE_TASKBAR_POS:
		{
			WindowAddEventToMasterQueue(pWindow, EVENT_MAXIMIZE, 0, 0);
			break;
		}
		case EVENT_BGREPAINT:
		{
			Rectangle rect = { GET_X_PARM(parm1), GET_Y_PARM(parm1), GET_X_PARM(parm2), GET_Y_PARM(parm2) };
			DtRedrawBackground(rect);
			break;
		}
		case EVENT_REPAINT_EVERYTHING:
		{
			DesktopProc(pWindow, EVENT_BGREPAINT, parm1, parm2);
			DtRedrawAllIcons();
			break;
		}
		case EVENT_DESTROY:
		{
			if (g_DraggingCursor >= 0)
			{
				ReleaseCursor(g_DraggingCursor);
				g_DraggingCursor = -1;
			}
			// fallthrough
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void DesktopEntry(__attribute__((unused)) int argument)
{
	cli;
	if (g_bDesktopRunning)
	{
		sti;
		MessageBox(NULL, "Please only run one icon test application.", "Icon test", MB_OK);
		return;
	}
	g_bDesktopRunning = true;
	sti;
	
	// create ourself a window:
	Window* pWindow = g_pDesktopWindow = CreateWindow ("Desktop", CW_AUTOPOSITION, CW_AUTOPOSITION, 400, 300, DesktopProc, WF_NOBORDER | WF_NOTITLE | WF_MAXIMIZE | WF_BACKGND2 | WF_SYSPOPUP);
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		g_bDesktopRunning = false;
		return;
	}
	
	pWindow->m_iconID = ICON_INFO;
	g_pDesktopWindow = pWindow;
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
	
	g_bDesktopRunning = false;
	g_pDesktopWindow = NULL;
}