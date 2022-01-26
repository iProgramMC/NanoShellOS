#include <nanoshell.h>
#include <window.h>

#define VERSION_BUTTON_OK_COMBO 0x1000
#define LISTTEST_WIDTH  500
#define LISTTEST_HEIGHT 500

enum {
	NO,
	MAIN_LISTVIEW,
	MAIN_MENUBAR
};
enum {
	MENU_ROOT = 0,
	MENU_ADDRED,
	MENU_ADDGREEN,
	MENU_ADDBLUE,
	MENU_REMOVEONE,
	MENU_CLEAR,
};

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             20
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ LISTTEST_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ LISTTEST_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			AddControl (pWindow, CONTROL_LISTVIEW, r, NULL, MAIN_LISTVIEW, 0, 0);
			
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, MAIN_MENUBAR, 0, 0);
			
			AddMenuBarItem(pWindow, MAIN_MENUBAR, MENU_ROOT, MENU_ADDRED,    "AddRed");
			AddMenuBarItem(pWindow, MAIN_MENUBAR, MENU_ROOT, MENU_ADDGREEN,  "AddGreen");
			AddMenuBarItem(pWindow, MAIN_MENUBAR, MENU_ROOT, MENU_ADDBLUE,   "AddBlue");
			AddMenuBarItem(pWindow, MAIN_MENUBAR, MENU_ROOT, MENU_REMOVEONE, "RemoveOne");
			AddMenuBarItem(pWindow, MAIN_MENUBAR, MENU_ROOT, MENU_CLEAR,     "RemoveAll");
			
			break;
		}
		case EVENT_COMMAND:
			if (parm1 == MAIN_MENUBAR)
			{
				switch (parm2)
				{
					case MENU_ADDRED: //Add red
						AddElementToList(pWindow, MAIN_LISTVIEW, "Red!", ICON_STOP);
						break;
					case MENU_ADDGREEN: //Add red
						AddElementToList(pWindow, MAIN_LISTVIEW, "Green!", ICON_GO);
						break;
					case MENU_ADDBLUE: //Add red
						AddElementToList(pWindow, MAIN_LISTVIEW, "Blue!", ICON_GLOBE);
						break;
					case MENU_REMOVEONE: //Add red
						if (GetElementStringFromList(pWindow, MAIN_LISTVIEW, 0) != NULL)
							RemoveElementFromList(pWindow, MAIN_LISTVIEW, 0);
						break;
					case MENU_CLEAR: //Add red
						ResetList(pWindow, MAIN_LISTVIEW);
						break;
				}
			}
			else if (parm1 == MAIN_LISTVIEW)
			{
				const char* pText = GetElementStringFromList(pWindow, parm1, parm2);
				MessageBox(pWindow, pText, "List View", MB_OK | ICON_GO << 16);
			}
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int main ()
{
	Window* pWindow = CreateWindow ("List testing application", 150, 150, LISTTEST_WIDTH, LISTTEST_HEIGHT, WndProc, 0);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}


