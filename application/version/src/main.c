#include <nanoshell.h>
#include <window.h>

#define VERSION_BUTTON_OK_COMBO 0x1000
void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			Rectangle r;
			RECT(r, 0, TITLE_BAR_HEIGHT, 320, 20);
			
			//parm1 is the button number that we're being fed in EVENT_COMMAND
			AddControl (pWindow, CONTROL_TEXTCENTER, r, "NanoShell Operating System 3rd Edition", 1, 0, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED);
			
			RECT(r, 0, TITLE_BAR_HEIGHT+20, 320, 50);
			AddControl (pWindow, CONTROL_ICON, r, NULL, 2, ICON_NANOSHELL, 0);
			
			RECT(r, 0, TITLE_BAR_HEIGHT+70, 320, 10);
			AddControl (pWindow, CONTROL_TEXTCENTER, r, "Copyright (C) 2019-2022, iProgramInCpp", 3, 0, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED);
			
			RECT(r, (320-70)/2, TITLE_BAR_HEIGHT+85, 70, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "OK", VERSION_BUTTON_OK_COMBO, 0, 0);
			
			break;
		}
		case EVENT_COMMAND:
			if (parm1 == VERSION_BUTTON_OK_COMBO)
			{
				DestroyWindow(pWindow);
			}
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int main ()
{
	Window* pWindow = CreateWindow ("NanoShell Version Applet", 150, 150, 320, 115 + TITLE_BAR_HEIGHT, WndProc, 0);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}


