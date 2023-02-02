/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Scroller controls
******************************************/
#include <widget.h>
#include <video.h>
#include <image.h>
#include <icon.h>
#include <clip.h>
#include <print.h>
#include <misc.h>
#include <keyboard.h>
#include <wmenu.h>
#include <string.h>

void CtlSetScrollBarMin (Control *pControl, int min)
{
	pControl->m_scrollBarData.m_min = min;
	if (pControl->m_scrollBarData.m_pos < pControl->m_scrollBarData.m_min)
		pControl->m_scrollBarData.m_pos = pControl->m_scrollBarData.m_min;
}
void CtlSetScrollBarMax (Control *pControl, int max)
{
	pControl->m_scrollBarData.m_max = max;
	if (pControl->m_scrollBarData.m_pos >= pControl->m_scrollBarData.m_max)
		pControl->m_scrollBarData.m_pos  = pControl->m_scrollBarData.m_max-1;
}
void CtlSetScrollBarPos (Control *pControl, int pos)
{
	pControl->m_scrollBarData.m_pos = pos;
}
int CtlGetScrollBarPos (Control *pControl)
{
	return pControl->m_scrollBarData.m_pos;
}
int CtlGetScrollBarMin(Control *pControl)
{
	return pControl->m_scrollBarData.m_min;
}
int CtlGetScrollBarMax(Control *pControl)
{
	return pControl->m_scrollBarData.m_max;
}

void SetScrollBarMin (Window *pWindow, int comboID, int min)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetScrollBarMin (&pWindow->m_pControlArray[i], min);
			return;
		}
	}
}
void SetScrollBarMax (Window *pWindow, int comboID, int max)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetScrollBarMax (&pWindow->m_pControlArray[i], max);
			return;
		}
	}
}
void SetScrollBarPos (Window *pWindow, int comboID, int pos)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetScrollBarPos (&pWindow->m_pControlArray[i], pos);
			return;
		}
	}
}
int GetScrollBarPos (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			return CtlGetScrollBarPos (&pWindow->m_pControlArray[i]);
	}
	return -1;
}
int GetScrollBarMin (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			return CtlGetScrollBarMin(&pWindow->m_pControlArray[i]);
	}
	return -1;
}
int GetScrollBarMax (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			return CtlGetScrollBarMax(&pWindow->m_pControlArray[i]);
	}
	return -1;
}

bool WidgetHScrollBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
go_back:;
	Rectangle basic_rectangle = this->m_rect;
	basic_rectangle.bottom = basic_rectangle.top + SCROLL_BAR_WIDTH-1;
	
	Rectangle left_button = basic_rectangle;
	left_button.right = left_button.left + SCROLL_BAR_WIDTH;
	
	Rectangle right_button = basic_rectangle;
	right_button.left = right_button.right - SCROLL_BAR_WIDTH;
	
	Rectangle clickable_rect = basic_rectangle;
	clickable_rect.left  = left_button.right;
	clickable_rect.right = right_button.left;
	
	int height = clickable_rect.right - clickable_rect.left;
	int final_height = height - SCROLL_BAR_WIDTH;
		
	int offset = this->m_scrollBarData.m_pos - this->m_scrollBarData.m_min;
	int posoff = 0, thing = (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min);
	if (thing <= 0)
		thing = 1;
	
	posoff = (int)((int64_t)offset * final_height / thing);
	
	Rectangle scroller = basic_rectangle;
	scroller.left  = clickable_rect.left + posoff;
	scroller.right = clickable_rect.left + posoff + SCROLL_BAR_WIDTH;
	
	switch (eventType)
	{
		case EVENT_CLICKCURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains(&basic_rectangle, &p) || this->m_scrollBarData.m_clickedBefore)
			{
				if (!this->m_scrollBarData.m_clickedBefore)
				{
					this->m_scrollBarData.m_yMinButton     = false;
					this->m_scrollBarData.m_yMaxButton     = false;
				}
				if (RectangleContains(&scroller, &p))
				{
					this->m_scrollBarData.m_isBeingDragged = true;
				}
				if (RectangleContains(&left_button, &p))
				{
					this->m_scrollBarData.m_yMinButton = true;
					this->m_scrollBarData.m_yMaxButton = false;
				}
				if (RectangleContains(&right_button, &p))
				{
					this->m_scrollBarData.m_yMaxButton = true;
					this->m_scrollBarData.m_yMinButton = false;
				}
				
				if (this->m_scrollBarData.m_isBeingDragged)
				{
					int posoff2 = p.x - clickable_rect.left - SCROLL_BAR_WIDTH/2;
					if (final_height > 0)
					{
						posoff2 = (int)((int64_t)posoff2 * (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min) / final_height);
						posoff2 = posoff2 +  this->m_scrollBarData.m_min;
						if (posoff2 <  this->m_scrollBarData.m_min) posoff2 = this->m_scrollBarData.m_min;
						if (posoff2 >= this->m_scrollBarData.m_max) posoff2 = this->m_scrollBarData.m_max - 1;
						this->m_scrollBarData.m_pos = posoff2;
					}
				}
				
				this->m_scrollBarData.m_clickedBefore = true;
				
				eventType = EVENT_PAINT;
				goto go_back;
			}
			else break;
		}
		case EVENT_RELEASECURSOR:
		{
			if (this->m_scrollBarData.m_isBeingDragged || this->m_scrollBarData.m_yMinButton || this->m_scrollBarData.m_yMaxButton)
			{
				eventType = EVENT_PAINT;
				this->m_scrollBarData.m_isBeingDragged = false;
				if (this->m_scrollBarData.m_yMinButton)
				{
					this->m_scrollBarData.m_pos -= this->m_scrollBarData.m_dbi;
					if (this->m_scrollBarData.m_pos < this->m_scrollBarData.m_min)
						this->m_scrollBarData.m_pos = this->m_scrollBarData.m_min;
				}
				if (this->m_scrollBarData.m_yMaxButton)
				{
					this->m_scrollBarData.m_pos += this->m_scrollBarData.m_dbi;
					if (this->m_scrollBarData.m_pos >= this->m_scrollBarData.m_max)
						this->m_scrollBarData.m_pos  = this->m_scrollBarData.m_max - 1;
				}
				
				//let everyone else know that we're done scrolling on this control
				CallWindowCallbackAndControls(pWindow, EVENT_SCROLLDONE, this->m_comboID, 0);
				
				eventType = EVENT_PAINT;
			}
			this->m_scrollBarData.m_clickedBefore  = false;
			this->m_scrollBarData.m_yMinButton     = false;
			this->m_scrollBarData.m_yMaxButton     = false;
			
			if (eventType == EVENT_PAINT)
				goto go_back;
			else break;
		}
		case EVENT_PAINT:
		{
			VidFillRectangle (0x7F7F7F, basic_rectangle);
		
			if (this->m_scrollBarData.m_yMinButton)
				RenderButtonShapeSmall (left_button,   BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeSmall (left_button,   BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			if (this->m_scrollBarData.m_yMaxButton)
				RenderButtonShapeSmall (right_button,  BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeSmall (right_button,  BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			RenderButtonShapeSmall (scroller, BUTTONDARK, BUTTONLITE, BUTTONMIDD + this->m_scrollBarData.m_isBeingDragged * 0x222222); // Green
			
			left_button .left++; left_button .right++; left_button .bottom++; left_button .top++;
			right_button.left++; right_button.right++; right_button.bottom++; right_button.top++;
			scroller    .left++; scroller    .right++; scroller    .bottom++; scroller    .top++;
			
			VidDrawText ("\x1B",   left_button,  TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			VidDrawText ("\x1A",   right_button, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			VidDrawText ("\x1D",   scroller,     TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			break;
		}
	}
	return false;//Fall through to other controls.
}
bool WidgetVScrollBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
go_back:;
	Rectangle basic_rectangle = this->m_rect;
	basic_rectangle.right = basic_rectangle.left + SCROLL_BAR_WIDTH-1;
	
	Rectangle top_button = basic_rectangle;
	top_button.bottom = top_button.top + SCROLL_BAR_WIDTH;
	
	Rectangle bottom_button = basic_rectangle;
	bottom_button.top = bottom_button.bottom - SCROLL_BAR_WIDTH;
	
	Rectangle clickable_rect = basic_rectangle;
	clickable_rect.top    = top_button.bottom;
	clickable_rect.bottom = bottom_button.top;
	
	int height = clickable_rect.bottom - clickable_rect.top;
	int final_height = height - SCROLL_BAR_WIDTH;
		
	int offset = this->m_scrollBarData.m_pos - this->m_scrollBarData.m_min;
	int test = (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min);
	if (test <= 0) test = 1;
	int posoff = (int)((int64_t)offset * final_height / test);
	
	Rectangle scroller = basic_rectangle;
	scroller.top    = clickable_rect.top + posoff;
	scroller.bottom = clickable_rect.top + posoff + SCROLL_BAR_WIDTH;
	
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (this->m_scrollBarData.m_isBeingDragged || this->m_scrollBarData.m_yMinButton || this->m_scrollBarData.m_yMaxButton)
			{
				this->m_scrollBarData.m_isBeingDragged = false;
				if (this->m_scrollBarData.m_yMinButton)
				{
					this->m_scrollBarData.m_pos -= this->m_scrollBarData.m_dbi;
					if (this->m_scrollBarData.m_pos < this->m_scrollBarData.m_min)
						this->m_scrollBarData.m_pos = this->m_scrollBarData.m_min;
				}
				if (this->m_scrollBarData.m_yMaxButton)
				{
					this->m_scrollBarData.m_pos += this->m_scrollBarData.m_dbi;
					if (this->m_scrollBarData.m_pos >= this->m_scrollBarData.m_max)
						this->m_scrollBarData.m_pos  = this->m_scrollBarData.m_max - 1;
				}
				
				//let everyone else know that we're done scrolling on this control
				CallWindowCallbackAndControls(pWindow, EVENT_SCROLLDONE, this->m_comboID, 0);
				
				eventType = EVENT_PAINT;
			}
			this->m_scrollBarData.m_clickedBefore  = false;
			this->m_scrollBarData.m_yMinButton     = false;
			this->m_scrollBarData.m_yMaxButton     = false;
			
			if (eventType == EVENT_PAINT)
				goto go_back;
			else break;
		}
		case EVENT_CLICKCURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			if (RectangleContains(&basic_rectangle, &p) || this->m_scrollBarData.m_clickedBefore)
			{
				if (!this->m_scrollBarData.m_clickedBefore)
				{
					this->m_scrollBarData.m_yMinButton     = false;
					this->m_scrollBarData.m_yMaxButton     = false;
				}
				if (RectangleContains(&scroller, &p))
				{
					this->m_scrollBarData.m_isBeingDragged = true;
				}
				if (RectangleContains(&top_button, &p))
				{
					this->m_scrollBarData.m_yMinButton = true;
					this->m_scrollBarData.m_yMaxButton = false;
				}
				if (RectangleContains(&bottom_button, &p))
				{
					this->m_scrollBarData.m_yMaxButton = true;
					this->m_scrollBarData.m_yMinButton = false;
				}
				
				if (this->m_scrollBarData.m_isBeingDragged)
				{
					int posoff2 = p.y - clickable_rect.top - SCROLL_BAR_WIDTH/2;
					if (final_height > 0)
					{
						posoff2 = (int)((int64_t)posoff2 * (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min) / final_height);
						posoff2 += this->m_scrollBarData.m_min;
						if (posoff2 <  this->m_scrollBarData.m_min) posoff2 = this->m_scrollBarData.m_min;
						if (posoff2 >= this->m_scrollBarData.m_max) posoff2 = this->m_scrollBarData.m_max - 1;
						this->m_scrollBarData.m_pos = posoff2;
					}
				}
				
				this->m_scrollBarData.m_clickedBefore = true;
				
				eventType = EVENT_PAINT;
				goto go_back;
			}
			else break;
		}
		case EVENT_PAINT:
		{
			VidFillRectangle (BUTTONDARK-0x111111, clickable_rect);
			if (this->m_scrollBarData.m_yMinButton)
				RenderButtonShapeSmall (top_button,     BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeSmall (top_button,     BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			if (this->m_scrollBarData.m_yMaxButton)
				RenderButtonShapeSmall (bottom_button,  BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeSmall (bottom_button,  BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			RenderButtonShapeSmall (scroller, BUTTONDARK, BUTTONLITE, BUTTONMIDD + this->m_scrollBarData.m_isBeingDragged * 0x222222); // Green
			
			top_button   .left++; top_button   .right++; top_button   .bottom++; top_button   .top++;
			bottom_button.left++; bottom_button.right++; bottom_button.bottom++; bottom_button.top++;
			scroller     .left++; scroller     .right++; scroller     .bottom++; scroller     .top++;
			
			VidDrawText ("\x18",   top_button,    TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			VidDrawText ("\x19",   bottom_button, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			VidDrawText ("\x12",   scroller,      TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			break;
		}
	}
	return false;//Fall through to other controls.
}
