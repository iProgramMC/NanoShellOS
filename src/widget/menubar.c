/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Widget library: Menu bar control
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

// If a menu bar item has no children, clicking it will trigger an EVENT_COMMAND to the parent window
// with the comboID the menu bar item was given.

// Note that having multiple menu items with the same comboID, just like having more than one control with
// the same comboID, is undefined as per specification.

// Please note that the menu bar comboIDs are not necessarily related to the control comboIDs normally.
// Instead, when you click on a menu bar item, it fires an EVENT_COMMAND with the host control's comboID
// in parm1, and the menu item's comboID in parm2.

#define MENU_BAR_HEIGHT 15

void WidgetMenuBar_InitializeMenuBarItemAsEmpty (MenuBarTreeItem* this, int comboID)
{
	// Call the generic initializor for the menu bar tree item.
	this->m_comboID          = comboID;
	this->m_childrenCount    = 0;
	this->m_childrenCapacity = 4;
	this->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * sizeof (MenuBarTreeItem));
	if (!this->m_childrenArray)
	{
		this->m_childrenCapacity = 0;
		return;
	}
	memset (this->m_childrenArray, 0, 4 * sizeof (MenuBarTreeItem));
	this->m_text[0]          = 0;
	this->m_isOpen           = false;
}
bool WidgetMenuBar_TryAddItemTo (MenuBarTreeItem* this, int comboID_to, int comboID_as, const char* text)
{
	if (this->m_comboID == comboID_to)
	{
		// Can it fit now?
		if (this->m_childrenCount >= this->m_childrenCapacity)
		{
			// Doesn't fit.  Need to expand.
			MenuBarTreeItem* new = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * 2 * sizeof (MenuBarTreeItem));
			if (!new) return false;
			memset (new, 0, this->m_childrenCapacity * 2 * sizeof (MenuBarTreeItem));
			memcpy (new, this->m_childrenArray, this->m_childrenCapacity * sizeof (MenuBarTreeItem));
			this->m_childrenCapacity *= 2;
			
			// Get rid of the old one.  We've moved.
			MmFreeK (this->m_childrenArray);
			
			this->m_childrenArray = new;
		}
		
		//If we've expanded or otherwise can fit our new item into, add it.
		MenuBarTreeItem* pItem = this->m_childrenArray + this->m_childrenCount;
		this->m_childrenCount++;
		
		strcpy (pItem->m_text, text);
		pItem->m_comboID = comboID_as;
		pItem->m_childrenCount = 0;
		
		//Allocate a default of 2 items.
		pItem->m_childrenCapacity = 2;
		pItem->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (2 * sizeof (MenuBarTreeItem));
		if (!pItem->m_childrenArray) return false;
		memset (pItem->m_childrenArray, 0, 2 * sizeof (MenuBarTreeItem));
		
		//We were able to add it.  Leave and tell the caller that we did it.
		return true;
	}
	else
	{
		//try adding it to one of the children:
		for (int i = 0; i < this->m_childrenCount; i++)
		{
			if (WidgetMenuBar_TryAddItemTo(&this->m_childrenArray[i], comboID_to, comboID_as, text))
				return true;//just added it, no need to add it anymore
		}
		return false;//couldn't add here, try another branch
	}
}
void WidgetMenuBar_AddMenuBarItem (Control* this, int comboID_to, int comboID_as, const char* text)
{
	WidgetMenuBar_TryAddItemTo(&this->m_menuBarData.m_root, comboID_to, comboID_as, text);
}
void WidgetMenuBar_InitializeRoot (Control* this)
{
	// Call the generic initializor for the menu bar tree item with a comboID of zero.
	WidgetMenuBar_InitializeMenuBarItemAsEmpty (&this->m_menuBarData.m_root, 0);
}
void WidgetMenuBar_DeInitializeChild(MenuBarTreeItem *this)
{
	// deinitialize the children first.
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		WidgetMenuBar_DeInitializeChild(&this->m_childrenArray[i]);
	}
	
	// then, deinitialize this
	MmFreeK(this->m_childrenArray);
}
void WidgetMenuBar_DeInitializeRoot(Control* this)
{
	WidgetMenuBar_DeInitializeChild (&this->m_menuBarData.m_root);
}
bool WidgetMenuBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	bool has3dBorder = !(pWindow->m_flags & WF_FLATBORD);
	
	Rectangle menu_bar_rect;
	menu_bar_rect.left   = 1 + 3 * has3dBorder;
	menu_bar_rect.top    = has3dBorder * 2 + TITLE_BAR_HEIGHT;
	menu_bar_rect.right  = pWindow->m_vbeData.m_width - 1 - 3 * has3dBorder;
	menu_bar_rect.bottom = menu_bar_rect.top + MENU_BAR_HEIGHT + 3;
	
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			// Initialize the root.
			WidgetMenuBar_InitializeRoot(this);
			break;
		}
		case EVENT_DESTROY:
		{
			WidgetMenuBar_DeInitializeRoot(this);
			break;
		}
		case EVENT_PAINT:
		{
			// Render the root.  If any children are opened, draw them.
			VidFillRectangle (BUTTONMIDD, menu_bar_rect);
			VidDrawHLine (0, menu_bar_rect.left, menu_bar_rect.right, menu_bar_rect.bottom);
			
			if (this->m_menuBarData.m_root.m_childrenArray)
			{
				int current_x = 8;
				for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
				{
					int width, height;
					MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
					const char* pText = pChild->m_text;
					VidTextOutInternal (pText, 0, 0, 0, 0, true, &width, &height);
					
					width += 10;
					
					if (pChild->m_isOpen)
					{
						Rectangle rect;
						rect.left   = menu_bar_rect.left + current_x;
						rect.right  = menu_bar_rect.left + current_x + width;
						rect.top    = menu_bar_rect.top  + 1;
						rect.bottom = menu_bar_rect.bottom - 2;
						
						VidFillRectangle (0x7F, rect);
						
						VidTextOut (pText, menu_bar_rect.left + current_x + 5, menu_bar_rect.top + 2 + (MENU_BAR_HEIGHT - GetLineHeight()) / 2, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
						//render the child menu as well:
						
						/*WidgetMenuBar_RenderSubMenu (pChild, rect.left, rect.bottom);*/
					}
					else
						VidTextOut (pText, menu_bar_rect.left + current_x + 5, menu_bar_rect.top + 2 + (MENU_BAR_HEIGHT - GetLineHeight()) / 2, WINDOW_TEXT_COLOR, TRANSPARENT);
					
					current_x += width;
				}
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point p = {GET_X_PARM(parm1), GET_Y_PARM(parm1)};
			// Determine what item we've clicked.
			if (this->m_menuBarData.m_root.m_childrenArray)
			{
				int  current_x = 8;
				bool needsUpdate = false;
				for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
				{
					int width, height;
					MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
					const char* pText = pChild->m_text;
					VidTextOutInternal (pText, 0, 0, 0, 0, true, &width, &height);
					
					width += 10;
					
					Rectangle rect;
					rect.left   = menu_bar_rect.left + current_x;
					rect.right  = menu_bar_rect.left + current_x + width;
					rect.top    = menu_bar_rect.top  + 2;
					rect.bottom = menu_bar_rect.bottom;
					
					if (RectangleContains (&rect, &p))
					{
						for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
						{
							MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
							pChild->m_isOpen = false;
						}
						// Open this and call the paint event.
						
						if (pChild->m_childrenCount)
						{
							pChild->m_isOpen = true;
							WindowMenu menu;
							ConvertMenuBarToWindowMenu(&menu, pChild, this->m_comboID);
							SpawnMenu(pWindow, &menu, pWindow->m_rect.left + rect.left, pWindow->m_rect.top + rect.top + TITLE_BAR_HEIGHT);
							MenuRecursivelyFreeEntries (&menu);
						}
						else
						{
							CallWindowCallback (pWindow, EVENT_COMMAND, this->m_comboID, pChild->m_comboID);
						}
						
						//WidgetMenuBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
						//break;
						needsUpdate = true;
					}
					
					if (pChild->m_isOpen)
					{
						needsUpdate = true;
					}
					
					current_x += width;
					if (needsUpdate) break;
				}
				if (needsUpdate)
					//CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
					RequestRepaintNew(pWindow);
			}
			break;
		}
		case EVENT_MENU_CLOSE:
		{
			if (parm1 == this->m_comboID)
			{
				for (int i = 0; i < this->m_menuBarData.m_root.m_childrenCount; i++)
				{
					MenuBarTreeItem* pChild = &this->m_menuBarData.m_root.m_childrenArray[i];
					pChild->m_isOpen = false;
				}
				RequestRepaintNew(pWindow);
			}
			break;
		}
	}
	return false;//Fall through to other controls.
}

// Works on the control with the comboID of 'menuBarControlId'.
// To that control, it adds a menu item with the comboID of 'comboIdAs' to the menu item with the comboID of 'comboIdTo'.
void AddMenuBarItem (Window* pWindow, int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == menuBarControlId && pWindow->m_pControlArray[i].m_type == CONTROL_MENUBAR)
		{
			WidgetMenuBar_AddMenuBarItem (&pWindow->m_pControlArray[i], comboIdTo, comboIdAs, pText);
			return;
		}
	}
}
