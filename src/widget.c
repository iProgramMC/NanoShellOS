/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

           Shell widget module
******************************************/
#include <widget.h>
#include <video.h>
#include <icon.h>
#include <print.h>
#include <misc.h>
#include <keyboard.h>

// Utilitary functions
#if 1

#define BUTTONDARK 0x808080
#define BUTTONMIDD 0xC0C0C0
#define BUTTONLITE 0xFFFFFF
#define BUTTONMIDC 0xA0A0A0

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
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right);
	VidDrawHLine (colorDark,  rect.left,  rect.right,      rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right,      rect.top);
	
	//shrink
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	
	//do the same
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right);
	VidDrawHLine (colorDark,  rect.left,  rect.right,      rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right,      rect.top);
	
	//shrink again
	rect.left++, rect.right--, rect.top++, rect.bottom -= 2;
	
	//fill the background:
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rect);
}
void RenderButtonShape(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawHLine (0x000000, rect.left+1,rect.right-1,  rect.top);
	VidDrawHLine (0x000000, rect.left+1,rect.right-1,  rect.bottom-1);
	VidDrawVLine (0x000000, rect.top+1, rect.bottom-2, rect.left);
	VidDrawVLine (0x000000, rect.top+1, rect.bottom-2, rect.right);
	
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	RenderButtonShapeNoRounding(rect, colorDark, colorLight, colorMiddle);
}
void RenderButtonShapeSmall(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right);
	VidDrawHLine (colorDark,  rect.left,  rect.right,      rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right,      rect.top);
	
	//shrink
	rect.left++, rect.top++, rect.right--, rect.bottom--;
	
	//do the same
	//VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right);
	VidDrawHLine (colorDark,  rect.left,  rect.right,      rect.bottom - 1);
	//VidDrawHLine (colorLight, rect.left,  rect.right,      rect.top);
	
	//shrink again
	//rect.left++, rect.top++,
	rect.right--, rect.bottom -= 2;
	
	//fill the background:
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rect);
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
	int posoff = offset * final_height / (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min);
	
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
					posoff2 = posoff2 * (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min) / final_height;
					posoff2 = posoff2 +  this->m_scrollBarData.m_min;
					if (posoff2 <  this->m_scrollBarData.m_min) posoff2 = this->m_scrollBarData.m_min;
					if (posoff2 >= this->m_scrollBarData.m_max) posoff2 = this->m_scrollBarData.m_max - 1;
					this->m_scrollBarData.m_pos = posoff2;
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
				RenderButtonShapeNoRounding (left_button,   BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeNoRounding (left_button,   BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			if (this->m_scrollBarData.m_yMaxButton)
				RenderButtonShapeNoRounding (right_button,  BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeNoRounding (right_button,  BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			RenderButtonShapeNoRounding (scroller, BUTTONDARK, BUTTONLITE, BUTTONMIDD + this->m_scrollBarData.m_isBeingDragged * 0x222222); // Green
			
			left_button .left++; left_button .right++; left_button .bottom++; left_button .top++;
			right_button.left++; right_button.right++; right_button.bottom++; right_button.top++;
			scroller    .left++; scroller    .right++; scroller    .bottom++; scroller    .top++;
			
			VidDrawText ("\x1B",   left_button,  TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			VidDrawText ("\x1A",   right_button, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			//VidDrawText ("\x1D",   scroller,     TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
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
					posoff2 = posoff2 * (this->m_scrollBarData.m_max-1 - this->m_scrollBarData.m_min) / final_height;
					posoff2 = posoff2 +  this->m_scrollBarData.m_min;
					if (posoff2 <  this->m_scrollBarData.m_min) posoff2 = this->m_scrollBarData.m_min;
					if (posoff2 >= this->m_scrollBarData.m_max) posoff2 = this->m_scrollBarData.m_max - 1;
					this->m_scrollBarData.m_pos = posoff2;
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
				RenderButtonShapeNoRounding (top_button,     BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeNoRounding (top_button,     BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			if (this->m_scrollBarData.m_yMaxButton)
				RenderButtonShapeNoRounding (bottom_button,  BUTTONLITE, BUTTONDARK, BUTTONMIDC);
			else
				RenderButtonShapeNoRounding (bottom_button,  BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			
			RenderButtonShapeNoRounding (scroller, BUTTONDARK, BUTTONLITE, BUTTONMIDD + this->m_scrollBarData.m_isBeingDragged * 0x222222); // Green
			
			top_button   .left++; top_button   .right++; top_button   .bottom++; top_button   .top++;
			bottom_button.left++; bottom_button.right++; bottom_button.bottom++; bottom_button.top++;
			scroller     .left++; scroller     .right++; scroller     .bottom++; scroller     .top++;
			
			VidDrawText ("\x18",   top_button,    TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			VidDrawText ("\x19",   bottom_button, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			//VidDrawText ("\x12",   scroller,      TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			break;
		}
	}
	return false;//Fall through to other controls.
}
#endif

// TextEdit view
#if 1

void CtlSetTextInputText (Control* this, Window* pWindow, const char* pText)
{
	if (this->m_textInputData.m_pText)
		MmFreeK(this->m_textInputData.m_pText);
	
	int slen = strlen (pText);
	int newCapacity = slen + 1;
	if (newCapacity < 4096) newCapacity = 4096;//paradoxically, a smaller allocation occupies the same space as a 4096 byte alloc
	this->m_textInputData.m_pText = MmAllocateK (newCapacity);
	strcpy (this->m_textInputData.m_pText, pText);
	this->m_textInputData.m_textCapacity = newCapacity;
	this->m_textInputData.m_textLength   = slen;
	this->m_textInputData.m_textCursorIndex = 0;
	this->m_textInputData.m_textCursorSelStart = -1;
	this->m_textInputData.m_textCursorSelEnd   = -1;
	this->m_textInputData.m_textCursorIndex    = this->m_textInputData.m_textLength;
	
	int c = CountLinesInText(this->m_textInputData.m_pText);
	SetScrollBarMax (pWindow, -this->m_comboID, c);
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
//TODO: other calls? Only add when necessary.

void CtlAppendChar(Control* this, Window* pWindow, char charToAppend)
{
	if (this->m_textInputData.m_textLength >= this->m_textInputData.m_textCapacity-1)
	{
		//can't fit, need to expand
		int newCapacity = this->m_textInputData.m_textCapacity * 2, oldCapacity = this->m_textInputData.m_textCapacity;
		if (newCapacity < 4096) newCapacity = 4096;//paradoxically, a smaller allocation occupies the same space as a 4096 byte alloc
		
		char* pText = (char*)MmAllocateK(newCapacity);
		memcpy (pText, this->m_textInputData.m_pText, oldCapacity);
		
		MmFreeK(this->m_textInputData.m_pText);
		this->m_textInputData.m_pText = pText;
		this->m_textInputData.m_textCapacity = newCapacity;
	}
	
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength++] = charToAppend;
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength  ] = 0;
	
	int c = CountLinesInText(this->m_textInputData.m_pText);
	SetScrollBarMax (pWindow, -this->m_comboID, c);
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
		memcpy (pText, this->m_textInputData.m_pText, oldCapacity);
		
		MmFreeK(this->m_textInputData.m_pText);
		this->m_textInputData.m_pText = pText;
		this->m_textInputData.m_textCapacity = newCapacity;
	}
	
	this->m_textInputData.m_textLength++;
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength  ] = 0;
	memmove (&this->m_textInputData.m_pText[indexToAppendTo+1], &this->m_textInputData.m_pText[indexToAppendTo], this->m_textInputData.m_textLength-1-indexToAppendTo);
	this->m_textInputData.m_pText[indexToAppendTo] = charToAppend;
	
	int c = CountLinesInText(this->m_textInputData.m_pText);
	SetScrollBarMax (pWindow, -this->m_comboID, c);
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
	
	int c = CountLinesInText(this->m_textInputData.m_pText);
	SetScrollBarMax (pWindow, -this->m_comboID, c);
}

bool WidgetTextEditView_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
			//TODO: Allow selection across the text.
			break;
		case EVENT_CLICKCURSOR:
		{
			//TODO: Allow change of cursor via click.
			int pos = GetScrollBarPos(pWindow, -this->m_comboID);
			if (this->m_textInputData.m_scrollY != pos)
			{
				this->m_textInputData.m_scrollY  = pos;
			}
			WidgetTextEditView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			//RequestRepaint (pWindow);
			break;
		}
		case EVENT_KEYRAW:
		{
			bool repaint = true;
			switch (parm1)
			{
				case KEY_ARROW_UP:
				case KEY_ARROW_DOWN:
				case KEY_HOME:
				case KEY_END:
				{
					if (this->m_textInputData.m_pText)
					{
						const char*text = this->m_textInputData.m_pText;
						int lastRowStartIdx = -1, lastRowEndIdx = -1, curRowStartIdx = 0, newEndIndex;
						int xPos = this->m_rect.left + 4;
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
								if (xPos + GetCharWidth(*text) >= this->m_rect.right - 4 || *text == '\n')
								{
									xPos = this->m_rect.left + 4;
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
									xPos += GetCharWidth (*text);
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
							newIndex = curRowStartIdx;
						else if (parm1 == KEY_END)
							newIndex = newEndIndex;
						
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
					break;
				case KEY_ARROW_RIGHT:
					this->m_textInputData.m_textCursorIndex++;
					if (this->m_textInputData.m_textCursorIndex > this->m_textInputData.m_textLength)
						this->m_textInputData.m_textCursorIndex = this->m_textInputData.m_textLength;
					break;
				case KEY_DELETE:
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
			if ((char)parm1 == '\b')
			{
				CtlRemoveCharFromAnywhere(this, pWindow, --this->m_textInputData.m_textCursorIndex);
				if (this->m_textInputData.m_textCursorIndex < 0)
					this->m_textInputData.m_textCursorIndex = 0;
			}
			else
			{
				CtlAppendCharToAnywhere(this, pWindow, (char)parm1, this->m_textInputData.m_textCursorIndex++);
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
			this->m_textInputData.m_showLineNumbers = true;
			
			Rectangle r;
			r.right = this->m_rect.right, 
			r.top   = this->m_rect.top, 
			r.bottom= this->m_rect.bottom, 
			r.left  = this->m_rect.right - SCROLL_BAR_WIDTH;
			
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, -this->m_comboID, 1, 1);
			
			//shrink our rectangle:
			this->m_rect.right -= SCROLL_BAR_WIDTH + 4;
			
			// initialize blank text:
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
			Rectangle rk = this->m_rect;
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			VidFillRectangle(0xFFFFFF, rk);
			
			if (this->m_textInputData.m_pText)
			{
				//HACK
				VidSetFont(FONT_TAMSYN_BOLD);
				
				const char*text = this->m_textInputData.m_pText;
				int lineHeight = GetLineHeight();
				int xPos = this->m_rect.left + 4,
					yPos = this->m_rect.top + 4 - lineHeight * this->m_textInputData.m_scrollY;
				int curLine = 0, scrollLine = this->m_textInputData.m_scrollY, linesPerScreen = (this->m_rect.bottom - this->m_rect.top) / lineHeight;
				int offset  = 0;
				
				while (*text)
				{
					if (offset == this->m_textInputData.m_textCursorIndex)
					{
						VidDrawVLine(0xFF, yPos, yPos + lineHeight, xPos);
					}
					//word wrap
					if (xPos + GetCharWidth(*text) >= this->m_rect.right - 4 || *text == '\n')
					{
						xPos = this->m_rect.left + 4;
						yPos += lineHeight;
						curLine ++;
					}
					if (*text != '\n')
					{
						// render this character:
						if (curLine >= scrollLine + linesPerScreen) break;//no point in drawing anymore.
						if (curLine >= scrollLine)
							VidPlotChar(*text, xPos, yPos, 0, TRANSPARENT);
						// Increment the X,Y positions
						xPos += GetCharWidth (*text);
					}
					
					text++;
					offset++;
				}
				if (offset == this->m_textInputData.m_textCursorIndex)
				{
					VidDrawVLine(0xFF, yPos, yPos + lineHeight, xPos);
				}
				VidSetFont(FONT_BASIC);
			}
			else
				VidTextOut("NOTHING!", this->m_rect.left, this->m_rect.top, 0xFF0000, 0x000000);
			
			RenderButtonShapeNoRounding (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			
			break;
		}
	}
	return false;
}

#endif

// List View.
#if 1
static void CtlAddElementToList (Control* pCtl, const char* pText, int optionalIcon, Window* pWindow)
{
	ListViewData* pData = &pCtl->m_listViewData;
	if (pData->m_elementCount == pData->m_capacity)
	{
		//have to expand first
		int oldSize = sizeof (ListItem) * pData->m_capacity;
		int newSize = oldSize * 2;
		ListItem* pNewItems = MmAllocateK(newSize);
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
	int c = pData->m_elementCount;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtl->m_comboID, c);
	
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
	int c = pData->m_elementCount;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtl->m_comboID, c);
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
	memset (pData->m_pItems, 0, itemsSize);
	
	//also update the scroll bar.
	int c = pData->m_elementCount;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtl->m_comboID, c);
}
static const char* CtlGetElementStringFromList (Control *pCtl, int index)
{
	ListViewData* pData = &pCtl->m_listViewData;
	
	if (index < 0 || index >= pData->m_elementCount) return NULL;
	
	return pData->m_pItems[index].m_contents;
}
//extern VBEData*g_vbeData,g_mainScreenVBEData;
bool WidgetListView_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
go_back:
	switch (eventType)
	{
	#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
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
	#pragma GCC diagnostic pop
		case EVENT_PAINT:
		{
			//draw a green rectangle:
			Rectangle rk = this->m_rect;
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			VidFillRectangle(0xFFFFFF, rk);
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
				uint32_t color = 0x000000, colorT = TRANSPARENT;
				if (pData->m_highlightedElementIdx == i)
				{
					/*int l = this->m_rect.left + 4, t = this->m_rect.top + 2 + j * LIST_ITEM_HEIGHT, 
						r = this->m_rect.right - 4, b = t + LIST_ITEM_HEIGHT - 1;
					VidFillRect (0x7F, l, t, r, b);*/
					color = 0xFFFFFF, colorT = 0x7F;
				}
				if (pData->m_hasIcons)
				{
					if (pData->m_pItems[i].m_icon)
						RenderIconForceSize (pData->m_pItems[i].m_icon, this->m_rect.left + 4, this->m_rect.top + 2 + j * LIST_ITEM_HEIGHT, 16);
				}
				VidTextOut (pData->m_pItems[i].m_contents, this->m_rect.left + 4 + pData->m_hasIcons * 24, this->m_rect.top + 4 + 2 + j * LIST_ITEM_HEIGHT, color, colorT);
			}
			
			RenderButtonShapeNoRounding (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			
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
			
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, -this->m_comboID, c, 1);
			
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
static void CtlIconAddElementToList (Control* pCtlIcon, const char* pText, int optionalIcon, Window* pWindow)
{
	ListViewData* pData = &pCtlIcon->m_listViewData;
	if (pData->m_elementCount == pData->m_capacity)
	{
		//have to expand first
		int oldSize = sizeof (ListItem) * pData->m_capacity;
		int newSize = oldSize * 2;
		ListItem* pNewItems = MmAllocateK(newSize);
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
	int elementColsPerScreen = (pCtlIcon->m_rect.right - pCtlIcon->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
	int c = pData->m_elementCount / elementColsPerScreen;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtlIcon->m_comboID, c);
	
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
	int elementColsPerScreen = (pCtlIcon->m_rect.right - pCtlIcon->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
	int c = pData->m_elementCount / elementColsPerScreen;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtlIcon->m_comboID, c);
}
//extern VBEData*g_vbeData,g_mainScreenVBEData;
bool WidgetIconView_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
go_back:
	switch (eventType)
	{
	#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
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
	#pragma GCC diagnostic pop
		case EVENT_PAINT:
		{
			//draw a green rectangle:
			Rectangle rk = this->m_rect;
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			VidFillRectangle(0xFFFFFF, rk);
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
				uint32_t color = 0x000000, colorT = TRANSPARENT;
				if (pData->m_highlightedElementIdx == i)
				{
					color = 0xFFFFFF, colorT = 0x7F;
				}
				int x = this->m_rect.left + 4 + elementX, y = this->m_rect.top + 4 + 2 + j * ICON_ITEM_HEIGHT + pData->m_hasIcons * 32;
				if (pData->m_hasIcons)
				{
					if (pData->m_pItems[i].m_icon)
						RenderIconForceSize (pData->m_pItems[i].m_icon, x + (ICON_ITEM_WIDTH - 32) / 2, this->m_rect.top + 2 + j * ICON_ITEM_HEIGHT, 32);
				}
				//VidTextOut (pData->m_pItems[i].m_contents, this->m_rect.left + 4 + elementX, this->m_rect.top + 4 + 2 + j * ICON_ITEM_HEIGHT + pData->m_hasIcons * 32, color, colorT);
				Rectangle br = { x, y, x + ICON_ITEM_WIDTH, y + ICON_ITEM_HEIGHT };
				VidDrawText (pData->m_pItems[i].m_contents, br, TEXTSTYLE_HCENTERED, color, colorT);
				
				elementX += ICON_ITEM_WIDTH;
				k++;
				if (k >= elementColsPerScreen)
				{
					elementX = 0;
					k = 0;
					j++;
				}
			}
			
			RenderButtonShapeNoRounding (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			
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
			
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, -this->m_comboID, c, 1);
			
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

#define MENU_BAR_HEIGHT 11

void WidgetMenuBar_InitializeMenuBarItemAsEmpty (MenuBarTreeItem* this, int comboID)
{
	// Call the generic initializor for the menu bar tree item.
	this->m_comboID          = comboID;
	this->m_childrenCount    = 0;
	this->m_childrenCapacity = 4;
	this->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * sizeof (MenuBarTreeItem));
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
	MmFree(this->m_childrenArray);
}

void WidgetMenuBar_DeInitializeRoot(Control* this)
{
	WidgetMenuBar_DeInitializeChild (&this->m_menuBarData.m_root);
}

void WidgetMenuBar_RenderSubMenu (MenuBarTreeItem* this, int x, int y)
{
	// calculate the width and height of the menu
	if (this->m_childrenCount == 0) return;
	int width = 1, height = 1;
	
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		// Measure the text.
		int twidth, theight = 0;
		VidTextOutInternal (this->m_childrenArray[i].m_text, 0, 0, 0, 0, true, &twidth, &theight);
		
		if ((width-20) < twidth) width = twidth + 20;
		height += theight + 2;
	}
	
	// Render the base rectangle.
	VidFillRect(0xFFFFFF, x, y, x+width, y+height);
	VidDrawRect(0x000000, x, y, x+width, y+height);
	
	height++;
	
	// Draw the text.
	int ypos = 1;
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		// Measure the text.
		int twidth, theight = 0;
		VidTextOutInternal (this->m_childrenArray[i].m_text, 0, 0, 0, 0, true, &twidth, &theight);
		if (this->m_childrenArray[i].m_isOpen)
		{
			VidFillRect(0x7F, x + 1, y + ypos, x + 1 + width, y + ypos + theight);
			VidTextOut(this->m_childrenArray[i].m_text, x + 6, y + ypos + 1, 0xFFFFFF, TRANSPARENT);
			
			// Render the sub-menu above this menu.
		}
		else
		{
			VidTextOut(this->m_childrenArray[i].m_text, x + 6, y + ypos + 1, 0, TRANSPARENT);
		}
		
		ypos += theight + 2;
	}
	
	// Render the open submenu.
	ypos = 1;
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		// Measure the text.
		int twidth, theight = 0;
		VidTextOutInternal (this->m_childrenArray[i].m_text, 0, 0, 0, 0, true, &twidth, &theight);
		if (this->m_childrenArray[i].m_isOpen)
		{
			// Render the sub-menu above this menu.
			WidgetMenuBar_RenderSubMenu(&this->m_childrenArray[i], x + width - 2, y + ypos);
		}
		
		ypos += theight;
	}
	
	height++;
}
void WidgetMenuBar_CheckChildHighlight (MenuBarTreeItem* this, Point mousePos, int x, int y)
{
	// calculate the width and height of the menu
	if (this->m_childrenCount == 0) return;
	int width = 1, height = 1;
	
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		// Measure the text.
		int twidth, theight;
		VidTextOutInternal (this->m_childrenArray[i].m_text, 0, 0, 0, 0, true, &twidth, &theight);
		
		if ((width-20) < twidth) width = twidth + 20;
		height += theight + 1;
	}
	height++;
	
	// Draw the text.
	int ypos = 1;
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		// Measure the text.
		int twidth, theight;
		VidTextOutInternal (this->m_childrenArray[i].m_text, 0, 0, 0, 0, true, &twidth, &theight);
		
		Rectangle rect = { x + 1, y + ypos, x + 1 + width, y + ypos + theight };
		
		if (RectangleContains(&rect, &mousePos))
		{
			for (int i = 0; i < this->m_childrenCount; i++)
			{
				this->m_childrenArray[i].m_isOpen = false;
			}
			this->m_childrenArray[i].m_isOpen = true;
		}
		
		ypos += theight + 2;
	}
	ypos = 1;
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		// Measure the text.
		int twidth, theight;
		VidTextOutInternal (this->m_childrenArray[i].m_text, 0, 0, 0, 0, true, &twidth, &theight);
		
		if (this->m_childrenArray[i].m_isOpen)
		{
			// Check this child for any highlights too.
			WidgetMenuBar_CheckChildHighlight(&this->m_childrenArray[i], mousePos, x + width - 2, y + ypos);
			
			return;
		}
		
		ypos += theight + 2;
	}
	//LogMsgNoCr("NO INTERSECTION! (comboID %d) ", this->m_comboID);
}
bool WidgetMenuBar_CheckOpenChildrenAndSendCommandEvent(MenuBarTreeItem* this, Control* pControl, Window* pWindow)
{
	if (!this->m_childrenCount)
	{
		CallWindowCallback (pWindow, EVENT_COMMAND, pControl->m_comboID, this->m_comboID);
		return true;
	}
	bool alright = false;
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		MenuBarTreeItem* pChild = &this->m_childrenArray[i];
		if (pChild->m_isOpen)
		{
			// attempt to call:
			alright |= WidgetMenuBar_CheckOpenChildrenAndSendCommandEvent (pChild, pControl, pWindow);
			//if (alright) return alright;
		}
		pChild->m_isOpen = false;
	}
	return alright;
}
bool WidgetMenuBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	Rectangle menu_bar_rect;
	menu_bar_rect.left   = 4;
	menu_bar_rect.top    = 2 + TITLE_BAR_HEIGHT;
	menu_bar_rect.right  = pWindow->m_vbeData.m_width - 4;
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
						
						VidTextOut (pText, menu_bar_rect.left + current_x + 5, menu_bar_rect.top + 2, 0xFFFFFF, TRANSPARENT);
						//render the child menu as well:
						
						WidgetMenuBar_RenderSubMenu (pChild, rect.left, rect.bottom);
					}
					else
						VidTextOut (pText, menu_bar_rect.left + current_x + 5, menu_bar_rect.top + 2, 0, TRANSPARENT);
					
					current_x += width;
				}
			}
			
			break;
		}
		case EVENT_CLICKCURSOR:
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
						pChild->m_isOpen = true;
						// Close out all of this child's children
						for (int i = 0; i < pChild->m_childrenCount; i++)
						{
							pChild->m_childrenArray[i].m_isOpen = false;
						}
						
						//WidgetMenuBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
						//break;
						needsUpdate = true;
					}
					
					if (pChild->m_isOpen)
					{
						//Check if the child has any children we need to highlight.
						WidgetMenuBar_CheckChildHighlight(pChild, p, menu_bar_rect.left + current_x, menu_bar_rect.bottom);
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
		case EVENT_RELEASECURSOR:
		{
			// Unopen all the controls. TODO
			if (this->m_menuBarData.m_root.m_childrenArray)
			{
				bool check = WidgetMenuBar_CheckOpenChildrenAndSendCommandEvent(&this->m_menuBarData.m_root, this, pWindow);
				
				CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
				
				return check;
			}
			break;
		}
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
			VidDrawText(this->m_text, this->m_rect, this->m_parm2, this->m_parm1, TRANSPARENT);
			break;
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
			if (RectangleContains (&r, &p))
			{
				this->m_buttonData.m_clicked = true;
				WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
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
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
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
			if (RectangleContains (&r, &p))
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
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
			}
			else
			{
				this->m_rect.right--;
				RenderButtonShapeSmall (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				this->m_rect.right++;//ugly hack
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0, TRANSPARENT);
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
	#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
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
	#pragma GCC diagnostic pop
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
			
			VidDrawRectangle(0x000000, r);
			
			// Draw the text
			VidTextOut(this->m_text, this->m_rect.left + 10, this->m_rect.top, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);
			VidTextOut(this->m_text, this->m_rect.left + 12, this->m_rect.top, 0x00000,             WINDOW_BACKGD_COLOR);
			
			break;
		}
	}
	return false;
}
#define CHECKBOX_SIZE 16
bool WidgetCheckbox_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	// this->rect only affects the top left position
	
	Rectangle check_rect = this->m_rect;
	check_rect.right  = check_rect.left + CHECKBOX_SIZE;
	check_rect.bottom = check_rect.top  + CHECKBOX_SIZE;
	
	Rectangle text_rect = check_rect;
	//check_rect
	
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
};

STATIC_ASSERT(ARRAY_COUNT(g_widgetEventHandlerLUT) == CONTROL_COUNT, "Change this array if adding widgets");

WidgetEventHandler GetWidgetOnEventFunction (int type)
{
	if (type < 0 || type >= CONTROL_COUNT)
		return NULL;
	return g_widgetEventHandlerLUT[type];
}
