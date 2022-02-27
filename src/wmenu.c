
#include <window.h>
#include <wbuiltin.h>
#include <wmenu.h>

void MenuRecursivelyCopyEntries (WindowMenu* pNewMenu, WindowMenu*pMenu)
{
	if (pNewMenu->nMenuEntries == 0)
	{
		pNewMenu->pMenuEntries = NULL;
		return;
	}
	
	pNewMenu->pMenuEntries = MmAllocate(sizeof(WindowMenu) * pMenu->nMenuEntries);
	memcpy(pNewMenu->pMenuEntries, pMenu->pMenuEntries, sizeof(WindowMenu) * pMenu->nMenuEntries);
	
	for (int i = 0; i < pNewMenu->nMenuEntries; i++)
	{
		MenuRecursivelyCopyEntries(&pNewMenu->pMenuEntries[i], &pMenu->pMenuEntries[i]);
	}
}

void MenuCopyRoot(WindowMenu**pOutMenu, WindowMenu* pMenu, Window* pWindow)
{
	// Allocate the new menu
	WindowMenu* pNewMenu      = MmAllocate(sizeof *pMenu);
	memset (pNewMenu, 0, sizeof *pNewMenu);
	
	// Set the entries
	pNewMenu->nMenuEntries    = pMenu->nMenuEntries;
	MenuRecursivelyCopyEntries(pNewMenu, pMenu);
	
	pNewMenu->pWindow         = pWindow;
	pNewMenu->nMenuComboID    = pMenu->nMenuComboID;
	pNewMenu->nOrigCtlComboID = pMenu->nOrigCtlComboID;
	
	pNewMenu->bOpen           = false;
	pNewMenu->pOpenWindow     = NULL;
	
	// Let our caller know where we allocated the menu
	*pOutMenu = pNewMenu;
}

int GetMenuWidth (UNUSED WindowMenu* pMenu)
{
	return 200;
}
int GetMenuHeight (UNUSED WindowMenu* pMenu)
{
	return 4 + pMenu->nMenuEntries * MENU_ITEM_HEIGHT;
}

#define MENU_ITEM_COMBOID (-1000000)

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

void SelectThisWindowAndUnselectOthers(Window* pWindow);
void CALLBACK MenuProc(Window* pWindow, int eventType, int parm1, int parm2)
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
				for (int i = 0; i < pData->nMenuEntries; i++)
				{
					RECT(r, 2, 2 + i * MENU_ITEM_HEIGHT, GetMenuWidth(pData)-5, MENU_ITEM_HEIGHT-1);
					
					char buffer[110];
					sprintf(buffer, "%s%s", pData->pMenuEntries[i].sText, pData->pMenuEntries[i].nMenuEntries ? "  >>" : "");
					
					AddControl (pWindow, CONTROL_BUTTON_LIST, r, buffer, MENU_ITEM_COMBOID+i, 0, 0);
				}
			}
			
			break;
		case EVENT_KILLFOCUS:
		{
			if (!pWindow->m_data) break;
			WindowMenu* pData = (WindowMenu*)pWindow->m_data;
			if (!pData->bOpen)
			{
				if (!pData->pWindow->m_bWindowManagerUpdated)//i.e. not a popup menu
				{
					DestroyWindow(pWindow);//kill self
				}
			}
			break;
		}
		case EVENT_COMMAND:
		{
			if (!pWindow->m_data) break;
			
			WindowMenu* pData = (WindowMenu*)pWindow->m_data;
			
			int index = parm1 - MENU_ITEM_COMBOID;
				
			if (index >= 0 && index < pData->nMenuEntries)
			{
				DestroyOpenChildWindowIfAvailable(pWindow);
				if (pData->pMenuEntries[index].pMenuEntries)
				{
					//Open submenu
					pData->bOpen = true;
					
					int newXPos = pWindow->m_rect.left + GetMenuWidth (pData),
					    newYPos = pWindow->m_rect.top + MENU_ITEM_HEIGHT * index;
					
					if (newXPos < 0)
						newXPos = 0;
					if (newYPos < 0)
						newYPos = 0;
					
					if (newXPos + GetMenuWidth (&pData->pMenuEntries[index]) >= GetScreenWidth())
						newXPos = pWindow->m_rect.left - GetMenuWidth (&pData->pMenuEntries[index]);
					if (newYPos + GetMenuHeight(&pData->pMenuEntries[index]) >= GetScreenHeight())
						newYPos = pWindow->m_rect.left - GetMenuHeight(&pData->pMenuEntries[index]);
					
					pData->pOpenWindow = SpawnMenu (pWindow, &pData->pMenuEntries[index], newXPos, newYPos);
				}
				else
				{
					//Send a command to parent window
					WindowAddEventToMasterQueue(pData->pWindow, EVENT_COMMAND, pData->pMenuEntries[index].nOrigCtlComboID, pData->pMenuEntries[index].nMenuComboID);
					DestroyWindow(pWindow);
				}
			}
			else
			{
				//Send a command to parent window
				WindowAddEventToMasterQueue(pData->pWindow, EVENT_COMMAND, parm1, parm2);
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
				SelectThisWindowAndUnselectOthers(pParentWindow);
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

void ConvertMenuBarToWindowMenu(WindowMenu* pMenuOut, MenuBarTreeItem* pTreeItem, int controlComboID)
{
	//Fill in the root
	pMenuOut->pWindow = NULL;
	pMenuOut->nMenuEntries = pTreeItem->m_childrenCount;
	
	if (pTreeItem->m_childrenCount)
		pMenuOut->pMenuEntries = MmAllocate(sizeof (WindowMenu) * pTreeItem->m_childrenCount);
	else
		pMenuOut->pMenuEntries = NULL;
	
	strcpy(pMenuOut->sText,     pTreeItem->m_text);
	pMenuOut->nMenuComboID    = pTreeItem->m_comboID;
	pMenuOut->nOrigCtlComboID = controlComboID;
	
	//Fill in the children
	for (int i = 0; i < pMenuOut->nMenuEntries; i++)
	{
		ConvertMenuBarToWindowMenu(&pMenuOut->pMenuEntries[i], &pTreeItem->m_childrenArray[i], controlComboID);
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
	Window *pMenuWnd = CreateWindow(root->sText, newXPos, newYPos, GetMenuWidth(pRoot), GetMenuHeight(pRoot), MenuProc, WF_NOCLOSE | WF_NOTITLE | WF_NOMINIMZ);
	cli;
	pMenuWnd->m_bWindowManagerUpdated = true;
	pMenuWnd->m_data = pRoot;
	sti;
	return pMenuWnd;
}
