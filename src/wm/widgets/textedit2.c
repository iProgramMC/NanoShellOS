/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

    Widget library: Text Edit controls
******************************************/

#include "../wi.h"

// maybe these properties should be controlled by system metrics
#define TAB_WIDTH        (4)
#define CURSOR_THICKNESS (2)
#define TEXT_EDIT_FONT   (FONT_TAMSYN_MED_REGULAR)
//#define TEXT_EDIT_FONT   (SYSTEM_FONT)

#define ROUND_TO_PO2(thing, po2) (((thing) + (po2) - 1) & ~(po2))

void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);

typedef enum
{
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_HOME,
	DIR_END,
	DIR_PAGEUP,
	DIR_PAGEDOWN,
}
eNavDirection;

static int GetHorzScrollBarComboID(int comboID)
{
	//no one will use combo IDs that large I hope
	return 0x70000000 - comboID;
}

static int GetVertScrollBarComboID(int comboID)
{
	return -comboID;
}

typedef struct
{
	char*  m_text;
	size_t m_length;
	size_t m_capacity;
	int    m_lengthPixels;
}
TextLine;

typedef struct
{
	TextLine* m_lines;
	size_t    m_num_lines;
	size_t    m_lines_capacity;
	int       m_scrollX;
	int       m_scrollY;
	int       m_lastScrollX;
	int       m_lastScrollY;
	int       m_maxScrollX;
	int       m_maxScrollY;
	int       m_cursorX;
	int       m_cursorY;
	char*     m_cachedData; // every time you call TextInput_GetRawText, this gets updated.
	bool      m_bShiftHeld;
	bool      m_dirty;
}
TextInputDataEx;

static TextInputDataEx* TextInput_GetData(Control* this)
{
	return (TextInputDataEx*)this->m_dataPtr;
}

static int TextInput_GetLineHeight()
{
	unsigned old = VidSetFont(TEXT_EDIT_FONT);
	int ch = GetLineHeight();
	VidSetFont(old);
	return ch;
}

UNUSED static int TextInput_GetCharWidth(char c)
{
	unsigned old = VidSetFont(TEXT_EDIT_FONT);
	int ch = GetCharWidth(c);
	VidSetFont(old);
	return ch;
}

static const char* TextInput_GetRawText(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// Free the old one.
	if (pData->m_cachedData)
	{
		MmFree(pData->m_cachedData);
		pData->m_cachedData = NULL;
	}
	
	size_t sz = 1;
	
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		sz += pData->m_lines[i].m_length + (i != 0);
	}
	
	pData->m_cachedData = MmAllocate(sz);
	pData->m_cachedData[0] = 0;
	
	char* head = pData->m_cachedData;
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		if (i != 0)
		{
			head[0] = '\n';
			head++;
			head[0] = 0;
		}
		
		memcpy(head, pData->m_lines[i].m_text, pData->m_lines[i].m_length);
		head += pData->m_lines[i].m_length;
		head[0] = 0;
	}
	
	pData->m_cachedData[sz - 1] = 0;
	
	return pData->m_cachedData;
}

static void TextInput_RequestRepaint(Control* this, Window* pWindow)
{
	WindowAddEventToMasterQueue(pWindow, EVENT_CTL_REPAINT, this->m_comboID, 0);
}

static void TextInput_Clear(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		MmFree(pData->m_lines[i].m_text);
		pData->m_lines[i].m_text = NULL;
	}
	
	MmFree(pData->m_lines);
	pData->m_lines          = NULL;
	pData->m_num_lines      = 0;
	pData->m_lines_capacity = 0;
	
	pData->m_cursorX = 0;
	pData->m_cursorY = 0;
	pData->m_scrollX = 0;
	pData->m_scrollY = 0;
	pData->m_maxScrollX = 0;
	pData->m_maxScrollY = 0;
}

TextLine* TextInput_AddLine(Control* ctl)
{
	TextInputDataEx* this = TextInput_GetData(ctl);
	
	if (this->m_num_lines + 1 >= this->m_lines_capacity)
	{
		// need to expand!
		this->m_lines_capacity += (4096 / sizeof(TextLine));
		this->m_lines = MmReAllocate(this->m_lines, sizeof(TextLine) * this->m_lines_capacity);
	}
	
	return &this->m_lines[this->m_num_lines++];
}

static void TextInput_UpdateScrollBars(Control* this, Window* pWindow)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int scrollMax;
	
	// Vertical Scroll bar
	scrollMax = (pData->m_maxScrollY) - (this->m_rect.bottom - this->m_rect.top) + 6;
	if (scrollMax < 1) scrollMax = 1;
	SetScrollBarMax(pWindow, GetVertScrollBarComboID(this->m_comboID), scrollMax);
	
	// Horizontal Scroll bar
	scrollMax = (pData->m_maxScrollX) - (this->m_rect.right - this->m_rect.left);
	if (scrollMax < 1) scrollMax = 1;
	SetScrollBarMax(pWindow, GetHorzScrollBarComboID(this->m_comboID), scrollMax);
}

static void TextInput_RecalcMaxScroll(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	pData->m_maxScrollX = 0;
	pData->m_maxScrollY = pData->m_num_lines * TextInput_GetLineHeight();
	
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		TextLine* line = &pData->m_lines[i];
		if (pData->m_maxScrollX < line->m_lengthPixels)
			pData->m_maxScrollX = line->m_lengthPixels;
	}
}

static void TextInput_CalculateLinePixelWidth(UNUSED Control* this, TextLine* line)
{
	unsigned oldFont = VidSetFont(TEXT_EDIT_FONT);
	
	const size_t sz = line->m_length;
	
	line->m_lengthPixels = 0;
	for (size_t i = 0; i < sz; i++)
	{
		const char chr = line->m_text[i];
		if (chr == '\t')
			line->m_lengthPixels += GetCharWidth('W') * TAB_WIDTH;
		else
			line->m_lengthPixels += GetCharWidth(chr);
	}
	
	VidSetFont(oldFont);
}

static const char* TextInput_AppendLineToEnd(Control* this, const char* pText, bool *bHitEnd)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	const char* nextNl = strchr(pText, '\n');
	
	// if we didn't find the next newline, we're on the last line
	if (!nextNl)
	{
		nextNl = pText + strlen(pText);
		*bHitEnd = true;
	}
	
	size_t sz = nextNl - pText;
	
	// Append a new line.
	TextLine* line = TextInput_AddLine(this);
	
	size_t cap = ROUND_TO_PO2(sz + 1, 4096);
	
	line->m_text     = MmAllocate(cap);
	line->m_capacity = cap;
	line->m_length   = sz;
	memcpy(line->m_text, pText, sz);
	line->m_text[sz] = 0;
	line->m_lengthPixels = 0;
	
	TextInput_CalculateLinePixelWidth(this, line);
	
	if (pData->m_maxScrollX < line->m_lengthPixels)
		pData->m_maxScrollX = line->m_lengthPixels;
	
	pData->m_maxScrollY += TextInput_GetLineHeight();
	
	return pText + sz + 1;
}

static void TextInput_RepaintLine(Control* pCtl, int lineNum);
static void TextInput_RepaintLineAndBelow(Control* pCtl, int lineNum);

static void TextInput_SetLineText(Control* pCtl, int lineNum, const char* pText)
{
	TextInputDataEx* pData = TextInput_GetData(pCtl);
	TextLine* pLine = &pData->m_lines[lineNum];
	
	// get rid of what was there before
	if (pLine->m_text)
	{
		MmFree(pLine->m_text);
	}
	
	size_t sLen = strlen(pText);
	
	pLine->m_text     = MmAllocate(sLen + 1);
	pLine->m_capacity = sLen + 1;
	pLine->m_length   = sLen;
	
	strcpy(pLine->m_text, pText);
	
	TextInput_CalculateLinePixelWidth(pCtl, pLine);
	
	TextInput_RepaintLine(pCtl, lineNum);
}

static void TextInput_AppendText(Control* this, Window* pWindow, int line, int pos, const char* pText)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	TextLine* pLine = &pData->m_lines[line];
	
	size_t sLen = strlen(pText);
	
	// Ensure that 'pos' is within bounds.
	if (pos < 0)
		pos = 0;
	if (pos >= (int)pLine->m_length)
		pos  = (int)pLine->m_length;
	
	if (pLine->m_length + sLen + 1 >= pLine->m_capacity)
	{
		// resize the thing
		pLine->m_capacity += sLen;
		void *pNewMem = MmReAllocate(pLine->m_text, pLine->m_capacity);
		if (!pNewMem)
		{
			pLine->m_capacity -= sLen;
			SLogMsg("TextInput_AppendText failed due to running out of virtual memory");
			return;
		}
		
		pLine->m_text = pNewMem;
	}
	
	// memmove it away
	char* text = pLine->m_text + pos;
	memmove(text + sLen, text, pLine->m_length - pos);
	
	// now we have space, memcpy the text in
	memcpy(text, pText, sLen);
	
	pLine->m_length += sLen;
	pLine->m_text[pLine->m_length] = 0;
	
	// Measure pText's width in characters.
	unsigned oldFont = VidSetFont(TEXT_EDIT_FONT);
	int width = 0;
	for (; *pText; pText++)
	{
		if (*pText == '\t')
			width += TAB_WIDTH * GetCharWidth('W');
		else
			width += GetCharWidth(*pText);
	}
	
	VidSetFont(oldFont);
	
	// Increase this line's length by 'width' pixels.
	pLine->m_lengthPixels += width;
	
	if (pData->m_maxScrollX < pLine->m_lengthPixels)
	{
		pData->m_maxScrollX = pLine->m_lengthPixels;
		TextInput_UpdateScrollBars(this, pWindow);
	}
	
	TextInput_RepaintLine(this, line);
}

static void TextInput_EraseChars(Control* this, Window* pWindow, int line, int pos, int sLen)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	TextLine* pLine = &pData->m_lines[line];
	
	// Ensure that the pos and sLen parms are valid.
	
	if (pos < 0)
	{
		sLen += pos;
		pos = 0;
	}
	
	if (pos >= (int)pLine->m_length) return;
	if (sLen < 0) return;
	
	if (pos + sLen >= (int)pLine->m_length)
	{
		sLen = (int)pLine->m_length - pos;
	}
	
	if (pLine->m_length + sLen + 1 >= pLine->m_capacity)
	{
		// resize the thing
		pLine->m_capacity += sLen;
		void *pNewMem = MmReAllocate(pLine->m_text, pLine->m_capacity);
		if (!pNewMem)
		{
			pLine->m_capacity -= sLen;
			SLogMsg("TextInput_AppendText failed due to running out of virtual memory");
			return;
		}
		
		pLine->m_text = pNewMem;
	}
	
	// memmove it away
	char* text = pLine->m_text + pos;
	memmove(text, text + sLen, pLine->m_length - pos - sLen);
	
	pLine->m_length -= sLen;
	pLine->m_text[pLine->m_length] = 0;
	
	// Recalculate the line's pixel width.
	TextInput_CalculateLinePixelWidth(this, pLine);
	
	// Recalculate the max scroll.
	TextInput_RecalcMaxScroll(this);
	
	// Update the scroll bars.
	TextInput_UpdateScrollBars(this, pWindow);
	
	// Repaint the line.
	TextInput_RepaintLine(this, line);
}

static void TextInput_InsertNewLines(Control* this, Window* pWindow, int linePos, int lineCount)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// If we can't fit 'lineCount' lines, make some space
	if (pData->m_lines_capacity < pData->m_num_lines + lineCount)
	{
		pData->m_lines_capacity += lineCount;
		
		void *pNewMem = MmReAllocate(pData->m_lines, pData->m_lines_capacity);
		if (!pNewMem)
		{
			pData->m_lines_capacity -= lineCount;
			SLogMsg("TextInput_InsertNewLines failed due to a lack of memory!");
			return;
		}
		
		pData->m_lines = pNewMem;
	}
	
	// memmove the other lines out of the way
	memmove(&pData->m_lines[linePos + lineCount], &pData->m_lines[linePos], sizeof(TextLine) * (pData->m_num_lines - linePos));
	pData->m_num_lines += lineCount;
	
	// initialize the lines to be empty.
	for (int i = linePos, j = 0; j < lineCount; ++i, ++j)
	{
		TextLine* pLine = &pData->m_lines[i];
		memset(pLine, 0, sizeof *pLine);
		
		pLine->m_text    = MmAllocate(4096);
		pLine->m_text[0] = 0;
		pLine->m_capacity = 4096;
	}
	
	pData->m_maxScrollY += TextInput_GetLineHeight() * lineCount;
	
	TextInput_UpdateScrollBars(this, pWindow);
	
	TextInput_RepaintLineAndBelow(this, linePos);
}

static void TextInput_InsertNewLine(Control* this, Window* pWindow, int linePos)
{
	TextInput_InsertNewLines(this, pWindow, linePos, 1);
}

static void TextInput_SplitLines(Control* this, Window* pWindow, int line, int splitAt)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// Create a new line.
	if (splitAt == 0)
	{
		TextInput_InsertNewLine(this, pWindow, line);
		return;
	}
	
	TextInput_InsertNewLine(this, pWindow, line + 1);
	
	// Set said line's text to the current line's text at `splitAt` offset.
	TextLine* pSrcLine = &pData->m_lines[line];
	
	// (if the split is in front of the string, we've created a new line!)
	if (splitAt >= (int)pSrcLine->m_length) return;
	
	TextInput_SetLineText(this, line + 1, pSrcLine->m_text + splitAt);
	
	pSrcLine->m_text[splitAt] = 0;
	pSrcLine->m_length = splitAt;
	
	TextInput_RepaintLine(this, line);
}

static void TextInput_EraseConsecutiveLines(Control* this, Window* pWindow, int lineStart, int lineCount)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// Make sure that the lineStart and lineCount are within bounds.
	if (lineStart < 0)
	{
		lineCount += lineStart;
		lineStart = 0;
	}
	
	if (lineStart >= (int)pData->m_num_lines) return;
	if (lineCount < 0) return;
	
	if (lineStart + lineCount >= (int)pData->m_num_lines)
		lineCount = (int)pData->m_num_lines - lineStart;
	
	// Free the 'text' pointer within these lines.
	for (int i = lineStart, j = 0; j < lineCount; ++i, ++j)
	{
		MmFree(pData->m_lines[i].m_text);
	}
	
	// Memmove the line data over
	memmove(&pData->m_lines[lineStart], &pData->m_lines[lineStart + lineCount], sizeof(TextLine) * (pData->m_num_lines - lineStart - lineCount));
	
	pData->m_num_lines  -= lineCount;
	pData->m_maxScrollY -= lineCount * TextInput_GetLineHeight();
	
	TextInput_UpdateScrollBars(this, pWindow);
	
	TextInput_RepaintLineAndBelow(this, lineStart);
}

static void TextInput_EraseLine(Control* this, Window* pWindow, int line)
{
	TextInput_EraseConsecutiveLines(this, pWindow, line, 1);
}

static void TextInput_AppendChar(Control* this, Window* pWindow, int line, int pos, char c)
{
	char str[2];
	str[0] = c;
	str[1] = 0;
	return TextInput_AppendText(this, pWindow, line, pos, str);
}

static void TextInput_EraseChar(Control* this, Window* pWindow, int line, int pos)
{
	return TextInput_EraseChars(this, pWindow, line, pos, 1);
}

static void TextInput_SetText(Control* this, Window* pWindow, const char* pText, bool bRepaint)
{
	TextInput_Clear(this);
	
	bool bHitEnd = false;	
	if (!*pText)
	{
		// There must be at least one line in the program.
		TextInput_AppendLineToEnd(this, pText, &bHitEnd);
	}
	else
	{
		while (pText && !bHitEnd)
		{
			pText = TextInput_AppendLineToEnd(this, pText, &bHitEnd);
		}
	}
	
	TextInput_UpdateScrollBars(this, pWindow);
	
	if (bRepaint)
	{
		TextInput_RequestRepaint(this, pWindow);
	}
}

// This concatenates 'lineSrc' to the end of 'lineDst'.
static void TextInput_JoinLines(Control* pCtl, Window* pWindow, int lineDst, int lineSrc)
{
	TextInputDataEx* pData = TextInput_GetData(pCtl);
	
	if (lineSrc < 0 || lineSrc >= (int)pData->m_num_lines) return;
	if (lineDst < 0 || lineDst >= (int)pData->m_num_lines) return;
	
	TextInput_AppendText(pCtl, pWindow, lineDst, (int)pData->m_lines[lineDst].m_length, pData->m_lines[lineSrc].m_text);
	TextInput_EraseLine(pCtl, pWindow, lineSrc);
}

static void TextInput_UpdateMode(Control* this, Window* pWindow)
{
	TextInput_RequestRepaint(this, pWindow);
}

static void TextInput_PartialDraw(Control* this, Rectangle rect)
{
	rect.right --;
	rect.bottom--;
	//VidFillRectangle(GetRandom(), rect);
	VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, rect);
	rect.right ++;
	rect.bottom++;
	unsigned oldFont = VidSetFont(TEXT_EDIT_FONT);
	VidSetClipRect(&rect);
	
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int scrollX = pData->m_scrollX - 1;
	int scrollY = pData->m_scrollY - 1;
	
	int lineX = this->m_rect.left + 2 - scrollX;
	int lineY = this->m_rect.top  + 2 - scrollY;
	
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		//VidTextOut(pData->m_lines[i].m_text, lineX, lineY, WINDOW_TEXT_COLOR, WINDOW_TEXT_COLOR_LIGHT);
		int charX = lineX;
		for (size_t j = 0; j < pData->m_lines[i].m_length && lineY + GetLineHeight() > rect.top; j++)
		{
			char chr = pData->m_lines[i].m_text[j];
			
			// well, we reached the end of the line right now
			if (charX >= rect.right) break;
			
			int chrWidth = GetCharWidth(chr);
			const bool isWithinScreenBounds = rect.left <= charX + chrWidth && charX < rect.right;
			
			if (chr == '\t')
			{
				chrWidth = GetCharWidth('W') * TAB_WIDTH;
			}
			else if (isWithinScreenBounds)
			{
				VidPlotChar(chr, charX, lineY, WINDOW_TEXT_COLOR, WINDOW_TEXT_COLOR_LIGHT);
			}
			
			if (isWithinScreenBounds && pData->m_cursorX == (int)j && pData->m_cursorY == (int)i)
			{
				VidFillRect(0xFF, charX, lineY, charX + CURSOR_THICKNESS - 1, lineY + GetLineHeight() - 1);
			}
			
			charX += chrWidth;
		}
		
		const bool isWithinScreenBounds = rect.left <= charX && charX < rect.right;
		if (isWithinScreenBounds && pData->m_cursorX == (int)pData->m_lines[i].m_length && pData->m_cursorY == (int)i)
		{
			VidFillRect(0xFF, charX, lineY, charX + CURSOR_THICKNESS - 1, lineY + GetLineHeight() - 1);
		}
		
		lineY += GetLineHeight();
		
		if (lineY >= rect.bottom) break;
	}
	
	VidSetFont(oldFont);
	VidSetClipRect(NULL);
}

SAI void ClampWithin(Rectangle* rect, const Rectangle* within)
{
	if (rect->left   < within->left)   rect->left   = within->left;
	if (rect->right  > within->right)  rect->right  = within->right;
	if (rect->top    < within->top)    rect->top    = within->top;
	if (rect->bottom > within->bottom) rect->bottom = within->bottom;
}

static void TextInput_OnScrollDone(Control* pCtl)
{
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int diffX = this->m_scrollX - this->m_lastScrollX;
	int diffY = this->m_scrollY - this->m_lastScrollY;
	
	if (diffX == 0 && diffY == 0) return;
	
	Rectangle rect = pCtl->m_rect;
	rect.left   += 2;
	rect.top    += 2;
	rect.right  -= 2;
	rect.bottom -= 2;
	
	Rectangle redraw1, redraw2;
	redraw1 = redraw2 = rect;
	
	ScrollRect(&rect, -diffX, -diffY);
	
	if (diffX < 0) redraw1.right = redraw1.left  - diffX;
	else           redraw1.left  = redraw1.right - diffX;
	
	if (diffY < 0) redraw2.bottom = redraw2.top    - diffY;
	else           redraw2.top    = redraw2.bottom - diffY;
	
	// ensure there is no overflow
	ClampWithin(&redraw1, &rect);
	ClampWithin(&redraw2, &rect);
	
	this->m_lastScrollX = this->m_scrollX;
	this->m_lastScrollY = this->m_scrollY;
	
	// TODO: make it so that redraw1 and redraw2 don't overlap.
	// Most likely this won't matter since I don't think we'll ever be able
	// to scroll two scroll bars at the same time.
	
	if (redraw1.left < redraw1.right && redraw1.top < redraw1.bottom)
		TextInput_PartialDraw(pCtl, redraw1);
	
	if (redraw2.left < redraw2.right && redraw2.top < redraw2.bottom)
		TextInput_PartialDraw(pCtl, redraw2);
}

static void TextInput_RepaintLineAndBelow(Control* pCtl, int lineNum)
{
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int lineHeight = TextInput_GetLineHeight();
	
	Rectangle refreshRect = pCtl->m_rect;
	refreshRect.left   += 2;
	refreshRect.right  -= 2;
	refreshRect.bottom -= 2;
	
	refreshRect.top += 3 + lineNum * lineHeight - this->m_scrollY;
	
	if (refreshRect.top < pCtl->m_rect.top + 3)
		refreshRect.top = pCtl->m_rect.top + 3;
	
	if (refreshRect.bottom <= refreshRect.top) return;
	
	TextInput_PartialDraw(pCtl, refreshRect);
}

static void TextInput_RepaintLine(Control* pCtl, int lineNum)
{
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int lineHeight = TextInput_GetLineHeight();
	
	Rectangle refreshRect = pCtl->m_rect;
	refreshRect.left  += 2;
	refreshRect.right -= 2;
	
	refreshRect.top += 3 + lineNum * lineHeight - this->m_scrollY;
	refreshRect.bottom = refreshRect.top + lineHeight;
	
	if (refreshRect.top < pCtl->m_rect.top + 3)
		refreshRect.top = pCtl->m_rect.top + 3;
	
	if (refreshRect.bottom > pCtl->m_rect.bottom - 3)
		refreshRect.bottom = pCtl->m_rect.bottom - 3;
	
	if (refreshRect.bottom <= refreshRect.top) return;
	
	TextInput_PartialDraw(pCtl, refreshRect);
}

static void TextInput_ClampCursorWithinScrollBounds(Control* this)
{
	bool scrolled = false;
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int rectWidth  = (this->m_rect.right  - this->m_rect.left) - 12;
	int rectHeight = (this->m_rect.bottom - this->m_rect.top)  - 6;
	int lineHeight = TextInput_GetLineHeight();
	
	int newScrollY = lineHeight * pData->m_cursorY;
	if (newScrollY < 0)
		newScrollY = 0;
	
	if (pData->m_scrollY < newScrollY + lineHeight - rectHeight)
	{
		pData->m_scrollY = newScrollY + lineHeight - rectHeight;
		scrolled = true;
	}
	else if (pData->m_scrollY > newScrollY)
	{
		pData->m_scrollY = newScrollY;
		scrolled = true;
	}
	
	int newScrollX = 0;
	TextLine* pLine = &pData->m_lines[pData->m_cursorY];
	for (size_t i = 0; i < pLine->m_length && i < (size_t)pData->m_cursorX; i++)
	{
		newScrollX += GetCharWidth(pLine->m_text[i]);
	}
	
	if (pData->m_scrollX < newScrollX - rectWidth)
	{
		pData->m_scrollX = newScrollX - rectWidth;
		scrolled = true;
	}
	else if (pData->m_scrollX > newScrollX)
	{
		pData->m_scrollX = newScrollX;
		scrolled = true;
	}
	
	if (!scrolled) return;
	
	TextInput_OnScrollDone(this);
}

void TextInput_OnNavPressed(Control* this, eNavDirection dir)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	unsigned oldFont = VidSetFont(TEXT_EDIT_FONT);
	
	int nPageLines = (this->m_rect.bottom - this->m_rect.top - (GetLineHeight() * 2)) / GetLineHeight();
	
	if (nPageLines < 1)
		nPageLines = 1;
	
	switch (dir)
	{
		case DIR_UP:
		{
			int oldCursorY = pData->m_cursorY;
			pData->m_cursorY--;
			
			if (pData->m_cursorY < 0)
			{
				pData->m_cursorY = 0;
				break;
			}
			
			// Check if the cursorX is still valid.
			if (pData->m_cursorX > (int)pData->m_lines[pData->m_cursorY].m_length)
				pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
			
			// Update the lines where the cursor was modified.
			TextInput_RepaintLine(this, oldCursorY);
			TextInput_RepaintLine(this, pData->m_cursorY);
			break;
		}
		case DIR_DOWN:
		{
			int oldCursorY = pData->m_cursorY;
			pData->m_cursorY++;
			
			if (pData->m_cursorY > (int)pData->m_num_lines - 1)
			{
				pData->m_cursorY = (int)pData->m_num_lines - 1;
				break;
			}
			
			// Check if the cursorX is still valid.
			if (pData->m_cursorX > (int)pData->m_lines[pData->m_cursorY].m_length)
				pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
			
			// Update the lines where the cursor was modified.
			TextInput_RepaintLine(this, oldCursorY);
			TextInput_RepaintLine(this, pData->m_cursorY);
			break;
		}
		case DIR_PAGEUP:
		{
			int oldCursorY = pData->m_cursorY;
			pData->m_cursorY -= nPageLines;
			
			if (pData->m_cursorY < 0)
			{
				pData->m_cursorY = 0;
				break;
			}
			
			// Update the lines where the cursor was modified.
			TextInput_RepaintLine(this, oldCursorY);
			TextInput_RepaintLine(this, pData->m_cursorY);
			break;
		}
		case DIR_PAGEDOWN:
		{
			int oldCursorY = pData->m_cursorY;
			pData->m_cursorY += nPageLines;
			
			if (pData->m_cursorY > (int)pData->m_num_lines - 1)
			{
				pData->m_cursorY = (int)pData->m_num_lines - 1;
				break;
			}
			
			// Check if the cursorX is still valid.
			if (pData->m_cursorX > (int)pData->m_lines[pData->m_cursorY].m_length)
				pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
			
			// Update the lines where the cursor was modified.
			TextInput_RepaintLine(this, oldCursorY);
			TextInput_RepaintLine(this, pData->m_cursorY);
			break;
		}
		case DIR_LEFT:
		{
			int oldCursorY = pData->m_cursorY;
			pData->m_cursorX--;
			if (pData->m_cursorX < 0)
			{
				if (pData->m_cursorY - 1 >= 0)
				{
					pData->m_cursorY--;
					pData->m_cursorX = pData->m_lines[pData->m_cursorY].m_length;
				}
				else
				{
					pData->m_cursorX = 0;
					break;
				}
			}
			
			if (oldCursorY != pData->m_cursorY)
			{
				TextInput_RepaintLine(this, oldCursorY);
				TextInput_RepaintLine(this, pData->m_cursorY);
			}
			else
			{
				TextInput_RepaintLine(this, pData->m_cursorY);
			}
			break;
		}
		case DIR_RIGHT:
		{
			int oldCursorY = pData->m_cursorY;
			pData->m_cursorX++;
			if (pData->m_cursorX > (int)pData->m_lines[pData->m_cursorY].m_length)
			{
				if (pData->m_cursorY + 1 < (int)pData->m_num_lines)
				{
					pData->m_cursorY++;
					pData->m_cursorX = 0;
				}
				else
				{
					pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
					break;
				}
			}
			
			if (oldCursorY != pData->m_cursorY)
			{
				TextInput_RepaintLine(this, oldCursorY);
				TextInput_RepaintLine(this, pData->m_cursorY);
			}
			else
			{
				TextInput_RepaintLine(this, pData->m_cursorY);
			}
			
			break;
		}
		case DIR_HOME:
		{
			pData->m_cursorX = 0;
			TextInput_RepaintLine(this, pData->m_cursorY);
			break;
		}
		case DIR_END:
		{
			pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
			TextInput_RepaintLine(this, pData->m_cursorY);
			break;
		}
	}
	
	TextInput_ClampCursorWithinScrollBounds(this);
	
	VidSetFont(oldFont);
}

bool WidgetTextEditView2_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			TextInputDataEx* pData = MmAllocate(sizeof(TextInputDataEx));
			memset(pData, 0, sizeof *pData);
			this->m_dataPtr = pData;
			
			// Create two scroll bars.
			if (this->m_parm1 & TEXTEDIT_MULTILINE)
			{
				Rectangle horzSBRect, vertSBRect;
				horzSBRect = vertSBRect = this->m_rect;
				horzSBRect.right  -= SCROLL_BAR_SIZE;
				vertSBRect.bottom -= SCROLL_BAR_SIZE;
				horzSBRect.top  = horzSBRect.bottom - SCROLL_BAR_SIZE;
				vertSBRect.left = vertSBRect.right  - SCROLL_BAR_SIZE;
				
				int horzSBAnchor = 0, vertSBAnchor = 0;
				if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				{
					horzSBAnchor |= ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM;
					vertSBAnchor |= ANCHOR_BOTTOM_TO_BOTTOM;
				}
				if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				{
					horzSBAnchor |= ANCHOR_RIGHT_TO_RIGHT;
					vertSBAnchor |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
				}
				if (this->m_anchorMode & ANCHOR_TOP_TO_BOTTOM)
				{
					vertSBAnchor |= ANCHOR_TOP_TO_BOTTOM;
				}
				if (this->m_anchorMode & ANCHOR_LEFT_TO_RIGHT)
				{
					horzSBAnchor |= ANCHOR_LEFT_TO_RIGHT;
				}
				
				AddControlEx(pWindow, CONTROL_VSCROLLBAR, vertSBAnchor, vertSBRect, NULL, GetVertScrollBarComboID(this->m_comboID), 100, 0);
				AddControlEx(pWindow, CONTROL_HSCROLLBAR, horzSBAnchor, horzSBRect, NULL, GetHorzScrollBarComboID(this->m_comboID), 100, 0);
				
				// shrink ourselves
				this->m_rect.right  -= SCROLL_BAR_SIZE;
				this->m_rect.bottom -= SCROLL_BAR_SIZE;
			}
			else
			{
				this->m_rect.bottom = this->m_rect.top + 6 + TextInput_GetLineHeight();
			}
			
			TextInput_SetText(this, pWindow, "", false);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		case EVENT_SCROLLDONE:
		{
			TextInputDataEx* pData = TextInput_GetData(this);
			pData->m_scrollX = GetScrollBarPos(pWindow, GetHorzScrollBarComboID(this->m_comboID));
			pData->m_scrollY = GetScrollBarPos(pWindow, GetVertScrollBarComboID(this->m_comboID));
			TextInput_OnScrollDone(this);
			break;
		}
		case EVENT_DESTROY:
		{
			TextInputDataEx* pData = TextInput_GetData(this);
			if (pData->m_cachedData)
			{
				MmFree(pData->m_cachedData);
				pData->m_cachedData = NULL;
			}
			
			TextInput_Clear(this);
			
			MmFree(this->m_dataPtr);
			break;
		}
		case EVENT_PAINT:
		{
			// Paint a border around the control.
			Rectangle rect = this->m_rect, borderRect;
			
			rect.right--;
			borderRect = rect;
			rect.right++;
			
			rect.left += 2;
			rect.top  += 2;
			rect.right  -= 2;
			rect.bottom -= 2;
			
			TextInput_PartialDraw(this, rect);
			
			RenderButtonShapeSmallInsideOut (borderRect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			break;
		}
		case EVENT_CTL_REPAINT:
		{
			WidgetTextEditView2_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (!RectangleContains(&this->m_rect, &mouseClickPos))
			{
				// if no one took focus from us before us, make us not focused at all
				if (this->m_bFocused)
					SetFocusedControl(pWindow, -1);
				break;
			}
			
			// set ourself as the focus
			SetFocusedControl(pWindow, this->m_comboID);
			
			// TODO text selection and stuff
			
			break;
		}
		case EVENT_KEYPRESS:
		{
			if (!this->m_bFocused) break;
			if (this->m_parm1 & TEXTEDIT_READONLY) break;
			
			TextInputDataEx* pData = TextInput_GetData(this);
			
			char inChar = (char)parm1;
			
			if (inChar == '\n' && (~this->m_parm1 & TEXTEDIT_MULTILINE))
				break;
			
			if ((char)parm1 == '\b')
			{
				if (pData->m_cursorX == 0)
				{
					if (pData->m_cursorY > 0)
					{
						TextInput_OnNavPressed(this, DIR_LEFT);
						TextInput_JoinLines(this, pWindow, pData->m_cursorY, pData->m_cursorY + 1);
					}
				}
				else
				{
					TextInput_EraseChar(this, pWindow, pData->m_cursorY, pData->m_cursorX - 1);
					TextInput_OnNavPressed(this, DIR_LEFT);
				}
			}
			else if (parm1 == '\n')
			{
				TextInput_SplitLines(this, pWindow, pData->m_cursorY, pData->m_cursorX);
				TextInput_OnNavPressed(this, DIR_RIGHT);
			}
			else
			{
				TextInput_AppendChar(this, pWindow, pData->m_cursorY, pData->m_cursorX, parm1);
				TextInput_OnNavPressed(this, DIR_RIGHT);
			}
			
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (!this->m_bFocused) break;
			
			TextInputDataEx* pData = TextInput_GetData(this);
			
			// If the key was released, break; we don't really need to process release events
			if (parm1 & 0x80)
			{
				int code = (int)(unsigned char)parm1;
				if ((code & 0x7F) == KEY_LSHIFT || (code & 0x7F) == KEY_RSHIFT)
					pData->m_bShiftHeld = false;
				break;
			}
			
			int code = parm1;
			
			switch (code)
			{
				case KEY_DELETE:
				{
					TextLine* pCurrentLine = &pData->m_lines[pData->m_cursorY];
					if (pData->m_cursorX == (int)pCurrentLine->m_length)
					{
						TextInput_JoinLines(this, pWindow, pData->m_cursorY, pData->m_cursorY + 1);
					}
					else
					{
						TextInput_EraseChar(this, pWindow, pData->m_cursorY, pData->m_cursorX);
					}
					
					break;
				}
				case KEY_LSHIFT:
				case KEY_RSHIFT:
				{
					pData->m_bShiftHeld = true;
					break;
				}
				case KEY_UP:
				{
					TextInput_OnNavPressed(this, DIR_UP);
					break;
				}
				case KEY_DOWN:
				{
					TextInput_OnNavPressed(this, DIR_DOWN);
					break;
				}
				case KEY_LEFT:
				{
					TextInput_OnNavPressed(this, DIR_LEFT);
					break;
				}
				case KEY_RIGHT:
				{
					TextInput_OnNavPressed(this, DIR_RIGHT);
					break;
				}
				case KEY_PAGEUP:
				{
					TextInput_OnNavPressed(this, DIR_PAGEUP);
					break;
				}
				case KEY_PAGEDOWN:
				{
					TextInput_OnNavPressed(this, DIR_PAGEDOWN);
					break;
				}
				case KEY_HOME:
				{
					TextInput_OnNavPressed(this, DIR_HOME);
					break;
				}
				case KEY_END:
				{
					TextInput_OnNavPressed(this, DIR_END);
					break;
				}
			}
			
			break;
		}
	}
	return false;
}

// Exposed API functions
void SetTextInputText(Window* pWindow, int comboID, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			TextInput_SetText(&pWindow->m_pControlArray[i], pWindow, pText, true);
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
			TextInput_GetData(&pWindow->m_pControlArray[i])->m_dirty = false;
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
			return TextInput_GetData(&pWindow->m_pControlArray[i])->m_dirty;
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
			return TextInput_GetRawText(&pWindow->m_pControlArray[i]);
		}
	}
	return NULL;
}

void TextInputSetMode (Window *pWindow, int comboID, int mode)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			Control *p = &pWindow->m_pControlArray[i];
			
			p->m_parm1 = mode;
			TextInput_UpdateMode(p, pWindow);
		}
	}
}
