/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: List view controls
******************************************/
#include "../wi.h"

#define LIST_ITEM_HEIGHT (GetLineHeight() + 6)
#define ICON_ITEM_WIDTH  (IsLowResolutionMode() ? 80 : 90)
#define ICON_ITEM_HEIGHT (IsLowResolutionMode() ? 50 : 60)

extern VBEData* g_vbeData, g_mainScreenVBEData;

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
static void CtlSetListItemText(Control* pCtl, int index, int icon, const char * pText)
{
	ListViewData* pData = &pCtl->m_listViewData;
	if (index < 0 || index >= pCtl->m_listViewData.m_elementCount) return;
	
	ListItem *pItem = &pData->m_pItems[index];
	
	pItem->m_icon = icon;
	strncpy(pItem->m_contents, pText, sizeof pItem->m_contents);
	pItem->m_contents[sizeof pItem->m_contents - 1] = 0;
	
	memcpy (pItem->m_contentsShown, pItem->m_contents, sizeof pItem->m_contentsShown);
	strcpy (pItem->m_contentsShown + sizeof(pItem->m_contentsShown) - 4, "...");
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
	
	pData->m_elementCount++;
	pData->m_highlightedElementIdx = -1;
	
	//also update the scroll bar.
	CtlUpdateScrollBarSize(pCtl, pWindow);
	CtlSetListItemText(pCtl, pData->m_elementCount - 1, optionalIcon, pText);
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
static int CtlGetSelectedIndexList(Control* pCtl)
{
	return pCtl->m_listViewData.m_highlightedElementIdx;
}
static void CtlSetSelectedIndexList(Control* pCtl, int index)
{
	if (index < -1) index = -1;
	if (index > 0)
	{
		if (index > pCtl->m_listViewData.m_elementCount - 1)
			index = pCtl->m_listViewData.m_elementCount - 1;
	}
	
	pCtl->m_listViewData.m_highlightedElementIdx = index;
}
static void CtlResetList (Control* pCtl, Window* pWindow)
{
	ListViewData* pData = &pCtl->m_listViewData;
	
	if (pData->m_pItems)
		MmFreeK (pData->m_pItems);
	
	pData->m_highlightedElementIdx = -1;
	pData->m_elementCount = 0;
	pData->m_capacity     = 10;
	
	pData->m_trackedListItem = -1;
	pData->m_bIsDraggingIt   = false;
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

void WidgetListView_GetViewableArea(Control* this, int* pElemStart, int* pElemEnd, bool bCapByElemCount)
{
	ListViewData* pData = &this->m_listViewData;
	
	int elementStart =   pData->m_scrollY;
	int elementEnd   =   pData->m_scrollY + (this->m_rect.bottom - this->m_rect.top + LIST_ITEM_HEIGHT - 1) / LIST_ITEM_HEIGHT - 1;
	
	if (elementStart >= pData->m_elementCount && bCapByElemCount) elementStart = pData->m_elementCount-1;
	if (elementStart < 0) elementStart = 0;
	
	if (elementEnd < 0) elementEnd = 0;
	if (elementEnd >= pData->m_elementCount && bCapByElemCount) elementEnd = pData->m_elementCount-1;
	
	*pElemStart = elementStart;
	*pElemEnd   = elementEnd;
}

void WidgetListView_DrawElement(Control* this, int index)
{
	ListViewData* pData = &this->m_listViewData;
	
	int elementStart, elementEnd;
	WidgetListView_GetViewableArea(this, &elementStart, &elementEnd, false);
	
	if (elementStart > elementEnd)
		return;
	
	if (index < elementStart || elementEnd < index)
		return;
	
	Rectangle rect = this->m_rect;
	
	int offsetLeft = 0, offsetTop = 0;
	
	if (~this->m_parm1 & LISTVIEW_NOBORDER)
	{
		rect.left += 2;
		rect.right -= 2;
		
		offsetLeft = 2;
		offsetTop = 2;
	}
	
	bool bSelected = pData->m_highlightedElementIdx == index;
	
	rect.top = this->m_rect.top + offsetTop + LIST_ITEM_HEIGHT * (index - pData->m_scrollY);
	rect.bottom = rect.top + LIST_ITEM_HEIGHT;
	
	// draw the background rectangle
	VidFillRect(bSelected ? SELECTED_ITEM_COLOR : WINDOW_TEXT_COLOR_LIGHT, rect.left, rect.top, rect.right - 1, rect.bottom - 1);
	
	// if we don't actually have an element here, return
	if (index < 0 || index >= pData->m_elementCount)
		return;
	
	rect.left += offsetLeft + 2;
	
	ListItem* pItem = &pData->m_pItems[index];
	if (pItem->m_icon)
	{
		RenderIconForceSize(pItem->m_icon, rect.left, rect.top + (rect.bottom - rect.top - 16) / 2, 16);
		rect.left += 22;
	}
	
	VidDrawText(pItem->m_contentsShown, rect, TEXTSTYLE_VCENTERED, bSelected ? SELECTED_TEXT_COLOR : WINDOW_TEXT_COLOR, TRANSPARENT);
}

bool WidgetListView_OnEvent(Control* this, UNUSED int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
{	
	switch (eventType)
	{
		case EVENT_SIZE:
		{
			CtlUpdateScrollBarSize (this, pWindow);
			break;
		}
		case EVENT_MOVECURSOR:
		case EVENT_RELEASECURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			int pos = GetScrollBarPos(pWindow, -this->m_comboID);
			if (pData->m_scrollY != pos)
			{
				pData->m_scrollY  = pos;
				WidgetListView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
				break;
			}
			
			if (!(eventType == EVENT_RELEASECURSOR || (eventType == EVENT_MOVECURSOR && (this->m_parm1 & LISTVIEW_SINGLECLICK)))) break;
			
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (!RectangleContains(&this->m_rect, &pt))
				break;
			
			pt.x -= this->m_rect.left;
			pt.y -= this->m_rect.top;
			
			if (~this->m_parm1 & LISTVIEW_NOBORDER)
			{
				pt.x -= 2;
				pt.y -= 2;
			}
			
			if (pt.y < 0) break;
	
			int elementStart, elementEnd;
			WidgetListView_GetViewableArea(this, &elementStart, &elementEnd, true);
			
			pos = pt.y / LIST_ITEM_HEIGHT + pData->m_scrollY;
			if (pos < elementStart || elementEnd < pos) break;
			
			int old = pData->m_highlightedElementIdx;
			pData->m_highlightedElementIdx = pos;
			
			if (old != pos)
			{
				WidgetListView_DrawElement(this, old);
				WidgetListView_DrawElement(this, pos);
			}
			
			bool bShouldActivate = (this->m_parm1 & LISTVIEW_SINGLECLICK);
			
			if (old == pos)
			{
				// double click!
				bShouldActivate = true;
			}
			
			if (eventType != EVENT_RELEASECURSOR)
				bShouldActivate = false;
			
			if (bShouldActivate)
				CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, pos);
			
			break;
		}
		case EVENT_PAINT:
		{
			VidSetClipRect(&this->m_rect);
			//draw a green rectangle:
			Rectangle rk = this->m_rect;
			
			if (~this->m_parm1 & LISTVIEW_NOBORDER)
			{
				rk.left   += 2;
				rk.top    += 2;
				rk.right  -= 2;
				rk.bottom -= 2;
			}
			
			VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, rk);
			
			int elementStart, elementEnd;
			WidgetListView_GetViewableArea(this, &elementStart, &elementEnd, true);
			
			if (elementStart > elementEnd)
				VidDrawText ("(Empty)", rk, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0x7F7F7F, TRANSPARENT);
			
			for (int i = elementStart, j = 0; i <= elementEnd; i++, j++)
			{
				WidgetListView_DrawElement(this, i);
			}
			
			if (~this->m_parm1 & LISTVIEW_NOBORDER)
				DrawEdge(this->m_rect, DRE_SUNKEN, 0);
			
			VidSetClipRect(NULL);
			
			break;
		}
		case EVENT_CREATE:
		{
			// Start out with an initial size of 10 elements.
			ListViewData* pData = &this->m_listViewData;
			pData->m_elementCount = 0;
			pData->m_capacity     = 10;
			pData->m_scrollY      = 0;
			//pData->m_hasIcons     = true;
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
			this->m_rect.right -= SCROLL_BAR_WIDTH;
			
			if (~this->m_parm1 & LISTVIEW_NOBORDER)
				this->m_rect.right -= 4;
			
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
			
			RemoveControl(pWindow, -this->m_comboID);
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
	
	memcpy (pItem->m_contentsShown, pItem->m_contents, 64);
	pItem->m_contentsShown[sizeof(pItem->m_contentsShown) - 1] = 0;
	
	char text_wrapped[256];
	WrapText(text_wrapped, pItem->m_contentsShown, ICON_ITEM_WIDTH);
	if (text_wrapped[0] == '\n')
		memcpy(text_wrapped, text_wrapped+1, sizeof(text_wrapped) - 1);
	
	memcpy (pItem->m_contentsShown, text_wrapped, 64);
	
	strcpy (pItem->m_contentsShown + sizeof(pItem->m_contentsShown) - 4, "...");
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
bool WidgetIconView_OnEvent(Control* this, UNUSED int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
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
				int elementRowsPerScreen = (this->m_rect.bottom - this->m_rect.top + ICON_ITEM_HEIGHT - 1) / ICON_ITEM_HEIGHT;
				
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
				if ((this->m_parm1 & LISTVIEW_SINGLECLICK) || (isDoubleClick && elementHighlightAttempt != -1))
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
			VidSetClipRect(&this->m_rect);
			
			//draw a green rectangle:
			Rectangle rk = this->m_rect;
			rk.left   += 2;
			rk.top    += 2;
			rk.right  -= 2;
			rk.bottom -= 2;
			VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, rk);
			ListViewData* pData = &this->m_listViewData;
			
			int elementColsPerScreen = (this->m_rect.right  - this->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
			int elementRowsPerScreen = (this->m_rect.bottom - this->m_rect.top + ICON_ITEM_HEIGHT - 1)  / ICON_ITEM_HEIGHT;
			
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
				int x = this->m_rect.left + 4 + elementX, y = this->m_rect.top + 5 + 4 + 2 + j * ICON_ITEM_HEIGHT + pData->m_hasIcons * 32;
				uint32_t color = WINDOW_TEXT_COLOR;
				Rectangle br = { x, y, x + ICON_ITEM_WIDTH, y + ICON_ITEM_HEIGHT };
				if (pData->m_highlightedElementIdx == i)
				{
					color = SELECTED_TEXT_COLOR;//, colorT = 0x7F;
					
					int w, h;
					VidTextOutInternal(pData->m_pItems[i].m_contentsShown, 0, 0, 0, 0, true, &w, &h);
					
					int mid = (br.left + br.right) / 2;
					
					VidFillRect(SELECTED_ITEM_COLOR, mid - w/2 - 2, br.top - 1, mid + w/2, br.top + h + 1);
				}
				if (pData->m_hasIcons)
				{
					if (pData->m_pItems[i].m_icon)
						RenderIconForceSize (pData->m_pItems[i].m_icon, x + (ICON_ITEM_WIDTH - 32) / 2, this->m_rect.top + 5 + j * ICON_ITEM_HEIGHT, 32);
				}
				//VidTextOut (pData->m_pItems[i].m_contentsShown, this->m_rect.left + 4 + elementX, this->m_rect.top + 4 + 2 + j * ICON_ITEM_HEIGHT + pData->m_hasIcons * 32, color, colorT);
				VidDrawText (pData->m_pItems[i].m_contentsShown, br, TEXTSTYLE_HCENTERED, color, TRANSPARENT);
				
				elementX += ICON_ITEM_WIDTH;
				k++;
				if (k >= elementColsPerScreen)
				{
					elementX = 0;
					k = 0;
					j++;
				}
			}
			
			DrawEdge(this->m_rect, DRE_SUNKEN, 0);
			
			VidSetClipRect(NULL);
			
			break;
		}
		case EVENT_CREATE:
		{
			// Start out with an initial size of 10 elements.
			ListViewData* pData = &this->m_listViewData;
			pData->m_elementCount = 0;
			pData->m_capacity     = 10;
			pData->m_scrollY      = 0;
			pData->m_scrollX      = 0;
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
			RemoveControl(pWindow, -this->m_comboID);
			
			break;
		}
	}
	return false;//Fall through to other controls.
}

#endif

// Icon list drag view
#if 1
static void CtlIconDragUpdateScrollBarSize(Control* pCtlIcon, Window* pWindow)
{
	int screenw = (pCtlIcon->m_rect.right  - pCtlIcon->m_rect.left);
	int screenh = (pCtlIcon->m_rect.bottom - pCtlIcon->m_rect.top );
	
	//also update the scroll bar.
	ListViewData* pData = &pCtlIcon->m_listViewData;
	int c = pData->m_extentY - screenh;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, -pCtlIcon->m_comboID, c);
	
	c = pData->m_extentX - screenw;
	if (c <= 0)
		c  = 1;
	SetScrollBarMax (pWindow, 0x70000000 - pCtlIcon->m_comboID, c);
}
void WidgetIconViewDrag_ArrangeIcons (Control *this);
void CtlIconDragRecalculateExtents (Control *this)
{
	ListViewData* pData = &this->m_listViewData;
	
	pData->m_extentX = pData->m_extentY = 0;
	for (int i = 0; i < pData->m_elementCount; i++)
	{
		if (pData->m_extentX < pData->m_pItems[i].m_posX + ICON_ITEM_WIDTH)
			pData->m_extentX = pData->m_pItems[i].m_posX + ICON_ITEM_WIDTH;
		if (pData->m_extentY < pData->m_pItems[i].m_posY + ICON_ITEM_HEIGHT)
			pData->m_extentY = pData->m_pItems[i].m_posY + ICON_ITEM_HEIGHT;
	}
}
static void CtlIconDragAddElementToList (Control* pCtlIcon, const char* pText, int optionalIcon, Window* pWindow)
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
	
	pItem->m_icon = optionalIcon;
	strcpy(pItem->m_contents, pText);
	
	memcpy (pItem->m_contentsShown, pItem->m_contents, 64);
	pItem->m_contentsShown[sizeof(pItem->m_contentsShown) - 1] = 0;
	
	char text_wrapped[256];
	WrapText(text_wrapped, pItem->m_contentsShown, ICON_ITEM_WIDTH);
	if (text_wrapped[0] == '\n')
		memmove(text_wrapped, text_wrapped+1, sizeof(text_wrapped) - 1);
	
	memcpy (pItem->m_contentsShown, text_wrapped, 64);
	
	strcpy (pItem->m_contentsShown + sizeof(pItem->m_contentsShown) - 4, "...");
	
	//TODO!
	//WidgetIconViewDrag_ArrangeIcons(pCtlIcon);
	
	int elemIndex = pData->m_elementCount - 1;
	int elementColsPerScreen = (pCtlIcon->m_rect.right  - pCtlIcon->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
	int x = 4, y = 4 + 2;
	x += ICON_ITEM_WIDTH  * (elemIndex % elementColsPerScreen);
	y += ICON_ITEM_HEIGHT * (elemIndex / elementColsPerScreen);
	pItem->m_posX = x;
	pItem->m_posY = y;
	
	pData->m_trackedListItem = -1;
	pData->m_bIsDraggingIt   = false;
	
	CtlIconDragRecalculateExtents (pCtlIcon);
	CtlIconDragUpdateScrollBarSize(pCtlIcon, pWindow);
	
	//WrapText(pItem->m_contents, pText, ICON_ITEM_WIDTH);
}
static void CtlIconDragRemoveElementFromList(Control* pCtlIcon, int index, Window* pWindow)
{
	ListViewData* pData = &pCtlIcon->m_listViewData;
	memcpy (pData->m_pItems + index, pData->m_pItems + index + 1, sizeof(ListItem) * (pData->m_elementCount - index - 1));
	pData->m_elementCount--;
	pData->m_highlightedElementIdx = -1;
	
	pData->m_trackedListItem = -1;
	pData->m_bIsDraggingIt   = false;
	
	//also update the scroll bar.
	CtlIconDragUpdateScrollBarSize(pCtlIcon, pWindow);
}
void WidgetIconViewDrag_ArrangeIcons (Control *this)
{
	ListViewData* pData = &this->m_listViewData;
	
	int elementColsPerScreen = (this->m_rect.right  - this->m_rect.left + ICON_ITEM_WIDTH/2) / ICON_ITEM_WIDTH;
	
	int elementStart =   0;
	int elementEnd   =   pData->m_elementCount - 1;
	
	int elementX = 0;
	
	for (int i = elementStart, j = 0, k = 0; i <= elementEnd; i++)
	{
		int x = 4 + elementX, y = 4 + 2 + j * ICON_ITEM_HEIGHT;
		
		ListItem* pItem = &pData->m_pItems[i];
		
		pItem->m_posX = x;
		pItem->m_posY = y;
		
		elementX += ICON_ITEM_WIDTH;
		k++;
		if (k >= elementColsPerScreen)
		{
			elementX = 0;
			k = 0;
			j++;
		}
	}
	
	CtlIconDragRecalculateExtents (this);
}

SAI int _Abs(int i)
{
	if (i < 0) return -i;
	else return i;
}

//extern VBEData*g_vbeData,g_mainScreenVBEData;
bool WidgetIconViewDrag_OnEvent(Control* this, UNUSED int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_SIZE:
		{
			CtlIconDragUpdateScrollBarSize (this, pWindow);
			break;
		}
		//fallthrough intentional
		case EVENT_CLICKCURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			bool bUpdate = false;
			int
			pos = GetScrollBarPos(pWindow, -this->m_comboID);
			if (pData->m_scrollY != pos)
			{
				pData->m_scrollY  = pos;
				bUpdate = true;
			}
			pos = GetScrollBarPos(pWindow, 0x70000000 - this->m_comboID);
			if (pData->m_scrollX != pos)
			{
				pData->m_scrollX  = pos;
				bUpdate = true;
			}
			
			// For each icon, see if it needs to be selected
			int nSelectedIcon = -1;
			
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (!bUpdate && RectangleContains (&this->m_rect, &pt))
			{
				for (int i = 0; i < pData->m_elementCount; i++)
				{
					ListItem *pItem = &pData->m_pItems[i];
					
					int x = this->m_rect.left + pItem->m_posX - pData->m_scrollX, y = this->m_rect.top + pItem->m_posY - pData->m_scrollY;
					Rectangle br = { x, y, x + ICON_ITEM_WIDTH, y + ICON_ITEM_HEIGHT };
					
					if (RectangleContains (&br, &pt))
					{
						nSelectedIcon = i;
						break;
					}
				}
			
				if (pData->m_trackedListItem == -1)
				{
					// Start tracking it.
					pData->m_trackedListItem = nSelectedIcon;
					pData->m_bIsDraggingIt   = false;
					pData->m_startDragX      = pt.x;
					pData->m_startDragY      = pt.y;
					bUpdate = true;
				}
				else if (_Abs(pData->m_startDragX - pt.x) > 4 || _Abs(pData->m_startDragY - pt.y) > 4)
				{
					// Start dragging it.
					Image* pImg = GetIconImage(pData->m_pItems[pData->m_trackedListItem].m_icon, 32);
					
					//prepare custom cursor info
					pWindow->m_customCursor.width  = pImg->width;
					pWindow->m_customCursor.height = pImg->height;
					pWindow->m_customCursor.boundsWidth  = pImg->width;
					pWindow->m_customCursor.boundsHeight = pImg->height;
					pWindow->m_customCursor.leftOffs = pImg->width  / 2;
					pWindow->m_customCursor.topOffs  = ICON_ITEM_HEIGHT / 2;
					pWindow->m_customCursor.bitmap   = pImg->framebuffer;
					pWindow->m_customCursor.m_transparency = true;
					pWindow->m_customCursor.m_flags  = 0;
					
					ChangeCursor (pWindow, CURSOR_CUSTOM); // for testing
					
					// only update if we were not dragging it before :^)
					bUpdate = !(pData->m_bIsDraggingIt);
					
					pData->m_bIsDraggingIt = true;
				}
			}
			
			if (bUpdate)
				goto paint_already;
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			ListViewData* pData = &this->m_listViewData;
			bool bUpdate = false;
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			if (pData->m_trackedListItem != -1 && pData->m_bIsDraggingIt)
			{
				// Release the tracked item at some position
				int newX = pt.x - this->m_rect.left + pData->m_scrollX - (ICON_ITEM_WIDTH  / 2);
				int newY = pt.y - this->m_rect.top  + pData->m_scrollY - (ICON_ITEM_HEIGHT / 2);
				if (newX < 0) newX = 0;
				if (newY < 0) newY = 0;
				// don't do any extent restrictions, the extent will be extended automatically
				// instead, do a restriction on some random imaginary board size
				if (newX > 4096) newX = 4096;
				if (newY > 4096) newY = 4096;
				
				// Place the icon there.
				ListItem *pItem = &pData->m_pItems[pData->m_trackedListItem];
				pItem->m_posX = newX;
				pItem->m_posY = newY;
				
				CtlIconDragRecalculateExtents  (this);
				
				CtlIconDragUpdateScrollBarSize (this, pWindow);
				
				bUpdate = true;
				
				ChangeCursor (pWindow, CURSOR_DEFAULT);
			}
			else
			{
				// For each icon, see if it needs to be selected
				int nSelectedIcon = -1;
				
				if (RectangleContains (&this->m_rect, &pt))
				{
					for (int i = 0; i < pData->m_elementCount; i++)
					{
						ListItem *pItem = &pData->m_pItems[i];
						
						int x = this->m_rect.left + pItem->m_posX - pData->m_scrollX, y = this->m_rect.top + pItem->m_posY - pData->m_scrollY;
						Rectangle br = { x, y, x + ICON_ITEM_WIDTH, y + ICON_ITEM_HEIGHT };
						
						if (RectangleContains (&br, &pt))
						{
							nSelectedIcon = i;
							break;
						}
					}
				}
				
				bool shouldActivate = (this->m_parm1 & LISTVIEW_SINGLECLICK);
				
				if (pData->m_highlightedElementIdx != nSelectedIcon)
				{
					pData->m_highlightedElementIdx = nSelectedIcon;
					bUpdate = true;
				}
				else if (nSelectedIcon != -1)
				{
					shouldActivate = true;
				}
				
				if (shouldActivate)
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, nSelectedIcon);
			}
			
			pData->m_trackedListItem = -1;
			pData->m_bIsDraggingIt   = false;
			
			if (!bUpdate)
				break;
		}
	//#pragma GCC diagnostic pop
		case EVENT_PAINT:
		{
		paint_already:
			VidSetClipRect(&this->m_rect);
			
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
			
			for (int i = 0; i < pData->m_elementCount; i++)
			{
				ListItem *pItem = &pData->m_pItems[i];
				
				int x = this->m_rect.left + pItem->m_posX - pData->m_scrollX, y = this->m_rect.top + pItem->m_posY - pData->m_scrollY;
				/*
				if (x - ICON_ITEM_WIDTH  < this->m_rect.left) continue;
				if (y - ICON_ITEM_HEIGHT < this->m_rect.top)  continue;
				if (x >= this->m_rect.right)  continue;
				if (y >= this->m_rect.bottom) continue;
				*/
				
				uint32_t color = WINDOW_TEXT_COLOR;
				Rectangle br = { x, y + 36 * pData->m_hasIcons, x + ICON_ITEM_WIDTH, y + ICON_ITEM_HEIGHT };
				
				if (pData->m_bIsDraggingIt && pData->m_trackedListItem == i)
					continue;
				
				if (pData->m_highlightedElementIdx == i || pData->m_trackedListItem == i)
				{
					color = SELECTED_TEXT_COLOR;//, colorT = 0x7F;
					
					int w, h;
					VidTextOutInternal(pData->m_pItems[i].m_contentsShown, 0, 0, 0, 0, true, &w, &h);
					
					int mid = (br.left + br.right) / 2;
					
					VidFillRect(SELECTED_ITEM_COLOR, mid - w/2 - 2, br.top - 1, mid + w/2, br.top + h + 1);
				}
				if (pData->m_hasIcons)
				{
					if (pData->m_pItems[i].m_icon)
						RenderIconForceSize (pData->m_pItems[i].m_icon, x + (ICON_ITEM_WIDTH - 32) / 2, y, 32);
				}
				
				VidDrawText (pData->m_pItems[i].m_contentsShown, br, TEXTSTYLE_HCENTERED, color, TRANSPARENT);
			}
			
			DrawEdge(this->m_rect, DRE_SUNKEN, 0);
			
			VidSetClipRect(NULL);
			
			break;
		}
		case EVENT_CREATE:
		{
			// Start out with an initial size of 10 elements.
			ListViewData* pData = &this->m_listViewData;
			pData->m_elementCount = 0;
			pData->m_capacity     = 10;
			pData->m_scrollY      = 0;
			pData->m_scrollX      = 0;
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
			r.bottom= this->m_rect.bottom - SCROLL_BAR_WIDTH, 
			r.left  = this->m_rect.right  - SCROLL_BAR_WIDTH;
			
			int c = pData->m_elementCount;
			if (c <= 0)
				c  = 1; 
			
			int flags = 0;
			if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				flags |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
			if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				flags |= ANCHOR_BOTTOM_TO_BOTTOM;
			
			AddControlEx (pWindow, CONTROL_VSCROLLBAR, flags, r, NULL, -this->m_comboID, c, 1);
			
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
			
			RemoveControl(pWindow, 0x70000000 - this->m_comboID);
			RemoveControl(pWindow,            - this->m_comboID);
			
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
			else if (pWindow->m_pControlArray[i].m_type == CONTROL_ICONVIEWDRAG)
				CtlIconDragAddElementToList (&pWindow->m_pControlArray[i], pText, optionalIcon, pWindow);
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
			else if (pWindow->m_pControlArray[i].m_type == CONTROL_ICONVIEWDRAG)
				CtlIconDragRemoveElementFromList (&pWindow->m_pControlArray[i], elementIndex, pWindow);
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

int GetSelectedIndexList (Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return CtlGetSelectedIndexList (&pWindow->m_pControlArray[i]);
		}
	}
	return -1;
}

void SetSelectedIndexList (Window* pWindow, int comboID, int index)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetSelectedIndexList(&pWindow->m_pControlArray[i], index);
			return;
		}
	}
}

void SetListItemText(Window* pWindow, int comboID, int index, int icon, const char * pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetListItemText(&pWindow->m_pControlArray[i], index, icon, pText);
			return;
		}
	}
}

#endif
