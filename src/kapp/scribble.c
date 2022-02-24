/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Scribble Application module
******************************************/

#include <wbuiltin.h>

int g_paint1X = -1, g_paint1Y = -1;
void CALLBACK PrgPaintProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
			g_paint1X = g_paint1Y = -1;
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
		case EVENT_PAINT:
			//VidFillRect (0xFF00FF, 10, 40, 100, 120);
			//VidTextOut ("Hey, it's the window :)", 50, 50, TRANSPARENT, 0xe0e0e0);
			break;
		case EVENT_CLICKCURSOR:
		case EVENT_MOVECURSOR:
			if (g_paint1X == -1)
			{
				VidPlotPixel(GET_X_PARM(parm1), GET_Y_PARM(parm1), parm1);
			}
			else
			{
				VidDrawLine(parm1, g_paint1X, g_paint1Y, GET_X_PARM(parm1), GET_Y_PARM(parm1));
			}
			g_paint1X = GET_X_PARM(parm1);
			g_paint1Y = GET_Y_PARM(parm1);
			break;
		case EVENT_RELEASECURSOR:
			g_paint1X = g_paint1Y = -1;
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void PrgPaintTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("Scribble!", 200, 300, 500, 400, PrgPaintProc, 0);
	
	if (!pWindow)
		DebugLogMsg("Hey, the window couldn't be created");
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
