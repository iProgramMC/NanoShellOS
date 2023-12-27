/*****************************************
		NanoShell Operating System
	   (C) 2022-2023 iProgramInCpp

        Cabinet Application module
******************************************/
#include "cabinet.h"

// TODO: Bring back the mount feature

void CabinetMountRamDisk(Window *pwnd, UNUSED const char *pfn)
{
	/*
	int fd = FiOpen (pfn, O_RDONLY);
	if (fd < 0)
	{
		MessageBox(pwnd, "Could not open that file!  Try another.", "Mount error", MB_OK | ICON_ERROR << 16);
	}
	else
	{
		int length = FiTellSize(fd);
		
		char* pData = (char*)MmAllocate(length + 1);
		pData[length] = 0;
		
		FiRead(fd, pData, length);
		
		FiClose(fd);
		
		KeVerifyInterruptsEnabled;
		cli;
		
		FsMountRamDisk(pData);
		
		KeVerifyInterruptsDisabled;
		sti;
	}
	*/
	
	// Sorry to disappoint, this probably won't ever actually be implemented
	MessageBox(pwnd, "This feature was removed.", "Cabinet", MB_OK | ICON_ERROR << 16);
}

void CALLBACK CabinetMountWindowProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			
			RECT(r, 10, 10, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, 1, ICON_CABINET_COMBINE, 0);
			
			RECT(r, 50, 15, 150, 32);
			AddControl(pWindow, CONTROL_TEXT, r, "Type in the file name of a drive you want to mount.", 3, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECT(r, 450 - 80, 10, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Mount", 2, 0, 0);
			RECT(r, 450 - 80, 40, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", 5, 0, 0);
			
			RECT(r, 50, 30,300, 20);
			AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, 4, 0, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == 2)
			{
				//Mount something
				
				const char* s = TextInputGetRawText(pWindow, 4);
				char buffer[2048];
				sprintf(buffer, "Would you like to mount the file '%s' as a read-only file system?", s);
				if (MessageBox (pWindow, buffer, pWindow->m_title, ICON_CABINET_COMBINE << 16 | MB_YESNO) == MBID_YES)
				{
					OnBusy (pWindow);
					CabinetMountRamDisk(pWindow, s);
					OnNotBusy(pWindow);
				}
			}
			if (parm1 == 2 || parm1 == 5)
				DestroyWindow(pWindow);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void PopupUserMountWindow(Window* pWindow)
{
	PopupWindow(pWindow, "Mount a RAM drive", pWindow->m_rect.left + 50, pWindow->m_rect.top + 50, 450, 90, CabinetMountWindowProc, WF_NOCLOSE | WF_NOMINIMZ);
}
