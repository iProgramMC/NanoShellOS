/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

      Control panel - Desktop Applet
******************************************/

#include <wbuiltin.h>
#include <wtheme.h>

#define COLORS_POPUP_WIDTH 300
#define COLORS_POPUP_HEITE 500

typedef struct
{
	int m_SelectedTheme;
	
	uint32_t m_BgColor;
	bool     m_BgIsSolid;
	
	int m_SelectedParm;
	
	uint32_t m_CustomParms[P_THEME_PARM_COUNT];
}
CplColorsData;

extern bool g_GlowOnHover;
extern bool g_TaskListCompact;
extern bool     g_BackgroundSolidColorActive, g_RenderWindowContents;
extern uint32_t g_BackgroundSolidColor;

uint32_t* GetThemingParameters();
uint32_t* GetThemeParms(int themeNumber);

void RefreshScreen();
void RefreshEverything();
void PaintWindowDecorStandard(Rectangle windowRect, const char* pTitle, uint32_t flags, uint32_t privflags, int iconID, bool selected, bool bDrawBorder, bool bDrawTitle);
void WmOnChangedBorderSize();
bool WouldThemeChange(int thNum);

enum
{
	COLORS_ENABLE_BACKGD = 4000,
	COLORS_SHOW_WINDOW_CONTENTS,
	COLORS_APPLY_CHANGES,
	COLORS_OK,
	COLORS_CHANGE_BACKGD,
	COLORS_CHOOSETHEMETEXT,
	COLORS_ORMAKEYOUROWNTEXT,
	COLORS_GLOW_ON_HOVER,
	COLORS_TASKBAR_COMPACT,
	COLORS_CANCEL,
	COLORS_THEMECOMBO,
	COLORS_COLORCOMBO,
	
	COLORS_R_TEXT,
	COLORS_G_TEXT,
	COLORS_B_TEXT,
	COLORS_R_BAR,
	COLORS_G_BAR,
	COLORS_B_BAR,
};

CplColorsData* CplColorsGetData(Window* pWindow)
{
	return (CplColorsData*)pWindow->m_data;
}

static const uint32_t* CplColorsGetThemeBasedOnID(CplColorsData* pData, int id)
{
	if (id < -1)
		return pData->m_CustomParms;
	if (id >= 0)
		return GetThemeParms(id);
	
	return GetThemingParameters();
}

uint32_t CplColorsGetScrollBarColor(Window* pWindow)
{
	uint32_t color = 0;
	
	color |= GetScrollBarPos(pWindow, COLORS_R_BAR) << 16;
	color |= GetScrollBarPos(pWindow, COLORS_G_BAR) << 8;
	color |= GetScrollBarPos(pWindow, COLORS_B_BAR);
	
	return color;
}

void CplColorsSetScrollBarColor(Window* pWindow, uint32_t color)
{
	union
	{
		uint32_t x;
		struct {
			unsigned b : 8;
			unsigned g : 8;
			unsigned r : 8;
			unsigned a : 8;
		};
	}
	col;
	
	col.x = color;
	
	SetScrollBarPos(pWindow, COLORS_R_BAR, col.r);
	SetScrollBarPos(pWindow, COLORS_G_BAR, col.g);
	SetScrollBarPos(pWindow, COLORS_B_BAR, col.b);
	
	for (int i = 0; i < 3; i++)
		CallControlCallback(pWindow, COLORS_R_BAR + i, EVENT_PAINT, 0, 0);
}

void CALLBACK CplColorsWndProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			CplColorsData * pData = CplColorsGetData(pWindow);
			
			// note: this is incredibly hacky... but it works, I think
			cli;
			const uint32_t* pNewParms = CplColorsGetThemeBasedOnID(pData, pData->m_SelectedTheme);
			
			uint32_t* pParms = GetThemingParameters();
			uint32_t ParmsBackup[P_THEME_PARM_COUNT];
			
			memcpy(ParmsBackup, pParms, sizeof ParmsBackup);
			memcpy(pParms, pNewParms, sizeof ParmsBackup);
			
			int y = COLORS_POPUP_HEITE - 40 - 150 - 20;
			
			Rectangle r;
			RECT (r, 10, y+3+10, COLORS_POPUP_WIDTH-20, 150);
			
			DrawEdge(r, DRE_SUNKEN | DRE_FILLED, pData->m_BgColor);
			
			RECT (r, 20, y+3+20, 250-1, 100-1);
			VidFillRectangle(WINDOW_BACKGD_COLOR, r);
			r.right++, r.bottom++;
			PaintWindowDecorStandard(r, "Inactive window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ, 0, ICON_APP_DEMO, false, true, true);
			
			RECT (r, 30, y+3+50, 250-1, 100-1);
			VidFillRectangle(WINDOW_BACKGD_COLOR, r);
			r.right++, r.bottom++;
			
			PaintWindowDecorStandard(r, "Active window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ, 0, ICON_APP_DEMO, true, true, true);
			
			RECT (r, 40, y+3+75, 180-1, 70-1);
			VidFillRectangle(WINDOW_BACKGD_COLOR, r);
			r.right++, r.bottom++;
			
			PaintWindowDecorStandard(r, "Active window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ | WF_FLATBORD, 0, ICON_APP_DEMO, true, true, true);
			
			memcpy(pParms, ParmsBackup, sizeof ParmsBackup);
			
			sti;
			
			break;
		}
		case EVENT_CREATE:
		{
			pWindow->m_iconID = ICON_HAND;//TODO: ICON_CRAYONS
			CplColorsData * pData = 
			pWindow->m_data = MmAllocate(sizeof(CplColorsData));
			memset(pWindow->m_data, 0, sizeof(CplColorsData));
			
			pData->m_SelectedTheme = -1;
			pData->m_SelectedParm  = P_BLACK;
			pData->m_BgIsSolid = g_BackgroundSolidColorActive;
			pData->m_BgColor   = GetThemingParameter(P_BACKGROUND_COLOR);
			
			//add a button
			Rectangle r;
			RECT(r,10, 15, COLORS_POPUP_WIDTH - 150, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Solid color background", COLORS_ENABLE_BACKGD, g_BackgroundSolidColorActive, 0);
			RECT(r,COLORS_POPUP_WIDTH-80, 10, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON,   r, "Change...", COLORS_CHANGE_BACKGD, 0, 0);
			RECT(r,10, 35, COLORS_POPUP_WIDTH - 20, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show window contents while moving", COLORS_SHOW_WINDOW_CONTENTS, g_RenderWindowContents, 0);
			RECT(r,10, 55, COLORS_POPUP_WIDTH - 20, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Make buttons glow when hovering over them", COLORS_GLOW_ON_HOVER, g_GlowOnHover, 0);
			RECT(r,10, 75, COLORS_POPUP_WIDTH - 20, 15);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Make task bar buttons compact", COLORS_TASKBAR_COMPACT, g_TaskListCompact, 0);
			
			RECT(r, 10, 100, COLORS_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Choose a default theme:", COLORS_CHOOSETHEMETEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED);
			
			RECT(r, 10, 120, COLORS_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_COMBOBOX, r, NULL, COLORS_THEMECOMBO, 0, 0);
			
			for (int i = TH_DEFAULT; i < TH_MAX; i++)
			{
				ComboBoxAddItem(pWindow, COLORS_THEMECOMBO, GetThemeName(i), i, 0);
			}
			
			RECT(r, 5, 150, COLORS_POPUP_WIDTH - 10, COLORS_POPUP_HEITE - 40 - 150 - 180);
			AddControl(pWindow, CONTROL_SURROUND_RECT, r, "Custom Color Scheme", COLORS_ORMAKEYOUROWNTEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED);
			
			RECT(r, 5, COLORS_POPUP_HEITE - 40 - 170, COLORS_POPUP_WIDTH - 10, 170);
			AddControl(pWindow, CONTROL_SURROUND_RECT, r, "Preview", COLORS_ORMAKEYOUROWNTEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED);
			
			RECT(r, 10, 165, COLORS_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_COMBOBOX, r, NULL, COLORS_COLORCOMBO, 0, 0);
			
			RECT(r,(COLORS_POPUP_WIDTH-230)/2, COLORS_POPUP_HEITE - 30,70,20);
			AddControl(pWindow, CONTROL_BUTTON, r, "OK",     COLORS_OK, 0, 0);
			RECT(r,(COLORS_POPUP_WIDTH-230)/2+80, COLORS_POPUP_HEITE - 30 ,70,20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", COLORS_CANCEL, 0, 0);
			RECT(r,(COLORS_POPUP_WIDTH-230)/2+160, COLORS_POPUP_HEITE - 30 ,70,20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Apply",  COLORS_APPLY_CHANGES, 0, 0);
			
			static struct {
				const char* m_name;
				int m_id;
			}
			s_ColorComboItems[] =
			{
				{ "(Select a color)", P_BLACK },
				{ "Background", P_BACKGROUND_COLOR },
				{ "3D object background", P_BUTTON_MIDDLE_COLOR },
				{ "Application background", P_WINDOW_BACKGD_COLOR },
				{ "Window border", P_WINDOW_BORDER_COLOR },
				{ "Active title bar 1", P_WINDOW_TITLE_ACTIVE_COLOR },
				{ "Active title bar 2", P_WINDOW_TITLE_ACTIVE_COLOR_B },
				{ "Inactive title bar 1", P_WINDOW_TITLE_INACTIVE_COLOR },
				{ "Inactive title bar 2", P_WINDOW_TITLE_INACTIVE_COLOR_B },
				{ "Window title text", P_WINDOW_TITLE_TEXT_COLOR },
				{ "Window title text shadow", P_WINDOW_TITLE_TEXT_COLOR_SHADOW },
				{ "Window text", P_WINDOW_TEXT_COLOR },
				{ "Window text (light)", P_WINDOW_TEXT_COLOR_LIGHT },
				{ "Selected menu item edge", P_SELECTED_MENU_ITEM_COLOR_B },
				{ "Selected menu item fill", P_SELECTED_MENU_ITEM_COLOR },
				{ "3D object shine", P_BUTTON_HILITE_COLOR },
				{ "3D object shadow", P_BUTTON_SHADOW_COLOR },
				{ "3D object surrounding edge", P_BUTTON_EDGE_COLOR },
				{ "3D object hovering", P_BUTTON_HOVER_COLOR },
				{ "Menu bar selection", P_MENU_BAR_SELECTION_COLOR },
				{ "Selected list item background", P_SELECTED_ITEM_COLOR },
				{ "Selected list item text", P_SELECTED_TEXT_COLOR },
				{ "List background color", P_LIST_BACKGD_COLOR },
				{ "Tooltip background", P_TOOLTIP_BACKGD_COLOR },
				{ "Tooltip text", P_TOOLTIP_TEXT_COLOR },
				{ "Scroll bar background", P_SCROLL_BAR_BACKGD_COLOR },
				{ "Selected menu item text", P_SELECTED_MENU_ITEM_TEXT_COLOR },
				{ "Unselected menu item text", P_DESELECTED_MENU_ITEM_TEXT_COLOR },
				{ "Table view odd row", P_TABLE_VIEW_ALT_ROW_COLOR },
				{ "3D object extreme shadow", P_BUTTON_XSHADOW_COLOR },
				{ "Caption button icon", P_CAPTION_BUTTON_ICON_COLOR },
			};
			
			for (size_t i = 0; i < ARRAY_COUNT(s_ColorComboItems); i++)
			{
				ComboBoxAddItem(pWindow, COLORS_COLORCOMBO, s_ColorComboItems[i].m_name, s_ColorComboItems[i].m_id, 0);
			}
			
			// In the 'custom color scheme' rect, add three scroll bars that adjust the R, G, B components of the current color.
			static const char* s_ColorComponentText[] = {
				"R", "G", "B",
			};
			int height = GetLineHeight() + 10;
			for (int i = 0; i < 3; i++)
			{
				int x = 10;
				int y = 190 + height * i;
				
				RECT(r, x, y, 15, height);
				AddControl(pWindow, CONTROL_TEXTCENTER, r, s_ColorComponentText[i], COLORS_R_TEXT + i, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED);
				
				RECT(r, x + 10, y + (height - SCROLL_BAR_WIDTH) / 2, COLORS_POPUP_WIDTH - 10 - x - 10, SCROLL_BAR_WIDTH);
				AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, COLORS_R_BAR + i, 256, 0);
			}
			
			break;
		}
		case EVENT_COMBOSELCHANGED:
		{
			CplColorsData* pData = CplColorsGetData(pWindow);
			
			if (parm1 == COLORS_THEMECOMBO)
			{
				int item = ComboBoxGetSelectedItemID(pWindow, COLORS_THEMECOMBO);
				
				if (pData->m_SelectedTheme != item)
				{
					pData->m_SelectedTheme = item;
					pData->m_BgColor = GetThemeParms(item)[P_BACKGROUND_COLOR];
					
					CplColorsWndProc(pWindow, EVENT_PAINT, 0, 0);
				}
			}
			else if (parm1 == COLORS_COLORCOMBO)
			{
				int selectedItem = ComboBoxGetSelectedItemID(pWindow, COLORS_COLORCOMBO);
				
				if (pData->m_SelectedParm == P_BLACK)
				{
					// make sure this is not our first time
					if (pData->m_SelectedTheme != -2)
					{
						// copy the parameters from the selected theme over
						const uint32_t* pThm = CplColorsGetThemeBasedOnID(pData, pData->m_SelectedTheme);
						
						memcpy(pData->m_CustomParms, pThm, sizeof pData->m_CustomParms);
						
						// note: we don't need to do a repaint here, it's already painted correctly and it'd be redundant.
						pData->m_SelectedTheme = -2;
					}
				}
				else if (pData->m_SelectedParm != selectedItem)
				{
					// commit the change
					pData->m_CustomParms[pData->m_SelectedParm] = CplColorsGetScrollBarColor(pWindow);
				}
				
				// set the new selected parm
				pData->m_SelectedParm = selectedItem;
				
				// load the color in
				CplColorsSetScrollBarColor(pWindow, pData->m_CustomParms[pData->m_SelectedParm]);
			}
			
			break;
		}
		case EVENT_SCROLLDONE:
		{
			CplColorsData* pData = CplColorsGetData(pWindow);
			
			if (pData->m_SelectedTheme < -1)
			{
				pData->m_CustomParms[pData->m_SelectedParm] = CplColorsGetScrollBarColor(pWindow);
				
				CplColorsWndProc(pWindow, EVENT_PAINT, 0, 0);
			}
			
			break;
		}
		case EVENT_CHECKBOX:
		{
			CplColorsData* pData = CplColorsGetData(pWindow);
			
			pData->m_BgIsSolid = CheckboxGetChecked(pWindow, COLORS_ENABLE_BACKGD);
			
			break;
		}
		case EVENT_COMMAND:
		{
			CplColorsData* pData = CplColorsGetData(pWindow);
			if (parm1 == COLORS_CHANGE_BACKGD)
			{
				uint32_t data = ColorInputBox(pWindow, "Choose a new background color:", "Background color");
				if (data != TRANSPARENT)
				{
					//SetThemingParameter(P_BACKGROUND_COLOR, data & 0xffffff);
					pData->m_BgColor = data & 0xFFFFFF;
					
					CplColorsWndProc(pWindow, EVENT_PAINT, 0, 0);
				}
				break;
			}
			else if (parm1 == COLORS_APPLY_CHANGES || parm1 == COLORS_OK)
			{
				bool bBgChanged = (g_BackgroundSolidColorActive != pData->m_BgIsSolid) || (GetThemingParameter(P_BACKGROUND_COLOR) != pData->m_BgColor);
				
				g_BackgroundSolidColorActive = pData->m_BgIsSolid;
				g_RenderWindowContents       = CheckboxGetChecked(pWindow, COLORS_SHOW_WINDOW_CONTENTS);
				g_GlowOnHover                = CheckboxGetChecked(pWindow, COLORS_GLOW_ON_HOVER);
				g_TaskListCompact            = CheckboxGetChecked(pWindow, COLORS_TASKBAR_COMPACT);
				
				int item = pData->m_SelectedTheme;//ComboBoxGetSelectedItemID(pWindow, COLORS_THEMECOMBO);
				
				bool bRefresh = true;
				if (item != -1 && WouldThemeChange(item))
				{
					int borderSize = BORDER_SIZE, titleBarHeight = TITLE_BAR_HEIGHT;
					
					if (item >= 0)
					{
						// use our shortcut function
						ApplyTheme(item);
					}
					else if (item < -1)
					{
						// do it manually
						for (int i = P_BLACK + 1; i < P_THEME_PARM_COUNT; i++)
							SetThemingParameter(i, pData->m_CustomParms[i]);
					}
					
					if (borderSize != BORDER_SIZE || titleBarHeight != TITLE_BAR_HEIGHT)
						WmOnChangedBorderSize();
					
					bRefresh = true;
				}
				if (bBgChanged)
				{
					bRefresh = true;
				}
				
				SetThemingParameter(P_BACKGROUND_COLOR, pData->m_BgColor);
				
				RefreshEverything();
			}
			
			if (parm1 == COLORS_OK || parm1 == COLORS_CANCEL)
				DestroyWindow(pWindow);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void CplColors(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Colors",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		COLORS_POPUP_WIDTH,
		COLORS_POPUP_HEITE,
		CplColorsWndProc,
		WF_NOMINIMZ
	);
}
