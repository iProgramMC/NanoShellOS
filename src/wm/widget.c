/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

           Shell widget module
******************************************/
#include "wi.h"

bool g_GlowOnHover = true;
extern VBEData* g_vbeData, g_mainScreenVBEData;

bool IsControlFocused(Window* pWindow, int comboID)
{
	Control* pCtl = GetControlByComboID(pWindow, comboID);
	
	if (!pCtl) return false;
	
	return pCtl->m_bFocused;
}

bool IsControlDisabled(Window* pWindow, int comboID)
{
	Control* pCtl = GetControlByComboID(pWindow, comboID);
	
	if (!pCtl) return false;
	
	return pCtl->m_bDisabled;
}

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
bool WidgetComboBox_OnEvent     (C_PARMS);
bool WidgetColorPicker_OnEvent  (C_PARMS);

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
	WidgetComboBox_OnEvent,
	WidgetColorPicker_OnEvent,
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
	static const int flags[] = { 4, 3, 1, 2, 0 };
	
	int colors[] =
	{
		BUTTON_EDGE_COLOR,
		BUTTON_XSHADOW_COLOR,
		BUTTON_SHADOW_COLOR,
		WINDOW_BORDER_COLOR,
		0,
		0,
		BUTTON_HILITE_COLOR,
	};
	
	if ((style & DRE_BLACKOUTER) && BUTTON_XSHADOW_COLOR == 0)
		colors[1] = colors[2];
	
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

void DrawArrow(Rectangle rect, eArrowType arrowType, int flags, unsigned color)
{
	Rectangle arrowRect = rect;
	
	if (flags & DRA_IGNORESIZE)
	{
		arrowRect.right  = arrowRect.left + ARROW_SIZE;
		arrowRect.bottom = arrowRect.top  + ARROW_SIZE;
	}
	else if (flags & DRA_IGNOREXSIZE)
	{
		arrowRect.right = arrowRect.left + (arrowRect.bottom - arrowRect.top);
	}
	else if (flags & DRA_IGNOREYSIZE)
	{
		arrowRect.bottom = arrowRect.top + (arrowRect.right - arrowRect.left);
	}
	
	// adjust the width/height so that we can actually center it
	switch (arrowType)
	{
		case DRA_UP:
		case DRA_DOWN:
		{
			int wid = (arrowRect.right - arrowRect.left);
			wid += wid % 2;
			arrowRect.bottom = arrowRect.top + wid / 2;
			break;
		}
		case DRA_LEFT:
		case DRA_RIGHT:
		{
			int hei = (arrowRect.bottom - arrowRect.top);
			hei += hei % 2;
			arrowRect.right = arrowRect.left + hei / 2;
			break;
		}
	}
	
	if (flags & DRA_CENTERX)
	{
		int width = arrowRect.right - arrowRect.left;
		arrowRect.left = rect.left + (rect.right - rect.left - width) / 2;
		arrowRect.right = arrowRect.left + width;
	}
	
	if (flags & DRA_CENTERY)
	{
		int height = arrowRect.bottom - arrowRect.top;
		arrowRect.top = rect.top + (rect.bottom - rect.top - height) / 2;
		arrowRect.bottom = arrowRect.top + height;
	}
	
	switch (arrowType)
	{
		case DRA_UP:
		{
			int yPos = arrowRect.bottom - 1;
			arrowRect.right--;
			
			while (yPos >= arrowRect.top && arrowRect.left <= arrowRect.right)
			{
				VidDrawHLine(color, arrowRect.left, arrowRect.right, yPos);
				yPos--;
				arrowRect.left++;
				arrowRect.right--;
			}
			break;
		}
		case DRA_DOWN:
		{
			int yPos = arrowRect.top;
			arrowRect.right--;
			
			while (yPos < arrowRect.bottom && arrowRect.left <= arrowRect.right)
			{
				VidDrawHLine(color, arrowRect.left, arrowRect.right, yPos);
				yPos++;
				arrowRect.left++;
				arrowRect.right--;
			}
			break;
		}
		case DRA_LEFT:
		{
			int xPos = arrowRect.right - 1;
			arrowRect.bottom--;
			while (xPos >= arrowRect.left && arrowRect.top <= arrowRect.bottom)
			{
				VidDrawVLine(color, arrowRect.top, arrowRect.bottom, xPos);
				arrowRect.top++;
				arrowRect.bottom--;
				xPos--;
			}
			break;
		}
		case DRA_RIGHT:
		{
			int xPos = arrowRect.left;
			arrowRect.bottom--;
			while (xPos < arrowRect.right && arrowRect.top <= arrowRect.bottom)
			{
				VidDrawVLine(color, arrowRect.top, arrowRect.bottom, xPos);
				arrowRect.top++;
				arrowRect.bottom--;
				xPos++;
			}
			break;
		}
	}
}
