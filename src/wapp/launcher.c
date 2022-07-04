/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Launcher Application module
******************************************/

#include <wtheme.h>
#include <wbuiltin.h>
#include <wterm.h>
#include <wmenu.h>
#include <config.h>
#include <process.h>
#include <vfs.h>
#include <elf.h>
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
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchSystem()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(SystemMonitorEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchIconTest()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(IconTestTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchTextShell()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(TerminalHostTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchPaint()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(PrgPaintTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}

RESOURCE_STATUS LaunchControlPanel()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(ControlEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchNotepad()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(BigTextEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchCabinet()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(CabinetEntry, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchVBuilder()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(PrgVBldTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	return RESOURCE_LAUNCH_SUCCESS;
}
RESOURCE_STATUS LaunchMagnifier()
{
	UNUSED int errorCode = 0;
	Task* pTask = KeStartTask(PrgMagnifyTask, 0, &errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
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
	else if (STREQ(pResourceID, "cmdshell")) return LaunchTextShell();
	else if (STREQ(pResourceID, "scribble")) return LaunchPaint();
	else if (STREQ(pResourceID, "cpanel"))   return LaunchControlPanel();
	else if (STREQ(pResourceID, "notepad"))  return LaunchNotepad();
	else if (STREQ(pResourceID, "cabinet"))  return LaunchCabinet();
	else if (STREQ(pResourceID, "vbuild"))   return LaunchVBuilder();
	else if (STREQ(pResourceID, "magnify"))  return LaunchMagnifier();
	else if (STREQ(pResourceID, "launcher")) { LaunchLauncher(); return RESOURCE_LAUNCH_SUCCESS; }
	else if (STREQ(pResourceID, "help"))     return HelpOpenResource("/Fat0/Help.md");//LaunchHelp();
	else return RESOURCE_LAUNCH_NOT_FOUND;
}


void WindowManagerShutdown(bool restart);
int  ShutdownBox (Window *pWindow);
void ConfirmShutdown(Window* pWindow)
{
	int result = ShutdownBox(pWindow);
	if (result != MBID_IGNORE)
	{
		WindowManagerShutdown (result == MBID_RETRY);
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
			AddControl(pWindow, CONTROL_TEXT, r, "Welcome to NanoShell!", LAUNCHER_LABEL1, 0, TRANSPARENT);
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (TITLE_BAR_HEIGHT + TITLE_BAR_HEIGHT + 15)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 270 - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
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

#endif

// Taskbar
#if 1

#define TASKBAR_WIDTH (GetScreenWidth())
#define TASKBAR_HEIGHT TITLE_BAR_HEIGHT + 16 // padding around button: 4 px, padding around text: 2 px
#define TASKBAR_BUTTON_WIDTH 80
#define TASKBAR_BUTTON_HEIGHT TITLE_BAR_HEIGHT + 8
#define TASKBAR_TIME_THING_WIDTH 50

//hack.
#undef TITLE_BAR_HEIGHT
#define TITLE_BAR_HEIGHT (GetThemingParameter(P_TITLE_BAR_HEIGHT) - 6)

WindowMenu g_taskbarMenu;

extern int g_TaskbarHeight;

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
	
	// Time
	TimeStruct* time = TmReadTime();
	sprintf(buffer, "  %02d:%02d:%02d", time->hours, time->minutes, time->seconds);
	SetLabelText(pWindow, TASKBAR_TIME_TEXT, buffer);
	
	CallControlCallback(pWindow, TASKBAR_TIME_TEXT,  EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, TASKBAR_START_TEXT, EVENT_PAINT, 0, 0);
}

__attribute__((always_inline))
static inline void CreateEntry (Window* pWindow, WindowMenu* pMenu, const char* pString, int nComboID, int nIconID)
{
	pMenu->pWindow = pWindow;
	pMenu->pMenuEntries = NULL;
	pMenu->nMenuEntries = 0;
	pMenu->nIconID      = nIconID;
	pMenu->nMenuComboID = nComboID;
	pMenu->nOrigCtlComboID = 999;
	pMenu->nWidth       = 200;
	pMenu->bOpen        = false;
	pMenu->pOpenWindow  = NULL;
	strcpy (pMenu->sText, pString);
}

void RunPopupError(Window* pWindow, const char* pResource, RESOURCE_STATUS erc)
{
	char* pString = MmAllocate(strlen(pResource)+200);
	const char* stringToFormat = "NanoShell cannot launch '%s'. Make sure you've typed the name properly, and then try again.";
	
	switch (erc)
	{
		case RESOURCE_LAUNCH_OUT_OF_MEMORY:
			stringToFormat = "NanoShell cannot launch '%s' because there is not enough memory available in the system.  Close some applications, and try again.";
			break;
		case RESOURCE_LAUNCH_INVALID_PROTOCOL:
			stringToFormat = "NanoShell does not recognize the protocol specified in '%s'.  Make sure you've typed in the name properly, and then try again.";
			break;
		case RESOURCE_LAUNCH_NO_PROTOCOL:
		case RESOURCE_LAUNCH_NOT_FOUND:
			stringToFormat = "NanoShell cannot find '%s'.  Make sure you've typed in the name properly, and then try again.";
			break;
	}
	
	sprintf (pString, stringToFormat, pResource);
	
	MessageBox(pWindow, pString, pResource, ICON_ERROR << 16 | MB_OK);
	
	MmFree(pString);
}

void CALLBACK RunPopupProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE: {
			
			Rectangle r;
			
			RECT(r, 10, 30, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, 1, ICON_RUN, 0);
			RECT(r, 64, 35, 300, 32);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Type the name of a program, folder, document, or resource, and NanoShell will open it for you.", 2, WINDOW_TEXT_COLOR, TEXTSTYLE_WORDWRAPPED);
			RECT(r, 10, 70, 54, 22);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Open:", 3, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED);
			RECT(r, 64, 70, 300, 1);
			AddControl(pWindow, CONTROL_TEXTINPUT,  r, "", 4, 0, 0);
			
			RECT(r, (400-160-10)/2, 120, 80, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "OK", 5, 0, 0);
			RECT(r, (400-160-10)/2+90, 120, 80, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "Cancel", 6, 0, 0);
			
			break;
		}
		case EVENT_COMMAND: {
			if (parm1 >= 5)
			{
				if (parm1 == 5)
				{
					//actually run the thing
					RESOURCE_STATUS status = LaunchResource(TextInputGetRawText(pWindow, 4));
					if (status != RESOURCE_LAUNCH_SUCCESS)
					{
						RunPopupError(pWindow, TextInputGetRawText(pWindow, 4), status);
						break;
					}
				}
				
				//cancel button: Destroy
				DestroyWindow(pWindow);
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void RunPopupEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = 400, wh = 150;//, sh = GetScreenHeight();
	int wx = 20, wy = (GetScreenHeight() - wh - 20);//(sh - wh)+2;
	
	Window* pWindow = CreateWindow ("Run", wx, wy, ww, wh, RunPopupProc, 0);
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	pWindow->m_iconID = ICON_NANOSHELL;
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}

void LaunchRun()
{
	int errorCode = 0;
	Task* pTask;
	
	errorCode = 0;
	pTask = KeStartTask(RunPopupEntry, 0, &errorCode);
	DebugLogMsg("Created run popup task. pointer returned:%x, errorcode:%x", pTask, errorCode);
}

Window* g_pTaskBarWindow;
void CALLBACK TaskbarProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE: {
			
			// Load config:
			HomeMenu$LoadConfig(pWindow);
			
			if (!g_taskbarMenu.nMenuEntries)
			{
				int nEntries = g_nLauncherItems + 2 + 2 + 2 + 3;
				
				g_taskbarMenu.nMenuEntries = nEntries;
				g_taskbarMenu.pMenuEntries = MmAllocate(sizeof (WindowMenu) * nEntries);
				memset (g_taskbarMenu.pMenuEntries, 0,  sizeof (WindowMenu) * nEntries);
				
				int index = 0;
				// Add an "About" icon
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "About NanoShell...", 1, ICON_NANOSHELL);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "", 2, ICON_NULL); // Empty string = dash
				for (int i = 0; i < g_nLauncherItems; i++)
				{
					CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], g_pLauncherItems[i].m_text, 2000 + i, g_pLauncherItems[i].m_icon); // Empty string = dash
				}
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "", 7, ICON_NULL);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "Old Launcher", 8, ICON_HOME);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "Settings", 9, ICON_FOLDER_SETTINGS);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "", 3, ICON_NULL);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "Run...", 4, ICON_RUN);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "", 5, ICON_NULL);
				CreateEntry (pWindow, &g_taskbarMenu.pMenuEntries[index++], "Shut Down", 6, ICON_COMPUTER_SHUTDOWN);
			}
			g_taskbarMenu.nLineSeparators = 3;
			g_taskbarMenu.nWidth    = 200;
			g_taskbarMenu.bHasIcons = true;
			
			Rectangle r;
			
			int wbutton, hbutton;
			const char *pNanoShellText = "\x05 NanoShell";
			VidTextOutInternal (pNanoShellText, 0, 0, 0, 0, true, &wbutton, &hbutton);
			
			int btn_width = TASKBAR_BUTTON_WIDTH;
			if (btn_width < wbutton + 10)
				btn_width = wbutton + 10;
			
			RECT (r, 4, 3, btn_width, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_BUTTON, r, pNanoShellText, TASKBAR_HELLO, 0, 0);
			RECT (r, GetScreenWidth() - 6 - TASKBAR_TIME_THING_WIDTH, 3, TASKBAR_TIME_THING_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "?", TASKBAR_TIME_TEXT, 0, TEXTSTYLE_VCENTERED | TEXTSTYLE_RJUSTIFY | TEXTSTYLE_FORCEBGCOL);
			
			int launcherItemPosX = 8 + btn_width;
			for (int i = 0; i < g_nLauncherItems; i++)
			{
				if (g_pLauncherItems[i].m_addToQuickLaunch)
				{
					RECT (r, launcherItemPosX, 2, TASKBAR_BUTTON_HEIGHT+1, TASKBAR_BUTTON_HEIGHT+1);
					AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, 1000+i, g_pLauncherItems[i].m_icon, 16);
					
					launcherItemPosX += TASKBAR_BUTTON_HEIGHT + 2;
				}
			}
			
			RECT (r, launcherItemPosX, 2, GetScreenWidth() - 6 - TASKBAR_TIME_THING_WIDTH - launcherItemPosX, TASKBAR_BUTTON_HEIGHT + 2);
			AddControl(pWindow, CONTROL_TASKLIST, r, NULL, TASKBAR_START_TEXT, 0, 0);
			
			pWindow->m_data = (void*)(launcherItemPosX + 400);
			
			break;
		}
		case EVENT_UPDATE:
		{
			UpdateTaskbar(pWindow);
			break;
		}
		case EVENT_PAINT:
		{
			
			//VidDrawHLine (WINDOW_TITLE_INACTIVE_COLOR_B, pWindow->m_rect.left, pWindow->m_rect.right, 1);
			
			VidDrawHLine (WINDOW_TITLE_INACTIVE_COLOR, pWindow->m_rect.left, pWindow->m_rect.right, pWindow->m_rect.bottom - pWindow->m_rect.top - 2);
			VidDrawHLine (0x0000000000000000000000000, pWindow->m_rect.left, pWindow->m_rect.right, pWindow->m_rect.bottom - pWindow->m_rect.top - 1);
			
			break;
		}
		case EVENT_COMMAND: {
		restrt:
			if (parm1 == TASKBAR_HELLO)
			{
				//LaunchLauncher();
				SpawnMenu (pWindow, &g_taskbarMenu, pWindow->m_rect.left + 4, pWindow->m_rect.bottom);
			}
			else if (parm1 == 999)
			{
				// check the menu
				switch (parm2)
				{
					case 1://About
						LaunchVersion();
						break;
					case 4:
						//Run
						LaunchRun();
						break;
					case 6:
						ConfirmShutdown(pWindow);
						break;
					case 8:
						LaunchLauncher();
						break;
					case 9:
						LaunchControlPanel();
						break;
					default:
						if (parm2 >= 2000)
						{
							parm1 = parm2 - 1000;
							goto restrt;
						}
				}
			}
			else if (parm1 >= 1000)
			{
				int idx = parm1-1000;
				if (idx < g_nLauncherItems)
				{
					RESOURCE_STATUS status = LaunchResource(g_pLauncherItems[idx].m_resourceID);
					if (status)
					{
						char Buffer [2048];
						sprintf (Buffer, GetResourceErrorText(status), g_pLauncherItems[idx].m_resourceID);
						MessageBox (pWindow, Buffer, "NanoShell Menu", MB_OK | ICON_ERROR << 16);
					}
				}
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}
void RequestTaskbarUpdate()
{
	if (!g_pTaskBarWindow) return;
	
	WindowRegisterEvent(g_pTaskBarWindow, EVENT_UPDATE, 0, 0);
}
void TaskbarEntry(__attribute__((unused)) int arg)
{
	if (g_pTaskBarWindow) return;
	
	// create ourself a window:
	int ww = TASKBAR_WIDTH, wh = TASKBAR_HEIGHT-1, sh = GetScreenHeight();
	int wx = 0, wy = 0;
	
	g_TaskbarHeight = TASKBAR_HEIGHT;
	
	Window* pWindow = CreateWindow ("Desktop", wx, wy, ww, wh, TaskbarProgramProc, WF_NOCLOSE | WF_NOTITLE | WF_NOBORDER | WF_EXACTPOS | WF_SYSPOPUP);
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the taskbar couldn't be created. Why?");
		return;
	}
	
	pWindow->m_iconID = ICON_DESKTOP2;
	
	g_pTaskBarWindow = pWindow;
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	int timeout = GetTickCount();
	while (HandleMessages (pWindow))
	{
		if (GetTickCount() > timeout)
		{
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			//WindowRegisterEvent(pWindow, EVENT_PAINT,  0, 0);
			timeout += 1000;
		}
	}
#endif
}


#endif


