/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Launcher Application module
******************************************/

#include "../wm/wi.h"
#include <resource.h>

#define LAUNCHER_CONFIG_PATH "/launcher_config.txt"

// Shell resource handlers
#if 1

RESOURCE_STATUS LaunchVersionApplet(const char *pTextIn, int iconID)
{
	UNUSED int errorCode = 0;
	
	char* pText;
	if (pTextIn)
	{
		pText = MmAllocate (strlen (pTextIn) + 5);
		strcpy (pText + 4, pTextIn);
		*((int*)pText) = iconID;
	}
	else
	{
		pText = MmAllocate (5);
		pText[4] = 0;
		*((int*)pText) = iconID;
	}
	
	Task* pTask = KeStartTask(VersionProgramTask, (int) pText, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchSystem()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(SystemMonitorEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchIconTest()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchListTest()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(ListTestTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchTextShell()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(TerminalHostTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchPaint()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(PrgPaintTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}

RESOURCE_STATUS LaunchControlPanel()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(ControlEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchNotepad()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(BigTextEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchCabinet()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(CabinetEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchMagnifier()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(PrgMagnifyTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}

#endif

// Misc. launcher stuff
#if 1

void LaunchVersion()
{
	LaunchVersionApplet (NULL, ICON_NANOSHELL);
}

void ShellAbout (const char *pText, int iconID)
{
	LaunchVersionApplet (pText, iconID);
}

#define STREQ(str1,str2) (!strcmp(str1,str2))
void LaunchLauncher();
RESOURCE_STATUS HelpOpenResource(const char* pResourceID);
RESOURCE_STATUS LaunchResourceLauncher(const char* pResourceID)
{
	/**/ if (STREQ(pResourceID, "about"))    return LaunchVersionApplet(NULL, ICON_NANOSHELL);
	else if (STREQ(pResourceID, "sysmon"))   return LaunchSystem();
	else if (STREQ(pResourceID, "icontest")) return LaunchIconTest();
	else if (STREQ(pResourceID, "listtest")) return LaunchListTest();
	else if (STREQ(pResourceID, "cmdshell")) return LaunchTextShell();
	else if (STREQ(pResourceID, "scribble")) return LaunchPaint();
	else if (STREQ(pResourceID, "cpanel"))   return LaunchControlPanel();
	else if (STREQ(pResourceID, "notepad"))  return LaunchNotepad();
	else if (STREQ(pResourceID, "cabinet"))  return LaunchCabinet();
	else if (STREQ(pResourceID, "magnify"))  return LaunchMagnifier();
	else if (STREQ(pResourceID, "launcher")) { LaunchLauncher(); return RESOURCE_LAUNCH_SUCCESS; }
	else if (STREQ(pResourceID, "help"))     return HelpOpenResource("/Fat0/Help.md");//LaunchHelp();
	else return RESOURCE_LAUNCH_NOT_FOUND;
}

void ConfirmShutdown(Window* pWindow);

enum {
	
	LAUNCHER_LISTVIEW = 0x10,
	LAUNCHER_MENUBAR  = 0xFE,
	LAUNCHER_LABEL1   = 0xFF,
};

/*const char* g_LauncherResources[] = {
	"shell:sysmon",
	"shell:cabinet",
	"shell:icontest",
	"shell:notepad",
	"shell:scribble",
	"shell:cmdshell",
	"shell:cpanel",
	"shell:shutdown",
};*/

int g_nLauncherItems = 0;
LauncherItem* g_pLauncherItems = NULL;
int           g_nLauncherDVer  = 0;

#endif

// Configuration loader:

#define Macro_ReadStringTillSeparator(lineData,strToRead,separator,size,index)\
	do {\
		index = 0;\
		while (*lineData != separator && *lineData != 0 && index < size)\
		{\
			strToRead[index++] = *lineData;\
			lineData++;\
		}\
		strToRead[idx] = 0;\
	} while (0)

void HomeMenu$LoadConfigLine(UNUSED Window* pWindow, char* lineData, int *itemIndex)
{
	if (lineData[0] == '/' && lineData[1] == '/') return;//comment
	char instruction[25]; int idx = 0;
	while (*lineData != '|' && *lineData != 0 && idx < 24)
	{
		instruction[idx++] = *lineData;
		lineData++;
	}
	instruction[idx] = 0;
	
	if (strcmp (instruction, "add_item") == 0)
	{
		char iconNumStr[11], text[31], resID[61], inQuickLaunch[2];
		inQuickLaunch[0] = '0', inQuickLaunch[1] = 0;
		
		lineData++;
		Macro_ReadStringTillSeparator(lineData, iconNumStr, '|', 10, idx);
		
		if (g_nLauncherDVer >= 2)
		{
			lineData++;
			inQuickLaunch[0] = 0;
			Macro_ReadStringTillSeparator(lineData, inQuickLaunch, '|', 1, idx);
		}
		
		lineData++;
		Macro_ReadStringTillSeparator(lineData, text, '|', 30, idx);
		lineData++;
		Macro_ReadStringTillSeparator(lineData, resID, '|', 60, idx);
		
		//LogMsg("Add_item: %s %s %s", iconNumStr, text, resID);
		strcpy(g_pLauncherItems[*itemIndex].m_text,       text);
		strcpy(g_pLauncherItems[*itemIndex].m_resourceID, resID);
		g_pLauncherItems[*itemIndex].m_addToQuickLaunch = (inQuickLaunch[0] == '1');
		g_pLauncherItems[*itemIndex].m_icon             = atoi(iconNumStr);
		
		(*itemIndex) ++;
	}
	else if (strcmp (instruction, "version") == 0)
	{
		char versionNumber[11];
		lineData++;
		Macro_ReadStringTillSeparator(lineData, versionNumber, '|', 10, idx);
		
		g_nLauncherDVer = atoi (versionNumber);
	}
}

int HomeMenu$GetNumLines(const char* pData)
{
	int idx = 0, numLines = 0;
	while (1)
	{
		idx = 0;
		//avoid going past 500, no stack overflows please
		while (*pData != '\r' && *pData != '\n' && *pData != '\0' && idx < 499)
		{
			pData++;
		}
		numLines++;
		if (*pData == 0) return numLines;
		while (*pData == '\r' || *pData == '\n') pData++;//skip newlines
	}
	return numLines;
}
void HomeMenu$LoadConfigFromString(Window* pWindow, const char* pData)
{
	char lineData[500];
	int idx = 0;
	while (1)
	{
		idx = 0;
		//avoid going past 500, no stack overflows please
		while (*pData != '\r' && *pData != '\n' && *pData != '\0' && idx < 499)
		{
			lineData[idx++] = *pData;
			pData++;
		}
		lineData[idx++] = 0;
		HomeMenu$LoadConfigLine(pWindow, lineData, &g_nLauncherItems);
		if (*pData == 0) return;
		while (*pData == '\r' || *pData == '\n') pData++;//skip newlines
	}
}

char g_launcher_path[PATH_MAX+2];

const char* GetLauncherConfigPath()
{
	if (g_launcher_path[0] != 0) return g_launcher_path;
	
	ConfigEntry *pEntry = CfgGetEntry ("Launcher::ConfigPath");
	if (!pEntry) return LAUNCHER_CONFIG_PATH;
	
	strcpy (g_launcher_path, pEntry->value);
	
	return g_launcher_path;
}

void HomeMenu$LoadConfig(Window* pWindow)
{
	if (g_pLauncherItems) return;//only load once
	
	//WORK: You can change the launcher config path here:
	int fd = FiOpen(GetLauncherConfigPath(), O_RDONLY);
	if (fd < 0)
	{
		MessageBox(pWindow, "Could not load launcher configuration files.  The Home menu will now close, and open a text shell.", "Home Menu", ICON_ERROR << 16 | MB_OK);
		LaunchTextShell();
		return;
	}
	int size = FiTellSize(fd);
	char* pText = MmAllocate(size + 1);
	pText[size] = 0;
	FiRead(fd, pText, size);
	FiClose(fd);
	
	int nLines = HomeMenu$GetNumLines(pText);
	g_nLauncherItems = 0;
	g_pLauncherItems = MmAllocate(nLines * sizeof(LauncherItem));
	
	HomeMenu$LoadConfigFromString(pWindow, pText);
	
	MmFree(pText);
}

// Main program

void CALLBACK HomeMenu$WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE: {
			#define START_X 20
			#define STEXT_X 60
			#define START_Y 40
			#define DIST_ITEMS 36
			// Add a label welcoming the user to NanoShell.
			Rectangle r;
			
			RECT(r, 0, 0, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, LAUNCHER_MENUBAR, 0, 0);
			
			// Add some testing elements to the menubar.  A comboID of zero means you're adding to the root.
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0, 1, "Help");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 1, 2, "About Launcher...");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 0, 3, "Testing recursion");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 3, 4, "Single Button");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 3, 5, "Menu with more entries");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 3, 6, "Another button");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 5, 7, "Hey, you found me!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 5, 8, "Ahhah.");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 5, 9, "Look, another! :eyes:");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 9,10, "The lonely button...");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,10,11, "...has a friend!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR, 3,12, "Menu1 with more entries");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,12,13, "Another button");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,12,14, "Hey, you found me!");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,12,15, "Ahhah.");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,12,16, "Look, another! :eyes:");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,12,17, "The lonely button...");
			AddMenuBarItem (pWindow, LAUNCHER_MENUBAR,12,18, "...has a friend!");
			
			RECT(r, START_X, 15, 200, 20);
			AddControl(pWindow, CONTROL_TEXT, r, "Welcome to NanoShell!", LAUNCHER_LABEL1, 0, TRANSPARENT);
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (TITLE_BAR_HEIGHT + 15)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 270 - PADDING_AROUND_LISTVIEW * 2 - TOP_PADDING
			);
			AddControlEx(pWindow, CONTROL_ICONVIEW, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_RIGHT_TO_RIGHT, r, NULL, LAUNCHER_LISTVIEW, 0, 0);
			
			// Load config:
			HomeMenu$LoadConfig(pWindow);
			if (g_nLauncherItems == 0)
				return;
			
			// Add list items:
			ResetList(pWindow, LAUNCHER_LISTVIEW);
			
			for (int i = 0; i < g_nLauncherItems; i++)
			{
				AddElementToList(pWindow, LAUNCHER_LISTVIEW, g_pLauncherItems[i].m_text, g_pLauncherItems[i].m_icon);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
				case LAUNCHER_MENUBAR:
				{
					switch (parm2)
					{
						case 2: 
							LaunchVersion ();
							break;
						default:
							SLogMsg("Hello! %d has been clicked", parm2);
							break;
					}
					break;
				}
				case LAUNCHER_LISTVIEW:
				{
					//hack:
					if (g_pLauncherItems)
					{
						if (strcmp(g_pLauncherItems[parm2].m_resourceID, "shell:shutdown") == 0)
							ConfirmShutdown(pWindow);
						else
							LaunchResource(g_pLauncherItems[parm2].m_resourceID);
					}
					else
					{
						MessageBox(pWindow, "Object does not exist.", "Home Menu", ICON_ERROR << 16 | MB_OK);
					}
					
					break;
				}
				default:
					LogMsg("Unknown command.  Parm1: %d Parm2: %d", parm1, parm2);
					break;
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void LauncherEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = 400, wh = 270, sw = GetScreenSizeX(), sh = GetScreenSizeY();
	int wx = (sw - ww) / 2, wy = (sh - wh) / 2;
	
	Window* pWindow = CreateWindow ("Home", wx, wy, ww, wh, HomeMenu$WndProc, WF_ALWRESIZ);//WF_NOCLOSE);
	pWindow->m_iconID = ICON_HOME;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}

void LaunchLauncher()
{
	int errorCode = 0;
	Task* pTask;
	
	//create the program manager task.
	errorCode = 0;
	pTask = KeStartTask(LauncherEntry, 0, &errorCode);
	
	KeUnsuspendTask(pTask);
	
	DebugLogMsg("Created launcher task. pointer returned:%x, errorcode:%x", pTask, errorCode);
}