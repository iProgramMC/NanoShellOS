/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Control panel Application module
******************************************/

#include <wbuiltin.h>
#include <wterm.h>
#include <wtheme.h>
#include <vfs.h>
#include <elf.h>
#include <keyboard.h>

extern VBEData g_mainScreenVBEData, *g_vbeData;
extern bool    g_ps2MouseAvail;
extern void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow);

extern bool     g_BackgroundSolidColorActive, g_RenderWindowContents;
extern uint32_t g_BackgroundSolidColor;

void RedrawEverything();
//POPUP WINDOWS: Set `pWindow->m_data` to anything to exit.
#define MOUSE_POPUP_WINDOW 1//stubbed out for now because it's buggy as hell
#define KEYBD_POPUP_WINDOW 1
#define DESKT_POPUP_WINDOW 1
#define NTERM_POPUP_WINDOW 1
#define TSKBR_POPUP_WINDOW 1

#if MOUSE_POPUP_WINDOW
	enum
	{
		MOUSEP_SPEED_SCROLL = 1000,
		MOUSEP_KEYBOARD_CONTROL_MOUSE,
	};
	int  GetMouseSpeedMultiplier();
	void SetMouseSpeedMultiplier(int spd);
	#define MOUSE_POPUP_WIDTH 200
	#define MOUSE_POPUP_HEITE 70
	void CALLBACK Cpl$MousePopupWndProc(Window* pWindow, int messageType, int parm1, int parm2)
	{
		switch (messageType)
		{
			case EVENT_CREATE:
			{
				pWindow->m_iconID = ICON_MOUSE;//TODO
				
				//add a button
				Rectangle r;
				RECT(r,8,8,MOUSE_POPUP_WIDTH-16,35);
				AddControl(pWindow, CONTROL_SURROUND_RECT, r, "Mouse tracking speed", 1, 0, 0);
				{
					//add stuff inside the rect.
					//this scope has no actual reason for its existence other than to mark that stuff we add here goes inside the rect above.
					
					RECT(r, 16,  24, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Slow", 2, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, MOUSE_POPUP_WIDTH - 40, 24, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Fast", 3, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, 50,  22, MOUSE_POPUP_WIDTH - 100, 1);
					AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, MOUSEP_SPEED_SCROLL, (0)<<16|(4), (1)<<16|(GetMouseSpeedMultiplier()));
				}
				
				break;
			}
			case EVENT_COMMAND:
				DestroyWindow(pWindow);
				break;
			case EVENT_RELEASECURSOR:
				SetMouseSpeedMultiplier(GetScrollBarPos(pWindow, MOUSEP_SPEED_SCROLL));
				break;
			default:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
		}
	}
#endif
#if DESKT_POPUP_WINDOW
	enum
	{
		DESKTOP_ENABLE_BACKGD = 4000,
		DESKTOP_SHOW_WINDOW_CONTENTS,
		DESKTOP_APPLY_CHANGES,
		DESKTOP_CHANGE_BACKGD,
		DESKTOP_CHOOSETHEMETEXT,
		DESKTOP_ORMAKEYOUROWNTEXT,
		DESKTOP_GLOW_ON_HOVER,
		DESKTOP_TASKBAR_COMPACT,
		
		DESKTOP_CANCEL,
		
		DESKTOP_THEME_DEFAULT = 5000,
		DESKTOP_THEME_DARK,
	};
	extern bool g_GlowOnHover;
	extern bool g_TaskListCompact;
	void SetDefaultTheme();
	void SetDarkTheme   ();
	#define DESKTOP_POPUP_WIDTH 600
	#define DESKTOP_POPUP_HEITE 400
	void PaintWindowBorderStandard(Rectangle windowRect, const char* pTitle, uint32_t flags, uint32_t privflags, int iconID, bool selected, bool maximized);
	void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);
	void WmOnChangedBorderSize();
	void CALLBACK Cpl$DesktopPopupWndProc(Window* pWindow, int messageType, int parm1, int parm2)
	{
		switch (messageType)
		{
			case EVENT_PAINT:
			{
				Rectangle r;
				RECT (r, DESKTOP_POPUP_WIDTH/2+10, 3+10, DESKTOP_POPUP_WIDTH/2-20, 150);
				RenderButtonShapeSmallInsideOut      (r, 0xBFBFBF, 0x808080, BACKGROUND_COLOR);
				
				RECT (r, DESKTOP_POPUP_WIDTH/2+20, 3+20, 250-1, 100-1);
				VidFillRectangle(WINDOW_BACKGD_COLOR, r);
				r.right++, r.bottom++;
				PaintWindowBorderStandard            (r, "Inactive window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ, 0, ICON_APP_DEMO, false, false);
				
				RECT (r, DESKTOP_POPUP_WIDTH/2+30, 3+50, 250-1, 100-1);
				VidFillRectangle(WINDOW_BACKGD_COLOR, r);
				r.right++, r.bottom++;
				
				PaintWindowBorderStandard            (r, "Active window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ, 0, ICON_APP_DEMO, true, false);
				
				RECT (r, DESKTOP_POPUP_WIDTH/2+40, 3+75, 180-1, 70-1);
				VidFillRectangle(WINDOW_BACKGD_COLOR, r);
				r.right++, r.bottom++;
				
				PaintWindowBorderStandard            (r, "Active window", WF_NOCLOSE | WF_NOMINIMZ | WF_NOMAXIMZ | WF_FLATBORD, 0, ICON_APP_DEMO, true, false);
				break;
			}
			case EVENT_CREATE:
			{
				pWindow->m_iconID = ICON_DESKTOP;//TODO
				
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
				
				RECT(r, 10, 95, DESKTOP_POPUP_WIDTH/2 - 20, 15);
				AddControl(pWindow, CONTROL_TEXTCENTER, r, "Choose a default theme:", DESKTOP_CHOOSETHEMETEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED);
				
				#define THEMES_PER_ROW 4
				
				#define ADD_THEME(themenum) \
					RECT(r, \
					     10 + ((DESKTOP_POPUP_WIDTH/2 - 20) * ((themenum) % THEMES_PER_ROW) / THEMES_PER_ROW),\
						 115 + ((themenum) / THEMES_PER_ROW) * 25,\
						 (DESKTOP_POPUP_WIDTH/2 - 20-5*THEMES_PER_ROW) / THEMES_PER_ROW,\
						 20);\
					AddControl(pWindow, CONTROL_BUTTON, r, GetThemeName(themenum), DESKTOP_THEME_DEFAULT + (themenum), 0, 0);
				
				ADD_THEME(TH_DEFAULT);
				ADD_THEME(TH_DARK);
				ADD_THEME(TH_REDMOND);
				ADD_THEME(TH_CALM);
				ADD_THEME(TH_BLACK);
				//ADD_THEME(TH_WHITE);
				ADD_THEME(TH_ROSE);
				ADD_THEME(TH_DESERT);
				ADD_THEME(TH_RAINYDAY);
				
				RECT(r, 10, 195, DESKTOP_POPUP_WIDTH/2 - 20, 15);
				AddControl(pWindow, CONTROL_TEXTCENTER, r, "Or make your own: (TODO)", DESKTOP_ORMAKEYOUROWNTEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED);
				
				RECT(r,(DESKTOP_POPUP_WIDTH/2-200)/2, DESKTOP_POPUP_HEITE - 30 ,95,20);
				AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", DESKTOP_CANCEL, 0, 0);
				RECT(r,(DESKTOP_POPUP_WIDTH/2-200)/2+95, DESKTOP_POPUP_HEITE - 30 ,95,20);
				AddControl(pWindow, CONTROL_BUTTON, r, "OK",  DESKTOP_APPLY_CHANGES, 0, 0);
				
				break;
			}
			case EVENT_COMMAND:
				if (parm1 == DESKTOP_CHANGE_BACKGD)
				{
					uint32_t data = ColorInputBox(pWindow, "Choose a new background color:", "Background color");
					if (data != TRANSPARENT)
					{
						SetThemingParameter(P_BACKGROUND_COLOR, data & 0xffffff);
						RedrawEverything();
					}
					break;
				}
				else if (parm1 >= DESKTOP_THEME_DEFAULT)
				{
					int borderSize = BORDER_SIZE;
					pWindow->m_data = (void*)borderSize;
					
					ApplyTheme(parm1 - DESKTOP_THEME_DEFAULT);
					
					if ((uint32_t)borderSize != BORDER_SIZE)
					{
						WmOnChangedBorderSize();
					}
					
					RedrawEverything();
					break;
				}
				else if (parm1 == DESKTOP_APPLY_CHANGES)
				{
					g_BackgroundSolidColorActive = CheckboxGetChecked(pWindow, DESKTOP_ENABLE_BACKGD);
					g_RenderWindowContents       = CheckboxGetChecked(pWindow, DESKTOP_SHOW_WINDOW_CONTENTS);
					g_GlowOnHover                = CheckboxGetChecked(pWindow, DESKTOP_GLOW_ON_HOVER);
					g_TaskListCompact            = CheckboxGetChecked(pWindow, DESKTOP_TASKBAR_COMPACT);
					RedrawEverything();
				}
				DestroyWindow(pWindow);
				break;
			case EVENT_RELEASECURSOR:
				
				break;
			default:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
		}
	}
#endif
#if KEYBD_POPUP_WINDOW
	enum
	{
		KEYBDP_REPEAT_CPS = 1000,
		KEYBDP_REPEAT_DELAY,
		KEYBDP_OK_BUTTON,
		KEYBDP_CANCEL_BUTTON,
		KEYBDP_TEST_BOX,
	};
	#define KEYBD_POPUP_WIDTH 400
	#define KEYBD_POPUP_HEITE 230
	
	uint8_t g_oldTypematicRepeatRate, g_oldTypematicRepeatDelay;
	void CALLBACK Cpl$KeybdPopupWndProc(Window* pWindow, int messageType, int parm1, int parm2)
	{
		switch (messageType)
		{
			case EVENT_CREATE:
			{
				pWindow->m_iconID = ICON_KEYBOARD;//TODO
				
				g_oldTypematicRepeatRate  = GetKeyboardProperty(KBPROPERTY_REPEAT_FREQUENCY);
				g_oldTypematicRepeatDelay = GetKeyboardProperty(KBPROPERTY_DELAY_BEFORE_REPEAT);
				
				//add a button
				Rectangle r;
				RECT(r,8,8,KEYBD_POPUP_WIDTH-16,180);
				AddControl(pWindow, CONTROL_SURROUND_RECT, r, "Character Repeat", 1, 0, 0);
				{
					//add stuff inside the rect.
					//this scope has no actual reason for its existence other than to mark that stuff we add here goes inside the rect above.
					
					RECT(r, 16,  34, 32, 32);
					AddControl(pWindow, CONTROL_ICON, r, NULL, 12, ICON_KEYB_REP_SPEED, 0);
					RECT(r, 66,  34, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Repeat rate:", 2, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, 66,  54, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Slow", 2, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, KEYBD_POPUP_WIDTH - 40, 54, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Fast", 3, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, 100, 52, KEYBD_POPUP_WIDTH - 150, 1);
					AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, KEYBDP_REPEAT_CPS,
						(0)<<16|(GetKeyboardProperty(KBPROPERTY_REPEAT_FREQUENCY_MAX)),
						(1)<<16|(GetKeyboardProperty(KBPROPERTY_REPEAT_FREQUENCY_MAX)-1-g_oldTypematicRepeatRate)
					);
					
					RECT(r, 16,  15+80, 32, 32);
					AddControl(pWindow, CONTROL_ICON, r, NULL, 22, ICON_KEYB_REP_DELAY, 0);
					RECT(r, 66,  15+80, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Repeat delay:", 2, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, 66,  35+80, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Slow", 2, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, KEYBD_POPUP_WIDTH - 40, 35+80, 32, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Fast", 3, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, 100, 33+80, KEYBD_POPUP_WIDTH - 150, 1);
					AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, KEYBDP_REPEAT_DELAY,
						(0)<<16|(GetKeyboardProperty(KBPROPERTY_DELAY_BEFORE_REPEAT_MAX)),
						(1)<<16|(GetKeyboardProperty(KBPROPERTY_DELAY_BEFORE_REPEAT_MAX)-1-g_oldTypematicRepeatDelay)
					);
					
					RECT(r, 16, 145, KEYBD_POPUP_WIDTH - 100, 20);
					AddControl(pWindow, CONTROL_TEXT, r, "Click here and hold down a key to test repeat rate:", 10000, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
					RECT(r, 16, 155, KEYBD_POPUP_WIDTH - 50, 20);
					AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, 10000, 0, 0);
				}
				RECT(r,(KEYBD_POPUP_WIDTH-160)/2,KEYBD_POPUP_HEITE - 30,75,20);
				AddControl(pWindow, CONTROL_BUTTON, r, "Revert", KEYBDP_CANCEL_BUTTON, 0, 0);
				RECT(r,(KEYBD_POPUP_WIDTH-160)/2+80,KEYBD_POPUP_HEITE - 30,75,20);
				AddControl(pWindow, CONTROL_BUTTON, r, "Apply",  KEYBDP_OK_BUTTON,     0, 0);
				/*
				RECT(r,8,8+80,KEYBD_POPUP_WIDTH-16,80);
				AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, KEYBDP_TEST_BOX, 0, 0);
				SetTextInputText(pWindow, KEYBDP_TEST_BOX, "Test");
				*/
				break;
			}
			
			case EVENT_COMMAND:
				if (parm1 != KEYBDP_CANCEL_BUTTON)
				{
					if (parm1 != KEYBDP_OK_BUTTON)
						break;
					
					//ok button:
					messageType = EVENT_CLOSE;
					//fallthrough
				}
				else
				{
					//cancel button:
					SetKeyboardProperty(KBPROPERTY_DELAY_BEFORE_REPEAT,  g_oldTypematicRepeatDelay);
					SetKeyboardProperty(KBPROPERTY_REPEAT_FREQUENCY,     g_oldTypematicRepeatRate);
					FlushKeyboardProperties();
					messageType = EVENT_CLOSE;
					//fallthrough
				}
				//fallthrough
			case EVENT_CLOSE:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
			
			case EVENT_RELEASECURSOR:
				SetKeyboardProperty(KBPROPERTY_DELAY_BEFORE_REPEAT,  GetKeyboardProperty(KBPROPERTY_DELAY_BEFORE_REPEAT_MAX)-1-GetScrollBarPos(pWindow, KEYBDP_REPEAT_DELAY));
				SetKeyboardProperty(KBPROPERTY_REPEAT_FREQUENCY,     GetKeyboardProperty(KBPROPERTY_REPEAT_FREQUENCY_MAX)   -1-GetScrollBarPos(pWindow, KEYBDP_REPEAT_CPS));
				FlushKeyboardProperties();
				break;
			default:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
		}
	}
#endif

#if NTERM_POPUP_WINDOW
	extern int g_TerminalFont;
	enum
	{
		NTERM_FONT_LIST = 500,
		NTERM_FONT_SURR,
		NTERM_ALLOW_SCROLLBACK,
		NTERM_APPLY_CHANGES,
		NTERM_CHANGED_TEXT,
	};
	#define NTERM_POPUP_WIDTH 600
	#define NTERM_POPUP_HEITE 400
	#define NTERM_POPUP_HEIT1 300
	
	uint8_t g_oldTypematicRepeatRate, g_oldTypematicRepeatDelay;
	void CALLBACK Cpl$NTermPopupWndProc(Window* pWindow, int messageType, int parm1, int parm2)
	{
		switch (messageType)
		{
			case EVENT_CREATE:
			{
				pWindow->m_iconID = ICON_COMMAND;
				
				pWindow->m_data = (void*)g_TerminalFont;
				
				Rectangle r;
				RECT(r, 10, 10, NTERM_POPUP_WIDTH - 20, NTERM_POPUP_HEIT1 - 45);
				AddControl(pWindow, CONTROL_SURROUND_RECT, r, "Terminal font", NTERM_FONT_SURR, 0, 0);
				RECT(r, 20, 25, NTERM_POPUP_WIDTH/2 - 40, NTERM_POPUP_HEIT1 - 70);
				AddControl(pWindow, CONTROL_LISTVIEW, r, NULL, NTERM_FONT_LIST, 0, 0);
				
				RECT(r, r.left, r.bottom + 20, 250, 20);
				AddControl(pWindow, CONTROL_CHECKBOX, r, "Allow scrollback (not implemented yet)", NTERM_ALLOW_SCROLLBACK, 0, 0);
				
				RECT(r, (NTERM_POPUP_WIDTH-45) / 2, NTERM_POPUP_HEITE-30, 45, 20);
				AddControl(pWindow, CONTROL_BUTTON, r, "Apply",  NTERM_APPLY_CHANGES, 0, 0);
				
				RECT(r, (NTERM_POPUP_WIDTH-120) / 2, NTERM_POPUP_HEITE-50, 120, 20);
				AddControl(pWindow, CONTROL_TEXTCENTER, r, "",  NTERM_CHANGED_TEXT, 0, TEXTSTYLE_VCENTERED | TEXTSTYLE_HCENTERED);
				
				// Add a list of built-in fonts
				for (int i = FONT_TAMSYN_REGULAR; i < FONT_LAST; i++)
				{
					AddElementToList(pWindow, NTERM_FONT_LIST, VidGetFontName(i), ICON_FONTS);
				}
				
				break;
			}
			case EVENT_COMMAND:
				if (parm1 == NTERM_FONT_LIST)
				{
					SetLabelText(pWindow, NTERM_CHANGED_TEXT, "Your settings changes will not apply to already open terminals, only NEW terminal windows.");
					
					pWindow->m_data = (void*)parm2;
					RequestRepaintNew(pWindow);
				}
				else if (parm1 == NTERM_APPLY_CHANGES)
				{
					g_TerminalFont = (int)pWindow->m_data;
					DestroyWindow(pWindow);
				}
				break;
			case EVENT_PAINT:
			{
				//Paint a little console window to show the user what they picked
				Rectangle r;
				//draw around the window
				RECT(r, NTERM_POPUP_WIDTH/2, 25, NTERM_POPUP_WIDTH/2-20, NTERM_POPUP_HEIT1-70);
				VidFillRectangle(0x000000, r);
				r.top++, r.bottom--, r.left++, r.right--;//Shrink the rect by 1
				VidDrawRectangle(0xFFFFFF, r);
				r.top++, r.bottom--, r.left++, r.right--;//Shrink the rect by 1 again
				
				//Draw the "title bar" mock
				r.bottom = r.top + 12;
				VidFillRectangle(WINDOW_TITLE_ACTIVE_COLOR, r);
				
				VidTextOut("Window preview", r.left + 1, r.top + 1, 0xFFFFFF, TRANSPARENT);
				
				RECT(r, NTERM_POPUP_WIDTH/2+5, 40, NTERM_POPUP_WIDTH/2-30, NTERM_POPUP_HEIT1-85);
				VidSetFont((int)pWindow->m_data);
				
				char buffer[1024];
				sprintf(
					buffer,
					"Selected font: %s\nThis is a font preview.  The quick brown fox jumps over the lazy dog.\n\nEach character is:\n%3d screen pixels wide\n%3d screen pixels high\n%s",
					VidGetFontName((int)pWindow->m_data),
					GetCharWidth('W'),
					GetLineHeight(),
					(int)pWindow->m_data == FONT_BASIC ? "\n\nMonospace fonts are recommended instead." : ""
				);
				
				VidDrawText(buffer, r, TEXTSTYLE_WORDWRAPPED, 0xFFFFFF, 0x000000);
				VidSetFont(SYSTEM_FONT);
				break;
			}
			case EVENT_CLOSE:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
			
			default:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
		}
	}
#endif

#if TSKBR_POPUP_WINDOW
	enum
	{
		TASKBAR_POPUP_DATECHECK = 1,
		TASKBAR_POPUP_TIMECHECK,
		TASKBAR_POPUP_APPLY,
		TASKBAR_POPUP_CANCEL,
	};
	#define TSKBR_POPUP_WIDTH 200
	#define TSKBR_POPUP_HEITE 140
	
	extern bool g_bShowDate, g_bShowTimeSeconds;
	void TaskbarSetProperties(bool bShowDate, bool bShowTimeSecs);
	
	void CALLBACK Cpl$TaskbarPopupWndProc(Window* pWindow, int messageType, int parm1, int parm2)
	{
		switch (messageType)
		{
			case EVENT_CREATE:
			{
				pWindow->m_iconID = ICON_HOME;
				
				Rectangle r;
				
				RECT(r, 10, 10, TSKBR_POPUP_WIDTH - 20, 20);
				AddControl(pWindow, CONTROL_CHECKBOX, r, "Show the date on the task bar", TASKBAR_POPUP_DATECHECK, g_bShowDate, 0);
				
				RECT(r, 10, 30, TSKBR_POPUP_WIDTH - 20, 20);
				AddControl(pWindow, CONTROL_CHECKBOX, r, "Show the seconds on the clock on the task bar", TASKBAR_POPUP_TIMECHECK, g_bShowTimeSeconds, 0);
				
				RECT(r, (TSKBR_POPUP_WIDTH - 160) / 2,     TSKBR_POPUP_HEITE - 30, 75, 20);
				AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", TASKBAR_POPUP_CANCEL, 0, 0);
				RECT(r, (TSKBR_POPUP_WIDTH - 160) / 2 + 80,TSKBR_POPUP_HEITE - 30, 75, 20);
				AddControl(pWindow, CONTROL_BUTTON, r, "OK",     TASKBAR_POPUP_APPLY,  0, 0);
				
				break;
			}
			case EVENT_COMMAND:
			{
				switch (parm1)
				{
					case TASKBAR_POPUP_CANCEL:
						Cpl$TaskbarPopupWndProc(pWindow, EVENT_CLOSE, 0, 0);
						break;
					case TASKBAR_POPUP_APPLY:
						TaskbarSetProperties(CheckboxGetChecked(pWindow, TASKBAR_POPUP_DATECHECK), CheckboxGetChecked(pWindow, TASKBAR_POPUP_TIMECHECK));
						Cpl$TaskbarPopupWndProc(pWindow, EVENT_CLOSE, 0, 0);
						break;
				}
				break;
			}
			case EVENT_PAINT:
			{
				break;
			}
			default:
				DefaultWindowProc(pWindow, messageType, parm1, parm2);
				break;
		}
	}
#endif

enum {
	CONTPNL_LISTVIEW = 0x10,
	CONTPNL_MENUBAR  = 0xFE,
};

int state=0;
void KbSetLedStatus(uint8_t status);

void PopupWindow(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags);
void CALLBACK Cpl$WndProc (Window* pWindow, int messageType, int parm1, int parm2)
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
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, CONTPNL_MENUBAR, 0, 0);
			
			// Add some testing elements to the menubar.  A comboID of zero means you're adding to the root.
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 0, 1, "Settings");
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 0, 2, "Help");
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 1, 3, "Exit");
			AddMenuBarItem (pWindow, CONTPNL_MENUBAR, 2, 4, "About Control Panel...");
			
			// Add a icon list view.
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (5)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ 400 - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ 260 - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			AddControlEx(pWindow, CONTROL_ICONVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, CONTPNL_LISTVIEW, 0, 0);
			
			// Add list items:
			ResetList(pWindow, CONTPNL_LISTVIEW);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Display",             ICON_ADAPTER);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Keyboard",            ICON_KEYBOARD);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Mouse",               ICON_MOUSE);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Desktop",             ICON_DESKTOP);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Terminal settings",   ICON_COMMAND);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Launcher",            ICON_HOME);
			/*
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Environment Paths",   ICON_DIRECTIONS);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Permissions",         ICON_RESTRICTED);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Serial Port",         ICON_SERIAL);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Download over Serial",ICON_BILLBOARD);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Date and Time",       ICON_CLOCK);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "Password Lock",       ICON_LOCK);
			AddElementToList(pWindow, CONTPNL_LISTVIEW, "App Memory Limit",    ICON_RESMON);
			*/
			
			break;
		}
		case EVENT_COMMAND: {
			if (parm1 == CONTPNL_MENUBAR)
			{
				switch (parm2)
				{
					case 3:
						DestroyWindow(pWindow);
						break;
					case 4:
						LaunchVersion();
						break;
				}
			}
			else if (parm1 == CONTPNL_LISTVIEW)
			{
				switch (parm2)
				{
					//NOTE: These are just mock for now.
					case 0:
					{
						char buff[2048];
						sprintf (buff, 
							"Display: %s\n"
							"Driver Name: %s\n\n"
							"Screen Size: %d x %d\n\n"
							"Framebuffer map address: 0x%X",
							
							"Generic VESA VBE-capable device",
							"NanoShell Basic VBE Display Driver",
							GetScreenWidth(), GetScreenHeight(),
							g_mainScreenVBEData.m_framebuffer32
						);
						MessageBox(pWindow, buff, "Display adapter info", MB_OK | ICON_ADAPTER << 16);
						break;
					}
					#if KEYBD_POPUP_WINDOW
					case 1:
					{
						PopupWindow(
							pWindow,
							"Keyboard",
							pWindow->m_rect.left + 50,
							pWindow->m_rect.top  + 50,
							KEYBD_POPUP_WIDTH,
							KEYBD_POPUP_HEITE,
							Cpl$KeybdPopupWndProc,
							WF_NOMINIMZ
						);
						
						break;
					}
					#else
					case 1:
					{
						char buff[2048];
						sprintf (buff, 
							"Keyboard: %s\n"
							"Driver Name: %s",
							
							"Generic 101/102 Key PS/2 Keyboard HID device",
							"NanoShell Basic PS/2 Keyboard Driver",
							GetScreenWidth(), GetScreenHeight()
						);
						MessageBox(pWindow, buff, "Keyboard info", MB_OK | ICON_KEYBOARD << 16);
						break;
					}
					#endif
					#if MOUSE_POPUP_WINDOW
					case 2:
					{
						PopupWindow(
							pWindow,
							"Mouse",
							pWindow->m_rect.left + 50,
							pWindow->m_rect.top  + 50,
							MOUSE_POPUP_WIDTH,
							MOUSE_POPUP_HEITE,
							Cpl$MousePopupWndProc,
							WF_NOMINIMZ
						);
						
						break;
					}
					#endif
					#if DESKT_POPUP_WINDOW
					case 3:
					{
						PopupWindow(
							pWindow,
							"Desktop",
							pWindow->m_rect.left + 50,
							pWindow->m_rect.top  + 50,
							DESKTOP_POPUP_WIDTH,
							DESKTOP_POPUP_HEITE,
							Cpl$DesktopPopupWndProc,
							WF_NOMINIMZ
						);
						
						break;
					}
					#endif
					#if NTERM_POPUP_WINDOW
					case 4:
					{
						PopupWindow(
							pWindow,
							"Terminal settings",
							pWindow->m_rect.left + 50,
							pWindow->m_rect.top  + 50,
							NTERM_POPUP_WIDTH,
							NTERM_POPUP_HEITE,
							Cpl$NTermPopupWndProc,
							WF_NOMINIMZ
						);
						
						break;
					}
					#endif
					#if TSKBR_POPUP_WINDOW
					case 5:
					{
						PopupWindow(
							pWindow,
							"Task Bar",
							pWindow->m_rect.left + 50,
							pWindow->m_rect.top  + 50,
							TSKBR_POPUP_WIDTH,
							TSKBR_POPUP_HEITE,
							Cpl$TaskbarPopupWndProc,
							WF_NOMINIMZ
						);
						
						break;
					}
					#endif
					default:
						MessageBox(pWindow, "Not Implemented!", "Control Panel", MB_OK | ICON_WARNING << 16);
						break;
				}
			}
			else
				LogMsg("Unknown command event.  Parm1: %d Parm2: %d", parm1, parm2);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void ControlEntry(__attribute__((unused)) int arg)
{
	// create ourself a window:
	int ww = 400, wh = 260;
	int wx = 150, wy = 150;
	
	Window* pWindow = CreateWindow ("Control Panel", wx, wy, ww, wh, Cpl$WndProc, WF_ALWRESIZ);//WF_NOCLOSE);
	pWindow->m_iconID = ICON_FOLDER_SETTINGS;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
