#include <nanoshell.h>
#include <window.h>

#define VERSION_BUTTON_OK_COMBO 0x1000

void LoadFileList()
{
	
}

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			// File browser: will be added manually.
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
	Window* pWindow = CreateWindow ("File Cabinet", 150, 150, 320, 115 + TITLE_BAR_HEIGHT, WndProc, 0);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}


