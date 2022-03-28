/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

   Window Popup Menu module header file
******************************************/
#ifndef _WMENU_H
#define _WMENU_H

#define MENU_ITEM_HEIGHT 16
#define MENU_SEPA_HEIGHT 5

typedef struct MenuEntry
{
	Window*           pWindow;
	struct MenuEntry* pMenuEntries;
	int               nMenuEntries;
	int               nMenuComboID;
	int               nOrigCtlComboID;
	char              sText[100];
	bool              bOpen;
	bool              bHasIcons;
	int               nLineSeparators;
	Window*           pOpenWindow;
	int               nIconID;
}
WindowMenu;

int GetMenuWidth  (WindowMenu* pMenu);
int GetMenuHeight (WindowMenu* pMenu);

Window* SpawnMenu(Window* pParentWindow, WindowMenu *root, int x, int y);
void ConvertMenuBarToWindowMenu(WindowMenu* pMenuOut, MenuBarTreeItem* pTreeItem, int controlComboID);
void MenuRecursivelyFreeEntries(WindowMenu* pMenu);

#endif//_WMENU_H