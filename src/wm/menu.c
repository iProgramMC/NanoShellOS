/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Window Popup Menu module
******************************************/
#include "wi.h"

bool MenuRecursivelyCopyEntries (WindowMenu* pNewMenu, WindowMenu*pMenu)
{
	if (pNewMenu->nMenuEntries == 0)
	{
		pNewMenu->pMenuEntries = NULL;
		pNewMenu->bHasIcons    = pNewMenu->nIconID != ICON_NULL;
		return pNewMenu->bHasIcons;
	}
	pNewMenu->nLineSeparators = 0;
	
	pNewMenu->bHasIcons = false;
	pNewMenu->bDisabled       = pMenu->bDisabled;
	pNewMenu->bPrivate        = pMenu->bPrivate;
	
	pNewMenu->pMenuEntries = MmAllocateK(sizeof(WindowMenu) * pMenu->nMenuEntries);
	memcpy(pNewMenu->pMenuEntries, pMenu->pMenuEntries, sizeof(WindowMenu) * pMenu->nMenuEntries);
	
	for (int i = 0; i < pNewMenu->nMenuEntries; i++)
	{
		if (pNewMenu->pMenuEntries[i].sText[0] == 0)
		{
			pNewMenu->nLineSeparators++;
		}
		
		if (MenuRecursivelyCopyEntries(&pNewMenu->pMenuEntries[i], &pMenu->pMenuEntries[i]))
			pNewMenu->bHasIcons = true;
	}
	if (!pNewMenu->bHasIcons)
		pNewMenu->bHasIcons = pNewMenu->nIconID != ICON_NULL;
	return pNewMenu->bHasIcons;
}

void MenuCopyRoot(WindowMenu**pOutMenu, WindowMenu* pMenu, Window* pWindow)
{
	// Allocate the new menu
	WindowMenu* pNewMenu      = MmAllocateK(sizeof *pMenu);
	memset (pNewMenu, 0, sizeof *pNewMenu);
	
	// Set the entries
	pNewMenu->nWidth          = pMenu->nWidth;
	if (pNewMenu->nWidth < 60)
		pNewMenu->nWidth = 60;
	pNewMenu->nLineSeparators = 0;
	pNewMenu->nMenuEntries    = pMenu->nMenuEntries;
	pNewMenu->bHasIcons       = MenuRecursivelyCopyEntries(pNewMenu, pMenu);
	
	pNewMenu->pWindow         = pWindow;
	pNewMenu->nMenuComboID    = pMenu->nMenuComboID;
	pNewMenu->nOrigCtlComboID = pMenu->nOrigCtlComboID;
	pNewMenu->bDisabled       = pMenu->bDisabled;
	pNewMenu->bPrivate        = pMenu->bPrivate;
	pNewMenu->bOpen           = false;
	pNewMenu->pOpenWindow     = NULL;
	
	// Let our caller know where we allocated the menu
	*pOutMenu = pNewMenu;
}

int GetMenuWidth (UNUSED WindowMenu* pMenu)
{
	return pMenu->nWidth;//150 + (pMenu->bHasIcons ? 50 : 0);
}

int GetMenuHeight (WindowMenu* pMenu)
{
	if (pMenu->nHeight)
		return pMenu->nHeight;
	
	int height = (MENU_ITEM_HEIGHT + (pMenu->bHasIcons ? 4 : 0));
	int y = 0;
	
	for (int i = 0; i < pMenu->nMenuEntries; i++)
	{
		if (pMenu->pMenuEntries[i].nItemY)
			y = pMenu->pMenuEntries[i].nItemY;
		
		if (pMenu->pMenuEntries[i].sText[0] == 0)
			y += MENU_SEPA_HEIGHT;
		else
			y += height;
		
		if (pMenu->nHeight < y)
			pMenu->nHeight = y;
	}
	
	return pMenu->nHeight;
}

#define MENU_ITEM_COMBOID (1000000)

void MenuRecursivelyFreeEntries(WindowMenu* pMenu)
{
	if (pMenu->pMenuEntries)
	{
		for (int i = 0; i < pMenu->nMenuEntries; i++)
		{
			MenuRecursivelyFreeEntries(&pMenu->pMenuEntries[i]);
		}
		MmFree(pMenu->pMenuEntries);
		pMenu->pMenuEntries = NULL;
	}
}

void DestroyOpenChildWindowIfAvailable(Window* pMenuWnd)
{
	if (pMenuWnd->m_data)
	{
		WindowMenu* pData = (WindowMenu*)pMenuWnd->m_data;
		if (pData->pOpenWindow)
		{
			DestroyOpenChildWindowIfAvailable(pData->pOpenWindow);
			DestroyWindow(pData->pOpenWindow);
			pData->bOpen       = false;
			pData->pOpenWindow = NULL;
		}
	}
}

void SelectWindow(Window* pWindow);
void CALLBACK MenuProc(Window* pWindow, int eventType, long parm1, long parm2)
{
	switch (eventType)
	{
		case EVENT_CREATE:
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			
			//Get the data
			if (pWindow->m_data)
			{
				WindowMenu* pData = (WindowMenu*)pWindow->m_data;
				pData->bOpen       = false;
				pData->pOpenWindow = NULL;
				
				//Add some controls
				Rectangle r;
				int height = (MENU_ITEM_HEIGHT + (pData->bHasIcons ? 4 : 0)), y = 0;
				for (int i = 0; i < pData->nMenuEntries; i++)
				{
					if (pData->pMenuEntries[i].sText[0] == 0)
					{
						RECT(r, 0, y, GetMenuWidth(pData), MENU_SEPA_HEIGHT);
						AddControl (pWindow, CONTROL_SIMPLE_HLINE, r, NULL, MENU_ITEM_COMBOID+i, 0, 0);
						y += MENU_SEPA_HEIGHT;
					}
					else
					{
						int width = pData->pMenuEntries[i].nItemWidth;
						if (width == 0)
							width = GetMenuWidth(pData);
						int xPos  = pData->pMenuEntries[i].nItemX;
						if (xPos  > GetMenuWidth(pData) - width)
							xPos  = GetMenuWidth(pData) - width;

						if (pData->pMenuEntries[i].nItemY)
							y = pData->pMenuEntries[i].nItemY;

						RECT(r, xPos, y, width, height - 1);
						
						AddControl (
							pWindow,
							CONTROL_BUTTON_LIST,
							r,
							pData->pMenuEntries[i].sText,
							MENU_ITEM_COMBOID+i,
							pData->pMenuEntries[i].nIconID,
							(pData->bHasIcons ? BTNLIST_HASICON : 0) | (pData->pMenuEntries[i].nMenuEntries ? BTNLIST_HASSUBS : 0)
						);
						
						if (pData->pMenuEntries[i].bDisabled)
							SetDisabledControl(pWindow, MENU_ITEM_COMBOID + i, true);
						
						y += height;
					}
				}
			}
			
			break;
		case EVENT_PAINT:
		{
			if (pWindow->m_data)
			{
				WindowMenu* pData = (WindowMenu*)pWindow->m_data;
				Rectangle re = pWindow->m_rect;
				re.right  -= re.left;
				re.bottom -= re.top;
				re.left = re.top = 0;
				
				int offs = 16;
				if (pData->bHasIcons) offs += 8;
				
				Rectangle re2 = re;
				re2.right = re2.left + offs;
				re.left = re2.right;
				
				VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, re);
				VidFillRectangle(WINDOW_BACKGD_COLOR,     re2);
			}
			break;
		}
		case EVENT_KILLFOCUS:
		{
			if (!pWindow->m_data) break;
			
			WindowMenu* pData = (WindowMenu*)pWindow->m_data;
			
			if (!(pData->pWindow->m_flags & WF_MENUITEM))//i.e. not a popup menu
			{
				DestroyWindow(pWindow);//kill self
			}
			break;
		}
		case EVENT_COMMAND:
		case EVENT_COMMAND_PRIVATE:
		{
			Control* p = GetControlByComboID(pWindow, parm1);
			
			if (!pWindow->m_data) break;
			
			WindowMenu* pData = (WindowMenu*)pWindow->m_data;
			
			int destEvent = EVENT_COMMAND;
			if (pData->bPrivate)
				destEvent = EVENT_COMMAND_PRIVATE;
			
			int index = parm1 - MENU_ITEM_COMBOID;
				
			if (index >= 0 && index < pData->nMenuEntries)
			{
				DestroyOpenChildWindowIfAvailable(pWindow);
				if (pData->pMenuEntries[index].pMenuEntries)
				{
					//Open submenu
					pData->bOpen = true;
					
					int y_pos = 0;
					if (p)
					{
						y_pos = p->m_rect.top;
					}
					
					int newXPos = pWindow->m_rect.left + GetMenuWidth (pData) + BORDER_SIZE_NORESIZE * 2,
					    newYPos = pWindow->m_rect.top  + y_pos;
					
					if (newXPos < 0)
						newXPos = 0;
					if (newYPos < 0)
						newYPos = 0;
					
					int menu_width  = GetMenuWidth (&pData->pMenuEntries[index]);
					int menu_height = GetMenuHeight(&pData->pMenuEntries[index]);
					
					if (newXPos + menu_width >= GetScreenWidth())
						newXPos = pWindow->m_rect.left - menu_width;
					if (newYPos + menu_height >= GetScreenHeight())
						newYPos = pWindow->m_rect.top - menu_height;
					
					pData->pOpenWindow = SpawnMenu (pWindow, &pData->pMenuEntries[index], newXPos, newYPos);
				}
				else
				{
					//Send a command to parent window
					WindowAddEventToMasterQueue(pData->pWindow, destEvent, pData->pMenuEntries[index].nOrigCtlComboID, pData->pMenuEntries[index].nMenuComboID);
					DestroyWindow(pWindow);
				}
			}
			else
			{
				//Send a command to parent window
				WindowAddEventToMasterQueue(pData->pWindow, destEvent, parm1, parm2);
				DestroyWindow(pWindow);
			}
			break;
		}
		case EVENT_DESTROY:
		{
			Window* pParentWindow = NULL;
			if (pWindow->m_data)
			{
				WindowMenu* pData = (WindowMenu*)pWindow->m_data;
				
				pParentWindow = pData->pWindow;
				WindowRegisterEvent(pData->pWindow, EVENT_MENU_CLOSE, pData->nOrigCtlComboID, pData->nMenuComboID);
				DestroyOpenChildWindowIfAvailable(pWindow);
				
				if (pData->pOpenWindow)
				{
					DestroyWindow(pData->pOpenWindow);
					while (pData->pOpenWindow->m_used)
						KeTaskDone();
					pData->pOpenWindow = NULL;
				}
				MenuRecursivelyFreeEntries(pData);
				
				MmFree(pWindow->m_data);
				pWindow->m_data = NULL;
			}
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			
			if (pParentWindow)
			{
				SLogMsg("Selecting parent window %p. We're %p", pParentWindow, pWindow);
				SelectWindow(pParentWindow);
			}
			if (pWindow->m_data)
			{
				WindowMenu* pData = (WindowMenu*)pWindow->m_data;
				pData->pWindow = NULL;
			}
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			break;
	}
}

#define MAX(a,b) ((a) < (b) ? (b) : (a))

void ConvertMenuBarToWindowMenu(WindowMenu* pMenuOut, MenuBarTreeItem* pTreeItem, int controlComboID)
{
	memset(pMenuOut, 0, sizeof *pMenuOut);
	//Fill in the root
	pMenuOut->pWindow      = NULL;
	pMenuOut->nMenuEntries = pTreeItem->m_childrenCount;
	pMenuOut->bHasIcons    = false;
	pMenuOut->nIconID      = ICON_NULL;
	
	if (pTreeItem->m_childrenCount)
		pMenuOut->pMenuEntries = MmAllocateK(sizeof (WindowMenu) * pTreeItem->m_childrenCount);
	else
		pMenuOut->pMenuEntries = NULL;
	
	strcpy(pMenuOut->sText,     pTreeItem->m_text);
	pMenuOut->nMenuComboID    = pTreeItem->m_comboID;
	pMenuOut->nOrigCtlComboID = controlComboID;
	
	//Measure the text inside
	int width, height;
	VidTextOutInternal (pMenuOut->sText, 0, 0, 0, 0, true, &width, &height);
	pMenuOut->nWidth       = width + 60;
	
	//Fill in the children
	for (int i = 0; i < pMenuOut->nMenuEntries; i++)
	{
		ConvertMenuBarToWindowMenu(&pMenuOut->pMenuEntries[i], &pTreeItem->m_childrenArray[i], controlComboID);
		
		pMenuOut->nWidth = MAX(pMenuOut->nWidth, pMenuOut->pMenuEntries[i].nWidth);
	}
}

Window* SpawnMenu(Window* pParentWindow, WindowMenu *root, int newXPos, int newYPos)
{
	//clone the root immediately
	WindowMenu* pRoot;
	MenuCopyRoot(&pRoot, root, pParentWindow);
	
	if (newXPos < 0)
		newXPos = 0;
	if (newYPos < 0)
		newYPos = 0;
	
	if (newXPos >= GetScreenWidth())
		newXPos = GetScreenWidth() - GetMenuWidth(pRoot);
	if (newYPos >= GetScreenHeight())
		newYPos = GetScreenHeight() - GetMenuHeight(pRoot);
	
	//Create a new window
	Window *pMenuWnd = CreateWindow(root->sText, newXPos, newYPos, GetMenuWidth(pRoot), GetMenuHeight(pRoot), MenuProc, WF_MENUITEM | WF_NOCLOSE | WF_NOTITLE |	WF_NOMINIMZ | WF_SYSPOPUP | WF_FOREGRND | WI_NEVERSEL);
	
	KeVerifyInterruptsEnabled;
	cli;
	
	if (!pMenuWnd)
	{
		sti;
		MenuRecursivelyFreeEntries(pRoot);
		MmFree(pRoot);
		pRoot = NULL;
		return NULL;
	}
	
	pMenuWnd->m_bWindowManagerUpdated = true;
	pMenuWnd->m_data = pRoot;
	sti;
	
	return pMenuWnd;
}

void OnRightClickShowMenu(Window* pWindow, int parm1)
{
	Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
	p.x += pWindow->m_fullRect.left;
	p.y += pWindow->m_fullRect.top;
	
	int MenuWidth = 100;
	
	// This kinda sucks.. but it should be okay
	WindowMenu *restoreEnt, *minimizeEnt, *maximizeEnt, *closeEnt, *spacerEnt, *smSnapEnt, *smTile[8];
	WindowMenu rootEnt;
	WindowMenu table[6];
	WindowMenu smSnapTable[8];
	restoreEnt  = &table[0];
	minimizeEnt = &table[1];
	maximizeEnt = &table[2];
	smSnapEnt   = &table[3];
	spacerEnt   = &table[4];
	closeEnt    = &table[5];
	
	for (int i = 0; i < 8; i++) smTile[i] = &smSnapTable[i];
	
	strcpy(rootEnt.sText, "Menu");
	rootEnt.pMenuEntries = table;
	rootEnt.nMenuEntries = (int)ARRAY_COUNT(table);
	rootEnt.bPrivate     = true;
	rootEnt.nLineSeparators = 1; // there's one
	rootEnt.nWidth       = MenuWidth;
	
	// fill in one of the entries, then memcpy it to the others
	memset(restoreEnt, 0, sizeof *restoreEnt);
	restoreEnt->pWindow = pWindow;
	restoreEnt->pMenuEntries = NULL;
	restoreEnt->nMenuEntries = 0;
	restoreEnt->nMenuComboID = 1;
	restoreEnt->nOrigCtlComboID = WINDOW_ACTION_MENU_ORIG_CID;
	restoreEnt->bOpen     = false;
	restoreEnt->bHasIcons = true;
	restoreEnt->nLineSeparators = 0;
	restoreEnt->pOpenWindow = NULL;
	restoreEnt->nIconID     = ICON_RESTORE;
	restoreEnt->nWidth      = MenuWidth;
	restoreEnt->bDisabled   = false;
	restoreEnt->bPrivate    = true;
	
	memcpy(minimizeEnt, restoreEnt, sizeof *restoreEnt);
	memcpy(maximizeEnt, restoreEnt, sizeof *restoreEnt);
	memcpy(closeEnt,    restoreEnt, sizeof *restoreEnt);
	memcpy(spacerEnt,   restoreEnt, sizeof *restoreEnt);
	memcpy(smSnapEnt,   restoreEnt, sizeof *restoreEnt);
	
	for (int i = 0; i < 8; i++)
	{
		memcpy(smTile[i], restoreEnt, sizeof *restoreEnt);
		strcpy(smTile[i]->sText, " "); // just a space. Having no text implies being a separator...
		smTile[i]->nMenuComboID = 6 + i;
		smTile[i]->bPrivate  = false;
		
		int x = i % 4, y = i / 4;
		smTile[i]->nItemX          = x * (MenuWidth / 4);
		smTile[i]->nItemY          = (MENU_ITEM_HEIGHT + 4) * y + 1;
		smTile[i]->nItemWidth      = MenuWidth / 4 - 1;
		smTile[i]->nIconID         = ICON_SNAP_U + i;
		smTile[i]->nMenuComboID    = CID_SMARTSNAP_0 + i;
		smTile[i]->nOrigCtlComboID = WINDOW_ACTION_MENU_ORIG_CID;
	}
	
	// copy the text
	strcpy(restoreEnt ->sText, "Restore");
	strcpy(minimizeEnt->sText, "Minimize");
	strcpy(maximizeEnt->sText, "Maximize");
	strcpy(closeEnt   ->sText, "Close");
	strcpy(smSnapEnt  ->sText, "SmartSnap");
	
	// Set up the smart snap menu. Disable it if we cannot resize.
	if (pWindow->m_flags & WF_ALWRESIZ)
	{
		smSnapEnt->bDisabled = false;
		smSnapEnt->bPrivate  = false;
		smSnapEnt->pMenuEntries = &smSnapTable[0];
		smSnapEnt->nMenuEntries = (int)ARRAY_COUNT(smSnapTable);
	}
	else
	{
		smSnapEnt->bDisabled = true;
	}
	
	restoreEnt ->nMenuComboID = CID_RESTORE;
	minimizeEnt->nMenuComboID = CID_MINIMIZE;
	maximizeEnt->nMenuComboID = CID_MAXIMIZE;
	spacerEnt  ->nMenuComboID = CID_SPACER;
	closeEnt   ->nMenuComboID = CID_CLOSE;
	smSnapEnt  ->nMenuComboID = CID_SMARTSNAP_PARENT;
	
	minimizeEnt->nIconID = ICON_MINIMIZE;
	maximizeEnt->nIconID = ICON_MAXIMIZE;
	closeEnt   ->nIconID = ICON_CLOSE;
	smSnapEnt  ->nIconID = ICON_TASKBAR_DOCK;
	
	// Disable the buttons on a case-by-case basis
	restoreEnt->bDisabled  = !(pWindow->m_flags & WF_MAXIMIZE) || !!(pWindow->m_flags & WF_NOMAXIMZ);
	minimizeEnt->bDisabled = (pWindow->m_flags & WF_MINIMIZE) || !!(pWindow->m_flags & WF_NOMINIMZ);
	maximizeEnt->bDisabled = (pWindow->m_flags & WF_MAXIMIZE) || !!(pWindow->m_flags & WF_NOMAXIMZ);
	closeEnt->bDisabled    = !!(pWindow->m_flags & WF_NOCLOSE);
	
	// then, I guess just spawn the menu!
	SpawnMenu(pWindow, &rootEnt, p.x, p.y);
}
