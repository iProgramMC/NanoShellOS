/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

    Widget library: Tab Picker control
******************************************/
#include "../wi.h"

#define TAB_HOVERED (1)

// The tab picker is a widget which allows the user to pick between several
// options.
// Note: Tab IDs should fit within an int16.
typedef struct
{
	int  m_id;
	int  m_nTabWidth;
	int  m_nTabTextWidth;
	int  m_xPos;
	int  m_flags;
	char m_text[108];
}
Tab;

typedef struct
{
	Tab *m_tabs;
	int  m_nTabs;
	int  m_nSelectedTab;
	bool m_clicked;
}
TabViewData;

TabViewData* WidgetTabView_GetData(Control* this)
{
	return (TabViewData*)this->m_dataPtr;
}

// Appends a tab to the end.
void WidgetTabView_AddTab(Control* this, int id, const char* pTabText, int tabWidth /* = -1 */)
{
	TabViewData* pData = WidgetTabView_GetData(this);
	
	// Allocate space for a new tab.
	Tab* pNewTabs = MmReAllocate(pData->m_tabs, (pData->m_nTabs + 1) * sizeof(Tab));
	if (!pNewTabs) return;
	
	pData->m_tabs = pNewTabs;
	Tab* pTab = &pData->m_tabs[pData->m_nTabs++];
	pTab->m_id = id;
	SafeStringCopy(pTab->m_text, sizeof pTab->m_text, pTabText);
	
	// Measure the text.
	int w, h;
	VidTextOutInternal(pTab->m_text, 0, 0, 0, 0, true, &w, &h);
	pTab->m_nTabTextWidth = w;
	
	w += 10;
	if (tabWidth < w)
		tabWidth = w;
	
	pTab->m_nTabWidth = w;
	
	int x = 0;
	if (pData->m_nTabs > 1)
		x = pData->m_tabs[pData->m_nTabs - 2].m_xPos + pData->m_tabs[pData->m_nTabs - 2].m_nTabWidth;
	
	pTab->m_xPos = x;
}

Tab* WidgetTabView_GetTabByID(Control* this, int id)
{
	TabViewData* pData = WidgetTabView_GetData(this);
	
	for (int i = 0; i < pData->m_nTabs; i++)
	{
		if (pData->m_tabs[i].m_id == id)
			return &pData->m_tabs[i];
	}
	
	return NULL;
}

void WidgetTabView_ClearTabs(Control* this)
{
	TabViewData* pData = WidgetTabView_GetData(this);
	MmFree(pData->m_tabs);
	pData->m_tabs = NULL;
	pData->m_nTabs = 0;
	pData->m_nSelectedTab = 0;
}

void WidgetTabView_RemoveTab(Control* this, int id)
{
	TabViewData* pData = WidgetTabView_GetData(this);
	
	for (int i = 0; i < pData->m_nTabs; i++)
	{
		if (pData->m_tabs[i].m_id == id)
		{
			//get rid of it
			pData->m_nTabs--;
			memmove(&pData->m_tabs[i], &pData->m_tabs[i+1], sizeof(Tab) * (pData->m_nTabs - i));
			
			Tab* pNewTabs = MmReAllocate(pData->m_tabs, pData->m_nTabs * sizeof(Tab));
			if (!pNewTabs) return;
			
			pData->m_tabs = pNewTabs;
			
			return;
		}
	}
}

void WidgetTabPicker_DrawTab(Control* this, Tab* tab, bool bIsSelected)
{
	int x = tab->m_xPos + this->m_rect.left;
	if (x >= this->m_rect.right || x + tab->m_nTabWidth < this->m_rect.left) return;
	
	Rectangle rect = { x, this->m_rect.top, x + tab->m_nTabWidth, this->m_rect.top + TITLE_BAR_HEIGHT + 2 };
	
	if (!bIsSelected)
		rect.top += 2;
	
	uint32_t bgc = WINDOW_BACKGD_COLOR;
	if (tab->m_flags & TAB_HOVERED)
		bgc = BUTTON_HOVER_COLOR;
	
	VidFillRect(bgc, rect.left + 1, rect.top + 1, rect.right - 2, rect.bottom - 1);
	
	VidDrawVLine(BUTTON_HILITE_COLOR, rect.top + 2, rect.bottom - 1, rect.left);
	VidDrawVLine(BUTTON_EDGE_COLOR,   rect.top + 2, rect.bottom - 1, rect.right - 1);
	
	VidDrawVLine(BUTTON_HOVER_COLOR , rect.top + 2, rect.bottom - 1, rect.left  + 1);
	VidDrawVLine(BUTTON_SHADOW_COLOR, rect.top + 2, rect.bottom - 1, rect.right - 2);
	
	VidDrawHLine(BUTTON_HILITE_COLOR, rect.left + 2, rect.right - 3, rect.top);
	VidDrawHLine(BUTTON_HOVER_COLOR,  rect.left + 2, rect.right - 3, rect.top + 1);
	
	VidPlotPixel(rect.left  + 1, rect.top + 1, BUTTON_HILITE_COLOR);
	VidPlotPixel(rect.right - 2, rect.top + 1, BUTTON_EDGE_COLOR);
	
	VidTextOut(tab->m_text, x + (tab->m_nTabWidth - tab->m_nTabTextWidth) / 2, rect.top + 5, WINDOW_TEXT_COLOR, TRANSPARENT);
	
	// if this isn't selected, also draw the bar at the bottom
	if (!bIsSelected)
	{
		VidDrawHLine(BUTTON_HILITE_COLOR, rect.left, rect.right, rect.bottom - 2);
		VidDrawHLine(BUTTON_HOVER_COLOR,  rect.left, rect.right, rect.bottom - 1);
	}
}

bool WidgetTabPicker_HitTest(Control* this, Tab* tab, Point* pt, bool bIsSelected)
{
	int x = tab->m_xPos + this->m_rect.left;
	if (x >= this->m_rect.right || x + tab->m_nTabWidth < this->m_rect.left) return false;
	
	Rectangle rect = { x, this->m_rect.top, x + tab->m_nTabWidth, this->m_rect.top + TITLE_BAR_HEIGHT + 2 };
	
	if (!bIsSelected)
		rect.top += 2;
	
	return RectangleContains(&rect, pt);
}

void WidgetTabPicker_ClearTab(Control* this, int tabID, bool bIsSelected)
{
	TabViewData* pData = WidgetTabView_GetData(this);
	
	Tab* tab = &pData->m_tabs[tabID];
	int x = tab->m_xPos + this->m_rect.left;
	Rectangle rect = { x, this->m_rect.top, x + tab->m_nTabWidth, this->m_rect.top + TITLE_BAR_HEIGHT + 2 };
	
	if (!bIsSelected)
		rect.top += 2;
	
	VidFillRect(WINDOW_BACKGD_COLOR, rect.left,rect.top, rect.right-1, rect.bottom-1);
}

bool WidgetTabPicker_OnEvent(Control* this, int eventType, UNUSED int parm1, UNUSED int parm2, Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			this->m_dataPtr = MmAllocate(sizeof(TabViewData));
			memset(this->m_dataPtr, 0, sizeof(TabViewData));
			
			WidgetTabView_AddTab(this, 1, "Tab Long 1", 100);
			WidgetTabView_AddTab(this, 2, "Tab 2", -1);
			WidgetTabView_AddTab(this, 3, "Tab Longer 3", 100);
			
			break;
		}
		case EVENT_PAINT:
		{
			TabViewData* pData = WidgetTabView_GetData(this);
			
			VidSetClipRect(&this->m_rect);
			
			//VidFillRectangle(WINDOW_BACKGD_COLOR, this->m_rect);
			
			for (int i = 0; i < pData->m_nTabs; i++)
			{
				WidgetTabPicker_DrawTab(this, &pData->m_tabs[i], false);
			}
			
			// Draw a frame around the lower part of the tab view's rectangle.
			Rectangle frameRect = { this->m_rect.left, this->m_rect.top + TITLE_BAR_HEIGHT, this->m_rect.right, this->m_rect.bottom };
			if (frameRect.bottom > frameRect.top + 4)
			{
				VidDrawVLine(BUTTON_HILITE_COLOR, frameRect.top, frameRect.bottom - 1, frameRect.left);
				VidDrawVLine(BUTTON_HOVER_COLOR,  frameRect.top, frameRect.bottom - 1, frameRect.left  + 1);
				VidDrawVLine(BUTTON_SHADOW_COLOR, frameRect.top, frameRect.bottom - 1, frameRect.right - 2);
				VidDrawVLine(BUTTON_EDGE_COLOR,   frameRect.top, frameRect.bottom - 1, frameRect.right - 1);
				VidDrawHLine(BUTTON_SHADOW_COLOR, frameRect.left + 1, frameRect.right - 2, frameRect.bottom - 2);
				VidDrawHLine(BUTTON_EDGE_COLOR,   frameRect.left,     frameRect.right - 1, frameRect.bottom - 1);
				VidDrawHLine(BUTTON_HOVER_COLOR,  frameRect.left + 1, frameRect.right - 2, frameRect.top + 1);
				VidDrawHLine(BUTTON_HILITE_COLOR, frameRect.left,     frameRect.right - 1, frameRect.top);
			}
			
			if (pData->m_nSelectedTab < pData->m_nTabs)
			{
				WidgetTabPicker_DrawTab(this, &pData->m_tabs[pData->m_nSelectedTab], true);
			}
			
			VidSetClipRect(NULL);
			break;
		}
		case EVENT_MOVECURSOR:
		{
			TabViewData* pData = WidgetTabView_GetData(this);
			if (!pData->m_nTabs) break;
			
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			for (int i = 0; i < pData->m_nTabs; i++)
			{
				int *flags = &pData->m_tabs[i].m_flags;
				int  oldFlags = *flags;
				if (WidgetTabPicker_HitTest(this, &pData->m_tabs[i], &pt, i == pData->m_nSelectedTab))
					*flags |= TAB_HOVERED;
				else
					*flags &=~TAB_HOVERED;
				
				if (*flags != oldFlags)
					WidgetTabPicker_DrawTab(this, &pData->m_tabs[i], i == pData->m_nSelectedTab);
			}
			
			WidgetTabPicker_DrawTab(this, &pData->m_tabs[pData->m_nSelectedTab], true);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			TabViewData* pData = WidgetTabView_GetData(this);
			if (!pData->m_nTabs) break;
			
			if (pData->m_clicked) break;
			pData->m_clicked = true;
			
			int oldTab = pData->m_nSelectedTab;
			
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			for (int i = 0; i < pData->m_nTabs; i++)
			{
				if (WidgetTabPicker_HitTest(this, &pData->m_tabs[i], &pt, i == pData->m_nSelectedTab))
				{
					pData->m_nSelectedTab = i;
					break;
				}
			}
			
			if (pData->m_nSelectedTab != oldTab)
			{
				//fill the tab's background color, and draw the neighboring tabs
				WidgetTabPicker_ClearTab(this, oldTab, true);
				
				WindowAddEventToMasterQueue(pWindow, EVENT_TABCHANGED, this->m_comboID, MAKE_MOUSE_PARM(oldTab, pData->m_nSelectedTab));
				WidgetTabPicker_DrawTab(this, &pData->m_tabs[oldTab], false);
				WidgetTabPicker_DrawTab(this, &pData->m_tabs[pData->m_nSelectedTab], true);
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			TabViewData* pData = WidgetTabView_GetData(this);
			pData->m_clicked = false;
			break;
		}
		case EVENT_DESTROY:
		{
			MmFree(this->m_dataPtr);
			this->m_dataPtr = NULL;
			break;
		}
	}
	return false;
}


