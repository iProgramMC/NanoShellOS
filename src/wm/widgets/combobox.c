/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

    Widget library:  Combo Box control
******************************************/
#include "../wi.h"

typedef struct
{
	int id;
	int icon;
	char data[120];
}
ComboBoxItem;

typedef struct
{
	Window       *m_pWindow;
	ComboBoxItem *m_Items;
	size_t        m_ItemCount;
	size_t        m_ItemCapacity;
	size_t        m_ShownItem;
	Window       *m_pSubWindow;
	bool          m_bHovered;
	bool          m_bClicked;
	int           m_CtlComboID;
}
ComboBoxData;

ComboBoxData* ComboBox_GetData(Control* this)
{
	return (ComboBoxData*) this->m_dataPtr;
}

Rectangle ComboBox_GetButtonRect(Control* this)
{
	Rectangle rect = this->m_rect;
	rect.left   += 2;
	rect.top    += 2;
	rect.right  -= 2;
	rect.bottom -= 2;
	int buttonSize = rect.bottom - rect.top;
	rect.right -= buttonSize;
	rect.left = rect.right;
	rect.right = rect.left + buttonSize;
	return rect;
}

void ComboBox_Paint(Control* this, bool paintContent, bool paintButton, bool paintEdge)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	VidSetClipRect(&this->m_rect);
	
	if (paintEdge)
		DrawEdge(this->m_rect, DRE_SUNKEN, 0);
	
	Rectangle rect = this->m_rect;
	rect.left   += 2;
	rect.top    += 2;
	rect.right  -= 2;
	rect.bottom -= 2;
	
	int buttonSize = rect.bottom - rect.top;
	
	Rectangle bgRect = rect;
	bgRect.right--;
	bgRect.bottom--;
	bgRect.right -= buttonSize;
	
	if (paintContent)
		VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, bgRect);
	
	if (paintButton)
	{
		Rectangle buttonRect = bgRect;
		buttonRect.left = bgRect.right + 1;
		buttonRect.right = buttonRect.left + buttonSize;
		buttonRect.bottom++;
		
		int flags = DRE_RAISED | DRE_FILLED;
		uint32_t color = BUTTON_MIDDLE_COLOR;
		if (pData->m_bHovered)
		{
			flags |= DRE_HOT;
			color = BUTTON_HOVER_COLOR;
		}
		if (pData->m_bClicked)
		{
			flags = DRE_SUNKENINNER | DRE_FILLED;
			color = BUTTONMIDC;
		}
		
		DrawEdge(buttonRect, flags, color);
		
		//buttonRect.top++;
		//buttonRect.bottom++;
		
		if (pData->m_bClicked)
		{
			buttonRect.left++;
			buttonRect.top++;
			buttonRect.right++;
			buttonRect.bottom++;
		}
		
		//VidDrawText ("\x19", buttonRect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
		DrawArrow(buttonRect, DRA_DOWN, DRA_CENTERALL | DRA_IGNORESIZE, WINDOW_TEXT_COLOR);
	}
	
	if (paintContent)
	{
		if (pData->m_ItemCount != 0)
		{
			Rectangle contentRect = bgRect;
			contentRect.right++;
			contentRect.bottom++;
			contentRect.left += 2;
			
			// paint the icon, if it exists
			ComboBoxItem* pItem = &pData->m_Items[pData->m_ShownItem];
			if (pItem->icon)
			{
				int size = contentRect.bottom - contentRect.top;
				
				RenderIconForceSize(pItem->icon, contentRect.left, contentRect.top, size);
				contentRect.left += size;
			}
			
			// Draw the text.
			VidDrawText(pItem->data, contentRect, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
		}
	}
	
	VidSetClipRect(NULL);
}

void ComboBox_AddItem(Control* this, const char* item, int itemID, int iconID)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	if (pData->m_ItemCapacity <= pData->m_ItemCount)
	{
		if (pData->m_ItemCapacity)
			pData->m_ItemCapacity *= 2;
		else
			pData->m_ItemCapacity = 32;
		
		pData->m_Items = MmReAllocate(pData->m_Items, sizeof(ComboBoxItem) * pData->m_ItemCapacity);
	}
	
	ComboBoxItem *pItem = &pData->m_Items[pData->m_ItemCount++];
	
	strncpy(pItem->data, item, sizeof pItem->data);
	pItem->data[sizeof pItem->data - 1] = 0;
	
	pItem->id = itemID;
	pItem->icon = iconID;
	
	ComboBox_Paint(this, true, false, false);
}

void WidgetComboBox_SubMenuEventProc(Window* pWindow, int eventType, long parm1, long parm2)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			Rectangle rect = GetWindowClientRect(pWindow, true);
			rect.right -= rect.left;
			rect.bottom -= rect.top;
			rect.left = rect.top = 0;
			
			AddControl(pWindow, CONTROL_LISTVIEW, rect, "", 1, LISTVIEW_NOBORDER | LISTVIEW_SINGLECLICK, 0);
			
			ComboBoxData* pData = (ComboBoxData*)pWindow->m_data;
			
			for (size_t i = 0; i < pData->m_ItemCount; i++)
			{
				ComboBoxItem* pItem = &pData->m_Items[i];
				
				AddElementToList(pWindow, 1, pItem->data, pItem->icon);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			ComboBoxData* pData = (ComboBoxData*)pWindow->m_data;
			
			if ((size_t)parm1 >= pData->m_ItemCount)
			{
				SLogMsg("Invalid cmd in combo box sub window");
				break;
			}
			
			pData->m_ShownItem = parm2;
			WindowAddEventToMasterQueue(pData->m_pWindow, EVENT_COMBOSELCHANGED_PTE, pData->m_CtlComboID, 0);
			DestroyWindow(pWindow);
			break;
		}
		case EVENT_KILLFOCUS:
		{
			DestroyWindow(pWindow);
			break;
		}
		case EVENT_DESTROY:
		{
			ComboBoxData* pData = (ComboBoxData*)pWindow->m_data;
			
			// send our parent a message that we're gone.
			// ComboBox_CloseSubWindow will send a parm1 of 4242 to let us know that
			// we already know it'll be gone
			if (parm1 != 4242)
				WindowAddEventToMasterQueue(pData->m_pWindow, EVENT_COMBOSUBGONE, pData->m_CtlComboID, 0);
			
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			break;
		}
		default:
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
	}
}

void ComboBox_CloseSubWindow(Control* this)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	if (!pData->m_pSubWindow) return;
	
	WindowAddEventToMasterQueue(pData->m_pSubWindow, EVENT_DESTROY, 4242, 0);
	
	// The window manager will take care of events, so just assume it's gone already to avoid a race condition.
	// The sub window will send us an EVENT_COMBOSUBGONE, so just treat it as if it's gone.
	pData->m_pSubWindow = NULL;
}

void ComboBox_OpenSubWindow(Control* this)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	// If there's already a sub window, get rid of that.
	ComboBox_CloseSubWindow(this);
	
	// determine how many items to show at once
	int numItems = pData->m_ItemCount;
	if (numItems > 16)
		numItems = 16;
	
	Rectangle pwRect = GetWindowClientRect(pData->m_pWindow, true);
	
	Rectangle windowRect;
	windowRect.left   = pwRect.left + this->m_rect.left;
	windowRect.top    = pwRect.top  + this->m_rect.bottom;
	windowRect.right  = pwRect.left + this->m_rect.right;
	windowRect.bottom = windowRect.top + 16 * numItems;
	windowRect.left  += 2; // correct for the combobox's edge
	windowRect.right -= 2;
	if (windowRect.bottom >= GetScreenHeight())
		windowRect.bottom = GetScreenHeight();
	
	Window* pSubWindow = CreateWindow(
		"Combobox Sub Menu",
		windowRect.left, windowRect.top,
		GetWidth(&windowRect), GetHeight(&windowRect),
		WidgetComboBox_SubMenuEventProc,
		WF_MENUITEM | WF_SYSPOPUP | WI_NEVERSEL | WF_NOMINIMZ | WF_NOCLOSE | WF_NOTITLE | WF_FLATBORD | WF_NOWAITWM
	);
	
	if (!pSubWindow)
	{
		SLogMsg("Could not create a sub window!");
		return;
	}
	
	pData->m_pSubWindow = pSubWindow;
	pSubWindow->m_data = pData;
	
	// handle the initial 'create' event
	if (!HandleMessages(pSubWindow))
		pData->m_pSubWindow = NULL;
	
	// now let the window manager handle events:
	pData->m_pSubWindow->m_bWindowManagerUpdated = true;
}

int ComboBox_GetSelectedItemID(Control* this)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	if (pData->m_ShownItem >= pData->m_ItemCount)
		return -1;
	
	return pData->m_Items[pData->m_ShownItem].id;
}

void ComboBox_SetSelectedItemID(Control* this, int iid)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	for (size_t i = 0; i < pData->m_ItemCount; i++)
	{
		if (pData->m_Items[i].id == iid)
		{
			pData->m_ShownItem = i;
			
			ComboBox_Paint(this, true, false, false);
		}
	}
}

void ComboBox_ClearItems(Control* this)
{
	ComboBoxData* pData = ComboBox_GetData(this);
	
	MmFree(pData->m_Items);
	pData->m_Items        = NULL;
	pData->m_ItemCount    = 0;
	pData->m_ItemCapacity = 0;
	pData->m_ShownItem    = 0;
	
	ComboBox_Paint(this, true, false, false);
}

bool WidgetComboBox_OnEvent(Control* this, int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			ComboBoxData* pData = MmAllocate(sizeof *pData);
			memset(pData, 0, sizeof *pData);
			
			this->m_dataPtr = pData;
			
			pData->m_pWindow = pWindow;
			pData->m_CtlComboID = this->m_comboID;
			
			break;
		}
		// the sub window told us that it's gone. We must obey
		case EVENT_COMBOSUBGONE:
		{
			if (parm1 != this->m_comboID)
				break; // ignore
			
			ComboBoxData* pData = ComboBox_GetData(this);
			pData->m_pSubWindow = NULL;
			break;
		}
		case EVENT_DESTROY:
		{
			ComboBoxData* pData = ComboBox_GetData(this);
			
			ComboBox_CloseSubWindow(this);
			ComboBox_ClearItems(this);
			
			MmFree(pData);
			this->m_dataPtr = NULL;
			
			break;
		}
		case EVENT_PAINT:
		{
			ComboBox_Paint(this, true, true, true);
			break;
		}
		case EVENT_MOVECURSOR:
		{
			Rectangle rect = ComboBox_GetButtonRect(this);
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			ComboBoxData* pData = ComboBox_GetData(this);
			
			if (RectangleContains(&rect, &p))
			{
				if (!pData->m_bHovered)
				{
					pData->m_bHovered = true;
					ComboBox_Paint(this, false, true, false);
				}
			}
			else if (pData->m_bHovered)
			{
				pData->m_bHovered = false;
				ComboBox_Paint(this, false, true, false);
			}
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Rectangle rect = ComboBox_GetButtonRect(this);
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			ComboBoxData* pData = ComboBox_GetData(this);
			
			if (RectangleContains(&rect, &p))
			{
				if (!pData->m_bClicked)
				{
					pData->m_bClicked = true;
					ComboBox_Paint(this, false, true, false);
				}
			}
			else if (pData->m_bClicked)
			{
				pData->m_bClicked = false;
				ComboBox_Paint(this, false, true, false);
			}
			
			break;
		}
		case EVENT_COMBOSELCHANGED_PTE:
		{
			// was this meant for us?
			if (parm1 != this->m_comboID) break;
			
			ComboBoxData* pData = ComboBox_GetData(this);
			
			ComboBox_Paint(this, true, false, false);
			
			WindowAddEventToMasterQueue(pWindow, EVENT_COMBOSELCHANGED, this->m_comboID, pData->m_Items[pData->m_ShownItem].id);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Rectangle rect = ComboBox_GetButtonRect(this);
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			ComboBoxData* pData = ComboBox_GetData(this);
			
			if (RectangleContains(&rect, &p) && pData->m_bClicked)
			{
				ComboBox_OpenSubWindow(this);
			}
			
			if (pData->m_bClicked || pData->m_bHovered)
			{
				pData->m_bClicked = pData->m_bHovered = false;
				ComboBox_Paint(this, false, true, false);
			}
			
			break;
		}
	}
	
	return false;
}

void ComboBoxAddItem(Window* pWindow, int comboID, const char* item, int itemID, int iconID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control* pCtl = &pWindow->m_pControlArray[i];
		if (pCtl->m_comboID != comboID) continue;
		if (pCtl->OnEvent != WidgetComboBox_OnEvent) continue;
		
		ComboBox_AddItem(pCtl, item, itemID, iconID);
	}
}

int ComboBoxGetSelectedItemID(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control* pCtl = &pWindow->m_pControlArray[i];
		if (pCtl->m_comboID != comboID) continue;
		if (pCtl->OnEvent != WidgetComboBox_OnEvent) continue;
		
		return ComboBox_GetSelectedItemID(pCtl);
	}
	
	return 0;
}

void ComboBoxSetSelectedItemID(Window* pWindow, int comboID, int itemID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control* pCtl = &pWindow->m_pControlArray[i];
		if (pCtl->m_comboID != comboID) continue;
		if (pCtl->OnEvent != WidgetComboBox_OnEvent) continue;
		
		return ComboBox_SetSelectedItemID(pCtl, itemID);
	}
}

void ComboBoxClearItems(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control* pCtl = &pWindow->m_pControlArray[i];
		if (pCtl->m_comboID != comboID) continue;
		if (pCtl->OnEvent != WidgetComboBox_OnEvent) continue;
		
		return ComboBox_ClearItems(pCtl);
	}
}
