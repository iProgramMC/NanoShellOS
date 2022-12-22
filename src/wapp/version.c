/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Version Application module
******************************************/

#include <wbuiltin.h>

#define VERSION_BUTTON_OK_COMBO 0x1000
void CALLBACK VersionProgramProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE: {
			//add a predefined list of controls:
			Rectangle r;
			RECT(r, 0, TITLE_BAR_HEIGHT + 5, 320, 20);
			
			char buffer [1024];
			
			int icon = ICON_NANOSHELL;
			if (pWindow->m_data)
			{
				const char* pText = (const char*)pWindow->m_data + 4;
				if (*pText)
				{
					sprintf (buffer, "NanoShell %s", (const char*)pWindow->m_data + 4);
				}
				else
				{
					strcpy (buffer, "NanoShell Operating System " VersionString);
				}
				
				icon = *((int*)pWindow->m_data);
				
				MmFree (pWindow->m_data);
				pWindow->m_data = NULL;
			}
			else
			{
				strcpy (buffer, "NanoShell Operating System " VersionString);
			}
			
			//parm1 is the button number that we're being fed in EVENT_COMMAND
			AddControl (pWindow, CONTROL_TEXTCENTER, r, buffer, 1, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED);
			
			RECT(r, 0, TITLE_BAR_HEIGHT+20, 320, 50);
			AddControl (pWindow, CONTROL_ICON, r, NULL, 2, icon, 0);
			
			RECT(r, 0, TITLE_BAR_HEIGHT+75, 320, 10);
			AddControl (pWindow, CONTROL_TEXTCENTER, r, "Copyright (C) 2019-2022, iProgramInCpp", 3, WINDOW_TEXT_COLOR, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED);
			
			RECT(r, (320-70)/2, TITLE_BAR_HEIGHT+95, 70, 20);
			AddControl (pWindow, CONTROL_BUTTON, r, "OK", VERSION_BUTTON_OK_COMBO, 0, 0);
			
			break;
		}
		case EVENT_PAINT: {
			break;
		}
		case EVENT_COMMAND: {
			if (parm1 == VERSION_BUTTON_OK_COMBO)
			{
				DestroyWindow(pWindow);
			}
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void VersionProgramTask (int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("NanoShell", 100, 100, 320, 125 + TITLE_BAR_HEIGHT, VersionProgramProc, WF_NOMINIMZ);
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created. Why?");
		return;
	}
	
	pWindow->m_iconID = ICON_NANOSHELL;
	pWindow->m_data   = (void*)argument;
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
