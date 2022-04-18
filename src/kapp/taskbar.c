/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        TaskBar Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <wmenu.h>
#include <config.h>
#include <process.h>
#include <vfs.h>
#include <elf.h>
#include <resource.h>


extern int g_nLauncherItems;
extern LauncherItem* g_pLauncherItems;
extern int           g_nLauncherDVer;


void ConfirmShutdown(Window* pWindow);
RESOURCE_STATUS LaunchControlPanel();
void HomeMenu$LoadConfig(Window* pWindow);
void LaunchLauncher();
void LaunchRun();

// Global Menu Bar
Window* g_pGlobalMenuBar;

WindowMenu g_taskbarMenu;

#define MENU_BAR_HEIGHT 15

void CALLBACK MenuBarProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
			
			VidDrawHLine(0x000000, 0, GetScreenWidth(), TITLE_BAR_HEIGHT-1);
			
			break;
		case EVENT_CREATE:
		{
			Rectangle r;
			
			//TODO
			RECT (r, 35, 0, GetScreenWidth() - 100, MENU_BAR_HEIGHT);
			
			AddControl (pWindow, CONTROL_MENUBAR, r, "HelloWorld", 1111, 1, 0);
			
			AddMenuBarItem(pWindow, 1111, 0, 2, "File");
			AddMenuBarItem(pWindow, 1111, 0, 3, "View");
			AddMenuBarItem(pWindow, 1111, 0, 4, "Help");
			AddMenuBarItem(pWindow, 1111, 2, 5, "New file");
			AddMenuBarItem(pWindow, 1111, 2, 6, "Open...");
			AddMenuBarItem(pWindow, 1111, 2, 7, "Save");
			AddMenuBarItem(pWindow, 1111, 2, 8, "Save as...");
			AddMenuBarItem(pWindow, 1111, 2, 9, "Exit");
			AddMenuBarItem(pWindow, 1111, 4, 10,"About Notepad");
			AddMenuBarItem(pWindow, 1111, 3, 11,"Syntax Highlighting");
			AddMenuBarItem(pWindow, 1111, 3, 12,"Line numbers");
			AddMenuBarItem(pWindow, 1111, 3, 13,"Font...");
			
			RECT (r, 4, -1, 17, 17);
			
			AddControl (pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, 1112, ICON_NANOSHELL16, 16);
			
			break;
		}
		case EVENT_COMMAND:
		{
		restrt:
			if (parm1 == 1112)
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
	}
}

Window* g_pMenuBarFocusWindow = NULL;

void OnWindowDestroyed(Window* pWnd)
{
	if (!g_pGlobalMenuBar) return;
	
	if (g_pMenuBarFocusWindow != pWnd) return;
	
	g_pMenuBarFocusWindow = NULL;
	
	Control* pCtl = GetControlByComboID (g_pGlobalMenuBar, 1111);
	if (!pCtl) return;
	pCtl->m_text[0] = 0;
	
	WindowAddEventToMasterQueue(g_pGlobalMenuBar, EVENT_PAINT, 0, 0);
	
	WidgetMenuBar_Reset(pCtl);
}
void OnUpdateFocusedWindow(Window* pWnd)
{
	if (!g_pGlobalMenuBar) return;
	
	if (pWnd->m_flags & WF_SYSPOPUP) return;
	
	Control* pCtl = GetControlByComboID (g_pGlobalMenuBar, 1111);
	if (!pCtl) return;
	OnWindowDestroyed (g_pMenuBarFocusWindow);
	
	strcpy (pCtl->m_text, pWnd->m_title);
	
	g_pMenuBarFocusWindow = pWnd;
}


// Taskbar

#define TASKBAR_WIDTH (GetScreenWidth())
#define TASKBAR_HEIGHT TITLE_BAR_HEIGHT + 16 // padding around button: 4 px, padding around text: 2 px
#define TASKBAR_BUTTON_WIDTH 80
#define TASKBAR_BUTTON_HEIGHT TITLE_BAR_HEIGHT + 8
#define TASKBAR_TIME_THING_WIDTH 50

//hack.
#undef  TITLE_BAR_HEIGHT
#define TITLE_BAR_HEIGHT 12


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
			
			RECT (r, GetScreenWidth() - 6 - TASKBAR_TIME_THING_WIDTH, 4, TASKBAR_TIME_THING_WIDTH, TASKBAR_BUTTON_HEIGHT);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "?", TASKBAR_TIME_TEXT, 0, TEXTSTYLE_VCENTERED | TEXTSTYLE_RJUSTIFY | TEXTSTYLE_FORCEBGCOL);
			
			int launcherItemPosX = 8;
			for (int i = 0; i < g_nLauncherItems; i++)
			{
				if (g_pLauncherItems[i].m_addToQuickLaunch)
				{
					RECT (r, launcherItemPosX, 3, TASKBAR_BUTTON_HEIGHT+1, TASKBAR_BUTTON_HEIGHT+1);
					AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, 1000+i, g_pLauncherItems[i].m_icon, 16);
					
					launcherItemPosX += TASKBAR_BUTTON_HEIGHT + 2;
				}
			}
			
			RECT (r, launcherItemPosX, 3, GetScreenWidth() - 6 - TASKBAR_TIME_THING_WIDTH - launcherItemPosX, TASKBAR_BUTTON_HEIGHT + 2);
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
			
			VidDrawHLine (WINDOW_TITLE_INACTIVE_COLOR_B, pWindow->m_rect.left, pWindow->m_rect.right, 1);
			
			//VidDrawHLine (WINDOW_TITLE_INACTIVE_COLOR, pWindow->m_rect.left, pWindow->m_rect.right, pWindow->m_rect.bottom - pWindow->m_rect.top - 2);
			//VidDrawHLine (0x0000000000000000000000000, pWindow->m_rect.left, pWindow->m_rect.right, pWindow->m_rect.bottom - pWindow->m_rect.top - 1);
			
			break;
		}
		case EVENT_COMMAND: {
		restrt:
			if (parm1 == TASKBAR_HELLO)
			{
				//LaunchLauncher();
				//SpawnMenu (pWindow, &g_taskbarMenu, pWindow->m_rect.left + 4, pWindow->m_rect.bottom);
				SpawnMenu (pWindow, &g_taskbarMenu, pWindow->m_rect.left + 4, pWindow->m_rect.top - 3 + TASKBAR_BUTTON_HEIGHT - GetMenuHeight(&g_taskbarMenu));
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

void TaskbarEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = TASKBAR_WIDTH, wh = TASKBAR_HEIGHT-2, sh = GetScreenHeight();
	int wx = 0, wy = (sh - wh);
	
	g_TaskbarHeight = TASKBAR_HEIGHT;
	
	// Create Taskbar
	Window* pTWindow = CreateWindow ("Task Bar", wx, wy, ww, wh, TaskbarProgramProc, WF_NOCLOSE | WF_NOTITLE | WF_NOBORDER | WF_EXACTPOS | WF_SYSPOPUP);
	
	if (!pTWindow)
	{
		DebugLogMsg("Hey, the taskbar couldn't be created. Why?");
		return;
	}
	
	pTWindow->m_iconID = ICON_DESKTOP2;
	
	// Create Menubar
	Window *pMWindow = CreateWindow ("Desktop", 0, 0, GetScreenWidth (), GetThemingParameter (P_TITLE_BAR_HEIGHT), MenuBarProc, WF_NOCLOSE | WF_NOTITLE | WF_NOBORDER | WF_EXACTPOS | WF_SYSPOPUP);
	
	if (!pMWindow)
	{
		DebugLogMsg("Hey, the menu bar couldn't be created. Why?");
		return;
	}
	
	pMWindow->m_iconID = ICON_DESKTOP2;
	pMWindow->m_bWindowManagerUpdated = true;
	
	g_pGlobalMenuBar = pMWindow;
	
	
	// event loop:
#if THREADING_ENABLED
	int timeout = 0;
	while (true)
	{
		if (pTWindow) if (!HandleMessages (pTWindow)) pTWindow = NULL;
		if (pMWindow) if (!HandleMessages (pMWindow)) pMWindow = g_pGlobalMenuBar = NULL;
		
		if (timeout == 0 && pTWindow)
		{
			WindowRegisterEvent(pTWindow, EVENT_UPDATE, 0, 0);
			WindowRegisterEvent(pTWindow, EVENT_PAINT,  0, 0);
			timeout = 100;
		}
		timeout--;
		
		if (!pTWindow && !pMWindow) break;
	}
#endif
}
