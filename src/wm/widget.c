/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

           Shell widget module
******************************************/
#include "wi.h"

bool g_GlowOnHover = true;
extern VBEData* g_vbeData, g_mainScreenVBEData;

void SetFocusedControl(Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control *pControl = &pWindow->m_pControlArray[i];
		pControl->m_bFocused = (pControl->m_comboID == comboID);
	}
}

void SetDisabledControl(Window *pWindow, int comboID, bool bDisabled)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control *pControl = &pWindow->m_pControlArray[i];
		if (pControl->m_comboID == comboID)
		{
			pControl->m_bDisabled = bDisabled;
			return;
		}
	}
}

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
void RenderButtonShapeSmallBorder(Rectangle rect, unsigned colorDarker, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	VidDrawHLine(colorDarker, rect.left, rect.right - 1, rect.bottom - 1);
	VidDrawVLine(colorDarker, rect.top, rect.bottom - 1, rect.right  - 1);
	
	rect.right--;
	rect.bottom--;
	
	//draw some lines
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink
	rect.left++, rect.right -= 2, rect.top++, rect.bottom -= 2;
	
	//fill the background:
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rect);
}
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
	
	VidDrawHLine(BUTTON_XSHADOW_COLOR, rectb.left, rectb.right-1,  rectb.bottom-1);
	VidDrawVLine(BUTTON_XSHADOW_COLOR, rectb.top,  rectb.bottom-1, rectb.right-1);
	
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

#define C_PARMS Control* this, int eventType, int parm1, int parm2, Window* pWindow

bool WidgetHScrollBar_OnEvent   (C_PARMS);
bool WidgetVScrollBar_OnEvent   (C_PARMS);
bool WidgetTextEditView_OnEvent (C_PARMS);
bool WidgetListView_OnEvent     (C_PARMS);
bool WidgetIconView_OnEvent     (C_PARMS);
bool WidgetIconViewDrag_OnEvent (C_PARMS);
bool WidgetMenuBar_OnEvent      (C_PARMS);
bool WidgetNone_OnEvent         (C_PARMS);
bool WidgetIcon_OnEvent         (C_PARMS);
bool WidgetText_OnEvent         (C_PARMS);
bool WidgetTextHuge_OnEvent     (C_PARMS);
bool WidgetTextCenter_OnEvent   (C_PARMS);
bool WidgetSurroundRect_OnEvent (C_PARMS);
bool WidgetSimpleLine_OnEvent   (C_PARMS);
bool WidgetButton_OnEvent       (C_PARMS);
bool WidgetButtonIcon_OnEvent   (C_PARMS);
bool WidgetButtonIconBar_OnEvent(C_PARMS);
bool WidgetButtonList_OnEvent   (C_PARMS);
bool WidgetButtonColor_OnEvent  (C_PARMS);
bool WidgetActionButton_OnEvent (C_PARMS);
bool WidgetClickLabel_OnEvent   (C_PARMS);
bool WidgetImage_OnEvent        (C_PARMS);
bool WidgetCheckbox_OnEvent     (C_PARMS);
bool WidgetTaskList_OnEvent     (C_PARMS);
bool WidgetTableView_OnEvent    (C_PARMS);
bool WidgetButtonIconChk_OnEvent(C_PARMS);
bool WidgetTextEditView2_OnEvent(C_PARMS);
bool WidgetTabPicker_OnEvent    (C_PARMS);
bool WidgetProgressBar_OnEvent  (C_PARMS);

WidgetEventHandler g_widgetEventHandlerLUT[] = {
	WidgetNone_OnEvent,
	WidgetText_OnEvent,
	WidgetIcon_OnEvent,
	WidgetButton_OnEvent,
	WidgetTextEditView2_OnEvent,
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
	WidgetIconViewDrag_OnEvent,
	WidgetTableView_OnEvent,
	WidgetButtonIconChk_OnEvent,
	WidgetTabPicker_OnEvent,
	WidgetProgressBar_OnEvent,
};

STATIC_ASSERT(ARRAY_COUNT(g_widgetEventHandlerLUT) == CONTROL_COUNT, "Change this array if adding widgets");

WidgetEventHandler GetWidgetOnEventFunction (int type)
{
	if (type < 0 || type >= CONTROL_COUNT)
		return NULL;
	return g_widgetEventHandlerLUT[type];
}

void DrawEdge(Rectangle rect, int style, unsigned bg)
{
	// adjust the rectangle so we can simply use VidFillRectangle and VidDrawRectangle.
	rect.right--;
	rect.bottom--;
	
	if (style & DRE_FLAT)
	{
		style &= ~DRE_FLAT;
		style &= ~(DRE_INNER | DRE_OUTER);
		style |=  DRE_BLACKOUTER;
	}
	
	// the depth levels are as follows:
	// 6 - BUTTON_HILITE_COLOR
	// 5 - Avg(BUTTON_HILITE_COLOR, BUTTON_MIDDLE_COLOR)
	// 4 - BUTTON_MIDDLE_COLOR
	// 3 - WINDOW_BORDER_COLOR
	// 2 - BUTTON_SHADOW_COLOR
	// 1 - BUTTON_XSHADOW_COLOR
	// 0 - BUTTON_EDGE_COLOR
	
	uint32_t tl = 0, br = 0;
	
	// the flags we need to check, in order of priority.
	// These match up with their definitions in window.h and represent the amount 1 is shifted by.
	static const int flags[] = { 4, 1, 3, 2, 0 };
	
	int colors[] =
	{
		BUTTON_EDGE_COLOR,
		0x404040,
		BUTTON_SHADOW_COLOR,
		WINDOW_BORDER_COLOR,
		0,
		0,
		BUTTON_HILITE_COLOR,
	};
	
	if (style & DRE_HOT)
		colors[4] = BUTTON_HOVER_COLOR;
	else
		colors[4] = BUTTON_MIDDLE_COLOR;
	
	// get color #5.
	unsigned colorAvg = 0;
	colorAvg |= ((colors[6] & 0xff0000) + (colors[4] & 0xff0000)) >> 1;
	colorAvg |= ((colors[6] & 0x00ff00) + (colors[4] & 0x00ff00)) >> 1;
	colorAvg |= ((colors[6] & 0x0000ff) + (colors[4] & 0x0000ff)) >> 1;
	colors[5] = colorAvg;
	
	// 4 pairs of ints corresponding to the border type. These ints are
	// indices into the colors array.
	static const int color_indices[] =
	{
		6, 2, // raised inner
		1, 2, // sunken inner
		5, 1, // raised outer
		2, 6,
		0, 0,
	};
	
	for (int i = 0; i < (int)ARRAY_COUNT(flags); i++)
	{
		if (~style & (1 << flags[i])) continue;
		
		tl = colors[color_indices[0 + 2 * flags[i]]];
		br = colors[color_indices[1 + 2 * flags[i]]];
		
		// top left
		VidDrawHLine(tl, rect.left, rect.right, rect.top);
		VidDrawVLine(tl, rect.top, rect.bottom, rect.left);
		
		// bottom right
		VidDrawHLine(br, rect.left, rect.right, rect.bottom);
		VidDrawVLine(br, rect.top, rect.bottom, rect.right);
		
		rect.left++;
		rect.top++;
		rect.right--;
		rect.bottom--;
	}
	
	if (style & DRE_FILLED)
		VidFillRectangle(bg, rect);
}
