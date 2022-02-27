/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Launcher Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <vfs.h>
#include <elf.h>
#include <resource.h>

#define LAUNCHER_CONFIG_PATH "/launcher_config.txt"

// Shell resource handlers
#if 1

RESOURCE_STATUS LaunchVersionApplet()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(VersionProgramTask, 0, &errorCode);
	DebugLogMsg("Created version window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchSystem()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(SystemMonitorEntry, 0, &errorCode);
	DebugLogMsg("Created System window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchIconTest()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	DebugLogMsg("Created icon test window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchTextShell()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(TerminalHostTask, 0, &errorCode);
	//Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	DebugLogMsg("Created Text Shell window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchPaint()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(PrgPaintTask, 0, &errorCode);
	DebugLogMsg("Created Paint window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}

RESOURCE_STATUS LaunchControlPanel()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(ControlEntry, 0, &errorCode);
	DebugLogMsg("Created Control Panel window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchNotepad()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(BigTextEntry, 0, &errorCode);
	DebugLogMsg("Created Notepad window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchCabinet()
{
	int errorCode = 0;
	Task* pTask = KeStartTask(CabinetEntry, 0, &errorCode);
	DebugLogMsg("Created Cabinet window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}

#endif

// Misc. launcher stuff
#if 1

void LaunchVersion()
{
	LaunchVersionApplet();
}

#define STREQ(str1,str2) (!strcmp(str1,str2))
RESOURCE_STATUS LaunchResourceLauncher(const char* pResourceID)
{
	/**/ if (STREQ(pResourceID, "about"))    return LaunchVersionApplet();
	else if (STREQ(pResourceID, "sysmon"))   return LaunchSystem();
	else if (STREQ(pResourceID, "icontest")) return LaunchIconTest();
	else if (STREQ(pResourceID, "cmdshell")) return LaunchTextShell();
	else if (STREQ(pResourceID, "scribble")) return LaunchPaint();
	else if (STREQ(pResourceID, "cpanel"))   return LaunchControlPanel();
	else if (STREQ(pResourceID, "notepad"))  return LaunchNotepad();
	else if (STREQ(pResourceID, "cabinet"))  return LaunchCabinet();
	else return RESOURCE_LAUNCH_NOT_FOUND;
}


void WindowManagerShutdown();
void ConfirmShutdown(Window* pWindow)
{
	if (MessageBox (pWindow, "This will end your NanoShell session.", "Shut Down", MB_OKCANCEL | ICON_COMPUTER_SHUTDOWN << 16) == MBID_OK)
	{
		WindowManagerShutdown ();
	}
}

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
typedef struct
{
	char m_text[61];
	char m_resourceID[31];
	int  m_icon;
	bool m_addToQuickLaunch;
}
LauncherItem;
LauncherItem* g_pLauncherItems = NULL;
int           g_nLauncherDVer  = 0;

#endif

// Configuration loader:
#if 1

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

void HomeMenu$LoadConfig(Window* pWindow)
{
	if (g_pLauncherItems) return;//only load once
	
	//WORK: You can change the launcher config path here:
	int fd = FiOpen(LAUNCHER_CONFIG_PATH, O_RDONLY);
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

#endif

// Main program
#if 1

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
			
			RECT(r, START_X, TITLE_BAR_HEIGHT * 2 + 15, 200, 20);
			AddControl (pWindow, CONTROL_TEXT, r, "Welcome to NanoShell!", LAUNCHER_LABEL1, 0, TRANSPARENT);
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (TITLE_BAR_HEIGHT + TITLE_BAR_HEIGHT + 15)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 270 - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			AddControl(pWindow, CONTROL_ICONVIEW, r, NULL, LAUNCHER_LISTVIEW, 0, 0);
			
			// Load config:
			HomeMenu$LoadConfig(pWindow);
			
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
	
	Window* pWindow = CreateWindow ("Home", wx, wy, ww, wh, HomeMenu$WndProc, 0);//WF_NOCLOSE);
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

#endif

// Taskbar
#if 1

#define TASKBAR_WIDTH (GetScreenWidth())
#define TASKBAR_HEIGHT TITLE_BAR_HEIGHT + 14 // padding around button: 4 px, padding around text: 2 px
#define TASKBAR_BUTTON_WIDTH 40
#define TASKBAR_BUTTON_HEIGHT TITLE_BAR_HEIGHT + 8
#define TASKBAR_TIME_THING_WIDTH 50

//hack.
#undef  TITLE_BAR_HEIGHT
#define TITLE_BAR_HEIGHT 12


enum {
	TASKBAR_HELLO = 0x1,
	TASKBAR_START_TEXT,
	TASKBAR_TIME_TEXT,
};

void LaunchLauncher()
{
	int errorCode = 0;
	Task* pTask;
	
	//create the program manager task.
	errorCode = 0;
	pTask = KeStartTask(LauncherEntry, 0, &errorCode);
	DebugLogMsg("Created launcher task. pointer returned:%x, errorcode:%x", pTask, errorCode);
}

void UpdateTaskbar (Window* pWindow)
{
	char buffer[1024];
	
	//TODO: Window buttons.
	
	// FPS
	sprintf(buffer, "FPS: %d     ", GetWindowManagerFPS());
	SetLabelText(pWindow, TASKBAR_START_TEXT, buffer);
	
	// Time
	TimeStruct* time = TmReadTime();
	sprintf(buffer, "%02d:%02d:%02d", time->hours, time->minutes, time->seconds);
	SetLabelText(pWindow, TASKBAR_TIME_TEXT, buffer);
}

void CALLBACK TaskbarProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE: {
			
			// Load config:
			HomeMenu$LoadConfig(pWindow);
			
			Rectangle r;
			
			RECT (r, 4, 2, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_BUTTON, r, "Start", TASKBAR_HELLO, 0, 0);
			RECT (r, GetScreenWidth() - 2 - TASKBAR_TIME_THING_WIDTH, 8, TASKBAR_TIME_THING_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_TEXT, r, "?", TASKBAR_TIME_TEXT, 0, WINDOW_BACKGD_COLOR);
			
			int launcherItemPosX = 8 + TASKBAR_BUTTON_WIDTH;
			for (int i = 0; i < g_nLauncherItems; i++)
			{
				if (g_pLauncherItems[i].m_addToQuickLaunch)
				{
					RECT (r, launcherItemPosX, 2, TASKBAR_BUTTON_HEIGHT+1, TASKBAR_BUTTON_HEIGHT+1);
					AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, 1000+i, g_pLauncherItems[i].m_icon, 16);
					
					launcherItemPosX += TASKBAR_BUTTON_HEIGHT + 2;
				}
			}
			
			RECT (r, launcherItemPosX, 8, 400, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_TEXT, r, "FPS: Wait...", TASKBAR_START_TEXT, 0, WINDOW_BACKGD_COLOR);
			
			pWindow->m_data = (void*)(launcherItemPosX + 400);
			
			break;
		}
		case EVENT_UPDATE: {
			UpdateTaskbar(pWindow);
			break;
		}
		case EVENT_PAINT: {
			
			break;
		}
		case EVENT_COMMAND: {
			if (parm1 == TASKBAR_HELLO)
				LaunchLauncher();
			else if (parm1 >= 1000)
			{
				int idx = parm1-1000;
				if (idx < g_nLauncherItems)
				{
					LaunchResource(g_pLauncherItems[idx].m_resourceID);
				}
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void TaskbarEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = TASKBAR_WIDTH, wh = TASKBAR_HEIGHT;//, sh = GetScreenHeight();
	int wx = 0, wy = 0;//(sh - wh)+2;
	
	Window* pWindow = CreateWindow ("Desktop", wx, wy, ww, wh, TaskbarProgramProc, WF_NOCLOSE | WF_NOTITLE);
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	pWindow->m_iconID = ICON_DESKTOP2;
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	int timeout = 0;
	while (HandleMessages (pWindow))
	{
		if (timeout == 0)
		{
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			WindowRegisterEvent(pWindow, EVENT_PAINT,  0, 0);
			timeout = 100;
		}
		timeout--;
	}
#endif
}


#endif


