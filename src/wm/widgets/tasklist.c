/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Task List controls
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

extern Window g_windows[WINDOWS_MAX];
extern short  g_windowDrawOrder[WINDOWS_MAX];

void SelectWindow (Window *pWnd);
void ShowWindow (Window *pWnd);

bool g_TaskListCompact = false;

extern bool g_GlowOnHover;
extern bool g_bUseLargeIcons;

int TaskListGetIconSize()
{
	return g_bUseLargeIcons ? 32 : 16;
}

int TaskListGetButtonHeight()
{
	return TaskListGetIconSize() + 6 + (TITLE_BAR_HEIGHT - 18);
}
typedef struct
{
	bool m_clicked[WINDOWS_MAX];
	bool m_hovered[WINDOWS_MAX];
	WindowQuery queries[WINDOWS_MAX];
	char titles[WINDOWS_MAX][256];
	int  m_nWindowsBefore;
}
TaskListData;

static void WidgetTaskList_PaintButton(Window* pWindow, Control* this, Rectangle r, int windowID, int iconID, const char* titleOut)
{
	TaskListData* pData = (TaskListData*)this->m_dataPtr;
	
	bool clicked = pData->m_clicked[windowID];
	bool hovered = pData->m_hovered[windowID];
	
	uint32_t flgs = 0;
	if (g_windows[windowID].m_isSelected)
	{
		// copied from DrawEdge..
		unsigned colorAvg = 0;
		colorAvg |= ((BUTTON_MIDDLE_COLOR & 0xff0000) + (BUTTON_HOVER_COLOR & 0xff0000)) >> 1;
		colorAvg |= ((BUTTON_MIDDLE_COLOR & 0x00ff00) + (BUTTON_HOVER_COLOR & 0x00ff00)) >> 1;
		colorAvg |= ((BUTTON_MIDDLE_COLOR & 0x0000ff) + (BUTTON_HOVER_COLOR & 0x0000ff)) >> 1;
		
		if (hovered)
			DrawEdge(r, DRE_FILLED | DRE_BLACKOUTER | DRE_HOT, BUTTON_HOVER_COLOR);
		else
			DrawEdge(r, DRE_FILLED | DRE_BLACKOUTER, colorAvg);
		
		VidDrawHLine(BUTTON_XSHADOW_COLOR, r.left + 1, r.right  - 2, r.top    + 1);
		VidDrawVLine(BUTTON_XSHADOW_COLOR, r.top  + 1, r.bottom - 2, r.left   + 1);
		VidDrawHLine(BUTTON_SHADOW_COLOR,  r.left + 2, r.right  - 2, r.top    + 2);
		VidDrawVLine(BUTTON_SHADOW_COLOR,  r.top  + 2, r.bottom - 2, r.left   + 2);
		VidDrawHLine(BUTTON_HILITE_COLOR,  r.left + 2, r.right  - 2, r.bottom - 2);
		VidDrawVLine(BUTTON_HILITE_COLOR,  r.top  + 2, r.bottom - 2, r.right  - 2);
		
		clicked = true;
		
		flgs |= TEXT_RENDER_BOLD;
	}
	else if (clicked)
	{
		DrawEdge(r, DRE_FILLED | DRE_BLACKOUTER | DRE_SUNKENINNER, BUTTONMIDC);
	}
	else if (hovered)
	{
		DrawEdge(r, DRE_FILLED | DRE_BLACKOUTER | DRE_RAISEDINNER | DRE_RAISEDOUTER | DRE_HOT, BUTTON_HOVER_COLOR);
	}
	else
	{
		DrawEdge(r, DRE_FILLED | DRE_BLACKOUTER | DRE_RAISEDINNER | DRE_RAISEDOUTER, BUTTONMIDD);
	}
	
	// hmm.
	Rectangle rt = r;
	rt.left   += pWindow->m_rect.left;
	rt.top    += pWindow->m_rect.top;
	rt.right  += pWindow->m_rect.left;
	rt.bottom += pWindow->m_rect.top;
	g_windows[windowID].m_taskbarRect = rt;
	
	int textX = r.left + 4;
	
	int iconSize = TaskListGetIconSize();
	
	// if this window has an icon:
	if (iconID)
	{
		RenderIconForceSize(iconID, textX + clicked, r.top + 3 + (((r.bottom - r.top - 6) - iconSize) / 2) + clicked, iconSize);
		textX += iconSize + 6;
	}
	
	int btnWidth = r.right - r.left;
	
	if (!g_TaskListCompact)
	{
		VidSetClipRect (&r);
		
		//VidTextOut (pWnd->m_title, textX, this->m_rect.top + 5, WINDOW_TEXT_COLOR, TRANSPARENT);
		
		RECT(r, textX + clicked, r.top + 2 + clicked, r.left + btnWidth - textX, GetHeight(&r) - 4);
		
		VidDrawText (titleOut, r, TEXTSTYLE_VCENTERED, FLAGS_TOO(flgs, WINDOW_TEXT_COLOR), TRANSPARENT);
		
		VidSetClipRect (NULL);
	}
}

bool WidgetTaskList_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	int btnHeight = TaskListGetButtonHeight(), thisHeight = GetHeight(&this->m_rect), thisWidth = GetWidth(&this->m_rect);
	
	if (btnHeight > thisHeight)
		btnHeight = thisHeight;
	
	int nRows = thisHeight / btnHeight;
	
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			TaskListData* pData = MmAllocate(sizeof(TaskListData));
			memset(pData, 0, sizeof *pData);
			this->m_dataPtr = pData;
			
			if (!pData) break;
			
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				pData->titles[i][0] = 0;
				
				WindowQuery* pQuery = &pData->queries[i];
				pQuery->titleOut     = pData->titles[i];
				pQuery->titleOutSize = sizeof (pData->titles[i]);
			}
			
			break;
		}
		case EVENT_DESTROY:
		{
			MmFree(this->m_dataPtr);
			break;
		}
		case EVENT_PAINT:
		{
			TaskListData* pData = (TaskListData*)this->m_dataPtr;
			
			size_t nWindowsSt = 0;
			QueryWindows(pData->queries, ARRAY_COUNT(pData->queries), &nWindowsSt);
			
			int nWindows = 0;
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (pData->m_nWindowsBefore != nWindows)
			{
				pData->m_nWindowsBefore = nWindows;
				this->m_rect.right--;
				this->m_rect.bottom--;
				VidFillRectangle (WINDOW_BACKGD_COLOR, this->m_rect);
				this->m_rect.right++;
				this->m_rect.bottom++;
			}
			
			if (!nWindows) break;
			
			int btns_per_row = (nWindows + nRows - 1) / nRows;
			int btn_width = thisWidth / btns_per_row;
			
			int btn_max_width = g_TaskListCompact ? (12 + TaskListGetIconSize()) : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left, btn_y = this->m_rect.top;
			
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				
				Rectangle r;
				RECT (r, btn_x, btn_y, btn_width - 4, btnHeight);
				
				WidgetTaskList_PaintButton(pWindow, this, r, pQuery->windowID, pQuery->iconID, pQuery->titleOut);
				
				btn_x += btn_width;
				
				if (btn_x + btn_width > this->m_rect.right)
				{
					btn_x = this->m_rect.left;
					btn_y += btnHeight;
				}
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		case EVENT_CLICKCURSOR:
		case EVENT_MOVECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			TaskListData* pData = (TaskListData*)this->m_dataPtr;
			
			size_t nWindowsSt = 0;
			QueryWindows(pData->queries, ARRAY_COUNT(pData->queries), &nWindowsSt);
			
			int nWindows = 0;
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (pData->m_nWindowsBefore != nWindows)
			{
				WidgetTaskList_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			}
			
			if (!nWindows) break;
			
			int btns_per_row = (nWindows + nRows - 1) / nRows;
			int btn_width = thisWidth / btns_per_row;
			
			int btn_max_width = g_TaskListCompact ? (12 + TaskListGetIconSize()) : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left, btn_y = this->m_rect.top;
			
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				
				bool bWasHoveredBefore = pData->m_clicked[pQuery->windowID];
				bool bWasClickedBefore = pData->m_hovered[pQuery->windowID];
				
				pData->m_hovered[pQuery->windowID] = false;
				
				Rectangle r;
				RECT (r, btn_x, btn_y, btn_width - 4, btnHeight);
				
				r.right--;
				r.bottom--;
				bool rc = RectangleContains (&r, &mouseClickPos);
				r.right++;
				r.bottom++;
				
				if (rc)
				{
					if (eventType == EVENT_MOVECURSOR)
					{
						pData->m_hovered[pQuery->windowID] = true;
					}
					else if (eventType == EVENT_CLICKCURSOR)
					{
						pData->m_clicked[pQuery->windowID] = true;
					}
					else if (pData->m_clicked[pQuery->windowID]) // EVENT_RELEASECURSOR
					{
						// switch to this window
						for (int j = 0; j < WINDOWS_MAX; j++)
							pData->m_clicked[j] = false;
						
						//hmm??
						Window* pWindow = &g_windows[pQuery->windowID];
						if (pWindow->m_hidden && (pWindow->m_flags & WF_MINIMIZE)) //TODO
						{
							//Unhide and unminimize
							WindowRegisterEvent (pWindow, EVENT_UNMINIMIZE, 0, 0);
							//TODO: wait for the animation to finish :)
							ShowWindow(pWindow);
						}
						
						SelectWindow (pWindow);
					}
					
					if (eventType != EVENT_MOVECURSOR)
						break;
				}
				
				bool bIsHovered = pData->m_clicked[pQuery->windowID];
				bool bIsClicked = pData->m_hovered[pQuery->windowID];
				
				if (eventType != EVENT_MOVECURSOR || g_GlowOnHover)
				{
					if (bWasHoveredBefore != bIsHovered || bWasClickedBefore != bIsClicked)
						WidgetTaskList_PaintButton(pWindow, this, r, pQuery->windowID, pQuery->iconID, pQuery->titleOut);
				}
				
				btn_x += btn_width;
				if (btn_x + btn_width > this->m_rect.right)
				{
					btn_x = this->m_rect.left;
					btn_y += btnHeight;
				}
			}
			
			break;
		}
	}
	
	return false;
}
