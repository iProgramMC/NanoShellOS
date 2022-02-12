/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        BigText Application module
******************************************/


#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>

enum
{
	NOTHING,
	NOTEP_TEXTVIEW,
};

void NotepadOpenFile (Window* pWindow, const char*pFileName)
{
	//SetTextInputText(pWindow, int comboID, pText)
	int fd = FiOpen (pFileName, O_RDONLY);
	if (fd < 0)
	{
		//no file
		char buff[1024];
		sprintf(buff, "Can't open '%s'!", pFileName);
		MessageBox(pWindow, buff, "NotepadOpenFile", MB_OK | ICON_ERROR << 16);
		return;
	}
	else
	{
		// get the size
		int fileSize = FiTellSize (fd);
		
		// allocate a buffer
		char* buffer = MmAllocate (fileSize + 1);
		buffer[fileSize] = 0;
		FiRead (fd, buffer, fileSize);
		
		// close the file
		FiClose (fd);
		
		// load the file into the box
		SetTextInputText(pWindow, NOTEP_TEXTVIEW, buffer);
		
		// internally this function copies the text anyway, so free it here:
		MmFree (buffer);
	}
}

#define NOTEP_WIDTH  800
#define NOTEP_HEIGHT 600

void CALLBACK BigTextWndProc (Window* pWindow, int msg, int parm1, int parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             36
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ NOTEP_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ NOTEP_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			AddControl (pWindow, CONTROL_TEXTINPUT, r, NULL, NOTEP_TEXTVIEW, 1 | 2, 0);
			
			NotepadOpenFile (pWindow, "/hello2.txt");
			
			break;
		}
		case EVENT_PAINT:
			
			break;
		default:
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
	}
}

void BigTextEntry (UNUSED int arg)
{
	Window *pWindow = CreateWindow ("Big Text", 50, 50, NOTEP_WIDTH, NOTEP_HEIGHT, BigTextWndProc, 0);
	
	if (!pWindow) {
		LogMsg("Could not create window.");
		return;
	}
	
	while (HandleMessages (pWindow));
}