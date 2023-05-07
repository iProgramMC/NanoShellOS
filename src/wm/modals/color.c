/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Modals - ColorPicker dialog
******************************************/
#include <window.h>
#include <widget.h>
#include <wbuiltin.h>
#include <icon.h>

extern SafeLock
g_CreateLock, 
g_BackgdLock;
extern VBEData* g_vbeData, g_mainScreenVBEData;
extern void WmPaintWindowTitle(Window* pWindow);
extern void SelectWindow(Window* pWindow);
extern void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2);

const uint32_t g_DefaultPickColors[] = {
	0x000000,0x00007F,0x007F00,0x007F7F,0x7F0000,0x7F007F,0x7F7F00,0x7F7F7F,
	0xa0a0a0,0x0000FF,0x00FF00,0x00FFFF,0xFF0000,0xFF00FF,0xFFFF00,0xFFFFFF,
};

const char* g_DefaultPickColorsText[] = {
	"Black",  "D-Blue", "Green", "Turq", "D-Red", "Purple",  "D-Yellow", "Grey",
	"D-Grey", "Blue",   "Lime",  "Cyan", "Red",   "Magenta", "Yellow",   "White",
};

STATIC_ASSERT(ARRAY_COUNT(g_DefaultPickColors) == ARRAY_COUNT(g_DefaultPickColorsText), "Change the other array too if adding colors");

void CALLBACK ColorPopupProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND)
	{
		//Which button did we click?
		if (parm1 >= 900 && parm1 < 900+(int)ARRAY_COUNT(g_DefaultPickColors))
		{
			SetScrollBarPos( pWindow, 100, ( g_DefaultPickColors[ parm1 - 900 ] >> 16 ) & 0xff );
			SetScrollBarPos( pWindow, 101, ( g_DefaultPickColors[ parm1 - 900 ] >>  8 ) & 0xff );
			SetScrollBarPos( pWindow, 102, ( g_DefaultPickColors[ parm1 - 900 ] >>  0 ) & 0xff );
		
			SetIcon( pWindow, 2000, g_DefaultPickColors[ parm1 - 900 ] | 0x01000000 );
			
			RequestRepaint( pWindow );
		}
		else if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
		{
			//We clicked a valid button.  Return.
			
			if (parm1 == MBID_CANCEL)
			{
				pWindow->m_data = (void*)TRANSPARENT;
				return;
			}
			
			int rd = GetScrollBarPos(pWindow, 100), gd = GetScrollBarPos(pWindow, 101), bd = GetScrollBarPos(pWindow, 102);
			uint32_t color = 0xF0000000 | rd<<16 | gd<<8 | bd;
			
			pWindow->m_data = (void*)color;
		}
	}
	else if (messageType == EVENT_CLICKCURSOR || messageType == EVENT_RELEASECURSOR)
	{
		int rd = GetScrollBarPos(pWindow, 100), gd = GetScrollBarPos(pWindow, 101), bd = GetScrollBarPos(pWindow, 102);
		uint32_t color = 0x01000000 | rd<<16 | gd<<8 | bd;
		SetIcon(pWindow, 2000, color);
		
		RequestRepaint( pWindow );
	}
	else if (messageType == EVENT_CREATE)
	{
		pWindow->m_fullVbeData.m_dirty = 1;
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
	}
	else if (messageType == EVENT_PAINT || messageType == EVENT_SETFOCUS || messageType == EVENT_KILLFOCUS ||
			 messageType == EVENT_RELEASECURSOR)
	{
		pWindow->m_fullVbeData.m_dirty = 1;
		pWindow->m_renderFinished  = 1;
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
	}
	else
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

#define COLOR_POPUP_WIDTH  500
#define COLOR_POPUP_HEIGHT 260
// Returns TRANSPARENT if the window is cancelled.
uint32_t ColorInputBox(Window* pWindow, const char* pPrompt, const char* pCaption)
{
	/*
	
	  +---------------------------------------------------------------------+
	  |                                                                     |
	  | pPrompt text goes here                                              |
	  |                                                                     |
	  | +-----------------------------------------------------------------+ |
	  | | R | < |                                                     | > | |
	  | +-----------------------------------------------------------------+ |
	  | | G | < |                                                     | > | |
	  | +-----------------------------------------------------------------+ |
	  | | B | < |                                                     | > | |
	  | +-----------------------------------------------------------------+ |
	  |                                                                     |
	  | Or choose one of the default colors:                                |
	  |                                                                     |
	  | [     ] [     ] [     ] [     ] [     ] ...                         |
	  |                                                                     |
	  |                                                                     |
	  |            +----------+                  +----------+               |
	  |            |    OK    |                  |  Cancel  |               |
	  |            +----------+                  +----------+               |
	  |                                                                     |
	  +---------------------------------------------------------------------+
	
	*/
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		wasSelectedBefore = pWindow->m_isSelected;
		if (wasSelectedBefore)
		{
			pWindow->m_isSelected = false;
			WmPaintWindowTitle (pWindow);
		}
	}
	
	VBEData* pBackup = g_vbeData;
	
	VidSetVBEData(NULL);
	// Freeze the current window.
	int old_flags = 0;
	WindowProc pProc;
	if (pWindow)
	{
		pProc = pWindow->m_callback;
		old_flags = pWindow->m_flags;
		pWindow->m_callback = MessageBoxWindowLightCallback;
		pWindow->m_flags |= WF_FROZEN;//Do not respond to user attempts to move/other
	}
	
	int wPosX = (GetScreenWidth()  - COLOR_POPUP_WIDTH)  / 2;
	int wPosY = (GetScreenHeight() - COLOR_POPUP_HEIGHT) / 2;
	// Spawn a new window.
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, COLOR_POPUP_WIDTH, COLOR_POPUP_HEIGHT, ColorPopupProc, WF_NOCLOSE | WF_NOMINIMZ | WI_MESSGBOX);
	
	// Add the basic controls required.
	int y = 10;
	
	Rectangle r;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-10, 25);
	AddControl(pBox, CONTROL_TEXT, r, pPrompt, 203, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
	y += 20;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-96-10, 15);
	AddControl (pBox, CONTROL_TEXT, r, "Red", 200, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
	RECT(r, 96, y, COLOR_POPUP_WIDTH-96-10, 1);
	AddControl (pBox, CONTROL_HSCROLLBAR, r, NULL, 100, 0<<16|256, 1<<16|0);
	y += 32;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-96-10, 15);
	AddControl (pBox, CONTROL_TEXT, r, "Green", 201, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
	RECT(r, 96, y, COLOR_POPUP_WIDTH-96-10, 1);
	AddControl (pBox, CONTROL_HSCROLLBAR, r, NULL, 101, 0<<16|256, 1<<16|0);
	y += 32;
	
	RECT(r, 10, y, COLOR_POPUP_WIDTH-96-10, 15);
	AddControl (pBox, CONTROL_TEXT, r, "Blue", 202, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
	RECT(r, 96, y, COLOR_POPUP_WIDTH-96-10, 1);
	AddControl (pBox, CONTROL_HSCROLLBAR, r, NULL, 102, 0<<16|256, 1<<16|0);
	
	y += 20;
	RECT(r, (COLOR_POPUP_WIDTH-100)/2, y, 100, 15);
	AddControl (pBox, CONTROL_TEXTCENTER, r, "Color Preview", 2000, 0x01000000, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED);
	
	y += 20;
	RECT(r, 10, y, COLOR_POPUP_WIDTH-10, 25);
	AddControl(pBox, CONTROL_TEXT, r, "Or choose one of the default colors:", 203, 0, WINDOW_BACKGD_COLOR);
	y += 10;
	
	RECT(r, (COLOR_POPUP_WIDTH - 250)/2, COLOR_POPUP_HEIGHT - 30, 100, 20);
	AddControl (pBox, CONTROL_BUTTON, r, "Cancel", MBID_CANCEL, 0, 0);
	RECT(r, (COLOR_POPUP_WIDTH - 250)/2+150, COLOR_POPUP_HEIGHT - 30, 100, 20);
	AddControl (pBox, CONTROL_BUTTON, r, "OK", MBID_OK, 0, 0);
	
	int bwidth = (COLOR_POPUP_WIDTH-10)/8, bheight = 25;
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			RECT(r, 10 + j * bwidth, y + i * bheight, bwidth-5, bheight-5);
			AddControl(pBox, CONTROL_BUTTON_COLORED, r, g_DefaultPickColorsText[i<<3|j], 900+(i<<3|j), (1-i)*0xffffff, g_DefaultPickColors[i<<3|j]);
		}
	}
	
	pBox->m_data   = NULL;
	pBox->m_iconID = ICON_NULL;
	
	// Handle messages for this modal dialog window.
	while (HandleMessages(pBox))
	{
		if (pBox->m_data)
		{
			break;//we're done.
		}
	}
	
	uint32_t dataReturned = (uint32_t)pBox->m_data;
	
	DestroyWindow(pBox);
	while (HandleMessages(pBox));
	
	if (pWindow)
	{
		pWindow->m_callback = pProc;
		pWindow->m_flags    = old_flags;
	}
	g_vbeData = pBackup;
	
	//NB: No null dereference, because if pWindow is null, wasSelectedBefore would be false anyway
	if (wasSelectedBefore)
	{
		//pWindow->m_isSelected = true;
		SelectWindow (pWindow);
		WmPaintWindowTitle (pWindow);
	}
	
	return dataReturned;
}
