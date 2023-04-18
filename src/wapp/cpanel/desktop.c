/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

      Control panel - Desktop Applet
******************************************/

#include <wbuiltin.h>
#include <wtheme.h>

#define DESKTOP_POPUP_WIDTH 600
#define DESKTOP_POPUP_HEITE 400

typedef struct
{
	int m_SelectedTheme;
}
CplDesktopData;

extern bool g_GlowOnHover;
extern bool g_TaskListCompact;
extern bool     g_BackgroundSolidColorActive, g_RenderWindowContents;
extern uint32_t g_BackgroundSolidColor;

uint32_t* GetThemingParameters();
uint32_t* GetThemeParms(int themeNumber);

void RefreshScreen();
void RefreshEverything();
void PaintWindowBorderStandard(Rectangle windowRect, const char* pTitle, uint32_t flags, uint32_t privflags, int iconID, bool selected);
void WmOnChangedBorderSize();
bool WouldThemeChange(int thNum);

enum
{
	DESKTOP_ENABLE_BACKGD = 4000,
	DESKTOP_SHOW_WINDOW_CONTENTS,
	DESKTOP_APPLY_CHANGES,
	DESKTOP_OK,
	DESKTOP_CHANGE_BACKGD,
	DESKTOP_CHOOSETHEMETEXT,
	DESKTOP_ORMAKEYOUROWNTEXT,
	DESKTOP_GLOW_ON_HOVER,
	DESKTOP_TASKBAR_COMPACT,
	
	DESKTOP_CANCEL,
	
	DESKTOP_THEMECOMBO,
};

CplDesktopData* CplDesktopGetData(Window* pWindow)
{
	return (CplDesktopData*)pWindow->m_data;
}

void CALLBACK CplDesktopWndProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			CplDesktopData * pData = CplDesktopGetData(pWindow);
			
			// note: this is incredibly hacky... but it works, I think
			cli;
			uint32_t* pNewParms = GetThemingParameters();
			if (pData->m_SelectedTheme >= 0)
				pNewParms = GetThemeParms(pData->m_SelectedTheme);
			
			uint32_t* pParms = GetThemingParameters();
			uint32_t ParmsBackup[P_THEME_PARM_COUNT];
			
			memcpy(ParmsBackup, pParms, sizeof ParmsBackup);
			memcpy(pParms, pNewParms, sizeof ParmsBackup);
			
			
			Rectangle r;
			RECT (r, DESKTOP_POPUP_WIDTH/2+10, 3+10, DESKTOP_POPUP_WIDTH/2-20, 150);
			
			DrawEdge(r, DRE_SUNKEN | DRE_FILLED, BACKGROUND_COLOR);
			
			RECT (r, DESKTOP_POPUP_WIDTH/2+20, 3+20, 250-1, 100-1);
			VidFillRectangle(WINDOW_BACKGD_COLOR, r);
			r.right++, r.bottom++;
			PaintWindowBorderStandard(r, "Inactive window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ, 0, ICON_APP_DEMO, false);
			
			RECT (r, DESKTOP_POPUP_WIDTH/2+30, 3+50, 250-1, 100-1);
			VidFillRectangle(WINDOW_BACKGD_COLOR, r);
			r.right++, r.bottom++;
			
			PaintWindowBorderStandard(r, "Active window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ, 0, ICON_APP_DEMO, true);
			
			RECT (r, DESKTOP_POPUP_WIDTH/2+40, 3+75, 180-1, 70-1);
			VidFillRectangle(WINDOW_BACKGD_COLOR, r);
			r.right++, r.bottom++;
			
			PaintWindowBorderStandard(r, "Active window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ | WF_FLATBORD, 0, ICON_APP_DEMO, true);
			
			memcpy(pParms, ParmsBackup, sizeof ParmsBackup);
			
			sti;
			
			break;
		}
		case EVENT_CREATE:
		{
			pWindow->m_iconID = ICON_DESKTOP;//TODO
			CplDesktopData * pData = 
			pWindow->m_data = MmAllocate(sizeof(CplDesktopData));
			memset(pWindow->m_data, 0, sizeof(CplDesktopData));
			
			pData->m_SelectedTheme = -1;
			
			//add a button
			Rectangle r;
			RECT(r,10, 15, DESKTOP_POPUP_WIDTH/2 - 150, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Solid color background", DESKTOP_ENABLE_BACKGD, g_BackgroundSolidColorActive, 0);
			RECT(r,DESKTOP_POPUP_WIDTH/2-80, 10, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON,   r, "Change...", DESKTOP_CHANGE_BACKGD, 0, 0);
			RECT(r,10, 35, DESKTOP_POPUP_WIDTH/2 - 20, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show window contents while moving", DESKTOP_SHOW_WINDOW_CONTENTS, g_RenderWindowContents, 0);
			RECT(r,10, 55, DESKTOP_POPUP_WIDTH/2 - 20, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Make buttons glow when hovering over them", DESKTOP_GLOW_ON_HOVER, g_GlowOnHover, 0);
			RECT(r,10, 75, DESKTOP_POPUP_WIDTH/2 - 20, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Make task bar buttons compact", DESKTOP_TASKBAR_COMPACT, g_TaskListCompact, 0);
			
			RECT(r, 10, 100, DESKTOP_POPUP_WIDTH/2 - 20, 20);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Choose a default theme:", DESKTOP_CHOOSETHEMETEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED);
			
			RECT(r, 10, 120, DESKTOP_POPUP_WIDTH/2 - 20, 20);
			AddControl(pWindow, CONTROL_COMBOBOX, r, NULL, DESKTOP_THEMECOMBO, 0, 0);
			
			for (int i = TH_DEFAULT; i < TH_MAX; i++)
			{
				ComboBoxAddItem(pWindow, DESKTOP_THEMECOMBO, GetThemeName(i), i, 0);
			}
			
			RECT(r, 10, 195, DESKTOP_POPUP_WIDTH/2 - 20, 15);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Or make your own: (TODO)", DESKTOP_ORMAKEYOUROWNTEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED);
			
			RECT(r,(DESKTOP_POPUP_WIDTH/2-210)/2, DESKTOP_POPUP_HEITE - 30,70,20);
			AddControl(pWindow, CONTROL_BUTTON, r, "OK",     DESKTOP_OK, 0, 0);
			RECT(r,(DESKTOP_POPUP_WIDTH/2-210)/2+80, DESKTOP_POPUP_HEITE - 30 ,70,20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", DESKTOP_CANCEL, 0, 0);
			RECT(r,(DESKTOP_POPUP_WIDTH/2-210)/2+160, DESKTOP_POPUP_HEITE - 30 ,70,20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Apply",  DESKTOP_APPLY_CHANGES, 0, 0);
			
			break;
		}
		case EVENT_COMBOSELCHANGED:
		{
			CplDesktopData* pData = CplDesktopGetData(pWindow);
			int item = ComboBoxGetSelectedItemID(pWindow, DESKTOP_THEMECOMBO);
			
			if (pData->m_SelectedTheme != item)
			{
				pData->m_SelectedTheme = item;
				CplDesktopWndProc(pWindow, EVENT_PAINT, 0, 0);
			}
			
			break;
		}
		case EVENT_COMMAND:
			if (parm1 == DESKTOP_CHANGE_BACKGD)
			{
				uint32_t data = ColorInputBox(pWindow, "Choose a new background color:", "Background color");
				if (data != TRANSPARENT)
				{
					SetThemingParameter(P_BACKGROUND_COLOR, data & 0xffffff);
					RefreshScreen();
				}
				break;
			}
			else if (parm1 == DESKTOP_APPLY_CHANGES || parm1 == DESKTOP_OK)
			{
				g_BackgroundSolidColorActive = CheckboxGetChecked(pWindow, DESKTOP_ENABLE_BACKGD);
				g_RenderWindowContents       = CheckboxGetChecked(pWindow, DESKTOP_SHOW_WINDOW_CONTENTS);
				g_GlowOnHover                = CheckboxGetChecked(pWindow, DESKTOP_GLOW_ON_HOVER);
				g_TaskListCompact            = CheckboxGetChecked(pWindow, DESKTOP_TASKBAR_COMPACT);
				
				int item = ComboBoxGetSelectedItemID(pWindow, DESKTOP_THEMECOMBO);
				
				if (item >= 0 && WouldThemeChange(item))
				{
					int borderSize = BORDER_SIZE, titleBarHeight = TITLE_BAR_HEIGHT;
					ApplyTheme(item);
					
					if (borderSize != BORDER_SIZE || titleBarHeight != TITLE_BAR_HEIGHT)
						WmOnChangedBorderSize();
					
					RefreshEverything();
				}
				
				RefreshScreen();
			}
			
			if (parm1 == DESKTOP_OK || parm1 == DESKTOP_CANCEL)
				DestroyWindow(pWindow);
			
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void CplDesktop(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Desktop",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		DESKTOP_POPUP_WIDTH,
		DESKTOP_POPUP_HEITE,
		CplDesktopWndProc,
		WF_NOMINIMZ
	);
}
