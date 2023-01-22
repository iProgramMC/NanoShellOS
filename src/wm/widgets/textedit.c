/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Text Area controls
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

#define TAB_WIDTH 4

void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);

void CtlTextInputUpdateScrollSize(Control* this, Window* pWindow)
{
	int c = CountLinesInText(this->m_textInputData.m_pText);
	
	VidSetFont(this->m_textInputData.m_enableStyling ? SYSTEM_FONT : FONT_TAMSYN_MED_REGULAR);
	
	SetScrollBarMax (pWindow, -this->m_comboID, c * GetLineHeight());
	
	//TODO: optimize this!
	const char *text = this->m_textInputData.m_pText;
	int nLengthCur = 0, nLengthMax = 0;
	while (*text)
	{
		if (*text == '\n')
		{
			if (nLengthMax < nLengthCur)
				nLengthMax = nLengthCur;
			
			nLengthCur = 0;
		}
		else nLengthCur++;
		text++;
	}
	if (nLengthMax < nLengthCur)
		nLengthMax = nLengthCur;
	
	if (nLengthMax == 0)
		nLengthMax = 1;
	
	int cw = GetCharWidth('W');
	SetScrollBarMax (pWindow, 0x70000000 - this->m_comboID, nLengthMax * cw);
	
	VidSetFont(SYSTEM_FONT);
	
	CallControlCallback(pWindow,            - this->m_comboID, EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, 0x70000000 - this->m_comboID, EVENT_PAINT, 0, 0);
}

void CtlTextEditRecalcCurXY(Control *this);
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
	this->m_textInputData.m_textCursorIndex = slen;
	this->m_textInputData.m_textCursorSelStart = -1;
	this->m_textInputData.m_textCursorSelEnd   = -1;
	this->m_textInputData.m_textCursorX        = 0;
	this->m_textInputData.m_textCursorY        = 0;
	this->m_textInputData.m_scrollX            = 0;
	this->m_textInputData.m_scrollY            = 0;
	
	CtlTextEditRecalcCurXY (this);
	CtlTextInputUpdateScrollSize (this, pWindow);
	
	SetScrollBarPos(pWindow,            - this->m_comboID, 0);
	SetScrollBarPos(pWindow, 0x70000000 - this->m_comboID, 0);
	CallControlCallback(pWindow,            - this->m_comboID, EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, 0x70000000 - this->m_comboID, EVENT_PAINT, 0, 0);
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

int CtlAppendTextToAnywhere(Control* this, Window* pWindow, const char* textToAppend, int indexToAppendTo)
{
	if (indexToAppendTo < 0)
		return 0;
	if (indexToAppendTo > this->m_textInputData.m_textLength)
		return 0;
	
	int sl = strlen (textToAppend);
	
	if (this->m_textInputData.m_textLength >= this->m_textInputData.m_textCapacity-1)
	{
		//can't fit, need to expand
		int newCapacity = this->m_textInputData.m_textCapacity * 2, oldCapacity = this->m_textInputData.m_textCapacity;
		if (newCapacity < this->m_textInputData.m_textCapacity + sl)
			newCapacity = this->m_textInputData.m_textCapacity + sl + 2;
		if (newCapacity < 4096) newCapacity = 4096;//paradoxically, a smaller allocation occupies the same space as a 4096 byte alloc
		
		char* pText = (char*)MmAllocateK(newCapacity);
		if (!pText)
			return 0;
		
		memcpy (pText, this->m_textInputData.m_pText, oldCapacity);
		
		MmFreeK(this->m_textInputData.m_pText);
		this->m_textInputData.m_pText = pText;
		this->m_textInputData.m_textCapacity = newCapacity;
	}
	
	this->m_textInputData.m_textLength += sl;
	this->m_textInputData.m_pText[this->m_textInputData.m_textLength  ] = 0;
	memmove (&this->m_textInputData.m_pText[indexToAppendTo + sl], &this->m_textInputData.m_pText[indexToAppendTo], this->m_textInputData.m_textLength - sl - indexToAppendTo);
	
	memcpy (this->m_textInputData.m_pText + indexToAppendTo, textToAppend, sl);
	
	CtlTextInputUpdateScrollSize (this, pWindow);
	
	this->m_textInputData.m_dirty = true;
	
	return sl;
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

//TODO: Severely optimize this!
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

//TODO: only do relative movement
void CtlTextEditUpdateScrollXY(Control *this)
{
	VidSetFont(this->m_textInputData.m_enableStyling ? SYSTEM_FONT : FONT_TAMSYN_MED_REGULAR);
	int wChar = GetCharWidth('W'), hChar = GetLineHeight();
	VidSetFont(SYSTEM_FONT);
	
	int cx = this->m_textInputData.m_textCursorX * wChar;
	int cy = this->m_textInputData.m_textCursorY * hChar;
	
	if (this->m_textInputData.m_scrollX > cx)
		this->m_textInputData.m_scrollX = cx;
	if (this->m_textInputData.m_scrollY > cy)
		this->m_textInputData.m_scrollY = cy;
	
	int sizeX = this->m_rect.right  - this->m_rect.left - 26;
	int sizeY = this->m_rect.bottom - this->m_rect.top  - 26;
	if (this->m_textInputData.m_scrollX < cx - sizeX - wChar - 1)
		this->m_textInputData.m_scrollX = cx - sizeX - wChar - 1;
	if (this->m_textInputData.m_scrollY < cy - sizeY - wChar - 1)
		this->m_textInputData.m_scrollY = cy - sizeY - wChar - 1;
}
void CtlTextEditRecalcCurXY(Control *this)
{
	int xPos = 0, yPos = 0;
	const char *text = this->m_textInputData.m_pText;
	int index = 0;
	while (*text)
	{
		if (index == this->m_textInputData.m_textCursorIndex)
		{
			this->m_textInputData.m_textCursorX = xPos;
			this->m_textInputData.m_textCursorY = yPos;
			return;
		}
		
		if (*text == '\n')
		{
			xPos = 0;
			yPos++;
			text++;
			index++;
			continue;
		}
		
		//TODO: proper tab ceils to next multiple of TAB_WIDTH instead
		if (*text == '\t')
			xPos += TAB_WIDTH;
		else
			xPos++;
		text++;
		index++;
	}
	this->m_textInputData.m_textCursorX = xPos;
	this->m_textInputData.m_textCursorY = yPos;
}
// Recalculate the index based on the requested X/Y position
void CtlTextEditRecalcIndex(Control *this)
{
	int xPos = 0, yPos = 0;
	const char *text = this->m_textInputData.m_pText;
	int index = 0;
	while (*text)
	{
		if (*text == '\n')
		{
			if (this->m_textInputData.m_textCursorY == yPos && this->m_textInputData.m_textCursorX >= xPos)
			{
				this->m_textInputData.m_textCursorX = xPos;
				this->m_textInputData.m_textCursorIndex = index;
				return;
			}
			
			xPos = 0;
			yPos++;
			text++;
			index++;
			continue;
		}
		if (this->m_textInputData.m_textCursorY == yPos && this->m_textInputData.m_textCursorX == xPos)
		{
			this->m_textInputData.m_textCursorIndex = index;
			return;
		}
		
		//TODO: proper tab ceils to next multiple of TAB_WIDTH instead
		if (*text == '\t')
			xPos += TAB_WIDTH;
		else
			xPos++;
		text++;
		index++;
	}
	this->m_textInputData.m_textCursorIndex = this->m_textInputData.m_textLength;
	this->m_textInputData.m_textCursorX = xPos;
	this->m_textInputData.m_textCursorY = yPos;
}
bool WidgetTextEditView_OnEvent(Control* this, int eventType, int parm1, UNUSED int parm2, Window* pWindow)
{
	switch (eventType)
	{
		//case EVENT_RELEASECURSOR:
			//TODO: Allow selection across the text.
			//break;
			
		// -- Uncomment this if you want to get smooth scrolling through text.
		// I like to keep it on
		case EVENT_CLICKCURSOR:
		{
			UNUSED Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			//TODO: Allow change of cursor via click.
		}
		//fallthrough intended
		case EVENT_SCROLLDONE:
		{
			if (!this->m_textInputData.m_onlyOneLine)
			{
				bool bUpdate = false;
				int
				pos = GetScrollBarPos(pWindow, -this->m_comboID);
				if (this->m_textInputData.m_scrollY != pos)
					this->m_textInputData.m_scrollY  = pos, bUpdate = true;
				
				pos = GetScrollBarPos(pWindow, 0x70000000 - this->m_comboID);
				if (this->m_textInputData.m_scrollX != pos)
					this->m_textInputData.m_scrollX  = pos, bUpdate = true;
				
				if (bUpdate)
					WidgetTextEditView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			if (!RectangleContains (&this->m_rect, &mouseClickPos))
			{
				if (this->m_bFocused)
					SetFocusedControl(pWindow, -1);
				break;
			}
			
			SetFocusedControl(pWindow, this->m_comboID);
			
			if (this->m_textInputData.m_enableStyling && this->m_textInputData.m_pText && !this->m_textInputData.m_onlyOneLine)
			{
				Rectangle rectAroundMouse;
				rectAroundMouse.left   = mouseClickPos.x - 10;
				rectAroundMouse.top    = mouseClickPos.y - 10;
				rectAroundMouse.right  = mouseClickPos.x + 10;
				rectAroundMouse.bottom = mouseClickPos.y + 10;
				
				VidSetFont(this->m_textInputData.m_enableStyling ? SYSTEM_FONT : FONT_TAMSYN_MED_REGULAR);
				// Hit test
				Rectangle rk = this->m_rect;
				rk.left   += 2;
				rk.top    += 2;
				rk.right  -= 2;
				rk.bottom -= 2;
				if (this->m_textInputData.m_showLineNumbers)
					rk.left += LINE_NUM_GAP;
				
				int xPos = rk.left + 2 - this->m_textInputData.m_scrollX;
				int yPos = rk.top  + 2 - this->m_textInputData.m_scrollY;
				
				int xiPos = xPos;
				int wChar, hChar;
				wChar = GetCharWidth ('W');
				hChar = GetLineHeight();
				
				//go through the characters:
				const char *text = this->m_textInputData.m_pText;
				bool bLink = false, bBold = false;
				
				while (*text)
				{
					if (*text == '\n')
					{
						xPos = xiPos;
						yPos += hChar;
						text++;
						continue;
					}
					// Styling. Only need to care about this if measuring text
					else
					{
						if (this->m_textInputData.m_enableStyling && (unsigned char)(*text) >= (unsigned char)TIST_BOLD && (unsigned char)(*text) < (unsigned char)TIST_COUNT)
						{
							switch (*text) {
								case TIST_BOLD:   bBold = true;  break;
								case TIST_UNBOLD: bBold = false; break;
								case TIST_LINK:   bLink = true;  break;
								case TIST_UNLINK: bLink = false; break;
							}
							
							text++;
							continue;
						}
					}
					
					//if we need to bother checking
					if (xPos + wChar <  rectAroundMouse.left)   goto dont_bother;
					if (yPos + hChar <  rectAroundMouse.top)    goto dont_bother;
					if (xPos         >= rectAroundMouse.right)  goto dont_bother;
					if (yPos         >= rectAroundMouse.bottom) break;
					
					// Perform the real hit test here
					Rectangle rr;
					RECT(rr, xPos, yPos, GetCharWidth(*text), GetLineHeight());
					
					if (RectangleContains (&rr, &mouseClickPos) && bLink)
					{
						CallWindowCallbackAndControls(pWindow, EVENT_CLICK_CHAR, this->m_comboID, text - this->m_textInputData.m_pText);
						break;
					}
					
				dont_bother:
					//still increase xPos/yPos
					
					//TODO: proper tab ceils to next multiple of TAB_WIDTH instead
					xPos +=  GetCharWidth (*text);
					if (bBold) xPos++;
					text++;
				}
				VidSetFont(SYSTEM_FONT);
			}
			
			break;
		}
		case EVENT_SIZE:
			CtlTextInputUpdateScrollSize (this, pWindow);
			break;
		case EVENT_KEYRAW:
		{
			if (!this->m_bFocused) break;
			if (this->m_textInputData.m_readOnly) break;
			VidSetFont(this->m_textInputData.m_enableStyling ? SYSTEM_FONT : FONT_TAMSYN_MED_REGULAR);
			bool repaint = true;
			switch (parm1)
			{
				case KEY_ARROW_UP:
					this->m_textInputData.m_textCursorY--;
					if (this->m_textInputData.m_textCursorY < 0)
						this->m_textInputData.m_textCursorY = 0;
					CtlTextEditRecalcIndex(this);
					CtlTextEditUpdateScrollXY(this);
					break;
				case KEY_ARROW_DOWN:
					this->m_textInputData.m_textCursorY++;
					//limit checks will be done here
					CtlTextEditRecalcIndex(this);
					CtlTextEditUpdateScrollXY(this);
					break;
				case KEY_ARROW_LEFT:
					this->m_textInputData.m_textCursorIndex--;
					if (this->m_textInputData.m_textCursorIndex < 0)
						this->m_textInputData.m_textCursorIndex = 0;
					CtlTextEditRecalcCurXY(this);
					CtlTextEditUpdateScrollXY(this);
					break;
				case KEY_ARROW_RIGHT:
					this->m_textInputData.m_textCursorIndex++;
					if (this->m_textInputData.m_textCursorIndex > this->m_textInputData.m_textLength)
						this->m_textInputData.m_textCursorIndex = this->m_textInputData.m_textLength;
					CtlTextEditRecalcCurXY(this);
					CtlTextEditUpdateScrollXY(this);
					break;
				case KEY_HOME:
					this->m_textInputData.m_textCursorX = 0;
					//limit checks will be done here
					CtlTextEditRecalcIndex(this);
					CtlTextEditUpdateScrollXY(this);
					break;
				case KEY_END:
					//hack
					this->m_textInputData.m_textCursorX = 2147483647;
					//limit checks will be done here
					CtlTextEditRecalcIndex(this);
					CtlTextEditUpdateScrollXY(this);
					break;
				//case KEY_F1:
				//case KEY_F2:
				case KEY_DELETE:
					if (this->m_textInputData.m_textLength   >   this->m_textInputData.m_textCursorIndex)
						CtlRemoveCharFromAnywhere(this, pWindow, this->m_textInputData.m_textCursorIndex);
					break;
				default:
					repaint = false;
					break;
			}
			
			VidSetFont(SYSTEM_FONT);
			
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
				CtlTextEditRecalcCurXY(this);
				CtlTextEditUpdateScrollXY(this);
				if (this->m_textInputData.m_textCursorIndex < 0)
					this->m_textInputData.m_textCursorIndex = 0;
			}
			else
			{
				CtlAppendCharToAnywhere(this, pWindow, (char)parm1, this->m_textInputData.m_textCursorIndex++);
				CtlTextEditRecalcCurXY(this);
				CtlTextEditUpdateScrollXY(this);
			}
			
			WidgetTextEditView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			break;
		}
		case EVENT_CREATE:
		{
			this->m_textInputData.m_pText = NULL;
			this->m_textInputData.m_scrollY = 0;
			
			if (!this->m_textInputData.m_onlyOneLine)
			{
				// Add the vertical scroll bar
				Rectangle r;
				r.right = this->m_rect.right, 
				r.top   = this->m_rect.top, 
				r.bottom= this->m_rect.bottom - SCROLL_BAR_WIDTH, 
				r.left  = this->m_rect.right  - SCROLL_BAR_WIDTH;
				
				int flags = 0;
				if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
					flags |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
				if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
					flags |= ANCHOR_BOTTOM_TO_BOTTOM;
				
				AddControlEx (pWindow, CONTROL_VSCROLLBAR, flags, r, NULL, -this->m_comboID, 1, (10 << 16) | 1);
				
				r.right = this->m_rect.right  - SCROLL_BAR_WIDTH, 
				r.top   = this->m_rect.bottom - SCROLL_BAR_WIDTH, 
				r.bottom= this->m_rect.bottom, 
				r.left  = this->m_rect.left;
				
				flags = 0;
				if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
					flags |= ANCHOR_RIGHT_TO_RIGHT;
				if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
					flags |= ANCHOR_TOP_TO_BOTTOM | ANCHOR_BOTTOM_TO_BOTTOM;
				
				//no one will use combo IDs that large I hope :^)
				AddControlEx (pWindow, CONTROL_HSCROLLBAR, flags, r, NULL, 0x70000000 - this->m_comboID, 1, 1);
			
				//shrink our rectangle:
				this->m_rect.right  -= SCROLL_BAR_WIDTH + 2;
				this->m_rect.bottom -= SCROLL_BAR_WIDTH + 2;
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
			
			RemoveControl(pWindow, 0x70000000 - this->m_comboID);
			RemoveControl(pWindow,            - this->m_comboID);
			
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
				VidSetFont(this->m_textInputData.m_enableStyling ? SYSTEM_FONT : FONT_TAMSYN_MED_REGULAR);
				Rectangle cr = rk;
				cr.left += 2;
				cr.top  += 2;
				cr.right  -= 4;
				cr.bottom -= 4;
				VidSetClipRect(&this->m_rect);
				
				int xPos = rk.left + 2 - this->m_textInputData.m_scrollX;
				int yPos = rk.top  + 2 - this->m_textInputData.m_scrollY;
				int nLine = 1;
				
				int xiPos = xPos, yiPos = yPos;
				int wChar, hChar;
				wChar = GetCharWidth ('W');
				hChar = GetLineHeight();
				
				//go through the characters:
				const char *text = this->m_textInputData.m_pText;
				
				int charsConsumed = 0;
				
				//write Line 1 text, if needed
				if (this->m_textInputData.m_showLineNumbers && (yPos + hChar >= this->m_rect.top))
				{
					char buffer[12];
					sprintf(buffer, "%6d", nLine);
					VidTextOut(buffer, this->m_rect.left + 2, yPos, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
				}
				
				while (*text)
				{
					if (*text == '\n')
					{
						xPos = xiPos;
						yPos += hChar;
						text++;
						nLine++;
						if (this->m_textInputData.m_showLineNumbers && (yPos + hChar >= this->m_rect.top))
						{
							char buffer[12];
							sprintf(buffer, "%6d", nLine);
							VidTextOut(buffer, this->m_rect.left + 2, yPos, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
						}
						continue;
					}
					// Styling. Only need to care about this if measuring text
					else
					{
						if (this->m_textInputData.m_enableSyntaxHilite)
						{
							if (charsConsumed == 0) {
								charsConsumed = 1;
								CtlTextInputDoSyntaxHighlight (this, &color, &charsConsumed, text - this->m_textInputData.m_pText);
							}
							charsConsumed--;
						}
						if (this->m_textInputData.m_enableStyling && (unsigned char)(*text) >= (unsigned char)TIST_BOLD && (unsigned char)(*text) < (unsigned char)TIST_COUNT)
						{
							switch (*text) {
								case TIST_BOLD:                       color |=  0x01000000;        break;
								case TIST_UNBOLD:                     color &= ~0x01000000;        break;
								case TIST_UNFORMAT:                   color =  color_default;      break;
								case TIST_RED:                        color =  color | 0x00FF0000; break;
								case TIST_GREEN:                      color =  color | 0x0000FF00; break;
								case TIST_LINK:    case TIST_BLUE:    color =  color | 0x000000FF; break;
								case TIST_UNLINK:  case TIST_UNCOLOR: color =  (color & 0xFF000000) | color_default; break;
							}
							
							text++;
							continue;
						}
					}
					
					//if we need to bother drawing
					if (xPos + wChar <  this->m_rect.left)   goto dont_draw;
					if (yPos + hChar <  this->m_rect.top)    goto dont_draw;
					if (xPos         >= this->m_rect.right)  goto dont_draw;
					if (yPos         >= this->m_rect.bottom) break;
					
					VidPlotChar (*text, xPos, yPos, color, TRANSPARENT);
					
				dont_draw:
					//still increase xPos/yPos
					//TODO: proper tab ceils to next multiple of TAB_WIDTH instead
					xPos +=  GetCharWidth (*text);
					if (color & 0x01000000) xPos++;
					text++;
				}
				
				// draw cursor
				if (this->m_bFocused)
				{
					int cx = xiPos + this->m_textInputData.m_textCursorX * wChar;
					int cy = yiPos + this->m_textInputData.m_textCursorY * hChar;
					
					if (cx <  this->m_rect.left)   goto dont_draw_cur;
					if (cy <  this->m_rect.top)    goto dont_draw_cur;
					if (cx >= this->m_rect.right)  goto dont_draw_cur;
					if (cy >= this->m_rect.bottom) goto dont_draw_cur;
				
					//draw the cursor:
					VidDrawVLine(0xFF, cy, cy + hChar, cx);
					
					dont_draw_cur:;
				}
				VidSetClipRect(NULL);
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
