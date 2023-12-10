/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Icon Test Application module
******************************************/

#include <wbuiltin.h>

#define LISTTEST_WIDTH  600
#define LISTTEST_HEIGHT 400

const char * const g_IconNames[] = {
	"ICON_NULL",
	"ICON_CABINET",
	"ICON_CHIP",
	"ICON_CHIP_SQ",
	"ICON_COMPUTER",
	"ICON_COMPUTER_SHUTDOWN",
	"ICON_DESKTOP",
	"ICON_DRAW",
	"ICON_EARTH",
	"ICON_ERROR",
	"ICON_EXECUTE_FILE",
	"ICON_FILE",
	"ICON_FILES",
	"ICON_FOLDER",
	"ICON_FOLDER_BLANK",
	"ICON_FOLDER_MOVE",
	"ICON_FOLDER_PARENT",
	"ICON_FOLDER16_CLOSED",
	"ICON_FOLDER16_OPEN",
	"ICON_GLOBE",
	"ICON_GO",
	"ICON_HAND",
	"ICON_HELP",
	"ICON_INFO",
	"ICON_KEYBOARD",
	"ICON_KEYBOARD2",
	"ICON_LAPTOP",
	"ICON_NOTES",
	"ICON_PAINT",
	"ICON_SERIAL",
	"ICON_STOP",
	"ICON_TEXT_FILE",
	"ICON_WARNING",
	"ICON_NANOSHELL_LETTERS",
	"ICON_NANOSHELL_LETTERS16",
	"ICON_NANOSHELL",
	"ICON_NANOSHELL16",
	"ICON_BOMB",
	"ICON_BOMB_SPIKEY",
	"ICON_FILE16",
	"ICON_TEXT_FILE16",
	"ICON_EXECUTE_FILE16",
	"ICON_FOLDER_PARENT16",
	//icons V1.1
	"ICON_FOLDER_SETTINGS",
	"ICON_CABINET16",
	"ICON_COMPUTER16",
	"ICON_COMMAND",
	"ICON_COMMAND16",
	"ICON_ERROR16",
	//icons V1.2
	"ICON_LOCK",
	"ICON_DIRECTIONS",
	"ICON_CERTIFICATE",
	"ICON_FILE_WRITE",
	"ICON_SCRAP_FILE",
	"ICON_SCRAP_FILE16",
	"ICON_RESMON",
	"ICON_BILLBOARD",
	"ICON_FILE_CSCRIPT",
	"ICON_FILE_CSCRIPT16",
	"ICON_FILE_CLICK",
	"ICON_KEYS",
	"ICON_RESTRICTED",
	"ICON_HOME",
	"ICON_HOME16",
	"ICON_ADAPTER",
	"ICON_CLOCK",
	"ICON_CLOCK16",
	//icons V1.3
	"ICON_APPLICATION",
	"ICON_APPLICATION16",
	"ICON_TASKBAR",
	"ICON_APP_DEMO",
	"ICON_COMPUTER_FLAT",
	"ICON_CALCULATOR",
	"ICON_CALCULATOR16",
	"ICON_DESKTOP2",
	"ICON_MOUSE",
    //Icons V1.31
	"ICON_AMBULANCE",
	//icons V1.32
	"ICON_FONTS",
	"ICON_FONTS16",
	//icons V1.33
	"ICON_RESMON16",
	"ICON_NOTES16",
	"ICON_FILE_NANO",
	//icons V1.34
	"ICON_CLOCK_EMPTY",//Special case which draws more stuff
	//icons V1.35
	"ICON_RUN",
	"ICON_RUN16",
	//icons V1.4
	"ICON_DEVTOOL",
	"ICON_DEVTOOL_FILE",
	"ICON_HEX_EDIT",
	"ICON_CHAIN",
	"ICON_CHAIN16",
	"ICON_DEVTOOL16",
	"ICON_TODO",
	"ICON_FOLDER_DOCU",
	"ICON_DLGEDIT",
	"ICON_DESK_SETTINGS",
	"ICON_SHUTDOWN",
	"ICON_NOTEPAD",
	"ICON_FILE_MKDOWN",
	"ICON_FILE_MKDOWN16",
	"ICON_COMPUTER_PANIC",
	"ICON_EXPERIMENT",
	"ICON_GRAPH",
	"ICON_CABINET_COMBINE",
	"ICON_REMOTE",
	"ICON_CABINET_OLD",
	//icons V1.5
	"ICON_DEVICE_CHAR",
	"ICON_DEVICE_BLOCK",
	"ICON_HARD_DRIVE",
	"ICON_HARD_DRIVE_MOUNT",
	"ICON_WINDOW",
	"ICON_WINDOW_SNAP",
	"ICON_WINDOW_OVERLAP",
	"ICON_SWEEP_SMILE",
	"ICON_SWEEP_CLICK",
	"ICON_SWEEP_DEAD",
	"ICON_SWEEP_CARET",
	"ICON_DLGEDIT16",
	"ICON_BOMB_SPIKEY16",
	"ICON_MAGNIFY",
	"ICON_MAGNIFY16",
	"ICON_TAR_ARCHIVE",
	"ICON_SYSMON",
	"ICON_SYSMON16",
	"ICON_COMPUTER_SHUTDOWN16",
	"ICON_EXIT",
	"ICON_KEY",
	"ICON_KEYB_REP_SPEED",
	"ICON_KEYB_REP_DELAY",
	"ICON_MONITOR",
	//icons V1.6
	"ICON_FILE_INI",
	"ICON_WMENU",
	"ICON_WMENU16",
	"ICON_FILE_IMAGE",
	"ICON_FILE_IMAGE16",
	"ICON_FILE_LOG",
	"ICON_STICKY_NOTES",
	"ICON_STICKY_NOTES16",
	"ICON_NOTE_YELLOW",
	"ICON_NOTE_BLUE",
	"ICON_NOTE_GREEN",
	"ICON_NOTE_WHITE",
	"ICON_FOLDER_OPEN",
	//icons V1.61
	"ICON_EXPERIMENT2",
	"ICON_FLOPPY",
	"ICON_ACTION_SAVE16",
	"ICON_ACTION_OPEN",
	"ICON_ACTION_OPEN16",
	"ICON_PLUS",
	//icons V1.7
	"ICON_PASTE",
	"ICON_PASTE16",
	"ICON_DELETE",
	"ICON_DELETE16",
	"ICON_COPY",
	"ICON_COPY16",
	"ICON_BACK",
	"ICON_BACK16",
	"ICON_FORWARD",
	"ICON_FORWARD16",
	"ICON_UNDO",
	"ICON_UNDO16",
	"ICON_REDO",
	"ICON_REDO16",
	"ICON_FILE_SEARCH",
	"ICON_FILE_SEARCH16",
	"ICON_FILE_PROPERTIES",
	"ICON_FILE_PROPERTIES16",
	"ICON_PROPERTIES",
	"ICON_PROPERTIES16",
	"ICON_WHATS_THIS",
	"ICON_WHATS_THIS16",
	"ICON_VIEW_ICON",
	"ICON_VIEW_ICON16",
	"ICON_VIEW_LIST",
	"ICON_VIEW_LIST16",
	"ICON_VIEW_TABLE",
	"ICON_VIEW_TABLE16",
	"ICON_SORT_ALPHA",
	"ICON_SORT_ALPHA16",
	"ICON_FORM",
	"ICON_FORM16",
	"ICON_JOURNAL",
	"ICON_JOURNAL16",
	"ICON_PACKAGER",
	"ICON_PACKAGER16",
	//icons V1.71
	"ICON_BOX_CHECK",
	"ICON_BOX_UNCHECK",
	"ICON_FOLDER_SETTINGS16",
	//icons V1.8
	"ICON_MINIMIZE",
	"ICON_MAXIMIZE",
	"ICON_RESTORE",
	"ICON_CLOSE",
	"ICON_ARROW_UP",
	"ICON_ARROW_DOWN",
	"ICON_ARROW_LEFT",
	"ICON_ARROW_RIGHT",
	"ICON_PIPE",
	"ICON_PIPE16",
	"ICON_VB_CURSOR",
	"ICON_VB_SELECT",
	"ICON_VB_TEXT",
	"ICON_VB_TEXT_CEN",
	"ICON_VB_INPUT_1LINE",
	"ICON_VB_INPUT_MLINE",
	"ICON_VB_CHECKBOX",
	"ICON_VB_SURR_RECT",
	"ICON_VB_BUTTON",
	"ICON_CLIPBOARD",
	"ICON_PAINT2",
	"ICON_FILE_BROKEN",
	"ICON_STOP_BLACK",
	"ICON_STOP_SMALL",
	"ICON_PAUSE_BLACK",
	"ICON_PAUSE_SMALL",
	"ICON_PLAY_BLACK",
	"ICON_PLAY_SMALL",
	"ICON_BROWSE_SMALL",
	//Icons V1.9
	"ICON_SNAP_U",
	"ICON_SNAP_D",
	"ICON_SNAP_L",
	"ICON_SNAP_R",
	"ICON_SNAP_UL",
	"ICON_SNAP_UR",
	"ICON_SNAP_DL",
	"ICON_SNAP_DR",
	"ICON_TASKBAR_POPOUT",
	"ICON_TASKBAR_DOCK",
	//Icons V2.0
	"ICON_CHESS",
	"ICON_CHESS16",
	"ICON_CRAYONS",
	"ICON_CRAYONS16",
	"ICON_SETTING_HOVER_GLOW",
	"ICON_SETTING_TB_SHOW_DATE",
	"ICON_SETTING_TB_SHOW_SECS",
	"ICON_SETTING_WINDOW_DRAG",
	"ICON_SETTING_SOLID_BG",
	//Icons V2.1
	"ICON_CHAIN_BROKEN",
	"ICON_CHAIN_BROKEN16",
	"ICON_SHORTCUT_OVERLAY",
	"ICON_SHORTCUT_OVERLAY16",
	//Icons V2.11
	"ICON_SETTING_ICON_SIZE",
	"ICON_SETTING_GRAPH_CPU",
	"ICON_SETTING_GRAPH_MEM",
	"ICON_SETTING_COMPACT_ICONS",
};

STATIC_ASSERT(ARRAY_COUNT(g_IconNames) == ICON_COUNT, "Modify This if you are adding icons!");

void CALLBACK ListTestProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
			break;
		case EVENT_USER:
		{
			pWindow->m_data = (void*)(((int)pWindow->m_data + 1) % 200);
			ProgBarSetProgress(pWindow, 3, (int)pWindow->m_data);
			CallControlCallback(pWindow, 3, EVENT_PAINT, 0, 0);
			break;		
		}
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			//RECT(r, 8, 8, LISTTEST_WIDTH - 16, 40);
			//AddControlEx(pWindow, CONTROL_TAB_PICKER, ANCHOR_RIGHT_TO_RIGHT, r, NULL, 2, 0, 0);
			
			RECT(r, 8, 8, LISTTEST_WIDTH - 16, 20);
			AddControlEx(pWindow, CONTROL_COMBOBOX, ANCHOR_RIGHT_TO_RIGHT, r, NULL, 2, 0, 0);
			
			RECT(r, 8, 8 + 40, LISTTEST_WIDTH - 16, 30);
			AddControlEx(pWindow, CONTROL_PROGRESS_BAR, ANCHOR_RIGHT_TO_RIGHT, r, NULL, 3, 84, 200);
			
			RECT(r, 8, 8 + 40 + 40, LISTTEST_WIDTH - 16, LISTTEST_HEIGHT - 16 - 40 - 40);
			AddControlEx(pWindow, CONTROL_COLORPICKER, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, 1, 0, 0);
			
			pWindow->m_data = (void*)42;
			
			AddTimer(pWindow, 10, EVENT_USER);
			
			/*
			ComboBoxAddItem(pWindow, 2, "Item #1", 1, 1);
			ComboBoxAddItem(pWindow, 2, "Item #2", 2, 2);
			ComboBoxAddItem(pWindow, 2, "Item #3", 3, 3);
			ComboBoxAddItem(pWindow, 2, "Item #4", 4, 4);
			ComboBoxAddItem(pWindow, 2, "Item #5", 5, 5);
			*/
			
			SLogMsg("ICON_COUNT=%d", ICON_COUNT);
			
			for (int i = 0; i < ICON_COUNT; i++)
			{
				ComboBoxAddItem(pWindow, 2, g_IconNames[i], i, i);
			}
			
			/*RECT (r, 20, 20, 1, 240-40);
			//goes from 0-99
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, 2, ((0<<16)|100), 50);
			
			RECT (r, 40, 20, 1, 240-40);
			//goes from 0-9
			AddControl (pWindow, CONTROL_VSCROLLBAR, r, NULL, 2, ((0<<16)|10), 5);
			
			RECT (r, 60, 20, 240-40, 1);
			//goes from 0-99
			AddControl (pWindow, CONTROL_HSCROLLBAR, r, NULL, 2, ((0<<16)|100), 50);
			
			RECT (r, 60, 40, 240-40, 1);
			//goes from 0-9
			AddControl (pWindow, CONTROL_HSCROLLBAR, r, NULL, 2, ((0<<16)|10), 5);*/
			
			break;
		}
		case EVENT_TABCHANGED:
		{
			int diff = GET_Y_PARM(parm2);
			
			if (diff == 0)
				SetControlVisibility(pWindow, 1, true);
			else
				SetControlVisibility(pWindow, 1, false);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void ListTestTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("List Test", 300, 200, LISTTEST_WIDTH, LISTTEST_HEIGHT, ListTestProc, WF_ALWRESIZ);
	pWindow->m_iconID = ICON_TEXT_FILE;
	
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
