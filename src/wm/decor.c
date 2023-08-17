/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Window Manager Decoration Module
******************************************/
#include "wi.h"

extern const unsigned char* g_pCurrentFont;

static void WindowBorderDrawButton(Rectangle rectBtn, uint32_t privFlags, int hoverMask, int clickMask, int icon)
{
	bool pressed = privFlags & clickMask, hovered = privFlags & hoverMask;
	
	uint32_t style1 = DRE_BLACKOUTER, style2 = DRE_RAISEDINNER | DRE_FILLED, midd = BUTTONMIDC;
	if (hovered)
		midd = BUTTONMIDD;
	if (pressed)
		style1 = DRE_SUNKENINNER | DRE_FILLED, style2 = 0, midd = BUTTONDARK;
	
	DrawEdge(rectBtn, style1, midd);
	rectBtn.right--;
	rectBtn.bottom--;
	DrawEdge(rectBtn, style2, midd);
	rectBtn.right++;
	rectBtn.bottom++;
	
	// draw the icon
	int rectWidth = (rectBtn.right - rectBtn.left);
	int iconSize;
	if (rectWidth > 32)
		iconSize  = 32;
	else if (rectWidth > 16)
		iconSize = 16;
	else
		iconSize = rectWidth - 2;
	
	int iconX = rectBtn.left + (rectBtn.right  - rectBtn.left - iconSize) / 2;
	int iconY = rectBtn.top  + (rectBtn.bottom - rectBtn.top  - iconSize) / 2 - 1;
	
	if (pressed)
		iconX++, iconY++;
	
	RenderIconForceSize(icon, iconX, iconY, iconSize);
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
)
{
	Rectangle rectb = windowRect;
	const bool maximized = flags & WF_MAXIMIZE;
	const int borderSize = GetBorderSize(flags);
	
	if (!(flags & WF_NOBORDER))
	{
		if (flags & WF_FLATBORD)
		{
			if (maximized)
			{
				rectb.right++;
				rectb.left --;
			}
			else
			{
				rectb.top++;
			}
		}
		else
		{
			rectb.left   += borderSize - 1;
			rectb.top    += borderSize - 1;
			rectb.right  -= borderSize - 1;
			rectb.bottom -= borderSize - 1;
		}
	}
	if (!(flags & WF_NOTITLE))
	{
		//draw the window title:
		rectb.left   ++;
		rectb.top    ++;
		rectb.right  -= 2;
		rectb.bottom = rectb.top + TITLE_BAR_HEIGHT;
		
		if (iconID != ICON_NULL)
			rectb.left  += C_ACTION_BUTTON_WIDTH;
		if (~flags & WF_NOCLOSE)
			rectb.right -= C_ACTION_BUTTON_WIDTH;
		if (~flags & WF_NOMINIMZ)
			rectb.right -= C_ACTION_BUTTON_WIDTH;
		if (!(flags & WF_NOMAXIMZ) && (flags & WF_ALWRESIZ))
			rectb.right -= C_ACTION_BUTTON_WIDTH;
		
		int rectLeft = rectb.left, rectRight = rectb.right - 1;
		
		*bTitleHas3dShape = false;
		if (maximized)
		{
			rectb.top--;
			rectb.bottom -= 2;
		}
		else if (!(flags & WF_FLATBORD))
		{
			rectb.left  ++;
			rectb.right --;
			*bTitleHas3dShape = true;
			*pTitleRect = rectb;
			rectb.left  ++;
			rectb.right --;
			rectb.top++;
			rectb.bottom -= 2;
		}
		else
		{
			rectb.bottom -= 3;
		}
		
		if ((flags & (WF_FLATBORD | WF_MAXIMIZE)) == WF_FLATBORD)
			rectb.top--, rectb.bottom++;
		
		if (!*bTitleHas3dShape)
			*pTitleRect = rectb;
		
		*pTitleGradientRect = rectb;
		
		if ((flags & (WF_FLATBORD | WF_MAXIMIZE)) == WF_FLATBORD)
			rectb.top++, rectb.bottom--;
		
		if (maximized)
		{
			rectb.top++;
			rectb.bottom--;
		}
		
		if (iconID != ICON_NULL)
		{
			Rectangle rectBtn;
			rectBtn.right  = rectLeft;
			rectBtn.top    = rectb.top    - 1;
			rectBtn.bottom = rectb.bottom + 2;
			rectBtn.left   = rectLeft     - C_ACTION_BUTTON_WIDTH;
			*pIconButtonRect = rectBtn;
		}
		
		// Draw some buttons.
		Rectangle rectBtn;
		rectBtn.left   = rectRight    + 2;
		rectBtn.top    = rectb.top    - 1;
		rectBtn.bottom = rectb.bottom + 2;
		rectBtn.right  = rectRight    + C_ACTION_BUTTON_WIDTH + 2;
		
		if (~flags & WF_NOMINIMZ)
		{
			*pMinimButtonRect = rectBtn;
			rectBtn.left  += C_ACTION_BUTTON_WIDTH;
			rectBtn.right += C_ACTION_BUTTON_WIDTH;
		}
		if (!(flags & WF_NOMAXIMZ) && (flags & WF_ALWRESIZ))
		{
			*pMaximButtonRect = rectBtn;
			rectBtn.left  += C_ACTION_BUTTON_WIDTH;
			rectBtn.right += C_ACTION_BUTTON_WIDTH;
		}
		if (~flags & WF_NOCLOSE)
		{
			*pCloseButtonRect = rectBtn;
			rectBtn.left  += C_ACTION_BUTTON_WIDTH;
			rectBtn.right += C_ACTION_BUTTON_WIDTH;
		}
	}
}

bool GetWindowTitleRect(Window* pWindow, Rectangle* pRectOut)
{
	Rectangle rect = pWindow->m_fullRect;
	rect.right  -= rect.left;
	rect.bottom -= rect.top;
	rect.left    = rect.top = 0;
	
	if (pWindow->m_flags & WF_MINIMIZE)
	{
		*pRectOut = rect;
		return true;
	}
	
	if (pWindow->m_flags & WF_NOTITLE) return false;
	
	Rectangle uselessGarbage[10]; bool moreUselessness = false;
	
	WindowTitleLayout(rect, pWindow->m_flags, pWindow->m_iconID, &moreUselessness, pRectOut, uselessGarbage, uselessGarbage+1, uselessGarbage+2, uselessGarbage+3, uselessGarbage+4);

	return true;
}

void PaintWindowDecorStandard(Rectangle windowRect, const char* pTitle, uint32_t flags, uint32_t privflags, int iconID, bool selected, bool bDrawBorder, bool bDrawTitle)
{
	Rectangle titleRect, titleGradRect, minimBtnRect, maximBtnRect, closeBtnRect, iconBtnRect;
	bool bTitleHas3dShape;
	WindowTitleLayout(windowRect, flags, iconID, &bTitleHas3dShape, &titleRect, &titleGradRect, &minimBtnRect, &maximBtnRect, &closeBtnRect, &iconBtnRect);
	
	bool maximized = flags & WF_MAXIMIZE;
	
	if ((~flags & WF_NOBORDER) && bDrawBorder)
	{
		if (flags & WF_FLATBORD)
		{
			if (~flags & WF_MAXIMIZE)
			{
				Rectangle rectb = windowRect;
				VidDrawRect(WINDOW_TEXT_COLOR, rectb.left, rectb.top, rectb.right - 1, rectb.bottom - 1);
			}
		}
		else
		{
			Rectangle rectb = windowRect;
			
			VidDrawHLine(BUTTON_XSHADOW_COLOR, rectb.left, rectb.right,  rectb.bottom-1);
			VidDrawVLine(BUTTON_XSHADOW_COLOR, rectb.top , rectb.bottom, rectb.right -1);
			
			rectb.left  ++;
			rectb.top   ++;
			rectb.right --;
			rectb.bottom--;
			
			VidDrawHLine(BUTTON_SHADOW_COLOR, rectb.left-1, rectb.right -1, rectb.bottom-1);
			VidDrawVLine(BUTTON_SHADOW_COLOR, rectb.top -1, rectb.bottom-1, rectb.right -1);
			
			VidDrawHLine(BUTTON_HILITE_COLOR, rectb.left, rectb.right -2, rectb.top );
			VidDrawVLine(BUTTON_HILITE_COLOR, rectb.top , rectb.bottom-2, rectb.left);
		}
	}
	
	if ((~flags & WF_NOTITLE) && bDrawTitle)
	{
		//Cut out the gap stuff so that the animation looks good
		//int iconGap = 0;
		
		//draw the window title:
		if (bTitleHas3dShape)
		{
			titleRect.right++;
			DrawEdge(titleRect, DRE_SUNKENOUTER, 0);
			titleRect.right--;
		}
		
		uint32_t
		color1 = selected ? WINDOW_TITLE_ACTIVE_COLOR   : WINDOW_TITLE_INACTIVE_COLOR,
		color2 = selected ? WINDOW_TITLE_ACTIVE_COLOR_B : WINDOW_TITLE_INACTIVE_COLOR_B;
		
		if (color1 == color2)
		{
			VidFillRect(
				color1,
				titleGradRect.left,
				titleGradRect.top,
				titleGradRect.right,
				titleGradRect.bottom
			);
		}
		else
		{
			VidFillRectHGradient(
				color1,
				color2,
				titleGradRect.left,
				titleGradRect.top,
				titleGradRect.right,
				titleGradRect.bottom
			);
		}
		
		int textwidth, height;
		VidSetFont(TITLE_BAR_FONT);
		uint32_t uflags = TEXT_RENDER_BOLD;
		if (TITLE_BAR_FONT != SYSTEM_FONT)
			uflags = 0;
		
		VidTextOutInternal(pTitle, 0, 0, FLAGS_TOO(uflags, 0), 0, true, &textwidth, &height);
		
		int offset = (titleGradRect.right - titleGradRect.left - textwidth) / 2;
		
		int textOffset = (TITLE_BAR_HEIGHT - height) / 2 + maximized + ((flags & WF_FLATBORD) != 0);
		
		VidTextOut(pTitle, titleGradRect.left + offset + 1, titleGradRect.top - 0 + textOffset, FLAGS_TOO(uflags, WINDOW_TITLE_TEXT_COLOR_SHADOW), TRANSPARENT);
		VidTextOut(pTitle, titleGradRect.left + offset + 0, titleGradRect.top - 1 + textOffset, FLAGS_TOO(uflags, WINDOW_TITLE_TEXT_COLOR       ), TRANSPARENT);
		VidSetFont(SYSTEM_FONT);
		
		if (iconID != ICON_NULL)
			WindowBorderDrawButton(iconBtnRect, privflags, WPF_ICONBUTNHOVERED, WPF_ICONBUTNCLICKED, iconID);
		
		// Draw some buttons.
		if (~flags & WF_NOMINIMZ)
			WindowBorderDrawButton(minimBtnRect, privflags, WPF_MINIMIZEHOVERED, WPF_MINIMIZECLICKED, ICON_MINIMIZE);
		
		if (!(flags & WF_NOMAXIMZ) && (flags & WF_ALWRESIZ))
			WindowBorderDrawButton(maximBtnRect, privflags, WPF_MAXIMIZEHOVERED, WPF_MAXIMIZECLICKED, maximized ? ICON_RESTORE : ICON_MAXIMIZE);
		
		if (~flags & WF_NOCLOSE)
			WindowBorderDrawButton(closeBtnRect, privflags, WPF_CLOSEBTNHOVERED, WPF_CLOSEBTNCLICKED, ICON_CLOSE);
	}
	
#undef X
}

void WmPaintWindowTitle(Window* pWindow)
{
	VBEData* bkp = VidSetVBEData(&pWindow->m_fullVbeData);
	
	Rectangle recta = pWindow->m_fullRect;
	recta.right  -= recta.left; recta.left = 0;
	recta.bottom -= recta.top;  recta.top  = 0;
	
	if (pWindow->m_privFlags & WPF_HUNGWIND)
	{
		char windowtitle_copy[WINDOW_TITLE_MAX + 100];
		strcpy (windowtitle_copy, pWindow->m_title);
		strcat (windowtitle_copy, " (not responding)");
		PaintWindowDecorStandard(recta, windowtitle_copy, pWindow->m_flags, pWindow->m_privFlags, pWindow->m_iconID, pWindow->m_isSelected, false, true);
	}
	else
		PaintWindowDecorStandard(recta, pWindow->m_title, pWindow->m_flags, pWindow->m_privFlags, pWindow->m_iconID, pWindow->m_isSelected, false, true);
	
	VidSetVBEData(bkp);
}

void WmPaintWindowBackgd(Window* pWindow)
{
	Rectangle rect = GetWindowClientRect(pWindow, false);
	//VidFillRectangle(WINDOW_BACKGD_COLOR, rect);
	CallWindowCallback(pWindow, EVENT_BGREPAINT, MAKE_MOUSE_PARM(rect.left, rect.top), MAKE_MOUSE_PARM(rect.right, rect.bottom));
}

void WmPaintWindowBorder(Window* pWindow)
{
	VBEData* bkp = VidSetVBEData(&pWindow->m_fullVbeData);
	
	Rectangle rect = pWindow->m_fullRect;
	rect.right  -= rect.left;
	rect.bottom -= rect.top;
	rect.left = rect.top = 0;
	
	// repaint edges.
	if (pWindow->m_flags & WF_MAXIMIZE)
	{
	}
	else if (pWindow->m_flags & WF_FLATBORD)
	{
		VidDrawRect(WINDOW_TEXT_COLOR, rect.left, rect.top, rect.right - 1, rect.bottom - 1);
	}
	else if (~pWindow->m_flags & WF_NOBORDER)
	{
		Rectangle margins = GetWindowMargins(pWindow);
		
		VidFillRect(WINDOW_BORDER_COLOR, rect.left, rect.top, rect.left + margins.left - 1, rect.bottom - 1);
		VidFillRect(WINDOW_BORDER_COLOR, rect.right - margins.right, rect.top, rect.right - 1, rect.bottom - 1);
		VidFillRect(WINDOW_BORDER_COLOR, rect.left + margins.left, rect.top, rect.right - margins.right, rect.top + margins.top - 1);
		VidFillRect(WINDOW_BORDER_COLOR, rect.left + margins.left, rect.bottom - margins.bottom, rect.right - margins.right, rect.bottom - 1);
	}
	
	PaintWindowDecorStandard(rect, pWindow->m_title, pWindow->m_flags, pWindow->m_privFlags, pWindow->m_iconID, pWindow->m_isSelected, true, false);
	
	VidSetVBEData(bkp);
}
