/*****************************************
		NanoShell Operating System
	   (C) 2022-2023 iProgramInCpp

       Launcher Application module
******************************************/

#include "../wm/wi.h"
#include <resource.h>

#define EVENT_UPDATE_TASKBAR_CTLS (EVENT_USER + 1)

bool g_bShowDate = true;
bool g_bShowTimeSeconds = true;

void HomeMenu$LoadConfig(Window* pWindow);

extern int g_nLauncherItems;
extern LauncherItem* g_pLauncherItems;
extern int           g_nLauncherDVer;

// Taskbar
#if 1

#define TASKBAR_WIDTH (GetScreenWidth())
#define TASKBAR_HEIGHT (TITLE_BAR_HEIGHT + 9) // padding around button: 4 px, padding around text: 2 px
#define TASKBAR_BUTTON_WIDTH 80
#define TASKBAR_BUTTON_HEIGHT (TITLE_BAR_HEIGHT + 2)
#define TASKBAR_TIME_THING_WIDTH 50
#define TASKBAR_DATE_THING_WIDTH 150

WindowMenu g_taskbarMenu;
WindowMenu g_taskbarRightClickMenu;

#define TASKBAR_MENU_ORIGINAL_ID    (999)
#define TASKBAR_RC_MENU_ORIGINAL_ID (998)

extern Rectangle g_TaskbarMargins;

enum
{
	POP_OUT,
	DOCK_TOP,
	DOCK_BOTTOM,
	DOCK_LEFT,   // TODO these
	DOCK_RIGHT,
};

int g_taskbarDock = DOCK_TOP;

enum {
	TASKBAR_HELLO = 0x1,
	TASKBAR_START_TEXT,
	TASKBAR_TIME_TEXT,
	TASKBAR_DATE_TEXT,
};

void ConfirmShutdown(Window* pWindow)
{
	int result = ShutdownBox(pWindow);
	if (result != MBID_IGNORE)
	{
		WindowManagerShutdown (result == MBID_RETRY);
	}
}

const char* TaskbarGetMonthName(int mo)
{
	const char * moName[] = {
		"",
		"January",
		"February",
		"March",
		"April",
		"May",
		"June",
		"July",
		"August",
		"September",
		"October",
		"November",
		"December",
	};
	
	return moName[mo];
}

const char* TaskbarGetNumeralEnding(int num)
{
	int lastDigit = num % 10;
	if (lastDigit == 1 && num != 11)
		return "st"; // 1st, 11th, 21st, 31st
	if (lastDigit == 2 && num != 12)
		return "nd"; // 2nd, 12th, 22nd
	if (lastDigit == 3 && num != 13)
		return "rd"; // 3nd, 13th, 23nd
	return "th";
}

void UpdateTaskbar (Window* pWindow)
{
	char buffer[1024];
	
	// Time
	TimeStruct* time = TmReadTime();
	sprintf(buffer, g_bShowTimeSeconds ? "  %02d:%02d:%02d" : "  %02d:%02d", time->hours, time->minutes, time->seconds);
	SetLabelText(pWindow, TASKBAR_TIME_TEXT, buffer);
	
	if (g_bShowDate)
	{
		sprintf(buffer, "    %s %d%s, %d", TaskbarGetMonthName(time->month), time->day, TaskbarGetNumeralEnding(time->day), time->year);
		SetLabelText(pWindow, TASKBAR_DATE_TEXT, buffer);
		CallControlCallback(pWindow, TASKBAR_DATE_TEXT,  EVENT_PAINT, 0, 0);
	}
	
	CallControlCallback(pWindow, TASKBAR_TIME_TEXT,  EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, TASKBAR_START_TEXT, EVENT_PAINT, 0, 0);
}

static void TaskbarCreateMenuEntry (Window* pWindow, WindowMenu* pMenu, const char* pString, int occi, int nComboID, int nIconID)
{
	pMenu->pWindow = pWindow;
	pMenu->pMenuEntries = NULL;
	pMenu->nMenuEntries = 0;
	pMenu->nIconID      = nIconID;
	pMenu->nMenuComboID = nComboID;
	pMenu->nOrigCtlComboID = occi;
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
			
			RECT(r, 10, 30-18, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, 1, ICON_RUN, 0);
			RECT(r, 64, 35-18, 300, 32);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Type the name of a program, folder, document, or resource, and NanoShell will open it for you.", 2, WINDOW_TEXT_COLOR, TEXTSTYLE_WORDWRAPPED);
			RECT(r, 10, 70-18, 54, 22);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Open:", 3, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED);
			RECT(r, 64, 70-18, 300, 1);
			AddControl(pWindow, CONTROL_TEXTINPUT,  r, "", 4, 0, 0);
			
			RECT(r, (400-160-10)/2, 120-18, 80, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "OK", 5, 0, 0);
			RECT(r, (400-160-10)/2+90, 120-18, 80, 20);
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
	int ww = 400, wh = 150-18;//, sh = GetScreenHeight();
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
	
	KeUnsuspendTask(pTask);
	
	DebugLogMsg("Created run popup task. pointer returned:%x, errorcode:%x", pTask, errorCode);
}

Window* g_pTaskBarWindow;

int TaskbarGetFlags()
{
	if (g_taskbarDock == POP_OUT)
		return WF_NOCLOSE | WF_EXACTPOS | WF_SYSPOPUP | WF_NOMINIMZ | WF_NOMAXIMZ | WF_ALWRESIZ;
	else
		return WF_NOCLOSE | WF_NOTITLE | WF_NOBORDER | WF_EXACTPOS | WF_SYSPOPUP | WF_BACKGRND;
}

const char* TaskbarGetPopoutToggleText()
{
	return (g_taskbarDock == POP_OUT) ? "Dock to top" : "Pop out";
}

void TaskbarSetMargins()
{
	g_TaskbarMargins.left   =
	g_TaskbarMargins.top    =
	g_TaskbarMargins.right  =
	g_TaskbarMargins.bottom = 0;
	
	switch (g_taskbarDock)
	{
		case DOCK_TOP:
			g_TaskbarMargins.top = TASKBAR_HEIGHT;
			break;
		case DOCK_BOTTOM:
			g_TaskbarMargins.bottom = TASKBAR_HEIGHT;
			break;
		// TODO: left, right. Measure the NanoShell button's width, and use that as reference.
	}
}

void TaskbarGetPosition(int* x, int* y, int* width, int* height)
{
	switch (g_taskbarDock)
	{
		case POP_OUT:
			*x = 0;
			*y = 0;
			*width = TASKBAR_WIDTH * 3 / 4;
			*height = TASKBAR_HEIGHT + TITLE_BAR_HEIGHT + BORDER_SIZE * 2;
			break;
		case DOCK_TOP:
		default: // DOCK_LEFT and DOCK_RIGHT are unimplemented right now
			*x = 0;
			*y = 0;
			*width = TASKBAR_WIDTH;
			*height = TASKBAR_HEIGHT;
			break;
		case DOCK_BOTTOM:
			*x = 0;
			*y = GetScreenHeight() - TASKBAR_HEIGHT;
			*width = TASKBAR_WIDTH;
			*height = TASKBAR_HEIGHT;
			break;
	}
}

void TaskbarCreateControls(Window* pWindow);
void TaskbarRemoveControls(Window *pWindow);

void TaskbarPopOut(int buttonID)
{
	// The idea behind this is something like this:
	
	// When the taskbar is docked, the menu will be like this, with the button IDs:
	//   - Open System Monitor - (1)
	//   ----------------------- (2)
	//   - Pop the taskbar out - (3)
	
	// However, when the taskbar is popped out, it will list more than that:
	//   - Open System Monitor - (1)
	//   ----------------------- (2)
	//   - Dock to top         - (3)
	//   - Dock to bottom      - (4)
	//   - Dock to left        - (5)
	//   - Dock to right       - (6)
	
	// The ID for 'pop the taskbar out' will be reused for 'dock to top'.
	
	if (buttonID == 3)
	{
		g_taskbarDock = g_taskbarDock == POP_OUT ? DOCK_TOP : POP_OUT;
		
		if (g_taskbarDock == POP_OUT)
			g_taskbarRightClickMenu.nMenuEntries = 6;
		else
			g_taskbarRightClickMenu.nMenuEntries = 3;
	}
	else
	{
		g_taskbarDock = buttonID - 3 + DOCK_TOP;
		g_taskbarRightClickMenu.nMenuEntries = 3;
	}
	
	strcpy(g_taskbarRightClickMenu.pMenuEntries[2].sText, TaskbarGetPopoutToggleText());
	
	int x = 0, y = 0, width = 0, height = 0;
	
	TaskbarSetMargins();
	TaskbarGetPosition(&x, &y, &width, &height);
	
	//WmOnUpdateTaskbarMargins(); // -- this would loop through all maximized windows and resize them to fit the new taskbar margins
	
	TaskbarRemoveControls(g_pTaskBarWindow);
	TaskbarCreateControls(g_pTaskBarWindow);
	
	g_pTaskBarWindow->m_flags = TaskbarGetFlags();
	ResizeWindow(g_pTaskBarWindow, x, y, width, height);
}

void TaskbarCreateControls(Window* pWindow)
{
	if (!g_taskbarMenu.nMenuEntries)
	{
		int nEntries = g_nLauncherItems + 2 + 2 + 2 + 3;
		
		WindowMenu* pMenu = &g_taskbarMenu;
		
		pMenu->nMenuEntries = nEntries;
		pMenu->pMenuEntries = MmAllocate(sizeof (WindowMenu) * nEntries);
		memset (pMenu->pMenuEntries, 0,  sizeof (WindowMenu) * nEntries);
		
		int index = 0;
		
	#define CI(text, id, icon) TaskbarCreateMenuEntry(pWindow, &pMenu->pMenuEntries[index++], text, TASKBAR_MENU_ORIGINAL_ID, id, icon);
		// Add an "About" icon
		CI("About NanoShell...", 1, ICON_NANOSHELL);
		CI("", 2, ICON_NULL); // Empty string = dash
		
		for (int i = 0; i < g_nLauncherItems; i++)
			CI(g_pLauncherItems[i].m_text, 2000 + i, g_pLauncherItems[i].m_icon);
		
		CI("", 7, ICON_NULL);
		CI("Old Launcher", 8, ICON_HOME);
		CI("Settings", 9, ICON_FOLDER_SETTINGS);
		CI("", 3, ICON_NULL);
		CI("Run...", 4, ICON_RUN);
		CI("", 5, ICON_NULL);
		CI("Shut Down", 6, ICON_COMPUTER_SHUTDOWN);
	#undef CI
		
		pMenu->nLineSeparators = 3;
		pMenu->nWidth    = 200;
		pMenu->bHasIcons = true;
	}
	
	if (!g_taskbarRightClickMenu.nMenuEntries)
	{
		int nEntries = 6;
		
		WindowMenu* pMenu = &g_taskbarRightClickMenu;
		
		pMenu->nMenuEntries = nEntries;
		pMenu->pMenuEntries = MmAllocate(sizeof (WindowMenu) * nEntries);
		memset (pMenu->pMenuEntries, 0,  sizeof (WindowMenu) * nEntries);
		
		int index = 0;
		
	#define CI(text, id, icon) TaskbarCreateMenuEntry(pWindow, &pMenu->pMenuEntries[index++], text, TASKBAR_RC_MENU_ORIGINAL_ID, id, icon);
		CI("Open System Monitor", 1, ICON_NULL);
		CI("", 2, ICON_NULL);
		CI(TaskbarGetPopoutToggleText(), 3, ICON_NULL);
		CI("Dock to bottom", 4, ICON_NULL);
		CI("Dock to left",   5, ICON_NULL);
		CI("Dock to right",  6, ICON_NULL);
	#endif
		
		pMenu->nLineSeparators = 1;
		pMenu->nWidth    = 150;
		pMenu->bHasIcons = false;
		
		// Adjust the number of menu entries. By default, the task bar is docked to the top
		// of the screen, so subtract by 3
		pMenu->nMenuEntries -= 3;
	}
	
	Rectangle rect = GetWindowClientRect(pWindow, true);
	
	int taskbarWidth  = rect.right - rect.left;
	int taskbarHeight = rect.bottom - rect.top;
	
	Rectangle r;
	
	int wbutton, hbutton;
	const char *pNanoShellText = "\x05 NanoShell";
	VidTextOutInternal (pNanoShellText, 0, 0, 0, 0, true, &wbutton, &hbutton);
	
	int btn_width = TASKBAR_BUTTON_WIDTH;
	if (btn_width < wbutton + 10)
		btn_width = wbutton + 10;
	
	int yOffset = (g_taskbarDock == DOCK_BOTTOM);
	
	RECT (r, 4, 3 + yOffset, btn_width, TASKBAR_BUTTON_HEIGHT);
	AddControl(pWindow, CONTROL_BUTTON, r, pNanoShellText, TASKBAR_HELLO, 0, 0);
	
	RECT (r, taskbarWidth - 6 - TASKBAR_TIME_THING_WIDTH, 3 + yOffset, TASKBAR_TIME_THING_WIDTH, TASKBAR_BUTTON_HEIGHT);
	AddControlEx(pWindow, CONTROL_TEXTCENTER, ANCHOR_LEFT_TO_RIGHT | ANCHOR_RIGHT_TO_RIGHT, r, "?", TASKBAR_TIME_TEXT, 0, TEXTSTYLE_VCENTERED | TEXTSTYLE_RJUSTIFY | TEXTSTYLE_FORCEBGCOL);
	
	if (g_bShowDate)
	{
		RECT (r, taskbarWidth - 6 - TASKBAR_TIME_THING_WIDTH - TASKBAR_DATE_THING_WIDTH, 3 + yOffset, TASKBAR_DATE_THING_WIDTH, TASKBAR_BUTTON_HEIGHT);
		AddControlEx(pWindow, CONTROL_TEXTCENTER, ANCHOR_LEFT_TO_RIGHT | ANCHOR_RIGHT_TO_RIGHT, r, "?", TASKBAR_DATE_TEXT, 0, TEXTSTYLE_VCENTERED | TEXTSTYLE_RJUSTIFY | TEXTSTYLE_FORCEBGCOL);
	}
	
	int launcherItemPosX = 8 + btn_width;
	for (int i = 0; i < g_nLauncherItems; i++)
	{
		if (g_pLauncherItems[i].m_addToQuickLaunch)
		{
			RECT (r, launcherItemPosX, 2 + yOffset, TASKBAR_BUTTON_HEIGHT+1, TASKBAR_BUTTON_HEIGHT+1);
			
			AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, 1000+i, g_pLauncherItems[i].m_icon, 16);
			
			launcherItemPosX += TASKBAR_BUTTON_HEIGHT + 2;
		}
	}
	
	RECT (r, launcherItemPosX, 2 + yOffset, taskbarWidth - 6 - TASKBAR_TIME_THING_WIDTH - g_bShowDate * TASKBAR_DATE_THING_WIDTH - launcherItemPosX, TASKBAR_BUTTON_HEIGHT + 2 + (taskbarHeight - TASKBAR_HEIGHT));
	AddControlEx(pWindow, CONTROL_TASKLIST, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_RIGHT_TO_RIGHT, r, NULL, TASKBAR_START_TEXT, 0, 0);
	
	pWindow->m_data = (void*)(launcherItemPosX + 400);
}

void TaskbarRemoveControls(Window *pWindow)
{
	RemoveControl (pWindow, TASKBAR_DATE_TEXT);
	RemoveControl (pWindow, TASKBAR_TIME_TEXT);
	RemoveControl (pWindow, TASKBAR_START_TEXT);
	RemoveControl (pWindow, TASKBAR_HELLO);
	
	// remove the quick launch icons
	for (int i = 0; i < g_nLauncherItems; i++)
	{
		if (g_pLauncherItems[i].m_addToQuickLaunch)
		{
			RemoveControl(pWindow, 1000 + i);
		}
	}
}

void TaskbarSetProperties(bool bShowDate, bool bShowTimeSecs)
{
	if (bShowDate != g_bShowDate || bShowTimeSecs != g_bShowTimeSeconds)
	{
		g_bShowDate = bShowDate;
		g_bShowTimeSeconds = bShowTimeSecs;
		WindowRegisterEvent(g_pTaskBarWindow, EVENT_UPDATE_TASKBAR_CTLS, 0, 0);
	}
}

RESOURCE_STATUS LaunchLauncher();
RESOURCE_STATUS LaunchControlPanel();
RESOURCE_STATUS LaunchSystem();

void CALLBACK TaskbarProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	//int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			// Load config:
			HomeMenu$LoadConfig(pWindow);
			TaskbarCreateControls(pWindow);
			
			break;
		}
		case EVENT_UPDATE:
		{
			UpdateTaskbar(pWindow);
			break;
		}
		case EVENT_UPDATE_TASKBAR_CTLS:
		{
			TaskbarRemoveControls(pWindow);
			TaskbarCreateControls(pWindow);
			VidFillRect(WINDOW_BACKGD_COLOR, 0, 0, pWindow->m_vbeData.m_width, pWindow->m_vbeData.m_height);
			RequestRepaintNew(pWindow);
			UpdateTaskbar(pWindow);
			break;
		}
		case EVENT_RIGHTCLICKRELEASE:
		{
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			Rectangle crect = GetWindowClientRect(pWindow, true);
			
			int taskbarWidth  = crect.right - crect.left;
			int taskbarHeight = crect.bottom - crect.top;
			
			Rectangle rect;
			RECT (rect, 0, 2, taskbarWidth - 6 - TASKBAR_TIME_THING_WIDTH - g_bShowDate * TASKBAR_DATE_THING_WIDTH, TASKBAR_BUTTON_HEIGHT + 2 + (taskbarHeight - TASKBAR_HEIGHT));
			
			if (!RectangleContains(&rect, &pt)) break;
			
			pt.x += pWindow->m_rect.left;
			pt.y += pWindow->m_rect.top;
			
			SpawnMenu (pWindow, &g_taskbarRightClickMenu, pt.x, pt.y);
			break;
		}
		case EVENT_PAINT:
		{
			
			//VidDrawHLine (WINDOW_TITLE_INACTIVE_COLOR_B, pWindow->m_rect.left, pWindow->m_rect.right, 1);
			
			switch (g_taskbarDock)
			{
				case DOCK_TOP:
					VidDrawHLine (BUTTON_SHADOW_COLOR, pWindow->m_rect.left, pWindow->m_rect.right, pWindow->m_rect.bottom - pWindow->m_rect.top - 2);
					VidDrawHLine (BUTTON_EDGE_COLOR,   pWindow->m_rect.left, pWindow->m_rect.right, pWindow->m_rect.bottom - pWindow->m_rect.top - 1);
					break;
				case DOCK_BOTTOM:
					VidDrawHLine (BUTTON_HILITE_COLOR, pWindow->m_rect.left, pWindow->m_rect.right, 0);
					VidDrawHLine (BUTTON_HOVER_COLOR,  pWindow->m_rect.left, pWindow->m_rect.right, 1);
					break;
				case DOCK_LEFT:
					VidDrawVLine (BUTTON_SHADOW_COLOR, pWindow->m_rect.top, pWindow->m_rect.bottom, pWindow->m_rect.right - pWindow->m_rect.left - 2);
					VidDrawVLine (BUTTON_EDGE_COLOR,   pWindow->m_rect.top, pWindow->m_rect.bottom, pWindow->m_rect.right - pWindow->m_rect.left - 1);
					break;
				case DOCK_RIGHT:
					VidDrawVLine (BUTTON_HILITE_COLOR, pWindow->m_rect.top, pWindow->m_rect.bottom, 0);
					VidDrawVLine (BUTTON_HOVER_COLOR,  pWindow->m_rect.top, pWindow->m_rect.bottom, 1);
					break;
			}
			
			break;
		}
		case EVENT_COMMAND: {
		restrt:
			if (parm1 == TASKBAR_HELLO)
			{
				//LaunchLauncher();
				SpawnMenu (pWindow, &g_taskbarMenu, pWindow->m_rect.left + 4, pWindow->m_rect.bottom);
			}
			else if (parm1 == TASKBAR_RC_MENU_ORIGINAL_ID)
			{
				// check the menu
				switch (parm2)
				{
					case 1://System Mon
						LaunchSystem();
						break;
					case 3://Pop out / dock top
					case 4://Dock bottom
					case 5://Dock left
					case 6://Dock right
						TaskbarPopOut(parm2);
						break;
				}
			}
			else if (parm1 == TASKBAR_MENU_ORIGINAL_ID)
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
	
	WindowAddEventToMasterQueue(g_pTaskBarWindow, EVENT_UPDATE, 0, 0);
}
void TaskbarEntry(__attribute__((unused)) int arg)
{
	if (g_pTaskBarWindow) return;
	
	int x = 0, y = 0, width = 0, height = 0;
	
	TaskbarSetMargins();
	TaskbarGetPosition(&x, &y, &width, &height);
	
	Window* pWindow = CreateWindow ("Taskbar", x, y, width, height, TaskbarProgramProc, TaskbarGetFlags());
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the taskbar couldn't be created. Why?");
		return;
	}
	
	pWindow->m_iconID = ICON_DESKTOP2;
	
	g_pTaskBarWindow = pWindow;
	g_pTaskBarWindow->m_EventQueueLock.m_held = false;
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
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
}

