/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
           Raycaster application

             Main source file
******************************************/
#include <nsstandard.h>
#include "game.h"

Window* g_pWindow = NULL;

void RequestRepaint(Window* pWindow);

int ScreenWidth = 640, ScreenHeight = 240;

uint32_t* g_screenBitmap = NULL;
Image   g_GameImage;
VBEData g_GameData;

//sub-millis timing?
const char* GetGameName();
void Update (int deltaTime);
void Render (int deltaTime);
void Init   ();

bool gKeyboardState[128];

bool IsKeyDown (int keyCode)
{
	return gKeyboardState [keyCode];
}
double sin(double x)
{
    double result;
    __asm__ volatile("fsin" : "=t"(result) : "0"(x));
    return result;
}
double sqrt(double x)
{
    double result;
    __asm__ volatile("fsqrt" : "=t"(result) : "0"(x));
    return result;
}
double cos(double x)
{
    return sin(x + PI / 2.0);
}
bool SetupGameVbeData()
{
	//create a framebuffer object
	g_screenBitmap = malloc (sizeof (uint32_t) * ScreenWidth * ScreenHeight);
	if (!g_screenBitmap)
	{
		LogMsg ("Cannot allocate screen framebuffer!");
		return false;
	}
	
	// setup the game VBEData
	g_GameData.m_available = true;
	g_GameData.m_width     = ScreenWidth;
	g_GameData.m_height    = ScreenHeight;
	g_GameData.m_bitdepth  = 2;
	g_GameData.m_pitch     = ScreenWidth * sizeof (uint32_t);
	g_GameData.m_pitch16   = ScreenWidth * sizeof (uint32_t) / 2;
	g_GameData.m_pitch32   = ScreenWidth;
	g_GameData.m_framebuffer32 = g_screenBitmap;
	g_GameData.m_clipRect.left = 
	g_GameData.m_clipRect.top  = 0;
	g_GameData.m_clipRect.right  = ScreenWidth;
	g_GameData.m_clipRect.bottom = ScreenHeight;
	
	// setup the game image s.t. we can draw it easily
	g_GameImage.width       = ScreenWidth;
	g_GameImage.height      = ScreenHeight;
	g_GameImage.framebuffer = g_screenBitmap;
	
	return true;
}
void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Init();
			break;
		}
		case EVENT_PAINT:
		case EVENT_UPDATE:
		{
			VidBlitImageResize(&g_GameImage, 3, 2 + TITLE_BAR_HEIGHT, ScreenWidth,ScreenHeight);
			break;
		}
		case EVENT_SIZE:
		{
			if (g_GameImage.framebuffer)
				free((void*)g_GameImage.framebuffer);
			
			ScreenWidth  = GET_X_PARM(parm1) - 6;
			ScreenHeight = GET_Y_PARM(parm1) - 6 - TITLE_BAR_HEIGHT;
			
			if (!SetupGameVbeData())
				// Bye!
				DefaultWindowProc(pWindow, EVENT_DESTROY, 0, 0);
			
			OnSize (ScreenWidth, ScreenHeight);
			break;
		}
		case EVENT_KEYRAW:
		{
			unsigned char key_code = (unsigned char) parm1;
			
			if (key_code != 0xE0)
			{
				gKeyboardState[key_code & 0x7F] = !(key_code & 0x80);
			}
			break;
		}
		default:
			DefaultWindowProc (pWindow, messageType, parm1, parm2);
			break;
	}
}

static bool SetupWindow (const char* TITLE)
{
	Window* pWindow = CreateWindow (TITLE, 200,200, ScreenWidth+6, ScreenHeight + 6 + TITLE_BAR_HEIGHT, WndProc, WF_ALWRESIZ);
	if (!pWindow)
	{
		LogMsg ("Could not create window");
		return false;
	}
	
	if (!SetupGameVbeData())
		return false;
	
	g_pWindow = pWindow;
	
	return true;
}

void CleanUp()
{
	//...
}

int NsMain (UNUSED int argc, UNUSED char **argv)
{
	if (!SetupWindow(GetGameName()))
		return 1;
	
	int nextTickIn;
	int lastTC = GetTickCount();
	bool fix = false;
	while (1)
	{
		int target = 16 + fix;
		fix ^= 1;
		
		VidSetVbeData(&g_GameData);
		
		// update game
		int deltaTime = GetTickCount() - lastTC;
		
		lastTC = GetTickCount();
		Update (deltaTime);
		
		// render game
		Render (deltaTime);
		
		VidSetVbeData(NULL);
		
		nextTickIn = GetTickCount() + target - (GetTickCount()-lastTC);
		
		// repaint the window
		//RequestRepaint (g_pWindow);
		RegisterEvent (g_pWindow, EVENT_UPDATE, 0, 0);
		
		// Handle window messages.
		bool result = HandleMessages (g_pWindow);
		if (!result)
		{
			g_pWindow = NULL;
			break;
		}
		
		while (nextTickIn > GetTickCount());
	}
	
	CleanUp();
	
	return 0;
}
