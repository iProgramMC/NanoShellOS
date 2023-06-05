/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
             Clock application

             Main source file
******************************************/

#include <nsstandard.h>

#define VERSION_BUTTON_OK_COMBO 0x1000

#define CLOCK_WIDTH   (200)
#define CLOCK_HEIGHT  (200)

int cosa[]={    0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,   0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995,-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105 };
int sina[]={-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105,   0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,    0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995 };

int nextUpdateIn = 0;
TimeStruct g_time;

#define min(a,b) ((a)<(b)?(a):(b))

void DrawHand (int deg, int len, int cenX, int cenY, unsigned color)
{
	int begPointX = cenX,                            begPointY = cenY;
	int endPointX = cenX + (cosa[deg] * len / 1000), endPointY = cenY + (sina[deg] * len / 1000);
	VidDrawLine (color, begPointX, begPointY, endPointX, endPointY);
}

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			//calculate stuff
			int windowWidth  = GetScreenSizeX()-4, windowHeight = GetScreenSizeY()-4;
			int centerX = windowWidth / 2 + 2, centerY = windowHeight / 2 + 2;
			int diameter = min (windowWidth, windowHeight);
			int handMaxLength = (2 * diameter / 5);
			
			//undraw old g_time
			DrawHand(g_time.hours % 12 * 5 + g_time.minutes / 12, 2 * handMaxLength / 3, centerX, centerY, WINDOW_BACKGD_COLOR);
			DrawHand(g_time.minutes,                            6 * handMaxLength / 9, centerX, centerY, WINDOW_BACKGD_COLOR);
			DrawHand(g_time.seconds,                            8 * handMaxLength / 9, centerX, centerY, WINDOW_BACKGD_COLOR);
			
			//read in new g_time
			g_time = *GetTime();
			
			//re-draw new g_time
			DrawHand(g_time.hours % 12 * 5 + g_time.minutes / 12, 4 * handMaxLength / 9, centerX, centerY, 0xFF0000);
			DrawHand(g_time.minutes,                            6 * handMaxLength / 9, centerX, centerY, 0x000000);
			DrawHand(g_time.seconds,                            8 * handMaxLength / 9, centerX, centerY, 0x000000);
			
			//draw surrounding circle:
			int len1 = handMaxLength, len2 = 10 * handMaxLength / 9;
			for (int i = 0; i < 60; i++)
			{
				unsigned col;
				if (i % 5 == 0)
					len2 = 10 * handMaxLength / 9, col = 0xFFFFFF;
				else
					len2 = 4  * handMaxLength / 5, col = 0x7F7F7F;
				
				int begPointX = centerX + (cosa[i] * len1 / 1000), begPointY = centerY + (sina[i] * len1 / 1000);
				int endPointX = centerX + (cosa[i] * len2 / 1000), endPointY = centerY + (sina[i] * len2 / 1000);
				
				VidDrawLine (col, begPointX, begPointY, endPointX, endPointY);
			}
			
			break;
		}
		case EVENT_UPDATE:
		{
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int main()
{
	int screenWidth = GetScreenSizeX(), screenHeight = GetScreenSizeY();
	
	Window* pWindow = CreateWindow (
		"Analog Clock",
		screenWidth  - 100 - CLOCK_WIDTH,
		screenHeight - 100 - CLOCK_HEIGHT,
		CLOCK_WIDTH,
		CLOCK_HEIGHT,
		WndProc,
		WF_NOMAXIMZ | WF_ALWRESIZ
	);
	SetWindowIcon(pWindow, ICON_CLOCK_EMPTY);
	
	if (!pWindow)
		return 1;
	
	//memset (&g_time, 0, sizeof(g_time));
	
	while (HandleMessages (pWindow))
	{
		if (nextUpdateIn <= GetTickCount())
		{
			RegisterEventInsideWndProc(pWindow, EVENT_PAINT, 0, 0);
			nextUpdateIn = GetTickCount() + 1000;
		}
	}
	
	return 0;
}

