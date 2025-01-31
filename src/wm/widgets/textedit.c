/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

    Widget library: Text Edit controls
******************************************/

#include "../wi.h"
#include <clip.h>

// TODO: Support Macintosh(CR) line ending?

// maybe these properties should be controlled by system metrics
#define TAB_WIDTH        (4)
#define CURSOR_THICKNESS (2)
#define TEXT_EDIT_FONT   (FONT_TAMSYN_MED_REGULAR)
//#define TEXT_EDIT_FONT   (SYSTEM_FONT)

#define MIN_CAPACITY     (4096)

#define INT_MAX          (0x7FFFFFFF)

#define ROUND_TO_PO2(thing, po2) (((thing) + (po2) - 1) & ~(po2 - 1))

int g_TextCursorFlashSpeed = 500; // 2 hz.

enum
{
	LEND_NONE,
	LEND_LF,
	LEND_CR,
	LEND_CRLF, // == LEND_CR | LEND_LF
};

typedef enum
{
	DIR_NONE,
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
	int    m_lineEnding;
	int    m_lengthPixels;
}
TextLine;

typedef struct
{
	Window*   m_pWindow;
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
	bool      m_ignoreNextKeyCode;
	bool      m_dirty;
	int       m_knownScrollBarX;
	int       m_knownScrollBarY;
	bool      m_bCursorFlash;
	int       m_ignoreTicksTill;
	unsigned  m_font;
	int       m_selectX1, m_selectY1; // The position of the first selected character
	int       m_selectX2, m_selectY2; // The position of the first unselected character. After m_select*1
	int       m_grabbedX, m_grabbedY; // used internally in the PreUpdateSelection function
	int       m_currentLineEnding;
}
TextInputDataEx;

static TextInputDataEx* TextInput_GetData(Control* this)
{
	return (TextInputDataEx*)this->m_dataPtr;
}

static bool TextInput_IsWithinSelectionBounds(Control* this, int x, int y)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int64_t max_int = INT_MAX;
	
	// Note: This is really hacky, however, this is quick to write, so we will just do it.
	int64_t limit1 = (int64_t)pData->m_selectY1 * max_int + pData->m_selectX1;
	int64_t limit2 = (int64_t)pData->m_selectY2 * max_int + pData->m_selectX2;
	
	int64_t check  = (int64_t)y * max_int + x;
	
	return limit1 <= check && check < limit2;
}

static bool TextInput_SelectedAnything(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int64_t max_int = INT_MAX;
	
	// Note: This is really hacky, however, this is quick to write, so we will just do it.
	int64_t limit1 = (int64_t)pData->m_selectY1 * max_int + pData->m_selectX1;
	int64_t limit2 = (int64_t)pData->m_selectY2 * max_int + pData->m_selectX2;
	
	return limit2 > limit1;
}

unsigned TextInput_GetFont(Control* this)
{
	return TextInput_GetData(this)->m_font;
}

static int TextInput_GetLineHeight(Control* this)
{
	unsigned old = VidSetFont(TextInput_GetFont(this));
	int ch = GetLineHeight();
	VidSetFont(old);
	return ch;
}

UNUSED static int TextInput_GetCharWidth(Control* this, char c)
{
	unsigned old = VidSetFont(TextInput_GetFont(this));
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
		sz += pData->m_lines[i].m_length + (i != 0) * 2;
	}
	
	pData->m_cachedData = MmAllocate(sz);
	pData->m_cachedData[0] = 0;
	
	char* head = pData->m_cachedData;
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		if (i != 0)
		{
			int lineEnd = pData->m_lines[i - 1].m_lineEnding;
			
			if (lineEnd & LEND_CR)
			{
				head[0] = '\r';
				head++;
			}
			if (lineEnd & LEND_LF)
			{
				head[0] = '\n';
				head++;
			}
			head[0] = 0;
		}
		
		memcpy(head, pData->m_lines[i].m_text, pData->m_lines[i].m_length);
		head += pData->m_lines[i].m_length;
		head[0] = 0;
	}
	
	pData->m_cachedData[sz - 1] = 0;
	
	return pData->m_cachedData;
}

// note: you MUST free this!!
static char* TextInput_GetSelectedText(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// if we have nothing selected
	if (!TextInput_SelectedAnything(this)) return NULL;
	
	// note: This will end up allocating some bytes more. This is fine, I'm not going
	// to change it unless it proves problematic
	size_t sz = 1;
	
	for (int i = pData->m_selectY1; i <= pData->m_selectY2; i++)
	{
		sz += pData->m_lines[i].m_length + (i != 0);
	}
	
	char* pMem = MmAllocate(sz);
	if (!pMem) return NULL;
	*pMem = 0;
	char* pHead = pMem;
	
	// all on one line
	if (pData->m_selectY1 == pData->m_selectY2)
	{
		int length = pData->m_selectX2 - pData->m_selectX1;
		memcpy(pHead, &pData->m_lines[pData->m_selectY1].m_text[pData->m_selectX1], (size_t)length);
		pHead += length;
		
		// make sure we also add the null terminator
		*pHead = '\0';
		pHead++;
	}
	else
	{
		// add the first line:
		TextLine*
		pLine = &pData->m_lines[pData->m_selectY1];
		
		size_t len = pLine->m_length - pData->m_selectX1;
		memcpy(pHead, &pLine->m_text[pData->m_selectX1], len);
		pHead += len;
		// add a new line
		*pHead = '\n';
		pHead++;
		
		// for each line between selectY1 and selectY2
		for (int ln = pData->m_selectY1 + 1; ln < pData->m_selectY2; ln++)
		{
			pLine = &pData->m_lines[ln];
			memcpy(pHead, pLine->m_text, pLine->m_length);
			pHead += pLine->m_length;
			*pHead = '\n';
			pHead++;
		}
		
		// finally, add selectY2
		pLine = &pData->m_lines[pData->m_selectY2];
		memcpy(pHead, pLine->m_text, (size_t)pData->m_selectX2);
		
		// don't forget the null terminator
		pHead += pData->m_selectX2;
		*pHead = '\0';
	}
	
	return pMem;
}

static void TextInput_RequestRepaint(Control* this)
{
	WindowAddEventToMasterQueue(TextInput_GetData(this)->m_pWindow, EVENT_CTLREPAINT, this->m_comboID, 0);
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
	
	pData->m_dirty = true;
	pData->m_currentLineEnding = LEND_LF;
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
	
	this->m_dirty = true;
	
	TextLine* pLine = &this->m_lines[this->m_num_lines++];
	
	memset(pLine, 0, sizeof *pLine);
	pLine->m_lineEnding = this->m_currentLineEnding;
	
	return pLine;
}

static void TextInput_UpdateScrollBars(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	Window* pWindow = pData->m_pWindow;
	
	int scrollPos, scrollMax;
	
	// Vertical Scroll bar
	scrollPos = pData->m_scrollY;
	scrollMax = (pData->m_maxScrollY) - (this->m_rect.bottom - this->m_rect.top) + 7;
	if (scrollPos < 0) scrollPos = 0;
	if (scrollMax < 1) scrollMax = 1;
	bool rpVert = GetScrollBarPos(pWindow, GetVertScrollBarComboID(this->m_comboID)) != scrollPos || GetScrollBarMax(pWindow, GetVertScrollBarComboID(this->m_comboID)) != scrollMax;
	SetScrollBarPos(pWindow, GetVertScrollBarComboID(this->m_comboID), scrollPos);
	SetScrollBarMax(pWindow, GetVertScrollBarComboID(this->m_comboID), scrollMax);
	
	// Horizontal Scroll bar
	scrollPos = pData->m_scrollX;
	scrollMax = (pData->m_maxScrollX) - (this->m_rect.right - this->m_rect.left) + 1;
	if (scrollPos < 0) scrollPos = 0;
	if (scrollMax < 1) scrollMax = 1;
	bool rpHorz = GetScrollBarPos(pWindow, GetHorzScrollBarComboID(this->m_comboID)) != scrollPos || GetScrollBarMax(pWindow, GetHorzScrollBarComboID(this->m_comboID)) != scrollMax;
	SetScrollBarPos(pWindow, GetHorzScrollBarComboID(this->m_comboID), scrollPos);
	SetScrollBarMax(pWindow, GetHorzScrollBarComboID(this->m_comboID), scrollMax);
	
	unsigned oldFont = VidSetFont(FONT_BASIC);
	if (rpHorz) CallControlCallback(pWindow, GetHorzScrollBarComboID(this->m_comboID), EVENT_PAINT, 0, 0);
	if (rpVert) CallControlCallback(pWindow, GetVertScrollBarComboID(this->m_comboID), EVENT_PAINT, 0, 0);
	VidSetFont(oldFont);
}

static void TextInput_RecalcMaxScroll(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	pData->m_maxScrollX = 0;
	pData->m_maxScrollY = pData->m_num_lines * TextInput_GetLineHeight(this);
	
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		TextLine* line = &pData->m_lines[i];
		if (pData->m_maxScrollX < line->m_lengthPixels)
			pData->m_maxScrollX = line->m_lengthPixels;
	}
	
	TextInput_UpdateScrollBars(this);
}

static void TextInput_CalculateLinePixelWidth(Control* this, TextLine* line)
{
	unsigned oldFont = VidSetFont(TextInput_GetFont(this));
	
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
	
	int lineEnding = LEND_LF;
	
	// if we didn't find the next newline, we're on the last line
	if (!nextNl)
	{
		nextNl = pText + strlen(pText);
		*bHitEnd = true;
	}
	
	size_t sz = nextNl - pText;
	if (pText[sz - 1] == '\r')
	{
		lineEnding |= LEND_CR;
		sz--;
	}
	
	// Append a new line.
	TextLine* line = TextInput_AddLine(this);
	
	size_t cap = ROUND_TO_PO2(sz + 1, MIN_CAPACITY);
	
	line->m_text     = MmAllocate(cap);
	line->m_capacity = cap;
	line->m_length   = sz;
	memcpy(line->m_text, pText, sz);
	line->m_text[sz] = 0;
	line->m_lengthPixels = 0;
	line->m_lineEnding   = lineEnding;
	
	TextInput_CalculateLinePixelWidth(this, line);
	
	if (pData->m_maxScrollX < line->m_lengthPixels)
		pData->m_maxScrollX = line->m_lengthPixels;
	
	pData->m_maxScrollY += TextInput_GetLineHeight(this);
	
	TextInput_UpdateScrollBars(this);
	
	return nextNl + 1;
}

static void TextInput_RepaintLine(Control* pCtl, int lineNum);
static void TextInput_RepaintLineAndBelow(Control* pCtl, int lineNum);

static void TextInput_MakeCursorStayUpFor(Control* this, int ms)
{
	TextInputDataEx* pData   = TextInput_GetData(this);
	pData->m_bCursorFlash    = false;
	pData->m_ignoreTicksTill = GetTickCount() + ms;
	TextInput_RepaintLine(this, pData->m_cursorY);
}

static void TextInput_SetLineText(Control* pCtl, int lineNum, const char* pText, size_t sLen)
{
	TextInputDataEx* pData = TextInput_GetData(pCtl);
	TextLine* pLine = &pData->m_lines[lineNum];
	
	// get rid of what was there before
	if (pLine->m_text)
	{
		MmFree(pLine->m_text);
	}
	
	pLine->m_text     = MmAllocate(sLen + 1);
	pLine->m_capacity = sLen + 1;
	pLine->m_length   = sLen;
	
	strcpy(pLine->m_text, pText);
	
	TextInput_CalculateLinePixelWidth(pCtl, pLine);
	
	TextInput_RepaintLine(pCtl, lineNum);
	
	pData->m_dirty = true;
}

static void TextInput_AppendText(Control* this, int line, int pos, const char* pText, size_t sLen)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	TextLine* pLine = &pData->m_lines[line];
	
	pData->m_dirty = true;
	
	// Ensure that 'pos' is within bounds.
	if (pos < 0)
		pos = 0;
	if (pos >= (int)pLine->m_length)
		pos  = (int)pLine->m_length;
	
	if (pLine->m_length + sLen + 2 >= pLine->m_capacity)
	{
		// resize the thing
		pLine->m_capacity += sLen + 2;
		//pLine->m_capacity = ROUND_TO_PO2(pLine->m_capacity, MIN_CAPACITY);
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
	unsigned oldFont = VidSetFont(TextInput_GetFont(this));
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
		TextInput_UpdateScrollBars(this);
	}
	
	TextInput_RepaintLine(this, line);
}

static void TextInput_EraseChars(Control* this, int line, int pos, int sLen)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	TextLine* pLine = &pData->m_lines[line];
	
	pData->m_dirty = true;
	
	// Ensure that the pos and sLen parms are valid.
	
	if (pos < 0)
	{
		sLen += pos;
		pos = 0;
	}
	
	if (pos >= (int)pLine->m_length) return;
	if (sLen < 0) return;
	
	if ((uint32_t)pos + sLen >= pLine->m_length)
	{
		sLen = (int)pLine->m_length - pos;
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
	TextInput_UpdateScrollBars(this);
	
	// Repaint the line.
	TextInput_RepaintLine(this, line);
}

static void TextInput_InsertNewLines(Control* this, int linePos, int lineCount)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	pData->m_dirty = true;
	
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
		
		pLine->m_text    = MmAllocate(MIN_CAPACITY);
		pLine->m_text[0] = 0;
		pLine->m_capacity = MIN_CAPACITY;
	}
	
	pData->m_maxScrollY += TextInput_GetLineHeight(this) * lineCount;
	
	TextInput_UpdateScrollBars(this);
	TextInput_RepaintLineAndBelow(this, linePos);
}

static void TextInput_InsertNewLine(Control* this, int linePos)
{
	TextInput_InsertNewLines(this, linePos, 1);
}

static void TextInput_SplitLines(Control* this, int line, int splitAt)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// Create a new line.
	if (splitAt == 0)
	{
		TextInput_InsertNewLine(this, line);
		return;
	}
	
	TextInput_InsertNewLine(this, line + 1);
	
	// Set said line's text to the current line's text at `splitAt` offset.
	TextLine* pSrcLine = &pData->m_lines[line];
	
	// (if the split is in front of the string, we've created a new line!)
	if (splitAt >= (int)pSrcLine->m_length) return;
	
	const char* pNewText = pSrcLine->m_text + splitAt;
	
	TextInput_SetLineText(this, line + 1, pNewText, strlen(pNewText));
	
	pSrcLine->m_text[splitAt] = 0;
	pSrcLine->m_length = splitAt;
	
	TextInput_RepaintLine(this, line);
}

static void TextInput_EraseConsecutiveLines(Control* this, int lineStart, int lineCount)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	pData->m_dirty = true;
	
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
	pData->m_maxScrollY -= lineCount * TextInput_GetLineHeight(this);
	
	TextInput_UpdateScrollBars(this);
	TextInput_RepaintLineAndBelow(this, lineStart);
}

static void TextInput_EraseLine(Control* this, int line)
{
	TextInput_EraseConsecutiveLines(this, line, 1);
}

static void TextInput_AppendChar(Control* this, int line, int pos, char c)
{
	char str[2];
	str[0] = c;
	str[1] = 0;
	return TextInput_AppendText(this, line, pos, str, 1);
}

static void TextInput_EraseChar(Control* this, int line, int pos)
{
	return TextInput_EraseChars(this, line, pos, 1);
}

SAI void Swap(int* a1, int* a2)
{
	int temp = *a1;
	*a1 = *a2;
	*a2 = temp;
}

static void OrderTwo(int* a1, int* a2)
{
	if (*a1 > *a2)
		Swap(a1, a2);
}

// Ensures that selectX1,selectY1 < selectX2,selectY2
static void TextInput_EnsureSelectionCorrectness(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int64_t max_int = INT_MAX;
	int64_t idx1 = (int64_t)pData->m_selectY1 * max_int + pData->m_selectX1;
	int64_t idx2 = (int64_t)pData->m_selectY2 * max_int + pData->m_selectX2;
	
	if (idx1 > idx2)
	{
		Swap(&pData->m_selectX1, &pData->m_selectX2);
		Swap(&pData->m_selectY1, &pData->m_selectY2);
	}
}

static void TextInput_Select(Control* this, int startY, int startX, int endY, int endX)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int selectY1 = pData->m_selectY1;
	int selectY2 = pData->m_selectY2;
	
	pData->m_selectX1 = startX;
	pData->m_selectY1 = startY;
	pData->m_selectX2 = endX;
	pData->m_selectY2 = endY;
	
	TextInput_EnsureSelectionCorrectness(this);
	
	for (int i = selectY1; i <= selectY2; i++)
	{
		TextInput_RepaintLine(this, i);
	}
	
	for (int i = pData->m_selectY1; i <= pData->m_selectY2; i++)
	{
		// only repaint this line if it's never been repainted before
		if (i < selectY1 || selectY2 < i)
			TextInput_RepaintLine(this, i);
	}
}

static void TextInput_DetermineLineEnding(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int linesCount[4] = { 0 };
	
	for (size_t i = 0; i < pData->m_num_lines; i++)
	{
		TextLine* pLine = &pData->m_lines[i];
		
		// this is fine as long as someone doesn't mess with m_lineEnding and set it to an unstandard value
		linesCount[pLine->m_lineEnding]++;
	}
	
	int maxLines = 0, lineEndingWinner = LEND_LF;
	for (int i = 0; i < 4; i++)
	{
		if (maxLines < linesCount[i])
			maxLines = linesCount[i], lineEndingWinner = i;
	}
	
	pData->m_currentLineEnding = lineEndingWinner;
	//SLogMsg("Line ending detected: %s%s", &"\0CR"[!!(lineEndingWinner & LEND_CR)], &"\0LF"[!!(lineEndingWinner & LEND_LF)]);
}

void TextInput_OnNavPressed(Control* this, eNavDirection dir);

static void TextInput_SetText(Control* this, const char* pText, bool bRepaint)
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
	
	TextInput_UpdateScrollBars(this);
	
	if (bRepaint)
	{
		TextInput_RequestRepaint(this);
	}
	
	TextInput_Select(this, -1, -1, -1, -1);
	TextInput_DetermineLineEnding(this);
	
	if (~this->m_parm1 & TEXTEDIT_MULTILINE)
	{
		TextInputDataEx* pData = TextInput_GetData(this);
		pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
	}
}

// This concatenates 'lineSrc' to the end of 'lineDst'.
static void TextInput_JoinLines(Control* pCtl, int lineDst, int lineSrc)
{
	TextInputDataEx* pData = TextInput_GetData(pCtl);
	
	if (lineSrc < 0 || lineSrc >= (int)pData->m_num_lines) return;
	if (lineDst < 0 || lineDst >= (int)pData->m_num_lines) return;
	
	TextInput_AppendText(pCtl, lineDst, (int)pData->m_lines[lineDst].m_length, pData->m_lines[lineSrc].m_text, strlen(pData->m_lines[lineSrc].m_text));
	TextInput_EraseLine(pCtl, lineSrc);
}

static void TextInput_UpdateMode(Control* this)
{
	TextInput_RequestRepaint(this);
}

static void TextInput_PartialDraw(Control* this, Rectangle rect)
{
	rect.right --;
	rect.bottom--;
	//VidFillRectangle(GetRandom(), rect);
	VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, rect);
	rect.right ++;
	rect.bottom++;
	unsigned oldFont = VidSetFont(TextInput_GetFont(this));
	VidSetClipRect(&rect);
	
	TextInputDataEx* pData = TextInput_GetData(this);
	
	int scrollX = pData->m_scrollX;
	int scrollY = pData->m_scrollY;
	
	int lineX = this->m_rect.left + 3 - scrollX;
	int lineY = this->m_rect.top  + 3 - scrollY;
	
	int start = (pData->m_scrollY + rect.top - this->m_rect.top);
	int end   = (start + (rect.bottom - rect.top) + GetLineHeight());
	
	bool drawSelected = false;
	
	int lineHeight = GetLineHeight();
	
	start /= lineHeight;
	start--;
	end   /= lineHeight;
	end++;
	
	if (start < 0) start = 0;
	if (start >= (int)pData->m_num_lines) start = (int)pData->m_num_lines;
	if (end < 0) end = 0;
	if (end >= (int)pData->m_num_lines) end = (int)pData->m_num_lines;
	
	lineY += start * lineHeight;
	
	for (size_t i = start; i < (size_t)end; i++)
	{
		//VidTextOut(pData->m_lines[i].m_text, lineX, lineY, WINDOW_TEXT_COLOR, WINDOW_TEXT_COLOR_LIGHT);
		int charX = lineX;
		for (size_t j = 0; j < pData->m_lines[i].m_length && lineY + lineHeight > rect.top; j++)
		{
			drawSelected = TextInput_IsWithinSelectionBounds(this, j, i);
			
			char chr = pData->m_lines[i].m_text[j];
			
			// well, we reached the end of the line right now
			if (charX >= rect.right) break;
			
			int chrWidth = GetCharWidth(chr);
			const bool isWithinScreenBounds = rect.left <= charX + chrWidth && charX < rect.right;
			
			if (chr == '\t')
			{
				chrWidth = GetCharWidth('W') * TAB_WIDTH;
			}
			
			uint32_t textColor = WINDOW_TEXT_COLOR, backgdColor = WINDOW_TEXT_COLOR_LIGHT;
			
			if (drawSelected)
			{
				VidFillRect(SELECTED_ITEM_COLOR, charX, lineY, charX + chrWidth - 1, lineY + lineHeight - 1);
				textColor   = SELECTED_TEXT_COLOR;
				backgdColor = TRANSPARENT;
			}
			
			if (isWithinScreenBounds && chr != '\t')
			{
				VidPlotChar(chr, charX, lineY, textColor, backgdColor);
			}
			
			if (isWithinScreenBounds && pData->m_cursorX == (int)j && pData->m_cursorY == (int)i && !pData->m_bCursorFlash)
			{
				VidFillRect(textColor, charX, lineY, charX + CURSOR_THICKNESS - 1, lineY + lineHeight - 1);
			}
			
			charX += chrWidth;
		}
		
		uint32_t cursorColor = WINDOW_TEXT_COLOR;
		if (TextInput_IsWithinSelectionBounds(this, -1, i + 1)) // kind of hacky
		{
			cursorColor = SELECTED_TEXT_COLOR;
			VidFillRect(SELECTED_ITEM_COLOR, charX, lineY, charX + GetCharWidth('W') - 1, lineY + lineHeight - 1);
		}
		
		const bool isWithinScreenBounds = rect.left <= charX && charX < rect.right;
		if (isWithinScreenBounds && pData->m_cursorX == (int)pData->m_lines[i].m_length && pData->m_cursorY == (int)i && !pData->m_bCursorFlash)
		{
			VidFillRect(cursorColor, charX, lineY, charX + CURSOR_THICKNESS - 1, lineY + lineHeight - 1);
		}
		
		lineY += lineHeight;
		
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
	rect.left   += 3;
	rect.top    += 3;
	rect.right  -= 3;
	rect.bottom -= 3;
	
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
	
	TextInput_UpdateScrollBars(pCtl);
}

static void TextInput_RepaintLineAndBelow(Control* pCtl, int lineNum)
{
	if (lineNum < 0) lineNum = 0;
	
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int lineHeight = TextInput_GetLineHeight(pCtl);
	
	Rectangle refreshRect = pCtl->m_rect;
	refreshRect.left   += 3;
	refreshRect.right  -= 3;
	refreshRect.bottom -= 3;
	
	refreshRect.top += 3 + lineNum * lineHeight - this->m_scrollY;
	
	if (refreshRect.top < pCtl->m_rect.top + 3)
		refreshRect.top = pCtl->m_rect.top + 3;
	
	if (refreshRect.bottom <= refreshRect.top) return;
	
	TextInput_PartialDraw(pCtl, refreshRect);
}

static void TextInput_RepaintLine(Control* pCtl, int lineNum)
{
	if (lineNum < 0) return;
	
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int lineHeight = TextInput_GetLineHeight(pCtl);
	
	Rectangle refreshRect = pCtl->m_rect;
	refreshRect.left  += 3;
	refreshRect.right -= 3;
	
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
	int lineHeight = TextInput_GetLineHeight(this);
	
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

void TextInput_PreUpdateSelection(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// if 'shift' is not held, we should release our selection immediately
	if (!pData->m_bShiftHeld)
	{
		TextInput_Select(this, -1, -1, -1, -1);
		return;
	}
	
	pData->m_grabbedX = pData->m_cursorX;
	pData->m_grabbedY = pData->m_cursorY;
}

void TextInput_PostUpdateSelection(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	if (!pData->m_bShiftHeld)
		// we already discarded our selection in the pre-update, don't do anything here
		return;
	
	bool bHadSelectedAnything = TextInput_SelectedAnything(this);
	
	int minY = pData->m_grabbedY, maxY = pData->m_cursorY;
	OrderTwo(&minY, &maxY);
	
	if (!bHadSelectedAnything)
	{
		pData->m_selectX1 = pData->m_grabbedX;
		pData->m_selectY1 = pData->m_grabbedY;
		pData->m_selectX2 = pData->m_cursorX;
		pData->m_selectY2 = pData->m_cursorY;
		
		TextInput_EnsureSelectionCorrectness(this);
	}
	else
	{
		// which extent did we 'grab'?
		if (pData->m_grabbedX == pData->m_selectX2 && pData->m_grabbedY == pData->m_selectY2)
		{
			pData->m_selectX2 = pData->m_cursorX;
			pData->m_selectY2 = pData->m_cursorY;
		}
		if (pData->m_grabbedX == pData->m_selectX1 && pData->m_grabbedY == pData->m_selectY1)
		{
			pData->m_selectX1 = pData->m_cursorX;
			pData->m_selectY1 = pData->m_cursorY;
		}
		
		TextInput_EnsureSelectionCorrectness(this);
	}
	
	for (int y = minY; y <= maxY; y++)
	{
		TextInput_RepaintLine(this, y);
	}
}

void TextInput_RemoveArbitrarySection(Control* pCtl, int startX, int startY, int endX, int endY);

void TextInput_RemoveSelectedText(Control* this)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	if (!TextInput_SelectedAnything(this))
		return;
	
	TextInput_RemoveArbitrarySection(this, pData->m_selectX1, pData->m_selectY1, pData->m_selectX2, pData->m_selectY2);
	TextInput_Select(this, -1, -1, -1, -1);
}

void TextInput_OnNavPressed(Control* this, eNavDirection dir)
{
	TextInputDataEx* pData = TextInput_GetData(this);
	
	unsigned oldFont = VidSetFont(TextInput_GetFont(this));
	
	int nPageLines = (this->m_rect.bottom - this->m_rect.top - (GetLineHeight() * 2)) / GetLineHeight();
	
	if (nPageLines < 1)
		nPageLines = 1;
	
	if (dir != DIR_NONE)
		TextInput_PreUpdateSelection(this);
	
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
			TextInput_UpdateScrollBars(this);
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
			TextInput_UpdateScrollBars(this);
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
			TextInput_UpdateScrollBars(this);
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
			TextInput_UpdateScrollBars(this);
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
			TextInput_UpdateScrollBars(this);
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
			
			TextInput_UpdateScrollBars(this);
			
			break;
		}
		case DIR_HOME:
		{
			pData->m_cursorX = 0;
			TextInput_RepaintLine(this, pData->m_cursorY);
			TextInput_UpdateScrollBars(this);
			break;
		}
		case DIR_END:
		{
			pData->m_cursorX = (int)pData->m_lines[pData->m_cursorY].m_length;
			TextInput_RepaintLine(this, pData->m_cursorY);
			TextInput_UpdateScrollBars(this);
			break;
		}
		case DIR_NONE:
			break;
	}
	
	if (dir != DIR_NONE)
	{
		TextInput_MakeCursorStayUpFor(this, 500);
		TextInput_PostUpdateSelection(this);
	}
	
	TextInput_UpdateScrollBars(this);
	TextInput_ClampCursorWithinScrollBounds(this);
	
	VidSetFont(oldFont);
}

void TextInput_GoToXY(Control* pCtl, int x, int y);

void TextInput_RemoveArbitrarySection(Control* pCtl, int startX, int startY, int endX, int endY)
{
	int y1 = startY + 1, y2 = endY - 1;
	
	TextInput_GoToXY(pCtl, startX, startY);
	
	// everything on the same line:
	if (startY == endY)
	{
		TextInput_EraseChars(pCtl, startY, startX, endX - startX);
	}
	else
	{
		TextInput_EraseChars(pCtl, startY, startX, INT_MAX);
		TextInput_EraseChars(pCtl, endY, 0, endX);
		
		//OPTIMIZE: don't erase the line...
		TextInput_JoinLines(pCtl, startY, endY);
		
		// if there are any lines in the middle to remove:
		if (y1 <= y2)
			TextInput_EraseConsecutiveLines(pCtl, y1, y2 - y1 + 1);
	}
}

void TextInput_InsertArbitraryText(Control* pCtl, const char* pText)
{
	if (!pText) return;
	if (*pText == '\0') return;
	
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	// Step 0. Update the dirty flag, if needed
	
	// There are up to three parts to a piece of text:
	// [Part before the first new line] [Middle of the text] [Last line of the text]
	
	// Step 1. Check if there are any new lines at all.
	const char* pFirstNewLine = strchr(pText, '\n');
	
	// If not, we can simply insert the text.
	if (!pFirstNewLine)
	{
		int len = (int)strlen(pText);
		TextInput_AppendText(pCtl, this->m_cursorY, this->m_cursorX, pText, len);
		this->m_cursorX += len;
		TextInput_RepaintLine(pCtl, this->m_cursorY);
		
		return;
	}
	
	bool bIsMultiLine = (pCtl->m_parm1 & TEXTEDIT_MULTILINE);
	
	// Step 2. Split the line down m_cursorX.
	if (bIsMultiLine)
		TextInput_SplitLines(pCtl, this->m_cursorY, this->m_cursorX);
	
	// Step 3. Insert the first part of the text.
	TextInput_AppendText(pCtl, this->m_cursorY, this->m_cursorX, pText, pFirstNewLine - pText);
	
	if (!bIsMultiLine)
		return;
	
	const char* pTextAfter = pFirstNewLine + 1;
	
	// Step 4. Count the number of new lines:
	int nNewLines = 0;
	const char* pTextWork = pTextAfter;
	while (*pTextWork)
	{
		if (*pTextWork == '\n') nNewLines++;
		pTextWork++;
	}
	
	// Step 5. Insert as many new lines as needed.
	TextInput_InsertNewLines(pCtl, this->m_cursorY + 1, nNewLines);
	
	// Step 6. Set the text of these lines accordingly.
	int nLineNum = this->m_cursorY + 1;
	pTextWork = pTextAfter;
	
	int nDestPos = 0;
	
	while (*pTextWork)
	{
		const char* pNewLine = strchr(pTextWork, '\n');
		if (!pNewLine)
		{
			// this is the last line.
			nDestPos = (int)strlen(pTextWork);
			TextInput_AppendText(pCtl, nLineNum, 0, pTextWork, nDestPos);
			break;
		}
		
		TextInput_SetLineText(pCtl, nLineNum++, pTextWork, pNewLine - pTextWork);
		pTextWork = pNewLine + 1;
	}
	
	// Step 7. Go to the end of the pasted text.
	int oldCursorY = this->m_cursorY;
	this->m_cursorY = nLineNum;
	this->m_cursorX = nDestPos;
	// repaint the old line
	TextInput_RepaintLine(pCtl, oldCursorY);
	// this is hacky, but OnNavPressed a no op key
	TextInput_OnNavPressed(pCtl, DIR_NONE);
}

static char* TextInput_FetchClipboardContents()
{
	ClipboardVariant* pClip = CbGetCurrentVariant();
	
	char* pOut = NULL;
	
	switch (pClip->m_type)
	{
		case CLIPBOARD_DATA_TEXT:
			pOut = MmStringDuplicate(pClip->m_short_str);
			break;
		case CLIPBOARD_DATA_LARGE_TEXT:
			pOut = MmStringDuplicate(pClip->m_char_str);
			break;
	}
	
	CbRelease(pClip);
	
	return pOut;
}

void TextInput_GoToXY(Control* pCtl, int x, int y)
{
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int oldCursorY = this->m_cursorY;
	// put it somewhere invalid, so it doesn't draw when we do...
	this->m_cursorY = -1;
	TextInput_RepaintLine(pCtl, oldCursorY);
	
	// make sure to keep them within bounds
	if (y >= (int)this->m_num_lines) y = (int)this->m_num_lines - 1;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= (int)this->m_lines[y].m_length) x = (int)this->m_lines[y].m_length;
	
	// now set the x/y coords:
	this->m_cursorX = x;
	this->m_cursorY = y;
	
	// and hackily trigger a no op OnNavPressed:
	TextInput_OnNavPressed(pCtl, DIR_NONE);
}

void TextInput_OffsetToXY(Control* pCtl, int* xOut, int* yOut, int offset)
{
	TextInputDataEx* this = TextInput_GetData(pCtl);
	
	int xo = 0, yo = 0;
	
	while (true)
	{
		if ((int)this->m_num_lines <= yo) break;
		
		int line_length = (int)this->m_lines[yo].m_length;
		
		if (offset <= line_length)
		{
			xo = offset;
			break;
		}
		
		offset -= line_length + 1;
		yo++;
	}
	
	*xOut = xo;
	*yOut = yo;
}

void TextInput_PerformCommand(Control *pCtl, int command, void* parm)
{
	switch (command)
	{
		case TEDC_GETLINEEND:
		{
			TextInputDataEx* pData = TextInput_GetData(pCtl);
			*((int*)parm) = pData->m_currentLineEnding;
			break;
		}
		case TEDC_SETLINEEND:
		{
			TextInputDataEx* pData = TextInput_GetData(pCtl);
			int prm = *((int*)parm);
			
			if (prm < 0 || prm >= 4)
				prm = LEND_LF;
			
			pData->m_currentLineEnding = prm;
			break;
		}
		case TEDC_PASTE:
		{
			char* pThing = TextInput_FetchClipboardContents();
			TextInput_RemoveSelectedText(pCtl);
			TextInput_InsertArbitraryText(pCtl, pThing);
			if (pThing) MmFree(pThing);
			break;
		}
		case TEDC_COPY:
		case TEDC_CUT:
		{
			char* pThing = TextInput_GetSelectedText(pCtl);
			
			if (pThing)
			{
				CbCopyText(pThing);
				MmFree(pThing);
			}
			
			// note: aside from copying the text to the clipboard, we also remove it
			if (command == TEDC_CUT)
			{
			case TEDC_DELETE:
				TextInput_RemoveSelectedText(pCtl);
			}
			
			break;
		}
		case TEDC_SELECT_ALL:
		{
			TextInputDataEx* pData = TextInput_GetData(pCtl);
			
			TextInput_Select(pCtl, 0, 0, pData->m_num_lines - 1, pData->m_lines[pData->m_num_lines - 1].m_length);
			break;
		}
		case TEDC_INSERT:
		{
			TextInput_InsertArbitraryText(pCtl, parm);
			break;
		}
		default:
		{
			SLogMsg("TextInput: The command %d is not implemented", command);
			break;
		}
	}
}

void TextInput_ShowContextMenu(Control* this, Window* pWindow, int x, int y)
{
	// manually construct a context menu.
	WindowMenu rootEnt;
	WindowMenu items[8];
	WindowMenu *undoEnt, *spacerEnt, *cutEnt, *copyEnt, *pasteEnt, *deleteEnt, *selectAllEnt, *spacer2Ent;
	undoEnt      = &items[0];
	spacerEnt    = &items[1];
	cutEnt       = &items[2];
	copyEnt      = &items[3];
	pasteEnt     = &items[4];
	deleteEnt    = &items[5];
	spacer2Ent   = &items[6];
	selectAllEnt = &items[7];
	
	memset(items, 0, sizeof items);
	memset(&rootEnt, 0, sizeof rootEnt);
	
	strcpy(rootEnt.sText, "Menu");
	rootEnt.pMenuEntries = items;
	rootEnt.nMenuEntries = (int)ARRAY_COUNT(items);
	rootEnt.nLineSeparators = 2;
	rootEnt.nWidth       = 100;
	rootEnt.pWindow      = pWindow;
	
	// fill in one of the entries, then copy it to others
	memset(undoEnt, 0, sizeof *undoEnt);
	undoEnt->pWindow         = pWindow;
	undoEnt->pMenuEntries    = NULL;
	undoEnt->nMenuEntries    = 0;
	undoEnt->nMenuComboID    = TEDC_UNDO;
	undoEnt->nOrigCtlComboID = this->m_comboID;
	undoEnt->bOpen           = false;
	undoEnt->bHasIcons       = false;
	undoEnt->nLineSeparators = 0;
	undoEnt->pOpenWindow     = NULL;
	undoEnt->nIconID         = ICON_NULL;
	undoEnt->nWidth          = rootEnt.nWidth;
	
	// TODO: if (TextInput_CanUndo(this))
	undoEnt->bDisabled = true;
	
	for (int i = 1; i < (int)ARRAY_COUNT(items); i++)
		memcpy(&items[i], undoEnt, sizeof *undoEnt);
	
	strcpy(spacerEnt   ->sText, "");
	strcpy(spacer2Ent  ->sText, "");
	strcpy(undoEnt     ->sText, "Undo");
	strcpy(cutEnt      ->sText, "Cut");
	strcpy(copyEnt     ->sText, "Copy");
	strcpy(pasteEnt    ->sText, "Paste");
	strcpy(deleteEnt   ->sText, "Delete");
	strcpy(selectAllEnt->sText, "Select all");
	
	spacerEnt   ->nMenuComboID = -1;
	spacer2Ent  ->nMenuComboID = -2;
	undoEnt     ->nMenuComboID = TEDC_UNDO;
	cutEnt      ->nMenuComboID = TEDC_CUT;
	copyEnt     ->nMenuComboID = TEDC_COPY;
	pasteEnt    ->nMenuComboID = TEDC_PASTE;
	deleteEnt   ->nMenuComboID = TEDC_DELETE;
	selectAllEnt->nMenuComboID = TEDC_SELECT_ALL;
	
	TextInputDataEx* pData = TextInput_GetData(this);
	
	// the "select all" is enabled if there is text:
	if (pData->m_num_lines > 1 || pData->m_lines[0].m_length > 0)
		selectAllEnt->bDisabled = false;
	
	// the 'cut', 'copy' and 'delete' items are enabled if there's a selection
	if (TextInput_SelectedAnything(this))
		copyEnt->bDisabled = cutEnt->bDisabled = deleteEnt->bDisabled = false;
	
	// the 'paste' item is enabled if there's anything in the clipboard
	ClipboardVariant* pVar = CbGetCurrentVariant();
	bool bAnythingInClip = pVar->m_type == CLIPBOARD_DATA_LARGE_TEXT || pVar->m_type == CLIPBOARD_DATA_TEXT;
	CbRelease(pVar);
	if (bAnythingInClip)
		pasteEnt->bDisabled = false;
	
	// the separators probably shouldn't be disabled
	spacerEnt->bDisabled = spacer2Ent->bDisabled = false;
	
	// spawn the menu now!
	SpawnMenu(pWindow, &rootEnt, x, y);
}

bool WidgetTextEditView2_OnEvent(Control* this, int eventType, long parm1, long parm2, Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			TextInputDataEx* pData = MmAllocate(sizeof(TextInputDataEx));
			memset(pData, 0, sizeof *pData);
			this->m_dataPtr = pData;
			
			pData->m_currentLineEnding = LEND_LF;
			pData->m_pWindow = pWindow;
			pData->m_selectX1 = pData->m_selectX2 = pData->m_selectY1 = pData->m_selectY2 = -1;
			
			// Create two scroll bars.
			if (this->m_parm1 & TEXTEDIT_MULTILINE)
			{
				pData->m_font = TEXT_EDIT_FONT;
				
				if (this->m_parm1 & TEXTEDIT_STYLING)
					pData->m_font = SYSTEM_FONT;
				
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
				
				unsigned oldFont = VidSetFont(TextInput_GetFont(this));
				AddControlEx(pWindow, CONTROL_VSCROLLBAR, vertSBAnchor, vertSBRect, NULL, GetVertScrollBarComboID(this->m_comboID), 100, GetLineHeight()   << 16);
				AddControlEx(pWindow, CONTROL_HSCROLLBAR, horzSBAnchor, horzSBRect, NULL, GetHorzScrollBarComboID(this->m_comboID), 100, GetCharWidth('W') << 16);
				VidSetFont(oldFont);
				
				// shrink ourselves
				this->m_rect.right  -= SCROLL_BAR_SIZE;
				this->m_rect.bottom -= SCROLL_BAR_SIZE;
			}
			else
			{
				pData->m_font = TEXT_EDIT_FONT;
				
				this->m_rect.bottom = this->m_rect.top + 6 + TextInput_GetLineHeight(this);
			}
			
			TextInput_SetText(this, "", false);
			
			break;
		}
		case EVENT_TICK:
		{
			TextInputDataEx* pData = TextInput_GetData(this);
			
			if (pData->m_ignoreTicksTill > GetTickCount()) break;
			
			if (this->m_parm1 & TEXTEDIT_READONLY) break;
			
			if (!this->m_bFocused || !pWindow->m_isSelected)
			{
				if (!pData->m_bCursorFlash)
				{
					pData->m_bCursorFlash = true;
					TextInput_RepaintLine(this, pData->m_cursorY);
				}
			}
			else
			{
				pData->m_ignoreTicksTill = GetTickCount() + g_TextCursorFlashSpeed;
				pData->m_bCursorFlash ^= 1;
				TextInput_RepaintLine(this, pData->m_cursorY); // TODO: More efficient way.
			}
			
			break;
		}
		case EVENT_CLICKCURSOR:
		case EVENT_SCROLLDONE:
		{
			TextInputDataEx* pData = TextInput_GetData(this);
			
			// if we're a single line text control, break.
			if (~this->m_parm1 & TEXTEDIT_MULTILINE) break;
			
			int lkX = pData->m_knownScrollBarX;
			int lkY = pData->m_knownScrollBarY;
			pData->m_knownScrollBarX = GetScrollBarPos(pWindow, GetHorzScrollBarComboID(this->m_comboID));
			pData->m_knownScrollBarY = GetScrollBarPos(pWindow, GetVertScrollBarComboID(this->m_comboID));
			
			if (lkX != pData->m_knownScrollBarX || lkY != pData->m_knownScrollBarY)
			{
				pData->m_scrollX = pData->m_knownScrollBarX;
				pData->m_scrollY = pData->m_knownScrollBarY;
				TextInput_OnScrollDone(this);
			}
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
			
			borderRect = rect;
			
			rect.left += 2;
			rect.top  += 2;
			rect.right  -= 2;
			rect.bottom -= 2;
			
			TextInput_PartialDraw(this, rect);
			
			DrawEdge(borderRect, DRE_SUNKENINNER | DRE_SUNKENOUTER, 0);
			
			break;
		}
		case EVENT_RIGHTCLICKRELEASE:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (!RectangleContains(&this->m_rect, &mouseClickPos))
				break;
			
			Rectangle rect = GetWindowClientRect(pWindow, true);
			
			mouseClickPos.x += rect.left;
			mouseClickPos.y += rect.top;
			
			TextInput_ShowContextMenu(this, pWindow, mouseClickPos.x, mouseClickPos.y);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (!RectangleContains(&this->m_rect, &mouseClickPos))
			{
				// if no one took focus from us before us, make us not focused at all
				//if (this->m_bFocused)
				//	SetFocusedControl(pWindow, -1);
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
			
			bool bWasAnythingSelected = TextInput_SelectedAnything(this);
			TextInput_RemoveSelectedText(this);
			
			// kind of hacky.. Make it so shift is disabled so we don't accidentally select the character(s) we just wrote
			bool bIsShiftHeld = pData->m_bShiftHeld;
			pData->m_bShiftHeld = false;
			
			char inChar = (char)parm1;
			
			if ((inChar == '\n' || inChar == '\r') && (~this->m_parm1 & TEXTEDIT_MULTILINE))
				break;
			
			if ((char)parm1 == '\x7F')
			{
				if (bWasAnythingSelected)
				{
				}
				else if (pData->m_cursorX == 0)
				{
					if (pData->m_cursorY > 0)
					{
						TextInput_OnNavPressed(this, DIR_LEFT);
						TextInput_JoinLines(this, pData->m_cursorY, pData->m_cursorY + 1);
					}
				}
				else
				{
					TextInput_EraseChar(this, pData->m_cursorY, pData->m_cursorX - 1);
					TextInput_OnNavPressed(this, DIR_LEFT);
				}
			}
			else if (parm1 == '\r' || parm1 == '\n')
			{
				TextInput_SplitLines(this, pData->m_cursorY, pData->m_cursorX);
				TextInput_OnNavPressed(this, DIR_RIGHT);
			}
			else
			{
				TextInput_AppendChar(this, pData->m_cursorY, pData->m_cursorX, parm1);
				TextInput_OnNavPressed(this, DIR_RIGHT);
			}
			
			pData->m_bShiftHeld = bIsShiftHeld;
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (!this->m_bFocused) break;
			
			TextInputDataEx* pData = TextInput_GetData(this);
			
			// TODO: handle these better.  Ignore 0xE0 for now.
			if (parm1 == 0xE0) {
				pData->m_ignoreNextKeyCode = true;
				break;
			}
			else if (pData->m_ignoreNextKeyCode) {
				pData->m_ignoreNextKeyCode = false;
				
				// don't actually if it's a press. Better safe than sorry
				if (parm1 & 0x80)
					break;
			}
			
			// If the key was released, break; we don't really need to process release events
			if (parm1 & 0x80)
			{
				int code = (int)(unsigned char)parm1;
				if ((code & 0x7F) == KEY_LSHIFT || (code & 0x7F) == KEY_RSHIFT) {
					pData->m_bShiftHeld = false;
				}
				break;
			}
			
			int code = parm1;
			
			switch (code)
			{
				case KEY_DELETE:
				{
					if (TextInput_SelectedAnything(this))
					{
						TextInput_RemoveSelectedText(this);
					}
					else
					{
						TextLine* pCurrentLine = &pData->m_lines[pData->m_cursorY];
						if (pData->m_cursorX == (int)pCurrentLine->m_length)
						{
							TextInput_JoinLines(this, pData->m_cursorY, pData->m_cursorY + 1);
						}
						else
						{
							TextInput_EraseChar(this, pData->m_cursorY, pData->m_cursorX);
						}
						TextInput_MakeCursorStayUpFor(this, 500);
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
		case EVENT_COMMAND:
		{
			if (parm1 != this->m_comboID) break;
			
			TextInput_PerformCommand(this, parm2, NULL);
			
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
			Control *p = &pWindow->m_pControlArray[i];
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return;
			TextInput_SetText(p, pText, true);
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
			Control *p = &pWindow->m_pControlArray[i];
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return;
			TextInput_GetData(p)->m_dirty = false;
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
			Control *p = &pWindow->m_pControlArray[i];
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return false;
			return TextInput_GetData(p)->m_dirty;
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
			Control *p = &pWindow->m_pControlArray[i];
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return NULL;
			return TextInput_GetRawText(p);
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
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return;
			
			p->m_parm1 = mode;
			TextInput_UpdateMode(p);
		}
	}
}

void TextInputSetFont(Window *pWindow, int comboID, unsigned font)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			Control *p = &pWindow->m_pControlArray[i];
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return;
			
			TextInputDataEx* pData = TextInput_GetData(p);
			if (pData)
				pData->m_font = font;
			
			return;
		}
	}
}

void TextInputRequestCommand(Window *pWindow, int comboID, int command, void* parm)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			Control *p = &pWindow->m_pControlArray[i];
			if (p->OnEvent != WidgetTextEditView2_OnEvent) return;
			
			TextInput_PerformCommand(p, command, parm);
			
			return;
		}
	}
}
