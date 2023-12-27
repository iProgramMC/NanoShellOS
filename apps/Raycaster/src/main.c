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

int ScreenWidth = 854 /* 640 */, ScreenHeight = 240;

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

// useful double functions
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
    double result;
    __asm__ volatile("fcos" : "=t"(result) : "0"(x));
    return result;
}

//trims off the bit over 1
//so if you have a number like -1.007, it would return -0.007, and if you
//have a number like 1455.4559, it would return 0.4559
double Trim(double b)
{
	int intpart = (int)(b);
	return b - intpart;
}

double Abs (double b)
{
	if (b < 0) return -b;
	return b;
}

//shamelessly stolen from https://github.com/managarm/mlibc/blob/master/options/ansi/musl-generic-math/fmod.c#L4
// NOTE: fpclassify always returns exactly one of those constants
// However making them bitwise disjoint simplifies isfinite() etc.
int Classify(double x)
{
	union {double f; uint64_t i;} u = {x};
	int e = u.i >> 52 & 0x7ff;
	if (!e)
		return u.i << 1 ? FP_SUBNORMAL : FP_ZERO;
	if (e == 0x7ff)
		return u.i << 12 ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}
double fmod(double x, double y)
{
	union {double f; uint64_t i;} ux = {x}, uy = {y};
	int ex = ux.i>>52 & 0x7ff;
	int ey = uy.i>>52 & 0x7ff;
	int sx = ux.i>>63;
	uint64_t i;

	/* in the followings uxi should be ux.i, but then gcc wrongly adds */
	/* float load/store to inner loops ruining performance and code size */
	uint64_t uxi = ux.i;

	if (uy.i<<1 == 0 || isnan(y) || ex == 0x7ff)
		return (x*y)/(x*y);
	if (uxi<<1 <= uy.i<<1) {
		if (uxi<<1 == uy.i<<1)
			return 0*x;
		return x;
	}

	/* normalize x and y */
	if (!ex) {
		for (i = uxi<<12; i>>63 == 0; ex--, i <<= 1);
		uxi <<= -ex + 1;
	} else {
		uxi &= -1ULL >> 12;
		uxi |= 1ULL << 52;
	}
	if (!ey) {
		for (i = uy.i<<12; i>>63 == 0; ey--, i <<= 1);
		uy.i <<= -ey + 1;
	} else {
		uy.i &= -1ULL >> 12;
		uy.i |= 1ULL << 52;
	}

	/* x mod y */
	for (; ex > ey; ex--) {
		i = uxi - uy.i;
		if (i >> 63 == 0) {
			if (i == 0)
				return 0*x;
			uxi = i;
		}
		uxi <<= 1;
	}
	i = uxi - uy.i;
	if (i >> 63 == 0) {
		if (i == 0)
			return 0*x;
		uxi = i;
	}
	for (; uxi>>52 == 0; uxi <<= 1, ex--);

	/* scale result */
	if (ex > 0) {
		uxi -= 1ULL << 52;
		uxi |= (uint64_t)ex << 52;
	} else {
		uxi >>= -ex + 1;
	}
	uxi |= (uint64_t)sx << 63;
	ux.i = uxi;
	return ux.f;
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
void CALLBACK WndProc (Window* pWindow, int messageType, long parm1, long parm2)
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
			VidBlitImageResize(&g_GameImage, 0, 0, ScreenWidth,ScreenHeight);
			break;
		}
		case EVENT_SIZE:
		{
			if (g_GameImage.framebuffer)
				free((void*)g_GameImage.framebuffer);
			
			ScreenWidth  = GET_X_PARM(parm1);
			ScreenHeight = GET_Y_PARM(parm1);
			
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
	Window* pWindow = CreateWindow (TITLE, 200,200, ScreenWidth, ScreenHeight, WndProc, WF_ALWRESIZ);
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
