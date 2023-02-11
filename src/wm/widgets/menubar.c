/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Widget library: Menu bar control
******************************************/
#include "../wi.h"

// If a menu bar item has no children, clicking it will trigger an EVENT_COMMAND to the parent window
// with the comboID the menu bar item was given.

// Note that having multiple menu items with the same comboID, just like having more than one control with
// the same comboID, is undefined as per specification.

// Please note that the menu bar comboIDs are not necessarily related to the control comboIDs normally.
// Instead, when you click on a menu bar item, it fires an EVENT_COMMAND with the host control's comboID
// in parm1, and the menu item's comboID in parm2.

bool WidgetMenuBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	Rectangle menu_bar_rect;
	menu_bar_rect.left   = 0;
	menu_bar_rect.top    = 0;
	menu_bar_rect.right  = pWindow->m_vbeData.m_width;
	menu_bar_rect.bottom = menu_bar_rect.top + MENU_BAR_HEIGHT + 1;
	
	MenuBarData* pData = pWindow->m_pMenuBar;
	
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			// Initialize the root.
			if (pData) break;
			pWindow->m_pMenuBar = pData = WmCreateMenuBar();
			break;
		}
		case EVENT_DESTROY:
		{
			WmDeleteMenuBar(pData);
			pWindow->m_pMenuBar = pData = NULL;
			break;
		}
		case EVENT_PAINT:
		{
			if (!pData) break;
			
			// Render the root.  If any children are opened, draw them.
			VidFillRectangle (WINDOW_BACKGD_COLOR, menu_bar_rect);
			VidDrawHLine (BUTTONDARK, menu_bar_rect.left, menu_bar_rect.right, menu_bar_rect.bottom);
			VidDrawHLine (BUTTONLITE, menu_bar_rect.left, menu_bar_rect.right, menu_bar_rect.bottom + 1);
			
			if (pData->m_root.m_childrenArray)
			{
				int current_x = 8;
				for (int i = 0; i < pData->m_root.m_childrenCount; i++)
				{
					int width, height;
					MenuBarTreeItem* pChild = &pData->m_root.m_childrenArray[i];
					const char* pText = pChild->m_text;
					VidTextOutInternal (pText, 0, 0, 0, 0, true, &width, &height);
					
					width += 15;
					
					if (pChild->m_isOpen)
					{
						Rectangle rect;
						rect.left   = menu_bar_rect.left + current_x;
						rect.right  = menu_bar_rect.left + current_x + width;
						rect.top    = menu_bar_rect.top  + 1;
						rect.bottom = menu_bar_rect.bottom - 2;
						
						VidFillRectangle (0x7F, rect);
						
						VidTextOut (pText, menu_bar_rect.left + current_x + 8, menu_bar_rect.top + 2 + (MENU_BAR_HEIGHT - GetLineHeight()) / 2, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
						//render the child menu as well:
						
						/*WmMenuRenderSubMenu (pChild, rect.left, rect.bottom);*/
					}
					else
						VidTextOut (pText, menu_bar_rect.left + current_x + 8, menu_bar_rect.top + 2 + (MENU_BAR_HEIGHT - GetLineHeight()) / 2, WINDOW_TEXT_COLOR, TRANSPARENT);
					
					current_x += width;
				}
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			if (!pData) break;
			
			Point p = {GET_X_PARM(parm1), GET_Y_PARM(parm1)};
			// Determine what item we've clicked.
			if (!pData->m_root.m_childrenArray) break;
			
			// if we already have a menu open, don't open another
			if (pWindow->m_privFlags & WPF_OPENMENU) break;
			
			int  current_x = 8;
			bool needsUpdate = false;
			for (int i = 0; i < pData->m_root.m_childrenCount; i++)
			{
				int width, height;
				MenuBarTreeItem* pChild = &pData->m_root.m_childrenArray[i];
				const char* pText = pChild->m_text;
				VidTextOutInternal (pText, 0, 0, 0, 0, true, &width, &height);
				
				width += 15;
				
				Rectangle rect;
				rect.left   = menu_bar_rect.left + current_x;
				rect.right  = menu_bar_rect.left + current_x + width;
				rect.top    = menu_bar_rect.top  + 2;
				rect.bottom = menu_bar_rect.bottom;
				
				if (RectangleContains (&rect, &p))
				{
					for (int i = 0; i < pData->m_root.m_childrenCount; i++)
					{
						MenuBarTreeItem* pChild = &pData->m_root.m_childrenArray[i];
						pChild->m_isOpen = false;
					}
					// Open this and call the paint event.
					
					if (pChild->m_childrenCount)
					{
						pChild->m_isOpen = true;
						WindowMenu menu;
						ConvertMenuBarToWindowMenu(&menu, pChild, this->m_comboID);
						SpawnMenu(pWindow, &menu, pWindow->m_rect.left + rect.left, pWindow->m_rect.top + rect.top + MENU_BAR_HEIGHT);
						pWindow->m_privFlags |= WPF_OPENMENU;
						MenuRecursivelyFreeEntries (&menu);
					}
					else
					{
						CallWindowCallback (pWindow, EVENT_COMMAND, this->m_comboID, pChild->m_comboID);
					}
					
					//WidgetMenuBar_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
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
			{
				//CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
				//RequestRepaintNew(pWindow);
				CallControlCallback(pWindow, this->m_comboID, EVENT_PAINT, 0, 0);
			}
			break;
		}
		case EVENT_MENU_CLOSE:
		{
			if (!pData) break;
			
			pWindow->m_privFlags &= ~WPF_OPENMENU;
			
			if (parm1 == this->m_comboID)
			{
				for (int i = 0; i < pData->m_root.m_childrenCount; i++)
				{
					MenuBarTreeItem* pChild = &pData->m_root.m_childrenArray[i];
					pChild->m_isOpen = false;
				}
				//RequestRepaintNew(pWindow);
				
				CallControlCallback(pWindow, this->m_comboID, EVENT_PAINT, 0, 0);
			}
			break;
		}
	}
	return false;//Fall through to other controls.
}

// Works on the control with the comboID of 'menuBarControlId'.
// To that control, it adds a menu item with the comboID of 'comboIdAs' to the menu item with the comboID of 'comboIdTo'.
void AddMenuBarItem (Window* pWindow, UNUSED int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText)
{
	if (!pWindow->m_pMenuBar)
		return;
	
	WmMenuTryAddItemTo(&pWindow->m_pMenuBar->m_root, comboIdTo, comboIdAs, pText);
}
