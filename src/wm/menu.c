/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

         Window Popup Menu module
******************************************/
#include <window.h>
#include <wbuiltin.h>
#include <wmenu.h>

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
	
	pNewMenu->pMenuEntries = MmAllocateK(sizeof(WindowMenu) * pMenu->nMenuEntries);
	memcpy(pNewMenu->pMenuEntries, pMenu->pMenuEntries, sizeof(WindowMenu) * pMenu->nMenuEntries);
	
	for (int i = 0; i < pNewMenu->nMenuEntries; i++)
	{
		if (pNewMenu->pMenuEntries[i].sText[0] == 0)
		{
			pNewMenu->nLineSeparators++;
		}
		pNewMenu->bHasIcons = pNewMenu->bHasIcons || MenuRecursivelyCopyEntries(&pNewMenu->pMenuEntries[i], &pMenu->pMenuEntries[i]);
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
	int haute = (MENU_ITEM_HEIGHT + (pMenu->bHasIcons ? 4 : 0));
	return 3 + 
	       pMenu->nMenuEntries * haute - 
		   pMenu->nLineSeparators * (haute - MENU_SEPA_HEIGHT);
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
				int height = (MENU_ITEM_HEIGHT + (pData->bHasIcons ? 4 : 0)), y = 2;
				for (int i = 0; i < pData->nMenuEntries; i++)
				{
					if (pData->pMenuEntries[i].sText[0] == 0)
					{
						RECT(r, 2, y, GetMenuWidth(pData)-5, MENU_SEPA_HEIGHT);
						AddControl (pWindow, CONTROL_SIMPLE_HLINE, r, NULL, MENU_ITEM_COMBOID+i, 0, 0);
						y += MENU_SEPA_HEIGHT;
					}
					else
					{
						RECT(r, 2, y, GetMenuWidth(pData)-5, height - 2);
						char buffer[110];
						sprintf(buffer, "%s%s", pData->pMenuEntries[i].sText, pData->pMenuEntries[i].nMenuEntries ? "  >>" : "");
						
						AddControl (pWindow, CONTROL_BUTTON_LIST, r, buffer, MENU_ITEM_COMBOID+i, pData->pMenuEntries[i].nIconID, pData->pMenuEntries[i].nMenuEntries);
						
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
				
				re.left += 2;
				re.top  += 2;
				re.right  -= 3;
				re.bottom -= 3;
				
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
			if (!pData->bOpen)
			{
				if (!(pData->pWindow->m_flags & WF_MENUITEM))//i.e. not a popup menu
				{
					DestroyWindow(pWindow);//kill self
				}
			}
			break;
		}
		case EVENT_COMMAND:
		{
			Control* p = GetControlByComboID(pWindow, parm1);
			
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
					
					int y_pos = 0;
					if (p)
					{
						y_pos = p->m_rect.top - 2;
					}
					
					int newXPos = pWindow->m_rect.left + GetMenuWidth (pData),
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
				SelectWindow(pParentWindow);
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
	Window *pMenuWnd = CreateWindow(root->sText, newXPos, newYPos, GetMenuWidth(pRoot), GetMenuHeight(pRoot), MenuProc, WF_MENUITEM | WF_NOCLOSE | WF_NOTITLE | WF_NOMINIMZ | WF_SYSPOPUP);
	
	KeVerifyInterruptsEnabled;
	cli;
	pMenuWnd->m_bWindowManagerUpdated = true;
	pMenuWnd->m_data = pRoot;
	sti;
	return pMenuWnd;
}
