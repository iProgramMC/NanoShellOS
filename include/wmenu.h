/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

   Window Popup Menu module header file
******************************************/
#ifndef _WMENU_H
#define _WMENU_H

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
	int               nWidth;           // The width of the menu window (if this has children)
	int               nHeight;          // The height of the menu window
	int               nItemWidth;       // The width of this item when the parent menu is opened. If 0, this fills to the width.
	int               nItemX;           // The X position of the item.
	int               nItemY;           // The Y position of the item. If 0, this is automatically laid out.
	bool              bDisabled;
	bool              bPrivate;         // uses a private event rather than a public one.
}
WindowMenu;

int GetMenuWidth  (WindowMenu* pMenu);
int GetMenuHeight (WindowMenu* pMenu);

Window* SpawnMenu(Window* pParentWindow, WindowMenu *root, int x, int y);
void ConvertMenuBarToWindowMenu(WindowMenu* pMenuOut, MenuBarTreeItem* pTreeItem, int controlComboID);
void MenuRecursivelyFreeEntries(WindowMenu* pMenu);

#endif//_WMENU_H