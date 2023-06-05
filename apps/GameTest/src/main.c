/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
           GameTest application

             Main source file
******************************************/

#include <nsstandard.h>

Window* g_pWindow = NULL;

void RequestRepaint(Window* pWindow);

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

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

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			VidBlitImageResize(&g_GameImage, 0, 0, SCREEN_WIDTH,SCREEN_HEIGHT);
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
	Window* pWindow = CreateWindow (TITLE, CW_AUTOPOSITION, CW_AUTOPOSITION, SCREEN_WIDTH, SCREEN_HEIGHT, WndProc, 0);
	if (!pWindow)
	{
		LogMsg ("Could not create window");
		return false;
	}
	
	//create a framebuffer object
	g_screenBitmap = malloc (sizeof (uint32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
	if (!g_screenBitmap)
	{
		LogMsg ("Cannot allocate screen framebuffer!");
		return false;
	}
	
	// setup the game VBEData
	g_GameData.m_available = true;
	g_GameData.m_width     = SCREEN_WIDTH;
	g_GameData.m_height    = SCREEN_HEIGHT;
	g_GameData.m_bitdepth  = 2;
	g_GameData.m_pitch     = SCREEN_WIDTH * sizeof (uint32_t);
	g_GameData.m_pitch16   = SCREEN_WIDTH * sizeof (uint32_t) / 2;
	g_GameData.m_pitch32   = SCREEN_WIDTH;
	g_GameData.m_framebuffer32 = g_screenBitmap;
	g_GameData.m_clipRect.left = 
	g_GameData.m_clipRect.top  = 0;
	g_GameData.m_clipRect.right  = SCREEN_WIDTH;
	g_GameData.m_clipRect.bottom = SCREEN_HEIGHT;
	
	// setup the game image s.t. we can draw it easily
	g_GameImage.width       = SCREEN_WIDTH;
	g_GameImage.height      = SCREEN_HEIGHT;
	g_GameImage.framebuffer = g_screenBitmap;
	
	g_pWindow = pWindow;
	
	return true;
}

void CleanUp()
{
	//...
}

int main()
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
		RequestRepaint (g_pWindow);
		
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
