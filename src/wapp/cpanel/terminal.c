/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

     Control panel - Terminal Applet
******************************************/

#include <wbuiltin.h>
#include <wterm.h>

#define NTERM_POPUP_WIDTH 600
#define NTERM_POPUP_HEITE 400
#define NTERM_POPUP_HEIT1 300

extern int g_TerminalFont;

enum
{
	NTERM_FONT_LIST = 500,
	NTERM_FONT_SURR,
	NTERM_ALLOW_SCROLLBACK,
	NTERM_APPLY_CHANGES,
	NTERM_CHANGED_TEXT,
};

void CALLBACK CplTerminalWndProc(Window* pWindow, int messageType, int parm1, int parm2)
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

void CplTerminal(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Terminal settings",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		NTERM_POPUP_WIDTH,
		NTERM_POPUP_HEITE,
		CplTerminalWndProc,
		WF_NOMINIMZ
	);
}
