/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Window Manager Decoration Module
******************************************/
#include "wi.h"

extern const unsigned char* g_pCurrentFont;

void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);

void PaintWindowBorderStandard(Rectangle windowRect, const char* pTitle, uint32_t flags, int iconID, bool selected, bool maximized)
{
	Rectangle rectb = windowRect;
	
	if (!(flags & WF_NOBORDER))
	{
		if (flags & WF_FLATBORD)
		{
			if (maximized)
			{
				//done so the thing would cancel out
				rectb.left--;
				rectb.top--;
				rectb.right++;
				rectb.bottom++;
			}
			else
			{
				VidDrawRect(WINDOW_TEXT_COLOR, rectb.left, rectb.top, rectb.right - 1, rectb.bottom - 1);
			}
		}
		else
		{
			VidDrawHLine(0x000000, rectb.left, rectb.right,  rectb.bottom-1);
			VidDrawVLine(0x000000, rectb.top , rectb.bottom, rectb.right -1);
			
			rectb.left  ++;
			rectb.top   ++;
			rectb.right --;
			rectb.bottom--;
			
			VidDrawHLine(0x808080, rectb.left-1, rectb.right -1, rectb.bottom-1);
			VidDrawVLine(0x808080, rectb.top -1, rectb.bottom-1, rectb.right -1);
			
			VidDrawHLine(0xFFFFFF, rectb.left, rectb.right -2, rectb.top );
			VidDrawVLine(0xFFFFFF, rectb.top , rectb.bottom-2, rectb.left);
			
			rectb.left  ++;
			rectb.top   ++;
			rectb.right --;
			rectb.bottom--;
		}
	}
	if (!(flags & WF_NOTITLE))
	{
		Rectangle rectc = rectb;
		rectc.left  ++;
		rectc.top   += TITLE_BAR_HEIGHT-2;
		rectc.right --;
		rectc.bottom--;
		
		//Cut out the gap stuff so that the animation looks good
		//int iconGap = 0;
		
		//draw the window title:
		rectb.left   ++;
		rectb.top    ++;
		rectb.right  -= 2;
		rectb.bottom = rectb.top + TITLE_BAR_HEIGHT;
		
		if (!(flags & WF_FLATBORD))
		{
			RenderButtonShapeSmallInsideOut (rectb, BUTTONLITE, BUTTONDARK, TRANSPARENT);
			
			rectb.left  ++;
			rectb.top   ++;
			rectb.right --;
			rectb.bottom -= 2;
		}
		else
			rectb.bottom -= 3;
		
		VidFillRectHGradient(
			selected ? WINDOW_TITLE_ACTIVE_COLOR   : WINDOW_TITLE_INACTIVE_COLOR, 
			selected ? WINDOW_TITLE_ACTIVE_COLOR_B : WINDOW_TITLE_INACTIVE_COLOR_B, 
			rectb.left,
			rectb.top,
			rectb.right,
			rectb.bottom
		);
	
		int textwidth, height;
		VidSetFont(TITLE_BAR_FONT);
		uint32_t uflags = TEXT_RENDER_BOLD;
		if (TITLE_BAR_FONT != SYSTEM_FONT)
			uflags = 0;
		
		int left = rectb.left, top = rectb.top;
		
		// Offset the rectangle based on the present buttons.
		if (iconID != ICON_NULL)
			rectb.left += 18;
		
		if (!(flags & WF_NOCLOSE))
		{
			rectb.right -= TITLE_BAR_HEIGHT;
			if (!(flags & WF_NOMINIMZ)) rectb.right -= TITLE_BAR_HEIGHT;
			if (!(flags & WF_NOMAXIMZ)) rectb.right -= TITLE_BAR_HEIGHT;
		}
		
		VidTextOutInternal(pTitle, 0, 0, FLAGS_TOO(uflags,0), 0, true, &textwidth, &height);
		
		int offset = (rectb.right - rectb.left - textwidth) / 2;
		
		int textOffset = (TITLE_BAR_HEIGHT - height) / 2;
		int iconOffset = (TITLE_BAR_HEIGHT - 16)     / 2 - 1;
		
		VidTextOut(pTitle, rectb.left + offset + 1, rectb.top - 0 + textOffset, FLAGS_TOO(uflags, WINDOW_TITLE_TEXT_COLOR_SHADOW), TRANSPARENT);
		VidTextOut(pTitle, rectb.left + offset + 0, rectb.top - 1 + textOffset, FLAGS_TOO(uflags, WINDOW_TITLE_TEXT_COLOR       ), TRANSPARENT);
		VidSetFont(SYSTEM_FONT);
		
		if (iconID != ICON_NULL)
			RenderIconForceSize(iconID, left+1, top + iconOffset, 16);
	}
	
#undef X
}

void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow)
{	
	Rectangle recta = pWindow->m_rect;
	recta.right  -= recta.left; recta.left = 0;
	recta.bottom -= recta.top;  recta.top  = 0;
	
	if (pWindow->m_flags & WI_HUNGWIND)
	{
		char windowtitle_copy[WINDOW_TITLE_MAX + 100];
		strcpy (windowtitle_copy, pWindow->m_title);
		strcat (windowtitle_copy, " (not responding)");
		PaintWindowBorderStandard(recta, windowtitle_copy, pWindow->m_flags, pWindow->m_iconID, pWindow->m_isSelected, pWindow->m_maximized);
	}
	else
		PaintWindowBorderStandard(recta, pWindow->m_title, pWindow->m_flags, pWindow->m_iconID, pWindow->m_isSelected, pWindow->m_maximized);
	
	/*char thing[32];
	sprintf(thing, "ID : %d", (int)(pWindow - g_windows));
	VidTextOut(thing, 0, 0, 0xFFFFFF, 0x000000);*/
}

void PaintWindowBorder(Window* pWindow)
{
	Rectangle recta = pWindow->m_rect;
	recta.right  -= recta.left; recta.left = 0;
	recta.bottom -= recta.top;  recta.top  = 0;
	
	//! X adjusts the size of the dropshadow on the window.
	//recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS+1;
	//recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS+1;
	
	VidFillRectangle(WINDOW_BACKGD_COLOR, recta);
	PaintWindowBorderStandard(recta, pWindow->m_title, pWindow->m_flags, pWindow->m_iconID, pWindow->m_isSelected, pWindow->m_maximized);
}

void PaintWindowBackgroundAndBorder(Window* pWindow)
{
	PaintWindowBorder(pWindow);
}
