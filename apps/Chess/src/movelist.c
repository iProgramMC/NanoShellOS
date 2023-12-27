/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

              Move List widget
******************************************/
#include <nanoshell/nanoshell.h>
#include "chess.h"

#define ITEM_THICKNESS (16)

void DrawRectangleFrame(Rectangle rect);

extern Window* g_pWindow;
extern BoardState* g_History, *g_CurrentState;
extern int g_HistorySize;

int WidgetMoveList_GetScrollY(int comboID)
{
	return GetScrollBarPos(g_pWindow, -comboID);
}

void WidgetMoveList_DrawItem(Rectangle rect, int scrollY, int index)
{
	bool val = (index % 2) == 0;
	
	BoardState* pState = &g_History[index];
	index--;
	
	int yPos = index / 2;
	if (yPos < scrollY || index < 0) return;
	
	
	Rectangle itemRect = rect;
	
	itemRect.top = rect.top + ITEM_THICKNESS * (yPos - scrollY);
	itemRect.bottom = itemRect.top + ITEM_THICKNESS;
	
	
	if (itemRect.top >= rect.bottom) return;
	
	int mid = (itemRect.right + itemRect.left) / 2;;
	// black's moves
	if (val)
		itemRect.left = mid;
	else // white's moves
		itemRect.right = mid;
	
	uint32_t bg = WINDOW_TEXT_COLOR_LIGHT;
	uint32_t fg = WINDOW_TEXT_COLOR;
	
	if (pState == g_CurrentState)
		bg = SELECTED_ITEM_COLOR, fg = SELECTED_TEXT_COLOR;
	
	if (itemRect.bottom >= rect.bottom)
		itemRect.bottom  = rect.bottom;
	if (itemRect.top < rect.top)
		itemRect.top = rect.top;
	
	VidSetClipRect(&itemRect);
	
	VidFillRect(bg, itemRect.left, itemRect.top, itemRect.right - 1, itemRect.bottom - 1);
	
	// build the PGN related to this move...
	MoveInfo* pmi = &pState->m_MoveInfo;
	
	char pgn[sizeof pmi->pgn];
	strcpy(pgn, pmi->pgn);
	
	char buf[64];
	buf[0] = 0;
	
	// if it's white's move, then put the index number first, and then add the PGN.
	if (!val)
		snprintf(buf, sizeof buf, "%d. ", yPos + 1);
	
	strcat(buf, pgn);
	
	itemRect.left++;
	itemRect.top ++;
	
	VidDrawText(buf, itemRect, 0, fg, TRANSPARENT);
	
	VidSetClipRect(NULL);
}

void PaintBoard();
void UpdatePlayerTurn();

bool WidgetMoveList_OnEvent(Control* this, int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWnd)
{
	switch (eventType)
	{
		case EVENT_UPDATE_MOVE_LIST:
		{
			int max = (g_HistorySize / 2);
			Rectangle rect = this->m_rect;
			
			rect.left += 2;
			rect.top += 2;
			rect.right -= 2;
			rect.bottom -= 2;
			
			int rows = 1 + (rect.bottom - rect.top) / ITEM_THICKNESS;
			max -= (rows - 1);
			
			if (max < 1)
				max = 1;
			
			SetScrollBarMax(pWnd, -this->m_comboID, max);
			WidgetMoveList_OnEvent(this, EVENT_PAINT, 0, 0, pWnd);
			break;
		}
		case EVENT_SCROLLDONE:
		{
			if (parm1 == -this->m_comboID)
				WidgetMoveList_OnEvent(this, EVENT_PAINT, 0, 0, pWnd);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Rectangle rect = this->m_rect;
			
			rect.left += 2;
			rect.top += 2;
			rect.right -= 2;
			rect.bottom -= 2;
			
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			if (!RectangleContains(&rect, &p)) break;
			
			int scrollY = WidgetMoveList_GetScrollY(this->m_comboID);
			
			int yPos = (p.y - rect.top) / ITEM_THICKNESS + scrollY;
			int halfwidth = (rect.right - rect.left) / 2;
			
			int xPos = 0;
			if (halfwidth)
				xPos = (p.x - rect.left) / halfwidth;
			
			int index = yPos * 2 + xPos + 1;
			
			int indexCurrState = (int)(g_CurrentState - g_History);
			
			if (index < 0 || index >= g_HistorySize) break;
			
			if (indexCurrState != index)
			{
				
				g_CurrentState = &g_History[index];
				PaintBoard();
				UpdatePlayerTurn();
				
				WidgetMoveList_DrawItem(rect, scrollY, indexCurrState);
				WidgetMoveList_DrawItem(rect, scrollY, index);
			}
			
			break;
		}
		case EVENT_PAINT:
		{
			Rectangle rect = this->m_rect;
			
			rect.left += 2;
			rect.top += 2;
			rect.right -= 2;
			rect.bottom -= 2;
			
			DrawRectangleFrame(rect);
			
			VidFillRect(WINDOW_TEXT_COLOR_LIGHT, rect.left, rect.top, rect.right - 1, rect.bottom - 1);
			
			int scrollY = WidgetMoveList_GetScrollY(this->m_comboID);
			
			for (int i = 0; i < g_HistorySize; i++)
			{
				WidgetMoveList_DrawItem(rect, scrollY, i);
			}
			
			break;
		}
	}
	
	return false;
}

void AddMoveList(Window* pWindow, Rectangle rect, int comboID)
{
	Rectangle scrollRect = rect;
	scrollRect.left = scrollRect.right - SCROLL_BAR_SIZE;
	AddControl(pWindow, CONTROL_VSCROLLBAR, scrollRect, NULL, -comboID, 1, 0);
	rect.right = scrollRect.left;
	
	AddControl (pWindow, CONTROL_NONE, rect, NULL, comboID, 0, 0);
	SetWidgetEventHandler (pWindow, comboID, WidgetMoveList_OnEvent);
}
