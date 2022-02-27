#ifndef _WMENU_H
#define _WMENU_H

#define MENU_ITEM_HEIGHT 16

typedef struct MenuEntry
{
	Window*           pWindow;
	struct MenuEntry* pMenuEntries;
	int               nMenuEntries;
	int               nMenuComboID;
	int               nOrigCtlComboID;
	char              sText[100];
	bool              bOpen;
	Window*           pOpenWindow;
}
WindowMenu;

Window* SpawnMenu(Window* pParentWindow, WindowMenu *root, int x, int y);
void ConvertMenuBarToWindowMenu(WindowMenu* pMenuOut, MenuBarTreeItem* pTreeItem, int controlComboID);
void MenuRecursivelyFreeEntries(WindowMenu* pMenu);

#endif//_WMENU_H