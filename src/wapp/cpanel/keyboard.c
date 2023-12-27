/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

     Control panel - Keyboard Applet
******************************************/

#include <wbuiltin.h>
#include <keyboard.h>

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

void CALLBACK CplKeyboardWndProc(Window* pWindow, int messageType, long parm1, long parm2)
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

void CplKeyboard(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Keyboard",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		KEYBD_POPUP_WIDTH,
		KEYBD_POPUP_HEITE,
		CplKeyboardWndProc,
		WF_NOMINIMZ
	);
	
	/*
	char buff[2048];
	sprintf (buff, 
		"Keyboard: %s\n"
		"Driver Name: %s",
		
		"Generic 101/102 Key PS/2 Keyboard HID device",
		"NanoShell Basic PS/2 Keyboard Driver",
		GetScreenWidth(), GetScreenHeight()
	);
	MessageBox(pWindow, buff, "Keyboard info", MB_OK | ICON_KEYBOARD << 16);
	*/
}
