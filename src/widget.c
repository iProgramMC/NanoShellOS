/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

           Shell widget module
******************************************/
#include <widget.h>
#include <video.h>
#include <image.h>
#include <icon.h>
#include <print.h>
#include <misc.h>
#include <keyboard.h>
#include <wmenu.h>
#include <string.h>

bool g_GlowOnHover = true;

// Utilitary functions
#if 1

void SetFocusedControl(Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control *pControl = &pWindow->m_pControlArray[i];
		pControl->m_bFocused = (pControl->m_comboID == comboID);
	}
}

#define BUTTONDARK 0x808080
#define BUTTONMIDD BUTTON_MIDDLE_COLOR
#define BUTTONLITE 0xFFFFFF
#define BUTTONMIDC WINDOW_BACKGD_COLOR

/***************************************************************************
	Explanation of how this is supposed to render:

	LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
	LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LLWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWDD
	LDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
	DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
***************************************************************************/
void RenderButtonShapeNoRounding(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	
	//do the same
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink again
	rect.left++, rect.right -= 2, rect.top++, rect.bottom -= 2;
	
	//fill the background:
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rect);
}
void RenderButtonShapeSmall(Rectangle rectb, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	rectb.bottom--;
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rectb);
	rectb.right++;
	rectb.bottom++;
	
	VidDrawHLine(WINDOW_TEXT_COLOR, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(WINDOW_TEXT_COLOR, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	VidDrawHLine(colorLight, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorLight, rectb.top,  rectb.bottom-1, rectb.left);
	
	rectb.left++;
	rectb.top++;
	rectb.right--;
	rectb.bottom--;
	
	VidDrawHLine(colorDark, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(colorDark, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	int colorAvg = 0;
	colorAvg |= ((colorLight & 0xff0000) + (colorMiddle & 0xff0000)) >> 1;
	colorAvg |= ((colorLight & 0x00ff00) + (colorMiddle & 0x00ff00)) >> 1;
	colorAvg |= ((colorLight & 0x0000ff) + (colorMiddle & 0x0000ff)) >> 1;
	VidDrawHLine(colorAvg, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorAvg, rectb.top,  rectb.bottom-1, rectb.left);
}
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle)
{
	rectb.bottom--;
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rectb);
	rectb.right++;
	rectb.bottom++;
	
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(WINDOW_TEXT_COLOR_LIGHT, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	VidDrawHLine(colorDark, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorDark, rectb.top,  rectb.bottom-1, rectb.left);
	
	rectb.left  ++;
	rectb.top   ++;
	rectb.right --;
	rectb.bottom--;
	
	VidDrawHLine(colorLight, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(colorLight, rectb.top,  rectb.bottom-1, rectb.right-1);
	
	int colorAvg = 0;
	colorAvg |= (colorDark & 0xff0000) >> 1;
	colorAvg |= (colorDark & 0x00ff00) >> 1;
	colorAvg |= (colorDark & 0x0000ff) >> 1;
	VidDrawHLine(colorAvg, rectb.left, rectb.right-1,  rectb.top);
	VidDrawVLine(colorAvg, rectb.top,  rectb.bottom-1, rectb.left);
}
void RenderButtonShape(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawHLine (WINDOW_TEXT_COLOR, rect.left,rect.right-1,  rect.top);
	VidDrawHLine (WINDOW_TEXT_COLOR, rect.left,rect.right-1,  rect.bottom-1);
	VidDrawVLine (WINDOW_TEXT_COLOR, rect.top, rect.bottom-1, rect.left);
	VidDrawVLine (WINDOW_TEXT_COLOR, rect.top, rect.bottom-1, rect.right-1);
	
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	RenderButtonShapeNoRounding(rect, colorDark, colorLight, colorMiddle);
	//RenderButtonShapeSmall(rect, colorDark, colorLight, colorMiddle);
}


void SetWidgetEventHandler (Window *pWindow, int comboID, WidgetEventHandler handler)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control *pControl = &pWindow->m_pControlArray[i];
		if (pControl->m_comboID == comboID)
		{
			// Set It
			if (pControl->OnEvent)
				pControl->OnEvent(pControl, EVENT_DESTROY, 0, 0, pWindow);
			
			pControl->OnEvent = handler;
			
			if (pControl->OnEvent)
				pControl->OnEvent(pControl, EVENT_CREATE,  0, 0, pWindow);
			
			return;
		}
	}
}

#endif

// Scroll bar
#if 1

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

#define SCROLL_BAR_WIDTH 16
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
	
	posoff = offset * final_height / thing;
	
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
						posoff2 = posoff2 * (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min) / final_height;
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
					this->m_scrollBarData.m_pos -= (this->m_scrollBarData.m_max - this->m_scrollBarData.m_min) / 10;
					if (this->m_scrollBarData.m_pos < this->m_scrollBarData.m_min)
						this->m_scrollBarData.m_pos = this->m_scrollBarData.m_min;
				}
				if (this->m_scrollBarData.m_yMaxButton)
				{
					this->m_scrollBarData.m_pos += (this->m_scrollBarData.m_max - this->m_scrollBarData.m_min) / 10;
					if (this->m_scrollBarData.m_pos >= this->m_scrollBarData.m_max)
						this->m_scrollBarData.m_pos  = this->m_scrollBarData.m_max - 1;
				}
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
			//VidDrawText ("\x1D",   scroller,     TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
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
	int posoff = offset * final_height / test;
	
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
						posoff2 = posoff2 * (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min) / final_height;
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
#endif

// TextEdit view
#if 1

void CtlTextInputUpdateScrollSize(Control* this, Window* pWindow)
{
	int c = CountLinesInText(this->m_textInputData.m_pText);
	SetScrollBarMax (pWindow, -this->m_comboID, c);
}

void CtlSetTextInputText (Control* this, Window* pWindow, const char* pText)
{
	
	int slen = strlen (pText);
	int newCapacity = slen + 1;
	if (newCapacity < 4096) newCapacity = 4096;//paradoxically, a smaller allocation occupies the same space as a 4096 byte alloc
	
	char *pNewText = MmAllocateK (newCapacity);
	if (!pNewText) return;
	
	if (this->m_textInputData.m_pText)
		MmFreeK(this->m_textInputData.m_pText);
	
	this->m_textInputData.m_pText = pNewText;
	
	strcpy (this->m_textInputData.m_pText, pText);
	this->m_textInputData.m_textCapacity = newCapacity;
	this->m_textInputData.m_textLength   = slen;
	this->m_textInputData.m_textCursorIndex = 0;
	this->m_textInputData.m_textCursorSelStart = -1;
	this->m_textInputData.m_textCursorSelEnd   = -1;
	this->m_textInputData.m_textCursorIndex    = this->m_textInputData.m_textLength;
	
	CtlTextInputUpdateScrollSize (this, pWindow);
}

void SetTextInputText(Window* pWindow, int comboID, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetTextInputText (&pWindow->m_pControlArray[i], pWindow, pText);
			return;
		}
	}
}
void TextInputClearDirtyFlag(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_textInputData.m_dirty = false;
			return;
		}
	}
}
bool TextInputQueryDirtyFlag(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return pWindow->m_pControlArray[i].m_textInputData.m_dirty;
		}
	}
	return false;
}
const char* TextInputGetRawText(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return pWindow->m_pControlArray[i].m_textInputData.m_pText;
		}
	}
	return NULL;
}

void CtlTextInputUpdateMode (Control *pControl)
{
	pControl->m_textInputData.m_onlyOneLine        = true;
	pControl->m_textInputData.m_enableStyling      = false;
	pControl->m_textInputData.m_enableSyntaxHilite = false;
	pControl->m_textInputData.m_showLineNumbers    = false;
	pControl->m_textInputData.m_focused            = false;
	if (pControl->m_parm1 & TEXTEDIT_MULTILINE)
		pControl->m_textInputData.m_onlyOneLine        = false;
	if (pControl->m_parm1 & TEXTEDIT_LINENUMS)
		pControl->m_textInputData.m_showLineNumbers    = true;
	if (pControl->m_parm1 & TEXTEDIT_READONLY)
		pControl->m_textInputData.m_readOnly           = true;
	if (pControl->m_parm1 & TEXTEDIT_STYLING)
		pControl->m_textInputData.m_enableStyling      = true;
	if (pControl->m_parm1 & TEXTEDIT_SYNTHILT)
		pControl->m_textInputData.m_enableSyntaxHilite = true;
}

void TextInputSetMode (Window *pWindow, int comboID, int mode)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			Control *p = &pWindow->m_pControlArray[i];
			
			p->m_parm1 = mode;
			CtlTextInputUpdateMode(p);
		}
	}
}

void CtlAppendChar(Control* this, Window* pWindow, char charToAppend)
{
	if (this->m_textInputData.m_textLength >= this->m_textInputData.m_textCapacity-1)
	{
		//can't fit, need to expand
		int newCapacity = this->m_textInputData.m_textCapacity * 2, oldCapacity = this->m_textInputData.m_textCapacity;
		if (newCapacity < 4096) newCapacity = 4096;//paradoxically, a smaller allocation occupies the same space as a 4096 byte alloc
		
		char* pText = (char*)MmAllocateK(newCapacity);
		
		if (!pText)
			return;
		
		memcpy (pText, this->m_textInputData.m_pText, oldCapacity);
		
		MmFreeK(this->m_textInputData.m_pText);
		this->m_textInputData.m_pText = pText;
		this->m_textInputData.m_textCapacity = newCapacity;
	}
	
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength++] = charToAppend;
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength  ] = 0;
	
	CtlTextInputUpdateScrollSize (this, pWindow);
	
	this->m_textInputData.m_dirty = true;
}

void CtlAppendCharToAnywhere(Control* this, Window* pWindow, char charToAppend, int indexToAppendTo)
{
	if (indexToAppendTo < 0)
		return;
	if (indexToAppendTo > this->m_textInputData.m_textLength)
		return;
	
	if (this->m_textInputData.m_textLength >= this->m_textInputData.m_textCapacity-1)
	{
		//can't fit, need to expand
		int newCapacity = this->m_textInputData.m_textCapacity * 2, oldCapacity = this->m_textInputData.m_textCapacity;
		if (newCapacity < 4096) newCapacity = 4096;//paradoxically, a smaller allocation occupies the same space as a 4096 byte alloc
		
		char* pText = (char*)MmAllocateK(newCapacity);
		if (!pText)
			return;
		
		memcpy (pText, this->m_textInputData.m_pText, oldCapacity);
		
		MmFreeK(this->m_textInputData.m_pText);
		this->m_textInputData.m_pText = pText;
		this->m_textInputData.m_textCapacity = newCapacity;
	}
	
	this->m_textInputData.m_textLength++;
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength  ] = 0;
	memmove (&this->m_textInputData.m_pText[indexToAppendTo+1], &this->m_textInputData.m_pText[indexToAppendTo], this->m_textInputData.m_textLength-1-indexToAppendTo);
	this->m_textInputData.m_pText[indexToAppendTo] = charToAppend;
	
	CtlTextInputUpdateScrollSize (this, pWindow);
	
	this->m_textInputData.m_dirty = true;
}

void CtlRemoveCharFromAnywhere(Control* this, Window* pWindow, int indexToRemoveFrom)
{
	if (indexToRemoveFrom < 0)
		return;
	if (indexToRemoveFrom > this->m_textInputData.m_textLength)
		return;
	
	memmove (&this->m_textInputData.m_pText[indexToRemoveFrom], &this->m_textInputData.m_pText[indexToRemoveFrom+1], this->m_textInputData.m_textLength-1-indexToRemoveFrom);
	this->m_textInputData.m_textLength--;
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength] = 0;
	
	CtlTextInputUpdateScrollSize (this, pWindow);
	
	this->m_textInputData.m_dirty = true;
}

#define LINE_NUM_GAP 56
#define RECT(rect,x,y,w,h) do {\
	rect.left = x, rect.top = y, rect.right = x+w, rect.bottom = y+h;\
} while (0)

enum {
	C_SYNTAX_HILITE_NONE     = 0x000000, //black
	C_SYNTAX_HILITE_DEFTYPE  = 0x00007F, //dark blue
	C_SYNTAX_HILITE_KEYWORD  = 0x0000FF, //blue
	C_SYNTAX_HILITE_IDENT    = 0x007F7F, //cyan
	C_SYNTAX_HILITE_OPERATOR = 0x00FF00, //green
	C_SYNTAX_HILITE_COMMENT  = 0x808080, //gray
	C_SYNTAX_HILITE_STRLITER = 0x7F0000, //red
	C_SYNTAX_HILITE_NUMBER   = 0xFF7F00, //orange
};

__attribute__((always_inline))
static inline bool IsAlphaNumeric (char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

__attribute__((always_inline))
static inline bool StrCompMulti (const char * _text, int num, ...)
{
	va_list list;
	va_start(list, num);
	
	for (int i = 0; i < num; i++)
	{
		const char * word = va_arg (list, const char *);
		
		const char * text = _text;
		
		/*while (true)
		{
			// If there's a different character
			if (*text != *word)
				break;//Skip  to the next word
			
			// If the words ended at the same time and they're zero
			if (*text == *word && *word == '\0')
				return true;
			
			// Proceed
			text++, word++;
		}*/
		
		if (strcmp (text, word) == 0)
			return true;
	}
	
	va_end(list);
	return false;
}

void CtlTextInputDoSyntaxHighlight (Control *this, uint32_t* color, int *chars_consumed, int text_pos)
{
	const char * text = this->m_textInputData.m_pText;
	
	// For starters, what is this character?
	
	const char * curtext = text + text_pos;
	
	if ((*curtext >= 'a' && *curtext <= 'z') || (*curtext >= 'A' && *curtext <= 'Z'))
	{
		// This is an identifier or a keyword
		const char *next_space = curtext;//strchr (curtext, ' ');
		
		while (IsAlphaNumeric(*next_space))
			next_space++;
		
		if (*next_space == 0)
			next_space = NULL;
		
		int id_length = 0;
		
		if (next_space == NULL)
		{
			// This is the end.
			id_length = strlen (curtext);
		}
		else
		{
			// This is not the end, the length of the keyword follows the text until you hit a space
			id_length = next_space - curtext;
		}
		
		// if the identifier is unusually long (like over 31 characters), it surely ain't a keyword :^)
		if (id_length > 31)
		{
			*chars_consumed = id_length;
			*color = C_SYNTAX_HILITE_IDENT;
			return;
		}
		
		char id[32];
		memcpy (id, curtext, id_length);
		id[id_length] = 0;
		//SLogMsg("Found Identifier: %s", id);
		
		if (StrCompMulti(id, 28, 
				"void", "int", "char", "short", "long", "signed",
				"unsigned", "double", "float", "const", "static", "extern",
				"auto", "register", "volatile", "bool", "uint8_t", "uint16_t",
				"uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t",
				"size_t", "FILE", "Window", "VBEData"))
		{
			*chars_consumed = id_length;
			*color = C_SYNTAX_HILITE_DEFTYPE;
			return;
		}
		if (StrCompMulti(id, 18, 
				"if", "else", "switch", "case", "default", "break",
				"goto", "return", "for", "while", "do", "continue",
				"typedef", "sizeof", "NULL", "LogMsg", "LogMsgNoCr", "printf"))
		{
			*chars_consumed = id_length;
			*color = C_SYNTAX_HILITE_KEYWORD;
			return;
		}
		
		*chars_consumed = id_length;
		*color = C_SYNTAX_HILITE_IDENT;
		return;
	}
	else if (*curtext == '"' || *curtext == '\'')
	{
		char to_escape = *curtext;
		const char *next_quote = curtext + 1;
		while (*next_quote)
		{
			if (*next_quote == '\\') //Allow escaping of " or '
			{
				next_quote++;
				if (*next_quote == 0) break;
			}
			
			if (*next_quote == to_escape)
				break;
			
			next_quote++;
		}
		
		*chars_consumed = next_quote - curtext + 1;
		*color          = C_SYNTAX_HILITE_STRLITER;
		
		return;
	}
	else if (*curtext >= '0' && *curtext <= '9')
	{
		const char *next_space = curtext;
		
		while (IsAlphaNumeric(*next_space))
			next_space++;
		
		*chars_consumed = next_space - curtext;
		*color          = C_SYNTAX_HILITE_NUMBER;
		
		return;
	}
	else if (*curtext == '/')
	{
		if (*(curtext+1) == '/')
		{
			//C++ style comment
			const char *next_char = curtext + 2;
			while (true)
			{
				if (*next_char == '\n' || !*next_char)
					break;
				
				next_char++;
			}
			
			*chars_consumed = next_char - curtext;
			*color          = C_SYNTAX_HILITE_COMMENT;
		}
		else if (*(curtext+1) == '*')
		{
			//C style old comment
			const char *next_char = curtext + 2;
			while (true)
			{
				if (*next_char == '*')
				{
					next_char++;
					if (*next_char == '/')
						break;
				}
				
				if (*next_char == '\0') break;
				
				next_char++;
			}
			
			*chars_consumed = next_char - curtext;
			*color          = C_SYNTAX_HILITE_COMMENT;
		}
	}
	else if (strchr ("(){}[]+-*%^|&=<>", *curtext) != NULL)
	{
		*chars_consumed = 1;
		*color = C_SYNTAX_HILITE_OPERATOR;
		return;
	}
	else
	{
		*chars_consumed = 1;
		*color = C_SYNTAX_HILITE_NONE;
		return;
	}
}

bool WidgetTextEditView_OnEvent(Control* this, int eventType, int parm1, UNUSED int parm2, Window* pWindow)
{
	int charsPerLine = (this->m_rect.right-this->m_rect.left)/8;
	switch (eventType)
	{
		//case EVENT_RELEASECURSOR:
			//TODO: Allow selection across the text.
			//break;
			
		// -- Uncomment this if you want to get smooth scrolling through text.
		// I like to keep it on
		case EVENT_CLICKCURSOR:
		case EVENT_RELEASECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			//TODO: Allow change of cursor via click.
			if (!this->m_textInputData.m_onlyOneLine)
			{
				int pos = GetScrollBarPos(pWindow, -this->m_comboID);
				if (this->m_textInputData.m_scrollY != pos)
				{
					this->m_textInputData.m_scrollY  = pos;
				}
				WidgetTextEditView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			}
			
			if (!RectangleContains (&this->m_rect, &mouseClickPos))
				break;
			
			SetFocusedControl(pWindow, this->m_comboID);
			
			if (this->m_textInputData.m_enableStyling && eventType == EVENT_RELEASECURSOR)
			{
				if (this->m_textInputData.m_pText)
				{
					//HACK
					//No need to change fonts.  m_enableStyling assumes FONT_BASIC.
					
					const char*text = this->m_textInputData.m_pText;
					int lineHeight = GetLineHeight();
					int xPos = this->m_rect.left + 4,
						yPos = this->m_rect.top  + 4;// - lineHeight * this->m_textInputData.m_scrollY;
					if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
					{
						xPos += LINE_NUM_GAP;
					}
					
					if (this->m_textInputData.m_onlyOneLine)
					{
						xPos = this->m_rect.left + 4 - 8 * this->m_textInputData.m_scrollY;//scrollY is scrollX for now in single line mode.
					}
					
					int curLine = 0, curLine2 = 0, scrollLine = this->m_textInputData.m_scrollY;// linesPerScreen = (this->m_rect.bottom - this->m_rect.top) / lineHeight;
					if (this->m_textInputData.m_onlyOneLine)
					{
						//linesPerScreen = 1;
						scrollLine     = 0;
					}
					int offset  = 0;
					
					
					bool bold = false, link = false;
					while (*text)
					{
						//word wrap
						if (xPos + GetCharWidth(*text) >= this->m_rect.right - 4 || *text == '\n')
						{
							if (this->m_textInputData.m_onlyOneLine)
								//Don't actually word-wrap if we hit the edge on a one-line textbox.
								break;
							xPos = this->m_rect.left + 4;
							
							if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
								xPos += LINE_NUM_GAP;
							
							if (curLine2 >= scrollLine)
								yPos += lineHeight;
							
							if (*text == '\n')
							{
								if (yPos >= this->m_rect.bottom - lineHeight) break;//no point in checking anymore.
							}
							curLine ++;
						}
						if (*text != '\n')
						{
							// If character should be rendered
							if (!this->m_textInputData.m_enableStyling || !((unsigned char)(*text) >= (unsigned char)TIST_BOLD && (unsigned char)(*text) < (unsigned char)TIST_COUNT))
							{
								// render this character:
								if (yPos >= this->m_rect.bottom - lineHeight) break;//no point in drawing anymore.
								if (curLine2 >= scrollLine && xPos >= this->m_rect.left)
								{
									//VidPlotChar(*text, xPos, yPos, color, TRANSPARENT);
									
									
									//If it's a link character, check if the mouse cursor intersects it; if it does, trigger the proper link.
									
									Rectangle rect;
									RECT(rect, xPos, yPos, GetCharWidth(*text), GetLineHeight());
									
									if (link && RectangleContains(&rect, &mouseClickPos))
									{
										CallWindowCallbackAndControls(pWindow, EVENT_CLICK_CHAR, this->m_comboID, offset);
										break;
									}
								}
									
								// Increment the X,Y positions
								xPos += GetCharWidth (*text) + bold;
							}
							//TODO cleanup
							else if (*text == TIST_LINK)
								link = true;
							else if (*text == TIST_UNLINK)
								link = false;
							else if (*text == TIST_BOLD)
								bold = true;
							else if (*text == TIST_UNBOLD)
								bold = false;
						}
						
						text++;
						offset++;
					}
					
					//Revert the font back if you decide to change stuff.
				}
			}
			//RequestRepaint (pWindow);
			break;
		}
		case EVENT_SIZE:
			CtlTextInputUpdateScrollSize (this, pWindow);
			break;
		case EVENT_KEYRAW:
		{
			if (!this->m_bFocused) break;
			if (this->m_textInputData.m_readOnly) break;
			bool repaint = true;
			switch (parm1)
			{
				case KEY_ARROW_UP:
				case KEY_ARROW_DOWN:
				case KEY_HOME:
				case KEY_END:
				{
					if (this->m_textInputData.m_onlyOneLine)
					{
						if (parm1 == KEY_ARROW_UP || parm1 == KEY_ARROW_DOWN)
							break;
					}
					if (this->m_textInputData.m_pText)
					{
						const char*text = this->m_textInputData.m_pText;
						int lastRowStartIdx = -1, lastRowEndIdx = -1, curRowStartIdx = 0, newEndIndex;
						int xPos = this->m_rect.left + 4;
						if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
						{
							xPos += LINE_NUM_GAP;
						}
						int offset  = 0;
						int offsetInsideLine = -1, offsetInsideLineCur = 0;
						int curLine = 0;
						
						if (parm1 != KEY_END)
						{
							while (*text)
							{
								if (offset == this->m_textInputData.m_textCursorIndex)
								{
									offsetInsideLine = offsetInsideLineCur;
									if (parm1 == KEY_ARROW_UP || parm1 == KEY_HOME)
										break;
								}
								//word wrap
								if (xPos + 7 >= this->m_rect.right - 4 || *text == '\n')
								{
									xPos = this->m_rect.left + 4;
									if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
									{
										xPos += LINE_NUM_GAP;
									}
									offsetInsideLineCur = 0;
									curLine ++;
									
									//TODO make this better?
									if (parm1 == KEY_ARROW_DOWN)
									{
										if (offsetInsideLine != -1)
										{
											if (lastRowStartIdx == -1)
											{
												lastRowStartIdx  = offset + 1;
											}
											else
											{
												lastRowEndIdx    = offset;
												break;
											}
										}
									}
									else if (parm1 == KEY_HOME)
									{
										curRowStartIdx = offset + 1;
									}
									else
									{
										lastRowStartIdx = curRowStartIdx;
										lastRowEndIdx   = offset;
										curRowStartIdx  = offset + 1;
									}
								}
								if (*text != '\n')
								{
									// Increment the X,Y positions
									xPos += 7;
									offsetInsideLineCur++;
								}
								
								text++;
								offset++;
							}
							if (parm1 == KEY_ARROW_DOWN)
							{
								if (offsetInsideLine != -1)
								{
									if (lastRowStartIdx == -1)
									{
										lastRowStartIdx  = offset + 1;
									}
									else
									{
										lastRowEndIdx    = offset;
									}
								}
							}
						}
						else
						{
							//END: special case.
							int index = this->m_textInputData.m_textCursorIndex;
							while (this->m_textInputData.m_pText[index] != '\n' && index < this->m_textInputData.m_textLength)
								index++;
							newEndIndex = index;
							
							if (this->m_textInputData.m_onlyOneLine)
							{
								this->m_textInputData.m_textCursorIndex = 0;
								this->m_textInputData.m_scrollY = index - charsPerLine + 5;
								if (this->m_textInputData.m_scrollY < 0)
									this->m_textInputData.m_scrollY = 0;
							}
						}
						
						if (offsetInsideLine > lastRowEndIdx-lastRowStartIdx)
							offsetInsideLine = lastRowEndIdx-lastRowStartIdx;
						int newIndex = -1;
						if (parm1 == KEY_ARROW_UP)
						{
							if (lastRowEndIdx >= lastRowStartIdx)
								newIndex = lastRowStartIdx + offsetInsideLine;
							else
								newIndex = 0;
						}
						else if (parm1 == KEY_ARROW_DOWN)
						{
							if (lastRowEndIdx >= lastRowStartIdx)
								newIndex = lastRowStartIdx + offsetInsideLine;
							else
								newIndex = this->m_textInputData.m_textLength;
						}
						else if (parm1 == KEY_HOME)
						{
							newIndex = curRowStartIdx;
							if (this->m_textInputData.m_onlyOneLine)
							{
								this->m_textInputData.m_scrollY = 0;
								newIndex = 0;
							}
						}
						else if (parm1 == KEY_END)
						{
							newIndex = newEndIndex;
						}
						
						if (newIndex >= 0)
							this->m_textInputData.m_textCursorIndex = newIndex;
						else
							LogMsg("Move not implemented?");
					}
					break;
				}
				case KEY_ARROW_LEFT:
					this->m_textInputData.m_textCursorIndex--;
					if (this->m_textInputData.m_textCursorIndex < 0)
						this->m_textInputData.m_textCursorIndex = 0;
					
					if (this->m_textInputData.m_onlyOneLine)
					{
						if (this->m_textInputData.m_scrollY > this->m_textInputData.m_textCursorIndex)
						{
							this->m_textInputData.m_scrollY -= 10;
							if (this->m_textInputData.m_scrollY < 0)
								this->m_textInputData.m_scrollY = 0;
						}
					}
					
					break;
				case KEY_ARROW_RIGHT:
					this->m_textInputData.m_textCursorIndex++;
					if (this->m_textInputData.m_textCursorIndex > this->m_textInputData.m_textLength)
						this->m_textInputData.m_textCursorIndex = this->m_textInputData.m_textLength;
					
					if (this->m_textInputData.m_onlyOneLine)
					{
						if (this->m_textInputData.m_scrollY + charsPerLine +2 <= this->m_textInputData.m_textCursorIndex)
						{
							this->m_textInputData.m_scrollY += 10;
						}
					}
					
					break;
				case KEY_DELETE:
					if (this->m_textInputData.m_textLength   >   this->m_textInputData.m_textCursorIndex)
						CtlRemoveCharFromAnywhere(this, pWindow, this->m_textInputData.m_textCursorIndex);
					break;
				default:
					repaint = false;
					break;
			}
			if (repaint)
				WidgetTextEditView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
				//RequestRepaint(pWindow);
			break;
		}
		case EVENT_KEYPRESS:
		{
			if (!this->m_bFocused) break;
			if (this->m_textInputData.m_readOnly) break;
			if ((char)parm1 == '\n' && this->m_textInputData.m_onlyOneLine)
				break;
			if ((char)parm1 == '\b')
			{
				CtlRemoveCharFromAnywhere(this, pWindow, --this->m_textInputData.m_textCursorIndex);
				if (this->m_textInputData.m_textCursorIndex < 0)
					this->m_textInputData.m_textCursorIndex = 0;
				
				if (this->m_textInputData.m_onlyOneLine)
				{
					//while (this->m_textInputData.m_scrollY >= this->m_textInputData.m_textCursorIndex)
					{
						this->m_textInputData.m_scrollY--;
						if (this->m_textInputData.m_scrollY < 0)
						{
							this->m_textInputData.m_scrollY = 0;
						}
					}
				}
			}
			else
			{
				CtlAppendCharToAnywhere(this, pWindow, (char)parm1, this->m_textInputData.m_textCursorIndex++);
				
				if (this->m_textInputData.m_onlyOneLine)
				{
					while (this->m_textInputData.m_scrollY + charsPerLine <= this->m_textInputData.m_textCursorIndex)
					{
						this->m_textInputData.m_scrollY++;
					}
				}
			}
			//RequestRepaintNew (pWindow);
			//WidgetTextEditView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			RequestRepaint (pWindow);
			//pWindow->m_vbeData.m_dirty = true;
			//pWindow->m_renderFinished  = true;
			break;
		}
		case EVENT_CREATE:
		{
			this->m_textInputData.m_pText = NULL;
			this->m_textInputData.m_scrollY = 0;
			
			if (!this->m_textInputData.m_onlyOneLine)
			{
				Rectangle r;
				r.right = this->m_rect.right, 
				r.top   = this->m_rect.top, 
				r.bottom= this->m_rect.bottom, 
				r.left  = this->m_rect.right - SCROLL_BAR_WIDTH;
				
				int flags = 0;
				if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
					flags |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
				if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
					flags |= ANCHOR_BOTTOM_TO_BOTTOM;
				
				AddControlEx (pWindow, CONTROL_VSCROLLBAR, flags, r, NULL, -this->m_comboID, 1, 1);
			
				//shrink our rectangle:
				this->m_rect.right -= SCROLL_BAR_WIDTH + 4;
			}
			
			if (this->m_textInputData.m_onlyOneLine)
				this->m_rect.bottom = this->m_rect.top + 14 + 8;
			
			// setup some blank text:
			CtlSetTextInputText(this, pWindow, "");
			
			break;
		}
		case EVENT_DESTROY:
		{
			if (this->m_textInputData.m_pText)
			{
				MmFreeK(this->m_textInputData.m_pText);
				this->m_textInputData.m_pText = NULL;
			}
			break;
		}
		case EVENT_PAINT:
		{
			// Render the basic container rectangle
			Rectangle rk = this->m_rect, rk1;
			
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			
			rk1 = rk;
			
			if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
			{
				rk.left += LINE_NUM_GAP;
				rk1.right = rk.left;
			}
			
			uint32_t bg_color = WINDOW_TEXT_COLOR_LIGHT;
			if (this->m_textInputData.m_enableStyling)
			{
				bg_color &= 0xFFFFFFAA;
			}
			VidFillRectangle(bg_color, rk);
			if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
				VidFillRectangle(0x3f3f3f, rk1);
		
			uint32_t color = WINDOW_TEXT_COLOR, color_default = WINDOW_TEXT_COLOR;
			
			if (this->m_textInputData.m_pText)
			{
				//HACK
				VidSetFont(this->m_textInputData.m_enableStyling ? SYSTEM_FONT : FONT_TAMSYN_MED_REGULAR);
				
				const char*text = this->m_textInputData.m_pText;
				int lineHeight = GetLineHeight();
				int xPos = this->m_rect.left + 4,
					yPos = this->m_rect.top  + 4;// - lineHeight * this->m_textInputData.m_scrollY;
				if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
				{
					xPos += LINE_NUM_GAP;
				}
				
				if (this->m_textInputData.m_onlyOneLine)
				{
					xPos = this->m_rect.left + 4 - GetCharWidth('W') * this->m_textInputData.m_scrollY;//scrollY is scrollX for now in single line mode.
				}
				
				int curLine = 0, curLine2 = 0, scrollLine = this->m_textInputData.m_scrollY;// linesPerScreen = (this->m_rect.bottom - this->m_rect.top) / lineHeight;
				if (this->m_textInputData.m_onlyOneLine)
				{
					//linesPerScreen = 1;
					scrollLine     = 0;
				}
				int offset  = 0;
				
				char line_string[10];
				curLine2 ++;
				if (curLine2 >= scrollLine && xPos >= this->m_rect.left && this->m_textInputData.m_showLineNumbers)
				{
					sprintf   (line_string, "%5d", curLine2);
					VidTextOut(line_string, this->m_rect.left + 6, yPos, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
				}
				int chars_consumed = 0;
				
				while (*text)
				{
					if (!this->m_textInputData.m_readOnly)
						if (offset == this->m_textInputData.m_textCursorIndex && !this->m_textInputData.m_readOnly)
						{
							VidDrawVLine(0xFF, yPos, yPos + lineHeight, xPos);
						}
					//word wrap
					if (xPos + GetCharWidth(*text) >= this->m_rect.right - 4 || *text == '\n')
					{
						if (this->m_textInputData.m_onlyOneLine)
							//Don't actually word-wrap if we hit the edge on a one-line textbox.
							break;
						xPos = this->m_rect.left + 4;
						if (this->m_textInputData.m_showLineNumbers && !this->m_textInputData.m_onlyOneLine)
						{
							xPos += LINE_NUM_GAP;
						}
						if (curLine2 >= scrollLine)
						{
							yPos += lineHeight;
						}
						if (*text == '\n')
						{
							curLine2 ++;
							if (yPos >= this->m_rect.bottom - lineHeight) break;//no point in drawing anymore.
							if (curLine2 >= scrollLine && xPos >= this->m_rect.left && this->m_textInputData.m_showLineNumbers)
							{
								sprintf   (line_string, "%5d", curLine2);
								VidTextOut(line_string, this->m_rect.left + 6, yPos, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
							}
						}
						curLine ++;
					}
					if (*text != '\n')
					{
						if (this->m_textInputData.m_enableSyntaxHilite)
						{
							if (chars_consumed == 0)
							{
								chars_consumed = 1;
								CtlTextInputDoSyntaxHighlight (this, &color, &chars_consumed, text - this->m_textInputData.m_pText);
							}
							chars_consumed--;
						}
						if (this->m_textInputData.m_enableStyling && (unsigned char)(*text) >= (unsigned char)TIST_BOLD && (unsigned char)(*text) < (unsigned char)TIST_COUNT)
						{
							//bool link = false;
							switch (*text)
							{
								case TIST_BOLD:
									color |= 0x01000000;
									break;
								case TIST_UNBOLD:
									color &= ~0x01000000;
									break;
								case TIST_UNFORMAT:
									color =  color_default;//Remove formatting
									break;
								case TIST_RED:
									color =  (color & 0xFF000000) | 0xFF0000;
									break;
								case TIST_GREEN:
									color =  (color & 0xFF000000) | 0x00FF00;
									break;
									
							//#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
								case TIST_LINK:
									//link = true;
								case TIST_BLUE:
									color =  (color & 0xFF000000) | 0x0000FF;
									break;
									
								case TIST_UNLINK:
									//link = false;
								case TIST_UNCOLOR:
									color =  (color & 0xFF000000) | color_default;
									break;
							//#pragma GCC diagnostic pop
							}
						}
						else
						{
							// render this character:
							if (yPos >= this->m_rect.bottom - lineHeight) break;//no point in drawing anymore.
							if (curLine2 >= scrollLine && xPos >= this->m_rect.left)
								VidPlotChar(*text, xPos, yPos, color, TRANSPARENT);
							// Increment the X,Y positions
							xPos += GetCharWidth (*text) + ((color & 0x01000000) != 0);
						}
					}
					
					text++;
					offset++;
				}
				if (offset == this->m_textInputData.m_textCursorIndex && !this->m_textInputData.m_readOnly && this->m_bFocused)
					VidDrawVLine(0xFF, yPos, yPos + lineHeight, xPos);
				
				VidSetFont(SYSTEM_FONT);
			}
			else
				VidTextOut("NOTHING!", this->m_rect.left, this->m_rect.top, 0xFF0000, WINDOW_TEXT_COLOR);
			
			RenderButtonShapeSmallInsideOut (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			
			break;
		}
	}
	return false;
}

#endif

// List View.
#if 1
static void CtlUpdateScrollBarSize(Control *pCtl, Window* pWindow)
{
	ListViewData* pData = &pCtl->m_listViewData;
	int c = pData->m_elementCount;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtl->m_comboID, c);
}
static void CtlAddElementToList (Control* pCtl, const char* pText, int optionalIcon, Window* pWindow)
{
	ListViewData* pData = &pCtl->m_listViewData;
	if (pData->m_elementCount == pData->m_capacity)
	{
		//have to expand first
		int oldSize = sizeof (ListItem) * pData->m_capacity;
		int newSize = oldSize * 2;
		ListItem* pNewItems = MmAllocateK(newSize);
		if (!pNewItems)
			return;
		
		ZeroMemory(pNewItems, newSize);
		memcpy (pNewItems, pData->m_pItems, oldSize);
		MmFreeK (pData->m_pItems);
		pData->m_pItems = pNewItems;
		pData->m_capacity *= 2;
		
		//then can add
	}
	ListItem *pItem = &pData->m_pItems[pData->m_elementCount];
	pData->m_elementCount++;
	pData->m_highlightedElementIdx = -1;
	
	//also update the scroll bar.
	CtlUpdateScrollBarSize(pCtl, pWindow);
	
	pItem->m_icon = optionalIcon;
	strcpy(pItem->m_contents, pText);
}
static void CtlRemoveElementFromList(Control* pCtl, int index, Window* pWindow)
{
	ListViewData* pData = &pCtl->m_listViewData;
	memcpy (pData->m_pItems + index, pData->m_pItems + index + 1, sizeof(ListItem) * (pData->m_elementCount - index - 1));
	pData->m_elementCount--;
	pData->m_highlightedElementIdx = -1;
	
	//also update the scroll bar.
	CtlUpdateScrollBarSize(pCtl, pWindow);
}
static void CtlResetList (Control* pCtl, Window* pWindow)
{
	ListViewData* pData = &pCtl->m_listViewData;
	
	if (pData->m_pItems)
		MmFreeK (pData->m_pItems);
	
	pData->m_highlightedElementIdx = -1;
	pData->m_elementCount = 0;
	pData->m_capacity     = 10;
	int itemsSize         = sizeof (ListItem) * pData->m_capacity;
	pData->m_pItems       = MmAllocateK (itemsSize);
	if (!pData->m_pItems)
	{
		return;
	}
	memset (pData->m_pItems, 0, itemsSize);
	
	//also update the scroll bar.
	CtlUpdateScrollBarSize(pCtl, pWindow);
}
static const char* CtlGetElementStringFromList (Control *pCtl, int index)
{
	ListViewData* pData = &pCtl->m_listViewData;
	
	if (index < 0 || index >= pData->m_elementCount) return NULL;
	
	return pData->m_pItems[index].m_contents;
}
extern VBEData*g_vbeData,g_mainScreenVBEData;
bool WidgetListView_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
go_back:
	switch (eventType)
	{
		case EVENT_SIZE:
		{
			CtlUpdateScrollBarSize (this, pWindow);
			break;
		}
	//#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case EVENT_RELEASECURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains(&this->m_rect, &p))
			{
				// Highlight some element.
				int elementStart =   pData->m_scrollY;
				int elementEnd   =   pData->m_scrollY + (this->m_rect.bottom - this->m_rect.top) / LIST_ITEM_HEIGHT - 1;
				
				if (elementStart >= pData->m_elementCount) elementStart = pData->m_elementCount-1;
				if (elementStart < 0) elementStart = 0;
				
				if (elementEnd < 0) elementEnd = 0;
				if (elementEnd >= pData->m_elementCount) elementEnd = pData->m_elementCount-1;
				
				int yPos = (p.y - 2 - this->m_rect.top) / LIST_ITEM_HEIGHT;
				int elementHighlightAttempt = elementStart + yPos;
				
				if (elementHighlightAttempt >= pData->m_elementCount || elementHighlightAttempt < 0)
					elementHighlightAttempt = -1;
				
				bool isDoubleClick = pData->m_highlightedElementIdx == elementHighlightAttempt;
				pData->m_highlightedElementIdx = elementHighlightAttempt;
				
				// Allow double clicking of elements inside the list.  Will call EVENT_COMMAND to the parent window.
				if (isDoubleClick && elementHighlightAttempt != -1)
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, elementHighlightAttempt);
				
				eventType = EVENT_PAINT;
				goto go_back;
			}
		}
		//fallthrough intentional
		case EVENT_CLICKCURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			int pos = GetScrollBarPos(pWindow, -this->m_comboID);
			if (pData->m_scrollY != pos)
			{
				pData->m_scrollY  = pos;
			}
			else break;
		}
	//#pragma GCC diagnostic pop
		case EVENT_PAINT:
		{
			//draw a green rectangle:
			Rectangle rk = this->m_rect;
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, rk);
			ListViewData* pData = &this->m_listViewData;
			
			int elementStart =   pData->m_scrollY;
			int elementEnd   =   pData->m_scrollY + (this->m_rect.bottom - this->m_rect.top) / LIST_ITEM_HEIGHT - 1;
			
			if (elementStart >= pData->m_elementCount) elementStart = pData->m_elementCount-1;
			if (elementStart < 0) elementStart = 0;
			
			if (elementEnd < 0) elementEnd = 0;
			if (elementEnd >= pData->m_elementCount) elementEnd = pData->m_elementCount-1;
			
			if (elementStart > elementEnd)
				VidDrawText ("(Empty)", rk, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0x7F7F7F, TRANSPARENT);
			
			for (int i = elementStart, j = 0; i <= elementEnd; i++, j++)
			{
				uint32_t color = WINDOW_TEXT_COLOR, colorT = TRANSPARENT;
				if (pData->m_highlightedElementIdx == i)
				{
					/*int l = this->m_rect.left + 4, t = this->m_rect.top + 2 + j * LIST_ITEM_HEIGHT, 
						r = this->m_rect.right - 4, b = t + LIST_ITEM_HEIGHT - 1;
					VidFillRect (0x7F, l, t, r, b);*/
					color = WINDOW_TEXT_COLOR_LIGHT, colorT = 0x7F;
				}
				if (pData->m_hasIcons)
				{
					if (pData->m_pItems[i].m_icon)
						RenderIconForceSize (pData->m_pItems[i].m_icon, this->m_rect.left + 4, this->m_rect.top + 2 + j * LIST_ITEM_HEIGHT, 16);
				}
				VidTextOut (pData->m_pItems[i].m_contents, this->m_rect.left + 4 + pData->m_hasIcons * 24, this->m_rect.top + 4 + 2 + j * LIST_ITEM_HEIGHT, color, colorT);
			}
			
			RenderButtonShapeSmallInsideOut (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			
			break;
		}
		case EVENT_CREATE:
		{
			// Start out with an initial size of 10 elements.
			ListViewData* pData = &this->m_listViewData;
			pData->m_elementCount = 0;
			pData->m_capacity     = 10;
			pData->m_scrollY      = 0;
			pData->m_hasIcons     = true;
			int itemsSize         = sizeof (ListItem) * pData->m_capacity;
			pData->m_pItems       = MmAllocateK (itemsSize);
			if (!pData->m_pItems)
				return false;
			memset (pData->m_pItems, 0, itemsSize);
			
			// Add a vertical scroll bar to its right.
			Rectangle r;
			r.right = this->m_rect.right, 
			r.top   = this->m_rect.top, 
			r.bottom= this->m_rect.bottom, 
			r.left  = this->m_rect.right - SCROLL_BAR_WIDTH;
			
			int c = pData->m_elementCount;
			if (c <= 0)
				c  = 1; 
			
			int flags = 0;
			if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				flags |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
			if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				flags |= ANCHOR_BOTTOM_TO_BOTTOM;
			
			AddControlEx (pWindow, CONTROL_VSCROLLBAR, flags, r, NULL, -this->m_comboID, c, 1);
			
			//shrink our rectangle:
			this->m_rect.right -= SCROLL_BAR_WIDTH + 4;
			
			break;
		}
		case EVENT_DESTROY:
		{
			ListViewData* pData = &this->m_listViewData;
			//free the items first
			if (pData->m_pItems)
			{
				MmFreeK (pData->m_pItems);
				pData->m_pItems = NULL;
			}
			//BUGFIX 23.1.2022 - Do NOT free the listviewdata as it's just a part of the control now!!
			break;
		}
	}
	return false;//Fall through to other controls.
}
#endif

// Icon list view
#if 1
static void CtlIconUpdateScrollBarSize(Control* pCtlIcon, Window* pWindow)
{
	//also update the scroll bar.
	ListViewData* pData = &pCtlIcon->m_listViewData;
	int elementColsPerScreen = (pCtlIcon->m_rect.right - pCtlIcon->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
	int c = pData->m_elementCount / elementColsPerScreen;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtlIcon->m_comboID, c);
}
static void CtlIconAddElementToList (Control* pCtlIcon, const char* pText, int optionalIcon, Window* pWindow)
{
	ListViewData* pData = &pCtlIcon->m_listViewData;
	if (pData->m_elementCount == pData->m_capacity)
	{
		//have to expand first
		int oldSize = sizeof (ListItem) * pData->m_capacity;
		int newSize = oldSize * 2;
		ListItem* pNewItems = MmAllocateK(newSize);
		if (!pNewItems)
			return;
		
		ZeroMemory(pNewItems, newSize);
		memcpy (pNewItems, pData->m_pItems, oldSize);
		MmFreeK (pData->m_pItems);
		pData->m_pItems = pNewItems;
		pData->m_capacity *= 2;
		
		//then can add
	}
	ListItem *pItem = &pData->m_pItems[pData->m_elementCount];
	pData->m_elementCount++;
	pData->m_highlightedElementIdx = -1;
	
	CtlIconUpdateScrollBarSize(pCtlIcon, pWindow);
	
	pItem->m_icon = optionalIcon;
	strcpy(pItem->m_contents, pText);
	
	//WrapText(pItem->m_contents, pText, ICON_ITEM_WIDTH);
}
static void CtlIconRemoveElementFromList(Control* pCtlIcon, int index, Window* pWindow)
{
	ListViewData* pData = &pCtlIcon->m_listViewData;
	memcpy (pData->m_pItems + index, pData->m_pItems + index + 1, sizeof(ListItem) * (pData->m_elementCount - index - 1));
	pData->m_elementCount--;
	pData->m_highlightedElementIdx = -1;
	
	//also update the scroll bar.
	CtlIconUpdateScrollBarSize(pCtlIcon, pWindow);
}
//extern VBEData*g_vbeData,g_mainScreenVBEData;
bool WidgetIconView_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
go_back:
	switch (eventType)
	{
		case EVENT_SIZE:
		{
			CtlIconUpdateScrollBarSize (this, pWindow);
			break;
		}
	//#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case EVENT_RELEASECURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains(&this->m_rect, &p))
			{
				// Highlight some element.
				int elementColsPerScreen = (this->m_rect.right  - this->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
				int elementRowsPerScreen = (this->m_rect.bottom - this->m_rect.top)  / ICON_ITEM_HEIGHT;
				
				int elementStart =   pData->m_scrollY * elementColsPerScreen;
				int elementEnd   =   pData->m_scrollY * elementColsPerScreen + elementRowsPerScreen * elementColsPerScreen-1;
				
				if (elementStart >= pData->m_elementCount) elementStart = pData->m_elementCount-1;
				if (elementStart < 0) elementStart = 0;
				
				if (elementEnd < 0) elementEnd = 0;
				if (elementEnd >= pData->m_elementCount) elementEnd = pData->m_elementCount-1;
				
				int xPos = (p.x - 2 - this->m_rect.left) / ICON_ITEM_WIDTH;
				int yPos = (p.y - 2 - this->m_rect.top)  / ICON_ITEM_HEIGHT;
				int elemId = yPos * elementColsPerScreen + xPos;
				int elementHighlightAttempt = elementStart + elemId;
				
				if (elementHighlightAttempt >= pData->m_elementCount || elementHighlightAttempt < 0)
					elementHighlightAttempt = -1;
				
				bool isDoubleClick = pData->m_highlightedElementIdx == elementHighlightAttempt;
				pData->m_highlightedElementIdx = elementHighlightAttempt;
				
				// Allow double clicking of elements inside the list.  Will call EVENT_COMMAND to the parent window.
				if (isDoubleClick && elementHighlightAttempt != -1)
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, elementHighlightAttempt);
				
				eventType = EVENT_PAINT;
				goto go_back;
			}
		}
		//fallthrough intentional
		case EVENT_CLICKCURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			int pos = GetScrollBarPos(pWindow, -this->m_comboID);
			if (pData->m_scrollY != pos)
			{
				pData->m_scrollY  = pos;
			}
			else break;
		}
	//#pragma GCC diagnostic pop
		case EVENT_PAINT:
		{
			//draw a green rectangle:
			Rectangle rk = this->m_rect;
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, rk);
			ListViewData* pData = &this->m_listViewData;
			
			int elementColsPerScreen = (this->m_rect.right  - this->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
			int elementRowsPerScreen = (this->m_rect.bottom - this->m_rect.top)  / ICON_ITEM_HEIGHT;
			
			int elementStart =   pData->m_scrollY * elementColsPerScreen;
			int elementEnd   =   pData->m_scrollY * elementColsPerScreen + elementRowsPerScreen * elementColsPerScreen-1;
			
			if (elementStart >= pData->m_elementCount) elementStart = pData->m_elementCount-1;
			if (elementStart < 0) elementStart = 0;
			
			if (elementEnd < 0) elementEnd = 0;
			if (elementEnd >= pData->m_elementCount) elementEnd = pData->m_elementCount-1;
			
			if (elementStart > elementEnd)
				VidDrawText ("(Empty)", rk, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0x7F7F7F, TRANSPARENT);
			
			int elementX = 0;
			
			for (int i = elementStart, j = 0, k = 0; i <= elementEnd; i++)
			{
				int x = this->m_rect.left + 4 + elementX, y = this->m_rect.top + 4 + 2 + j * ICON_ITEM_HEIGHT + pData->m_hasIcons * 32;
				uint32_t color = WINDOW_TEXT_COLOR;
				Rectangle br = { x, y, x + ICON_ITEM_WIDTH, y + ICON_ITEM_HEIGHT };
				if (pData->m_highlightedElementIdx == i)
				{
					color = WINDOW_TEXT_COLOR_LIGHT;//, colorT = 0x7F;
					
					int w, h;
					VidTextOutInternal(pData->m_pItems[i].m_contents, 0, 0, 0, 0, true, &w, &h);
					
					int mid = (br.left + br.right) / 2;
					
					VidFillRect(0x7F, mid - w/2 - 2, br.top - 1, mid + w/2, br.top + h + 1);
				}
				if (pData->m_hasIcons)
				{
					if (pData->m_pItems[i].m_icon)
						RenderIconForceSize (pData->m_pItems[i].m_icon, x + (ICON_ITEM_WIDTH - 32) / 2, this->m_rect.top + 2 + j * ICON_ITEM_HEIGHT, 32);
				}
				//VidTextOut (pData->m_pItems[i].m_contents, this->m_rect.left + 4 + elementX, this->m_rect.top + 4 + 2 + j * ICON_ITEM_HEIGHT + pData->m_hasIcons * 32, color, colorT);
				VidDrawText (pData->m_pItems[i].m_contents, br, TEXTSTYLE_HCENTERED, color, TRANSPARENT);
				
				elementX += ICON_ITEM_WIDTH;
				k++;
				if (k >= elementColsPerScreen)
				{
					elementX = 0;
					k = 0;
					j++;
				}
			}
			
			RenderButtonShapeSmallInsideOut (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			
			break;
		}
		case EVENT_CREATE:
		{
			// Start out with an initial size of 10 elements.
			ListViewData* pData = &this->m_listViewData;
			pData->m_elementCount = 0;
			pData->m_capacity     = 10;
			pData->m_scrollY      = 0;
			pData->m_hasIcons     = true;
			int itemsSize         = sizeof (ListItem) * pData->m_capacity;
			pData->m_pItems       = MmAllocateK (itemsSize);
			if (!pData->m_pItems)
				return false;
			
			memset (pData->m_pItems, 0, itemsSize);
			
			// Add a vertical scroll bar to its right.
			Rectangle r;
			r.right = this->m_rect.right, 
			r.top   = this->m_rect.top, 
			r.bottom= this->m_rect.bottom, 
			r.left  = this->m_rect.right - SCROLL_BAR_WIDTH;
			
			int c = pData->m_elementCount;
			if (c <= 0)
				c  = 1; 
			
			int flags = 0;
			if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				flags |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
			if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				flags |= ANCHOR_BOTTOM_TO_BOTTOM;
			
			AddControlEx (pWindow, CONTROL_VSCROLLBAR, flags, r, NULL, -this->m_comboID, c, 1);
			
			//shrink our rectangle:
			this->m_rect.right -= SCROLL_BAR_WIDTH + 4;
			
			break;
		}
		case EVENT_DESTROY:
		{
			ListViewData* pData = &this->m_listViewData;
			//free the items first
			if (pData->m_pItems)
			{
				MmFreeK (pData->m_pItems);
				pData->m_pItems = NULL;
			}
			//BUGFIX 23.1.2022 - Do NOT free the listviewdata as it's just a part of the control now!!
			break;
		}
	}
	return false;//Fall through to other controls.
}
#endif

// List view modifiers
#if 1

void AddElementToList (Window* pWindow, int comboID, const char* pText, int optionalIcon)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			if (pWindow->m_pControlArray[i].m_type == CONTROL_ICONVIEW)
				CtlIconAddElementToList (&pWindow->m_pControlArray[i], pText, optionalIcon, pWindow);
			else
				CtlAddElementToList     (&pWindow->m_pControlArray[i], pText, optionalIcon, pWindow);
			return;
		}
	}
}
const char* GetElementStringFromList (Window* pWindow, int comboID, int index)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return CtlGetElementStringFromList (&pWindow->m_pControlArray[i], index);
		}
	}
	return NULL;
}
void RemoveElementFromList (Window* pWindow, int comboID, int elementIndex)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			if (pWindow->m_pControlArray[i].m_type == CONTROL_ICONVIEW)
				CtlIconRemoveElementFromList (&pWindow->m_pControlArray[i], elementIndex, pWindow);
			else
				CtlRemoveElementFromList     (&pWindow->m_pControlArray[i], elementIndex, pWindow);
			return;
		}
	}
}
void ResetList (Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlResetList (&pWindow->m_pControlArray[i], pWindow);
			return;
		}
	}
}
#endif

// Menu Bar.
#if 1

// If a menu bar item has no children, clicking it will trigger an EVENT_COMMAND to the parent window
// with the comboID the menu bar item was given.

// Note that having multiple menu items with the same comboID, just like having more than one control with
// the same comboID, is undefined as per specification.

// Please note that the menu bar comboIDs are not necessarily related to the control comboIDs normally.
// Instead, when you click on a menu bar item, it fires an EVENT_COMMAND with the host control's comboID
// in parm1, and the menu item's comboID in parm2.

#define MENU_BAR_HEIGHT 15

void WidgetMenuBar_InitializeMenuBarItemAsEmpty (MenuBarTreeItem* this, int comboID)
{
	// Call the generic initializor for the menu bar tree item.
	this->m_comboID          = comboID;
	this->m_childrenCount    = 0;
	this->m_childrenCapacity = 4;
	this->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * sizeof (MenuBarTreeItem));
	if (!this->m_childrenArray)
	{
		this->m_childrenCapacity = 0;
		return;
	}
	memset (this->m_childrenArray, 0, 4 * sizeof (MenuBarTreeItem));
	this->m_text[0]          = 0;
	this->m_isOpen           = false;
}
bool WidgetMenuBar_TryAddItemTo (MenuBarTreeItem* this, int comboID_to, int comboID_as, const char* text)
{
	if (this->m_comboID == comboID_to)
	{
		// Can it fit now?
		if (this->m_childrenCount >= this->m_childrenCapacity)
		{
			// Doesn't fit.  Need to expand.
			MenuBarTreeItem* new = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * 2 * sizeof (MenuBarTreeItem));
			if (!new) return false;
			memset (new, 0, this->m_childrenCapacity * 2 * sizeof (MenuBarTreeItem));
			memcpy (new, this->m_childrenArray, this->m_childrenCapacity * sizeof (MenuBarTreeItem));
			this->m_childrenCapacity *= 2;
			
			// Get rid of the old one.  We've moved.
			MmFreeK (this->m_childrenArray);
			
			this->m_childrenArray = new;
		}
		
		//If we've expanded or otherwise can fit our new item into, add it.
		MenuBarTreeItem* pItem = this->m_childrenArray + this->m_childrenCount;
		this->m_childrenCount++;
		
		strcpy (pItem->m_text, text);
		pItem->m_comboID = comboID_as;
		pItem->m_childrenCount = 0;
		
		//Allocate a default of 2 items.
		pItem->m_childrenCapacity = 2;
		pItem->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (2 * sizeof (MenuBarTreeItem));
		if (!pItem->m_childrenArray) return false;
		memset (pItem->m_childrenArray, 0, 2 * sizeof (MenuBarTreeItem));
		
		//We were able to add it.  Leave and tell the caller that we did it.
		return true;
	}
	else
	{
		//try adding it to one of the children:
		for (int i = 0; i < this->m_childrenCount; i++)
		{
			if (WidgetMenuBar_TryAddItemTo(&this->m_childrenArray[i], comboID_to, comboID_as, text))
				return true;//just added it, no need to add it anymore
		}
		return false;//couldn't add here, try another branch
	}
}
void WidgetMenuBar_AddMenuBarItem (Control* this, int comboID_to, int comboID_as, const char* text)
{
	WidgetMenuBar_TryAddItemTo(&this->m_menuBarData.m_root, comboID_to, comboID_as, text);
}
void WidgetMenuBar_InitializeRoot (Control* this)
{
	// Call the generic initializor for the menu bar tree item with a comboID of zero.
	WidgetMenuBar_InitializeMenuBarItemAsEmpty (&this->m_menuBarData.m_root, 0);
}

void WidgetMenuBar_DeInitializeChild(MenuBarTreeItem *this)
{
	// deinitialize the children first.
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		WidgetMenuBar_DeInitializeChild(&this->m_childrenArray[i]);
	}
	
	// then, deinitialize this
	MmFreeK(this->m_childrenArray);
}

void WidgetMenuBar_DeInitializeRoot(Control* this)
{
	WidgetMenuBar_DeInitializeChild (&this->m_menuBarData.m_root);
}

bool WidgetMenuBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	bool has3dBorder = !(pWindow->m_flags & WF_FLATBORD);
	
	Rectangle menu_bar_rect;
	menu_bar_rect.left   = 1 + 3 * has3dBorder;
	menu_bar_rect.top    = has3dBorder * 2 + TITLE_BAR_HEIGHT;
	menu_bar_rect.right  = pWindow->m_vbeData.m_width - 1 - 3 * has3dBorder;
	menu_bar_rect.bottom = menu_bar_rect.top + MENU_BAR_HEIGHT + 3;
	
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			// Initialize the root.
			WidgetMenuBar_InitializeRoot(this);
			break;
		}
		case EVENT_DESTROY:
		{
			WidgetMenuBar_DeInitializeRoot(this);
			break;
		}
		case EVENT_PAINT:
		{
			// Render the root.  If any children are opened, draw them.
			VidFillRectangle (BUTTONMIDD, menu_bar_rect);
			VidDrawHLine (0, menu_bar_rect.left, menu_bar_rect.right, menu_bar_rect.bottom);
			
			if (this->m_menuBarData.m_root.m_childrenArray)
			{
				int current_x = 8;
				for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
				{
					int width, height;
					MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
					const char* pText = pChild->m_text;
					VidTextOutInternal (pText, 0, 0, 0, 0, true, &width, &height);
					
					width += 10;
					
					if (pChild->m_isOpen)
					{
						Rectangle rect;
						rect.left   = menu_bar_rect.left + current_x;
						rect.right  = menu_bar_rect.left + current_x + width;
						rect.top    = menu_bar_rect.top  + 1;
						rect.bottom = menu_bar_rect.bottom - 2;
						
						VidFillRectangle (0x7F, rect);
						
						VidTextOut (pText, menu_bar_rect.left + current_x + 5, menu_bar_rect.top + 2 + (MENU_BAR_HEIGHT - GetLineHeight()) / 2, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
						//render the child menu as well:
						
						/*WidgetMenuBar_RenderSubMenu (pChild, rect.left, rect.bottom);*/
					}
					else
						VidTextOut (pText, menu_bar_rect.left + current_x + 5, menu_bar_rect.top + 2 + (MENU_BAR_HEIGHT - GetLineHeight()) / 2, WINDOW_TEXT_COLOR, TRANSPARENT);
					
					current_x += width;
				}
			}
			
			break;
		}
		//case EVENT_CLICKCURSOR:
		case EVENT_RELEASECURSOR:
		{
			Point p = {GET_X_PARM(parm1), GET_Y_PARM(parm1)};
			// Determine what item we've clicked.
			if (this->m_menuBarData.m_root.m_childrenArray)
			{
				int  current_x = 8;
				bool needsUpdate = false;
				for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
				{
					int width, height;
					MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
					const char* pText = pChild->m_text;
					VidTextOutInternal (pText, 0, 0, 0, 0, true, &width, &height);
					
					width += 10;
					
					Rectangle rect;
					rect.left   = menu_bar_rect.left + current_x;
					rect.right  = menu_bar_rect.left + current_x + width;
					rect.top    = menu_bar_rect.top  + 2;
					rect.bottom = menu_bar_rect.bottom;
					
					if (RectangleContains (&rect, &p))
					{
						for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
						{
							MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
							pChild->m_isOpen = false;
						}
						// Open this and call the paint event.
						
						if (pChild->m_childrenCount)
						{
							pChild->m_isOpen = true;
							WindowMenu menu;
							ConvertMenuBarToWindowMenu(&menu, pChild, this->m_comboID);
							SpawnMenu(pWindow, &menu, pWindow->m_rect.left + rect.left, pWindow->m_rect.top + rect.top + TITLE_BAR_HEIGHT);
							MenuRecursivelyFreeEntries (&menu);
						}
						else
						{
							CallWindowCallback (pWindow, EVENT_COMMAND, this->m_comboID, pChild->m_comboID);
						}
						
						//WidgetMenuBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
						//break;
						needsUpdate = true;
					}
					
					if (pChild->m_isOpen)
					{
						needsUpdate = true;
					}
					
					current_x += width;
					if (needsUpdate) break;
				}
				if (needsUpdate)
					//CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
					RequestRepaintNew(pWindow);
			}
			break;
		}
		case EVENT_MENU_CLOSE:
		{
			if (parm1 == this->m_comboID)
			{
				for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
				{
					MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
					pChild->m_isOpen = false;
				}
				RequestRepaintNew(pWindow);
			}
			break;
		}
		/*case EVENT_RELEASECURSOR:
		{
			// Unopen all the controls. TODO
			if (this->m_menuBarData.m_root.m_childrenArray)
			{
				bool check = WidgetMenuBar_CheckOpenChildrenAndSendCommandEvent(&this->m_menuBarData.m_root, this, pWindow);
				
				CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
				
				return check;
			}
			break;
		}*/
	}
	return false;//Fall through to other controls.
}

// Works on the control with the comboID of 'menuBarControlId'.
// To that control, it adds a menu item with the comboID of 'comboIdAs' to the menu item with the comboID of 'comboIdTo'.
void AddMenuBarItem (Window* pWindow, int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == menuBarControlId && pWindow->m_pControlArray[i].m_type == CONTROL_MENUBAR)
		{
			WidgetMenuBar_AddMenuBarItem (&pWindow->m_pControlArray[i], comboIdTo, comboIdAs, pText);
			return;
		}
	}
}

#endif

// Misc setters
#if 1

void SetLabelText (Window *pWindow, int comboID, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			strcpy(pWindow->m_pControlArray[i].m_text, pText);
			return;
		}
	}
}
void SetIcon (Window *pWindow, int comboID, int icon)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_parm1 = icon;
			return;
		}
	}
}

#endif

// Basic controls
#if 1
bool WidgetNone_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	return false;
}
bool WidgetText_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
			VidTextOut(this->m_text, this->m_rect.left, this->m_rect.top, this->m_parm1, this->m_parm2);
			break;
	}
	return false;
}
bool WidgetTextCenter_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			if (this->m_parm2 & TEXTSTYLE_FORCEBGCOL)
			{
				VidFillRectangle(WINDOW_BACKGD_COLOR, this->m_rect);
			}
			VidDrawText(this->m_text, this->m_rect, this->m_parm2, this->m_parm1, TRANSPARENT);
			break;
		}
	}
	return false;
}
bool WidgetTextHuge_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
			this->m_dataPtr = NULL;
			break;
		case EVENT_PAINT:
			if (this->m_dataPtr)
				VidDrawText((const char*)this->m_dataPtr, this->m_rect, this->m_parm2, this->m_parm1, TRANSPARENT);
			break;
		case EVENT_DESTROY:
			if (this->m_dataPtr)
				MmFreeK(this->m_dataPtr);
			this->m_dataPtr = NULL;
			break;
	}
	return false;
}
void SetHugeLabelText (Window *pWindow, int comboID, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			//strcpy(pWindow->m_pControlArray[i].m_text, pText);
			char* ptr = (char*)pWindow->m_pControlArray[i].m_dataPtr;
			if (ptr)
				MmFreeK(ptr);
			ptr = MmAllocateK(strlen(pText)+1);
			pWindow->m_pControlArray[i].m_dataPtr = ptr;
			strcpy (ptr, pText);
			
			return;
		}
	}
}

bool WidgetIcon_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
			RenderIcon(this->m_parm1, this->m_rect.left + (this->m_rect.right - this->m_rect.left - 32) / 2, this->m_rect.top + (this->m_rect.bottom - this->m_rect.top - 32) / 2);
			break;
	}
	return false;
}
bool WidgetButton_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
			{
				//send a command event to the window:
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
			}
			this->m_buttonData.m_clicked = false;
			WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
			{
				this->m_buttonData.m_clicked = true;
				WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_KILLFOCUS:
			this->m_buttonData.m_hovered = false;
			if (g_GlowOnHover)
				WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		case EVENT_MOVECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p))
			{
				if (!this->m_buttonData.m_hovered)
				{
					this->m_buttonData.m_hovered = true;
					if (g_GlowOnHover)
						WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			else if (this->m_buttonData.m_hovered)
			{
				this->m_buttonData.m_hovered = false;
				if (g_GlowOnHover)
					WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_PAINT:
		{
			VidSetClipRect (&this->m_rect);
			if (this->m_buttonData.m_clicked)
			{
				Rectangle r = this->m_rect;
				//draw the button as slightly pushed in
				r.left++; r.right++; r.bottom++; r.top++;
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else if (this->m_buttonData.m_hovered && g_GlowOnHover)
			{
				uint32_t bm = BUTTONMIDD;
				if (bm > 0xDFDFDF)//avoid overflow
					bm = 0xDFDFDF;
				bm += 0x202020;
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, bm);
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			VidSetClipRect (NULL);
			
			break;
		}
	}
	return false;
}
bool WidgetButtonIcon_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
			{
				//send a command event to the window:
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
			}
			this->m_buttonData.m_clicked = false;
			WidgetButtonIcon_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
			{
				this->m_buttonData.m_clicked = true;
				WidgetButtonIcon_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_MOVECURSOR:
		{
			break;
		}
		case EVENT_PAINT:
		{
			int x = this->m_rect.left + (this->m_rect.right  - this->m_rect.left - this->m_parm2) / 2;
			int y = this->m_rect.top  + (this->m_rect.bottom - this->m_rect.top  - this->m_parm2) / 2;
			
			if (this->m_buttonData.m_clicked)
			{
				x++, y++;
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			}
			
			RenderIconForceSize (this->m_parm1, x, y, this->m_parm2);
			
			break;
		}
	}
	return false;
}
bool WidgetButtonIconBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
			{
				//send a command event to the window:
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
			}
			this->m_buttonData.m_clicked = false;
			WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
			{
				this->m_buttonData.m_clicked = true;
				WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_MOVECURSOR:
		{
			break;
		}
		case EVENT_PAINT:
		{
			int x = this->m_rect.left + (this->m_rect.right  - this->m_rect.left - this->m_parm2) / 2;
			int y = this->m_rect.top  + (this->m_rect.bottom - this->m_rect.top  - this->m_parm2) / 2;
			x++, y++;
			
			if (this->m_buttonData.m_clicked)
			{
				x++, y++;
				RenderButtonShapeSmall (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
			}
			else
			{
				//RenderButtonShapeSmall (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				VidFillRectangle(WINDOW_BACKGD_COLOR, this->m_rect);
			}
			
			RenderIconForceSize (this->m_parm1, x, y, this->m_parm2);
			
			break;
		}
	}
	return false;
}
bool WidgetButtonList_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
			{
				//send a command event to the window:
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
			}
			this->m_buttonData.m_clicked = false;
			WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
			{
				this->m_buttonData.m_clicked = true;
				WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_KILLFOCUS:
			this->m_buttonData.m_hovered = false;
			if (g_GlowOnHover)
				WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		case EVENT_MOVECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p))
			{
				if (!this->m_buttonData.m_hovered)
				{
					this->m_buttonData.m_hovered = true;
					if (g_GlowOnHover)
						WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			else if (this->m_buttonData.m_hovered)
			{
				this->m_buttonData.m_hovered = false;
				if (g_GlowOnHover)
					WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_PAINT:
		{
			Rectangle r = this->m_rect;
			
			int offs = 20;
			if (this->m_parm1) offs += 10;
			
			if (this->m_buttonData.m_clicked)
			{
				//draw the button as slightly pushed in
				VidFillRectangle(0x7F, r);
				r.left++; r.right++; r.bottom++; r.top++;
				r.left += offs;
				r.top += 1;
				r.bottom += 1;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
			}
			else if (this->m_buttonData.m_hovered && g_GlowOnHover)
			{
				VidFillRectangle(0x7F, r);
				r.left += offs;
				r.top += 1;
				r.bottom += 1;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
			}
			else
			{
				VidFillRectangle(WINDOW_BACKGD_COLOR, r);
				r.left += offs;
				r.top += 1;
				r.bottom += 1;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			r.left -= offs;
			if (this->m_parm1)
				RenderIconForceSize (this->m_parm1, r.left + 4, r.top + (r.bottom - r.top - 16) / 2, 16);
			
			break;
		}
	}
	return false;
}
bool WidgetButtonColor_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
			{
				//send a command event to the window:
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
			}
			this->m_buttonData.m_clicked = false;
			WidgetButtonColor_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
			{
				this->m_buttonData.m_clicked = true;
				WidgetButtonColor_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_PAINT:
		{
			if (this->m_buttonData.m_clicked)
			{
				Rectangle r = this->m_rect;
				//draw the button as slightly pushed in
				r.left++; r.right++; r.bottom++; r.top++;
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, this->m_parm2);
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, this->m_parm1, TRANSPARENT);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, this->m_parm2);
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, this->m_parm1, TRANSPARENT);
			}
			
			break;
		}
	}
	return false;
}
//for the top bar of the window.  Uses this->m_parm1 as the event type.
bool WidgetActionButton_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
			{
				//send a command event to the window:
				//CallWindowCallback(pWindow, this->m_parm1, this->m_comboID, this->m_parm2);
				WindowRegisterEventUnsafe(pWindow, this->m_parm1, this->m_comboID, this->m_parm2);
			}
			this->m_buttonData.m_clicked = false;
			WidgetActionButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
			{
				this->m_buttonData.m_clicked = true;
				WidgetActionButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_PAINT:
		{
			//draw a green rectangle:
			if (this->m_buttonData.m_clicked)
			{
				Rectangle r = this->m_rect;
				//draw the button as slightly pushed in
				r.left++; r.right++; r.bottom++; r.top++;
				RenderButtonShapeSmall (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else
			{
				this->m_rect.right--;
				RenderButtonShapeSmall (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				this->m_rect.right++;//ugly hack
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			
			break;
		}
	}
	return false;
}
bool WidgetClickLabel_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
	//#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case EVENT_RELEASECURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p))
			{
				//send a command event to the window:
				//WindowRegisterEvent(pWindow, EVENT_COMMAND, this->m_parm1, this->m_parm2);
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
			}
		}
		//! fallthrough intentional - need the button to redraw itself as pushing back up
		case EVENT_PAINT:
	//#pragma GCC diagnostic pop
		{
			//then fill in the text:
			VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_VCENTERED, 0x1111FF, TRANSPARENT);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle r = this->m_rect;
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&r, &p))
			{
				//then fill in the text:
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_VCENTERED, 0x11, TRANSPARENT);
			}
			break;
		}
	}
	return false;
}
bool WidgetSurroundRect_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			// Draw a rectangle to surround the things we put inside
			Rectangle r = this->m_rect;
			r.top += GetLineHeight() / 2;
			
			VidDrawRectangle(WINDOW_TEXT_COLOR, r);
			
			// Draw the text
			VidTextOut(this->m_text, this->m_rect.left + 10, this->m_rect.top, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);
			VidTextOut(this->m_text, this->m_rect.left + 12, this->m_rect.top, 0x00000,             WINDOW_BACKGD_COLOR);
			
			break;
		}
	}
	return false;
}
bool WidgetSimpleLine_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			// Draw a rectangle to surround the things we put inside
			VidDrawHLine(WINDOW_BACKGD_COLOR - 0x0F0F0F, this->m_rect.left + 8, this->m_rect.right - 8, (this->m_rect.top + this->m_rect.bottom) / 2);
			
			break;
		}
	}
	return false;
}

void SetImageCtlMode (Window *pWindow, int comboID, int mode)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_parm2 = mode;
			return;
		}
	}
}
void SetImageCtlColor (Window *pWindow, int comboID, uint32_t color)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_imageCtlData.nCurrentColor = color;
			return;
		}
	}
}
void SetImageCtlCurrentImage (Window *pWindow, int comboID, Image* pImage)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			// Force a refresh of the image.
			pWindow->m_pControlArray[i].m_parm1 = (int)pImage;
			if (pWindow->m_pControlArray[i].m_imageCtlData.pImage)
				MmFree(pWindow->m_pControlArray[i].m_imageCtlData.pImage);
			pWindow->m_pControlArray[i].m_imageCtlData.pImage = NULL;
			
			WindowRegisterEventUnsafe (pWindow, EVENT_IMAGE_REFRESH, 0, 0);
			
			return;
		}
	}
}
Image* GetImageCtlCurrentImage (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return pWindow->m_pControlArray[i].m_imageCtlData.pImage;
		}
	}
	return NULL;
}
void ImageCtlZoomToFill (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_imageCtlData.nZoomedWidth = GetWidth (&pWindow->m_pControlArray[i].m_rect);
		}
	}
}
bool WidgetImage_OnEvent(Control* this, int eventType, int parm1, UNUSED int parm2, Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		case EVENT_IMAGE_REFRESH:
		{
			if (!this->m_imageCtlData.pImage)
			{
				Image* pImageToCopy = (Image*)this->m_parm1;
				
				size_t n_pixels = pImageToCopy->width * pImageToCopy->height;
				size_t new_size = sizeof (Image) + n_pixels * sizeof (uint32_t);
				
				Image* pNewImage    = (Image*)MmAllocateK(new_size);
				pNewImage->framebuffer = (uint32_t*)((uint8_t*)pNewImage + sizeof (Image));
				
				//yes I am aware that I break the const-ness rule, but it's ok because we're supposed to do initialization
				memcpy_ints ((uint32_t*)pNewImage->framebuffer, pImageToCopy->framebuffer, n_pixels);
				pNewImage->width  = pImageToCopy->width;
				pNewImage->height = pImageToCopy->height;
				
				this->m_imageCtlData.pImage = pNewImage;
				
				this->m_imageCtlData.nZoomedWidth  = pNewImage->width;
			}
			this->m_imageCtlData.nLastXGot = -1;
			this->m_imageCtlData.nLastYGot = -1;
			break;
		}
		case EVENT_DESTROY:
		{
			if (this->m_imageCtlData.pImage)
			{
				MmFree(this->m_imageCtlData.pImage);
				this->m_imageCtlData.pImage = NULL;
			}
			break;
		}
		case EVENT_PAINT:
		{
			// Draw a rectangle to surround the things we put inside
			//VidDrawHLine(WINDOW_BACKGD_COLOR - 0x0F0F0F, this->m_rect.left + 8, this->m_rect.right - 8, (this->m_rect.top + this->m_rect.bottom) / 2);
			
			VidSetClipRect(&this->m_rect);
			
			Image *pImage = (Image*)this->m_imageCtlData.pImage;
			if (pImage)
			{
				bool ogSize = false;
				int  width  = pImage->width;
				int  height = pImage->height;
				
				if (this->m_imageCtlData.nZoomedWidth != this->m_imageCtlData.pImage->width)
				{
					ogSize = false;
					width  = this->m_imageCtlData.nZoomedWidth;
					
					uint32_t  tmp = height;
					tmp *= this->m_imageCtlData.nZoomedWidth;
					tmp /= this->m_imageCtlData.pImage->width;
					
					height = (int)tmp;
				}
				
				int x = (GetWidth (&this->m_rect) - width)  / 2;
				int y = (GetHeight(&this->m_rect) - height) / 2;
				
				if (this->m_parm2 & IMAGECTL_PAN)
				{
					x += this->m_imageCtlData.nCurPosX;
					y += this->m_imageCtlData.nCurPosY;
				}
				
				x += this->m_rect.left;
				y += this->m_rect.top;
				
				if (x <= this->m_rect.right && y <= this->m_rect.bottom && x + width >= this->m_rect.left && y + height >= this->m_rect.top)
				{
					if (GetWidth(&this->m_rect) > this->m_imageCtlData.nZoomedWidth)
					{
						VidFillRectangle(0x3f0000, this->m_rect);
					}
					if (ogSize)
						VidBlitImage (pImage, x, y);
					else
						VidBlitImageResize(pImage, x, y, width, height);
				}
				else
				{
					if (!parm2)
					{
						VidFillRectangle(0x3f0000, this->m_rect);
					}
					VidDrawText ("(Out of View)", this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0xFF0000, TRANSPARENT);
				}
			}
			else
			{
				if (!parm2)
				{
					VidFillRectangle(0x3f0000, this->m_rect);
				}
				VidDrawText ("(No Image)", this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0xFF0000, TRANSPARENT);
			}
			
			VidSetClipRect(NULL);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (this->m_parm2  & (IMAGECTL_PAN | IMAGECTL_PEN))
			{
				if (RectangleContains(&this->m_rect, &p))
				{
					int deltaX = 0, deltaY = 0;
					if (this->m_imageCtlData.nLastXGot != -1)
					{
						deltaX = p.x - this->m_imageCtlData.nLastXGot;
						deltaY = p.y - this->m_imageCtlData.nLastYGot;
					}
					else
					{
						this->m_imageCtlData.nLastXGot = p.x;
						this->m_imageCtlData.nLastYGot = p.y;
					}
					
					bool dragged = false;
					if (this->m_parm2 & IMAGECTL_PAN)
					{
						if (!(this->m_parm2 & IMAGECTL_PEN) || KbGetKeyState(KEY_CONTROL) == KEY_PRESSED)
						{
							this->m_imageCtlData.nCurPosX += deltaX;
							this->m_imageCtlData.nCurPosY += deltaY;
							
							dragged = true;
						}
					}
					if ((this->m_parm2 & IMAGECTL_PEN) && !dragged)
					{
						int lx = (GetWidth (&this->m_rect) - this->m_imageCtlData.pImage->width)  / 2;
						int ly = (GetHeight(&this->m_rect) - this->m_imageCtlData.pImage->height) / 2;
						
						if (this->m_parm2 & IMAGECTL_PAN)
						{
							lx += this->m_imageCtlData.nCurPosX;
							ly += this->m_imageCtlData.nCurPosY;
						}
						
						lx += this->m_rect.left;
						ly += this->m_rect.top;
						
						VBEData *pBackup = g_vbeData, m_gdc;
						
						BuildGraphCtxBasedOnImage(&m_gdc, this->m_imageCtlData.pImage);
						
						VidSetVBEData(&m_gdc);
						
						//VidBlitImage(GetIconImage (ICON_COMPUTER_PANIC, 96),50, 50);
						VidDrawLine(this->m_imageCtlData.nCurrentColor, this->m_imageCtlData.nLastXGot - lx, this->m_imageCtlData.nLastYGot - ly, p.x - lx, p.y - ly);
						
						VidSetVBEData(pBackup);
					}
					
					this->m_imageCtlData.nLastXGot = p.x;
					this->m_imageCtlData.nLastYGot = p.y;
					
					WidgetImage_OnEvent(this, EVENT_PAINT, 0, !dragged, pWindow);
				}
			}
			else
			{
				this->m_imageCtlData.nCurPosX = 0;
				this->m_imageCtlData.nCurPosY = 0;
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			this->m_imageCtlData.nLastXGot = this->m_imageCtlData.nLastYGot = -1;
			break;
		}
	}
	return false;
}
#define CHECKBOX_SIZE 14
bool CheckboxGetChecked(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return pWindow->m_pControlArray[i].m_checkBoxData.m_checked;
		}
	}
	return false;
}
void CheckboxSetChecked(Window* pWindow, int comboID, bool checked)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_checkBoxData.m_checked = checked;
		}
	}
}
bool WidgetCheckbox_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	// this->rect only affects the top left position
	
	Rectangle check_rect = this->m_rect;
	check_rect.right  = check_rect.left + CHECKBOX_SIZE;
	check_rect.bottom = check_rect.top  + CHECKBOX_SIZE;
	
	Rectangle text_rect = this->m_rect;
	text_rect.left =  check_rect.right + 6;
	text_rect.top  += (check_rect.bottom - check_rect.top - GetLineHeight()) / 2 + 1;
	
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			//VidFillRectangle(this->m_checkBoxData.m_clicked ? 0xcccccc : WINDOW_TEXT_COLOR_LIGHT, check_rect);
			//VidDrawRectangle(WINDOW_TEXT_COLOR,  
			//RenderButtonShapeSmallInsideOut (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			check_rect.right--;
			//check_rect.bottom--;
			RenderButtonShapeSmallInsideOut (check_rect, 0xBFBFBF, BUTTONDARK, this->m_checkBoxData.m_clicked ? 0xcccccc : WINDOW_TEXT_COLOR_LIGHT);
			
			//if checked, mark it as "checked"
			if (this->m_checkBoxData.m_checked)
			{
				VidTextOut("\x15", check_rect.left + 3, check_rect.top + 3, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			
			VidDrawText(this->m_text, text_rect, TEXTSTYLE_WORDWRAPPED, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&check_rect, &p) || RectangleContains (&text_rect, &p))
			{
				this->m_checkBoxData.m_clicked = true;
				WidgetCheckbox_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			bool contains = (RectangleContains (&check_rect, &p) || RectangleContains (&text_rect, &p));
			if ( this->m_checkBoxData.m_clicked )
			{
				if (contains)
					this->m_checkBoxData.m_checked ^= 1;
				this->m_checkBoxData.m_clicked = false;
				
				CallWindowCallback(pWindow, EVENT_CHECKBOX, this->m_comboID, this->m_checkBoxData.m_checked);
				WidgetCheckbox_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			
			break;
		}
	}
	
	return false;
}

extern Window g_windows[WINDOWS_MAX];
extern short  g_windowDrawOrder[WINDOWS_MAX];

void SelectWindow (Window *pWnd);

bool g_TaskListCompact = false;

bool WidgetTaskList_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			VidFillRectangle (WINDOW_BACKGD_COLOR, this->m_rect);
			
			int nWindows = 0;
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (!nWindows) break;
			
			int btn_width = GetWidth (&this->m_rect) / nWindows;
			
			int btn_max_width = g_TaskListCompact ? 28 : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left;
			
			
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				
				
				Rectangle r;
				RECT (r, btn_x, this->m_rect.top, btn_width - 4, GetHeight(&this->m_rect));
				
				bool clicked = this->m_taskListData.m_clicked[i];
				bool hovered = this->m_taskListData.m_hovered[i];
				
				uint32_t flgs = 0;
				
				if (g_windows[i].m_isSelected)
				{
					RenderButtonShape(r, BUTTONLITE, BUTTONDARK, BUTTONMIDD);
					
					flgs |= TEXT_RENDER_BOLD;
				}
				else if (clicked)
				{
					RenderButtonShape(r, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				}
				else if (hovered)
				{
					uint32_t bm = BUTTONMIDD;
					if (bm > 0xDFDFDF)//avoid overflow
						bm = 0xDFDFDF;
					bm += 0x202020;
					RenderButtonShape(r, BUTTONDARK, BUTTONLITE, bm);
				}
				else
					RenderButtonShape(r, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				
				Window *pWnd = &g_windows[i];
				
				int textX = btn_x + 4;
				// if this window has an icon:
				if (pWnd->m_iconID)
				{
					RenderIconForceSize(pWnd->m_iconID, textX + clicked, this->m_rect.top + 3 + clicked, 16);
					textX += 22; 
				}
				
				if (!g_TaskListCompact)
				{
					VidSetClipRect (&r);
					
					//VidTextOut (pWnd->m_title, textX, this->m_rect.top + 5, WINDOW_TEXT_COLOR, TRANSPARENT);
					
					RECT(r, textX + clicked, this->m_rect.top + 2 + clicked, btn_x + btn_width - textX - 2, GetHeight(&this->m_rect) - 4);
					
					VidDrawText (pWnd->m_title, r, TEXTSTYLE_VCENTERED, FLAGS_TOO(flgs, WINDOW_TEXT_COLOR), TRANSPARENT);
					
					VidSetClipRect (NULL);
				}
				
				btn_x += btn_width;
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		case EVENT_CLICKCURSOR:
		case EVENT_MOVECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			int nWindows = 0;
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (!nWindows) break;
			
			int btn_width = GetWidth (&this->m_rect) / nWindows;
			
			int btn_max_width = g_TaskListCompact ? 28 : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left;
			
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				
				this->m_taskListData.m_hovered[i] = false;
				
				Rectangle r;
				RECT (r, btn_x, this->m_rect.top, btn_width - 4, GetHeight(&this->m_rect));
				
				if (RectangleContains (&r, &mouseClickPos))
				{
					if (eventType == EVENT_MOVECURSOR)
					{
						this->m_taskListData.m_hovered[i] = true;
					}
					else if (eventType == EVENT_CLICKCURSOR)
					{
						this->m_taskListData.m_clicked[i] = true;
					}
					else if (this->m_taskListData.m_clicked[i]) // EVENT_RELEASECURSOR
					{
						// switch to this window
						for (int j = 0; j < WINDOWS_MAX; j++)
							this->m_taskListData.m_clicked[j] = false;
						
						SelectWindow (&g_windows[i]);
					}
					
					if (eventType != EVENT_MOVECURSOR || g_GlowOnHover)
						WidgetTaskList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
					
					if (eventType != EVENT_MOVECURSOR)
						break;
				}
				
				btn_x += btn_width;
			}
			
			break;
		}
	}
	
	return false;
}
#endif

WidgetEventHandler g_widgetEventHandlerLUT[] = {
	WidgetNone_OnEvent,
	WidgetText_OnEvent,
	WidgetIcon_OnEvent,
	WidgetButton_OnEvent,
	WidgetTextEditView_OnEvent,
	WidgetCheckbox_OnEvent,
	WidgetClickLabel_OnEvent,
	WidgetTextCenter_OnEvent,
	WidgetActionButton_OnEvent,
	WidgetListView_OnEvent,
	WidgetVScrollBar_OnEvent,
	WidgetHScrollBar_OnEvent,
	WidgetMenuBar_OnEvent,
	WidgetTextHuge_OnEvent,
	WidgetIconView_OnEvent,
	WidgetSurroundRect_OnEvent,
	WidgetButtonColor_OnEvent,
	WidgetButtonList_OnEvent,
	WidgetButtonIcon_OnEvent,
	WidgetButtonIconBar_OnEvent,
	WidgetSimpleLine_OnEvent,
	WidgetImage_OnEvent,
	WidgetTaskList_OnEvent,
};

STATIC_ASSERT(ARRAY_COUNT(g_widgetEventHandlerLUT) == CONTROL_COUNT, "Change this array if adding widgets");

WidgetEventHandler GetWidgetOnEventFunction (int type)
{
	if (type < 0 || type >= CONTROL_COUNT)
		return NULL;
	return g_widgetEventHandlerLUT[type];
}
