/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

           Window Manager module
******************************************/

// NOTE ABOUT PROCESSES:
// This _should_ run in the kernel task!!!!
// Why?

// The action queues shall resort to using the unsafe versions if the caller running
// Hide/Show/Nuke/ResizeWindow if it's called by window manager itself (i.e. menus)

#define THREADING_ENABLED 1 //0
#if THREADING_ENABLED
#define MULTITASKED_WINDOW_MANAGER
#endif

void KeTaskDone(void);

#define DebugLogMsg  SLogMsg

#include <window.h>
#include <icon.h>
#include <print.h>
#include <task.h>
#include <widget.h>
#include <misc.h>
#include <keyboard.h>
#include <wbuiltin.h>
#include <wcall.h>
#include <vfs.h>
#include <image.h>
#include <config.h>

#undef cli
#undef sti
#define cli
#define sti

extern Window*  g_focusedOnWindow;

// Window Internal Action Queue
#if 1

static WindowAction s_internal_action_queue[4096];
static int          s_internal_action_queue_head,
                    s_internal_action_queue_tail;
//

bool ActionQueueIsOneMoreFromOverflowingQueue()
{
	if (s_internal_action_queue_tail == 0)
		return s_internal_action_queue_head == 4095;
	return s_internal_action_queue_tail == s_internal_action_queue_head - 1;
}

WindowAction* ActionQueueAdd(WindowAction action)
{
	while (ActionQueueIsOneMoreFromOverflowingQueue())
		KeTaskDone();
	
	WindowAction *pAct = &s_internal_action_queue[s_internal_action_queue_head];
	*pAct = action;
	
	s_internal_action_queue_head = (s_internal_action_queue_head + 1) % 4096;
	
	return pAct;
}

WindowAction* ActionQueueGetFront()
{
	return &s_internal_action_queue[s_internal_action_queue_tail];
}

void ActionQueuePop()
{
	s_internal_action_queue_tail = (s_internal_action_queue_tail + 1) % 4096;
}

void ActionQueueWaitForFrontToFinish()
{
	while (ActionQueueGetFront()->bInProgress);
}

bool ActionQueueEmpty()
{
	return s_internal_action_queue_tail == s_internal_action_queue_head;
}

#endif

//fps counter:
#if 1

int  g_FPS, g_FPSThisSecond, g_FPSLastCounted;
bool g_RenderWindowContents = false;//while moving

void UpdateFPSCounter()
{
	if (g_FPSLastCounted + 1000 <= GetTickCount())
	{
		g_FPS = g_FPSThisSecond;
		g_FPSThisSecond = 0;
		g_FPSLastCounted = GetTickCount();
	}
	g_FPSThisSecond++;
}

int GetWindowManagerFPS()
{
	return g_FPS;
}

#endif

// Theming
#if 1
uint32_t g_ThemingParms[P_THEME_PARM_COUNT];

uint32_t GetThemingParameter(int type)
{
	if (type < 0 || type >= P_THEME_PARM_COUNT) return 0;
	return g_ThemingParms[type];
}
void SetThemingParameter(int type, uint32_t parm)
{
	if (type < 0 || type >= P_THEME_PARM_COUNT) return;
	g_ThemingParms[type] = parm;
}
void SetDefaultTheme(void);

uint8_t* ThmLoadEntireFile (const char *pPath, int *pSizeOut)
{
	int fd = FiOpen (pPath, O_RDONLY);
	if (fd < 0) return NULL;
	
	int size = FiTellSize (fd);
	
	uint8_t* pMem = MmAllocate (size + 1);
	int read_size = FiRead (fd, pMem, size);
	pMem[read_size] = 0;//for text parsers
	
	*pSizeOut = read_size;
	return pMem;
}

void ThmLoadFont(ConfigEntry *pEntry)
{
	//load some properties
	ConfigEntry
	* pBmpPath, * pFntPath, * pSysFont, * pTibFont, * pBmpSize, * pChrHeit;
	
	char buffer1[128], buffer2[128], buffer3[128], buffer4[128], buffer5[128], buffer6[128];
	//TODO: Ensure safety
	sprintf(buffer1, "%s::Bitmap",       pEntry->value);
	sprintf(buffer2, "%s::FontData",     pEntry->value);
	sprintf(buffer3, "%s::SystemFont",   pEntry->value);
	sprintf(buffer4, "%s::TitleBarFont", pEntry->value);
	sprintf(buffer5, "%s::BmpSize",      pEntry->value);
	sprintf(buffer6, "%s::ChrHeight",    pEntry->value);
	
	pBmpPath = CfgGetEntry (buffer1),
	pFntPath = CfgGetEntry (buffer2),
	pSysFont = CfgGetEntry (buffer3),
	pTibFont = CfgGetEntry (buffer4);
	pBmpSize = CfgGetEntry (buffer5);
	pChrHeit = CfgGetEntry (buffer6);
	
	if (!pBmpPath || !pFntPath) return;
	
	bool bSysFont = false, bTibFont = false;
	if (pSysFont)
		bSysFont = strcmp (pSysFont->value, "yes") == 0;
	if (pTibFont)
		bTibFont = strcmp (pTibFont->value, "yes") == 0;
	
	// Load Data
	int nBmpSize = 128;
	if (pBmpSize)
		nBmpSize = atoi (pBmpSize->value);
	int nChrHeit = 16;
	if (pChrHeit)
		nChrHeit = atoi (pChrHeit->value);
	
	int sizeof_bmp = 0, sizeof_fnt = 0;
	uint8_t
	*pBmp = ThmLoadEntireFile (pBmpPath->value, &sizeof_bmp),
	*pFnt = ThmLoadEntireFile (pFntPath->value, &sizeof_fnt);
	
	int font_id = CreateFont (pFnt, pBmp, nBmpSize, nBmpSize, nChrHeit);
	if (bSysFont)
		SetThemingParameter (P_SYSTEM_FONT, font_id);
	if (bTibFont)
		SetThemingParameter (P_TITLE_BAR_FONT, font_id);
}

void ThmLoadExtraFonts()
{
	ConfigEntry *pFontsToLoadEntry = CfgGetEntry ("Theming::FontsToLoad");
	
	if (!pFontsToLoadEntry) return;
	
	//we only support one font for now
	ThmLoadFont(pFontsToLoadEntry);
}

//make sure pOut is initialized first - if the config entry is missing this won't work!
void ThmLoadFromConfig(uint32_t *pOut, const char *pString)
{
	ConfigEntry *pEntry = CfgGetEntry (pString);
	if (!pEntry) return;
	
	uint32_t number = (uint32_t) atoi (pEntry->value);
	
	*pOut = number;
}
void LoadDefaultThemingParms()
{
	SetThemingParameter(P_BLACK, 0x000000);
	
	// Dark mode:
	SetDefaultTheme();
	
	// Load config stuff
	ThmLoadFromConfig(&g_ThemingParms[P_TITLE_BAR_HEIGHT], "Theming::TitleBarHeight");
	ThmLoadExtraFonts();
}
void LoadThemingParmsFromFile(const char* pString)
{
	//TODO
	SLogMsg("TODO: LoadThemingParmsFromFile (\"%s\")", pString);
}
void SaveThemingParmsToFile(const char* pString)
{
	//TODO
	SLogMsg("TODO: SaveThemingParmsToFile (\"%s\")", pString);
}

#endif

// Window effects when you maximize/minimize a window
bool g_EffectRunning;
Rectangle g_EffectDest, g_EffectSrc;
Rectangle g_EffectStep;
int g_NextEffectUpdateIn = 0;
extern VBEData* g_vbeData, g_mainScreenVBEData;
char g_EffectText[1000];

void RefreshPixels(int oldX, int oldY, int oldWidth, int oldHeight);
void KillEffect()
{
	if (!g_EffectRunning) return;
	RefreshPixels (g_EffectSrc.left, g_EffectSrc.top, g_EffectSrc.right - g_EffectSrc.left, g_EffectSrc.bottom - g_EffectSrc.top);
	g_EffectRunning = false;
}
void RunOneEffectFrame()
{
	if (!g_EffectRunning) return;
	if (g_NextEffectUpdateIn < GetTickCount())
	{
		VBEData data = g_mainScreenVBEData;
		VidSetVBEData(&data); // Hack to avoid it also drawing on the clone
		
		g_NextEffectUpdateIn += 10;
		
		int l,t,w,h;
		l = g_EffectSrc.left, t = g_EffectSrc.top, w = g_EffectSrc.right - g_EffectSrc.left, h = g_EffectSrc.bottom - g_EffectSrc.top;
		
		RefreshPixels (l,t,w,h);
		
		// Update
		g_EffectSrc.left    += g_EffectStep.left;
		g_EffectSrc.top     += g_EffectStep.top;
		g_EffectSrc.right   += g_EffectStep.right;
		g_EffectSrc.bottom  += g_EffectStep.bottom;
		
		// Do some boring checks if we've skipped the destrect
		#define EFFSTEPCHECK(dir) \
		if (g_EffectStep.dir < 0)\
		{\
			if (g_EffectSrc.dir < g_EffectDest.dir)\
			{\
				g_EffectSrc.dir = g_EffectDest.dir;\
				g_EffectStep.dir = 0;\
			}\
		}\
		else if (g_EffectStep.dir > 0)\
		{\
			if (g_EffectSrc.dir > g_EffectDest.dir)\
			{\
				g_EffectSrc.dir = g_EffectDest.dir;\
				g_EffectStep.dir = 0;\
			}\
		}
		
		EFFSTEPCHECK(left)
		EFFSTEPCHECK(top)
		EFFSTEPCHECK(right)
		EFFSTEPCHECK(bottom)
		
		// If all effect steps are zero, is there any point in continuing ?
		if (g_EffectStep.left == 0 && g_EffectStep.top == 0 && g_EffectStep.right == 0 && g_EffectStep.bottom == 0)
		{
			KillEffect();
			return;
		}
		
		// Render the effect now
		VidSetClipRect(&g_EffectSrc);
		
		VidFillRectHGradient(
			WINDOW_TITLE_ACTIVE_COLOR,
			WINDOW_TITLE_ACTIVE_COLOR_B,
			g_EffectSrc.left,
			g_EffectSrc.top,
			g_EffectSrc.right,
			g_EffectSrc.bottom
		);
		
		VidSetFont(TITLE_BAR_FONT);
		VidDrawText(g_EffectText, g_EffectSrc, TEXTSTYLE_VCENTERED | TEXTSTYLE_HCENTERED, FLAGS_TOO(TEXT_RENDER_BOLD, WINDOW_TITLE_TEXT_COLOR), TRANSPARENT);
		VidSetFont(SYSTEM_FONT);
		
		VidSetVBEData(NULL);
	}
}
void CreateMovingRectangleEffect(Rectangle src, Rectangle dest, const char* text)
{
	KillEffect();
	g_EffectRunning = true;
	g_EffectDest = dest;
	g_EffectSrc  = src;
	
	// Decide on what stepping variables we should have
	g_EffectStep.left   = (dest.left   - src.left  ) / 16;
	g_EffectStep.top    = (dest.top    - src.top   ) / 16;
	g_EffectStep.right  = (dest.right  - src.right ) / 16;
	g_EffectStep.bottom = (dest.bottom - src.bottom) / 16;
	
	g_NextEffectUpdateIn = GetTickCount();
	
	int sl = strlen (text);
	if (sl > 999) sl = 999;
	memcpy(g_EffectText, text, sl + 1);
	g_EffectText[sl] = 0;
}

//background code:
#if 1

#define g_BackgroundSolidColor (GetThemingParameter(BACKGROUND_COLOR))
//uint32_t g_BackgroundSolidColor = BACKGROUND_COLOR;
bool     g_BackgroundSolidColorActive = true;

#define CHECKER_PATTERN
#ifdef CHECKER_PATTERN
	#define B 0x000000,
	#define X 0xFFFFFF,
	#define o 0x7F0000,
	const uint32_t g_placeholderBackground[] = {
	B B B B B B B B B B B B B o
	B B X X X X X X X X X B B o
	B B B B B B B B B B B B B o
	o B X X X X X X X X X B o o
	o B X X X X X X X X X B o o
	o B X X B X B X B X X B o o
	o B X X X B X B X X X B o o
	o B B X X X B X X X B B o o
	o o B B X X X X X B B o o o
	o o o B B X B X B B o o o o
	o o o o B B X B B o o o o o
	o o o o B B X B B o o o o o
	o o o B B X X X B B o o o o
	o o B B X X B X X B B o o o
	o B B X X X X X X X B B o o
	o B X X X X B X X X X B o o
	o B X X X B X B X X X B o o
	o B X X B X B X B X X B o o
	o B X B X B X B X B X B o o
	B B B B B B B B B B B B B o
	B B X X X X X X X X X B B o
	B B B B B B B B B B B B B o
	};
	#undef B
	#undef X
	#undef o
	
	Image* g_background, g_defaultBackground = {
		14, 22, g_placeholderBackground
	};
#else
	const uint32_t g_placeholderBackground[] = {
		BACKGROUND_COLOR,
	};
	
	Image* g_background, g_defaultBackground = {
		1, 1, g_placeholderBackground
	};
#endif
extern uint32_t* g_framebufferCopy;
extern VBEData * g_vbeData;

__attribute__((always_inline))
inline void VidPlotPixelToCopyInlineUnsafeRF(unsigned x, unsigned y, unsigned color)
{
	g_framebufferCopy[x + y * g_vbeData->m_width] = color;
}
__attribute__((always_inline))
inline void VidPlotPixelRaw32IRF (unsigned x, unsigned y, unsigned color)
{
	g_vbeData->m_dirty = 1;
	g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32] = color;
}
short GetWindowIndexInDepthBuffer (int x, int y);
__attribute__((always_inline))
inline void VidPlotPixelInlineRF(unsigned x, unsigned y, unsigned color)
{
	if (!((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()))
	{
		if (GetWindowIndexInDepthBuffer(x, y) < 0)
		{
			VidPlotPixelToCopyInlineUnsafeRF(x, y, color);
			VidPlotPixelRaw32IRF (x, y, color);
		}
	}
}

extern const unsigned char g_BasicFontData[];
extern const unsigned char g_TestFont216x16[];
void WinPlotCharBkgd (char c, unsigned ox, unsigned oy, unsigned colorFg)
{
	VidSetVBEData(NULL);
	bool bold = false;
	if (colorFg & TEXT_RENDER_BOLD)
	{
		bold = true;
	}
	colorFg &= 0xFFFFFF;
	
	//big text?
	/*
	if (c > '~' || c < ' ') c = '?';
	int width = g_TestFont216x16[0], height = g_TestFont216x16[1];
	const unsigned char* testa = (const unsigned char*)(g_TestFont216x16 + 3);
	for (int y = 0; y < height; y++)
	{
		int to = ((c-' ') * height + y)*2;
		unsigned short test1 = testa[to+1]|testa[to]<<8;
		
		for (int x = 0, bitmask = 1; x < width; x++, bitmask <<= 1)
		{
			if (test1 & bitmask)
			{
				VidPlotPixelInlineRF(ox + x, oy + y, colorFg);
				if (bold) VidPlotPixelInlineRF(ox + x + bold, oy + y, colorFg);
			}
		}
	}
	*/
	
	//standard font?
	int width = g_BasicFontData[0], height = g_BasicFontData[1];
	const unsigned char* test = (const unsigned char*)(g_BasicFontData + 3);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0, bitmask = (1 << (width - 1)); x < width; x++, bitmask >>= 1)
		{
			if (test[c * height + y] & bitmask)
			{
				VidPlotPixelInlineRF(ox + x, oy + y, colorFg);
				if (bold) VidPlotPixelInlineRF(ox + x + bold, oy + y, colorFg);
			}
		}
	}
}
enum
{
	JUSTIFY_LEFT,
	JUSTIFY_CENTER,
	JUSTIFY_RIGHT,
};
//really basic, no lf support
void WinRenderTextBkgd(const char* text, int yaxis, int justify, unsigned color)
{
	int text_width = 0;
	const char* text1 = text;
	bool bold = false;
	if (color & TEXT_RENDER_BOLD)
	{
		bold = true;
	}
	
	while (*text1)
	{
		//text_width += GetCharWidth(*text);
		text_width += g_BasicFontData[3 + 256 * g_BasicFontData[1] + (*text1)];
		if (bold) text_width++;
		text1++;
	}
	int xaxis = 0;
	if (justify > JUSTIFY_LEFT)
		xaxis = (GetScreenWidth()-text_width);
	if (justify == JUSTIFY_CENTER)
		xaxis /= 2;
	while (*text)
	{
		WinPlotCharBkgd (*text, xaxis, yaxis, color);
		xaxis += g_BasicFontData[3 + 256 * g_BasicFontData[1] + (*text)];
		if (bold) xaxis++;
		text++;
	}
}
	
extern void VidBlitImageForceOpaque(Image* pImage, int x, int y);
void RedrawBackground (Rectangle rect)
{
	if (g_BackgroundSolidColorActive)
	{
		VidFillRectangle(GetThemingParameter(P_BACKGROUND_COLOR), rect);
		return;
	}
	
	if (rect.left < 0) rect.left = 0;
	if (rect.top  < 0) rect.top  = 0;
	if (rect.right  >= GetScreenWidth ()) rect.right  = GetScreenWidth ()-1;
	if (rect.bottom >= GetScreenHeight()) rect.bottom = GetScreenHeight();
	// if the rectangle is FULLY inside the 0,0 tile:
	// (TODO: Make this work on any tile)
	// (Another TODO: If there's one horz seam or one vert seam, split the main rect into 2 rects across the seam
	//  and call RedrawBackground on them)
	int rlc = rect.left / g_background->width,  rrc = rect.right  / g_background->width;
	int rtc = rect.top  / g_background->height, rbc = rect.bottom / g_background->height;
	if (rlc == rrc && rtc == rbc && rlc == 0 && rtc == 0)
	{
		//just draw the clipped portion
		for (int y = rect.top; y < rect.bottom; y++)
		{
			memcpy_ints (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + rect.left], &g_background->framebuffer[y * g_background->width + rect.left], rect.right-rect.left+1);
			memcpy_ints (&g_framebufferCopy         [y * g_vbeData->m_width   + rect.left], &g_background->framebuffer[y * g_background->width + rect.left], rect.right-rect.left+1);
		}
		
		return;
	}
	
	// Fill in the grid pattern ...
	int grid_left  = (rect.left  - 1 + g_background->width)  / g_background->width;
	int grid_right = (rect.right + 1 - g_background->width)  / g_background->width;
	int grid_top   = (rect.top   - 1 + g_background->height) / g_background->height;
	int grid_bottom= (rect.bottom+ 1 - g_background->height) / g_background->height;
	
	for (int y = grid_top, yi = grid_top * g_background->height; y <= grid_bottom; y++, yi += g_background->height)
	{
		for (int x = grid_left, xi = grid_left * g_background->width; x <= grid_right; x++, xi += g_background->width)
		{
			VidBlitImageForceOpaque (g_background, xi, yi);
		}
	}
	
	// Then fill in the edges.
	int xl = grid_left  * g_background->width;
	int xr = grid_right * g_background->width;
	int yt = grid_top   * g_background->height;
	int yb = grid_bottom* g_background->height;
	
	rect.bottom++, rect.right++;
	for (int y = rect.top; y != rect.bottom; y++)
	{
		int ymod = (y % g_background->height);
		for (int x = rect.left, xa = rect.left % g_background->width; x != rect.right; x++, xa++)
		{
			if (y >= yt && y < yb)
			{
				if (x == xl) 
				{
					x = xr;
					xa = x % g_background->width;
				}
				else if (x >= xr)
					xa = xa % g_background->width;
			}
			else
				xa = xa % g_background->width;
			
			VidPlotPixel (x, y, g_background->framebuffer[xa + g_background->width * ymod]);
		}
	}
	
	//simple background:
	/*VidFillRectangle (BACKGROUND_COLOR, rect);*/
}

void RedrawBackgdDetails()
{
	//WinRenderTextBkgd("New 32-bit NanoShell (NEWX86BLD) "VersionString, 0, JUSTIFY_CENTER, 0xFFFFFF);
	WinRenderTextBkgd("New 32-bit NanoShell (NEWX86BLD) "VersionString, GetScreenHeight()-22, JUSTIFY_RIGHT, 0xFFFFFF);
	WinRenderTextBkgd("For evaluation purposes only.",                  GetScreenHeight()-12, JUSTIFY_RIGHT, 0xFFFFFF);
}

void SetDefaultBackground()
{
	SLogMsg("Loading Wallpaper...");
	g_background = &g_defaultBackground;
	
	//Try to open a file now.
	//WORK: Change the file name here.  Should work
	int fd = FiOpen("/Fat0/penile.bmp", O_RDONLY);
	if (fd < 0)
	{
		SLogMsg("Could not open wallpaper. Using default one!");
		g_BackgroundSolidColorActive = true;
		return;
	}
	
	int size = FiTellSize(fd);
	
	uint8_t*pData=MmAllocate(size);
	if (!pData)
	{
		SLogMsg("Could not allocate %d bytes for wallpaper data... Using default wallpaper!", pData);
		g_BackgroundSolidColorActive = true;
		return;
	}
	
	FiRead(fd, pData, size);
	
	FiClose(fd);
	
	int errorCode = 0;
	Image* pImage = LoadBitmap(pData, &errorCode);
	MmFree(pData);
	
	if (pImage)
	{
		g_background = pImage;
		g_BackgroundSolidColorActive = false;
	}
	else
	{
		SLogMsg("Could not load wallpaper data (errorcode: %d). Using default one!", errorCode);
		g_BackgroundSolidColorActive = true;
	}
}

#endif

//util:
#if 1
bool RectangleContains(Rectangle*r, Point*p) 
{
	return (r->left <= p->x && r->right >= p->x && r->top <= p->y && r->bottom >= p->y);
}

Window g_windows [WINDOWS_MAX];

Window* GetWindowFromIndex(int i)
{
	if (i >= 0x1000) i -= 0x1000;
	return &g_windows[i];
}

bool g_windowManagerRunning = false;

extern ClickInfo g_clickQueue [CLICK_INFO_MAX];
extern int       g_clickQueueSize;
extern bool      g_clickQueueLock;

bool g_windowLock = false;
bool g_screenLock = false;
bool g_bufferLock = false;
bool g_createLock = false;
bool g_backgdLock = false;

bool g_shutdownSentDestroySignals = false;
bool g_shutdownWaiting 			  = false;
bool g_shutdownRequest 			  = false;
bool g_shutdownSentCloseSignals   = false;

void VersionProgramTask(int argument);
void IconTestTask   (int argument);
void PrgPaintTask   (int argument);
#endif

// Window depth buffer
#if 1
short* g_windowDepthBuffer = NULL; //must be allocated
short  g_windowDrawOrder[WINDOWS_MAX];

void ResetWindowDrawOrder()
{
	memset(g_windowDrawOrder, 0xFF, sizeof(g_windowDrawOrder));
}

void AddWindowToDrawOrder(short windowIndex)
{
	memcpy (g_windowDrawOrder, g_windowDrawOrder+1, sizeof(g_windowDrawOrder)-sizeof(short));
	g_windowDrawOrder[WINDOWS_MAX-1] = windowIndex;
}

void MovePreExistingWindowToFront(short windowIndex)
{
	//where is our window located?
	for (int i = WINDOWS_MAX - 1; i >= 0; i--)
	{
		if (g_windowDrawOrder[i] == windowIndex)
		{
			g_windowDrawOrder[i] = -1;
			//move everything after it back
			memcpy (g_windowDrawOrder + i, g_windowDrawOrder + i + 1, sizeof (short) * (WINDOWS_MAX - i - 1));
			g_windowDrawOrder[WINDOWS_MAX-1] = windowIndex;
			
			return;
		}
	}
}

size_t g_windowDepthBufferSzBytes = 0;

void KillWindowDepthBuffer ()
{
	if (g_windowDepthBuffer)
	{
		MmFreeK(g_windowDepthBuffer);
		g_windowDepthBuffer = NULL;
		g_windowDepthBufferSzBytes = 0;
	}
}
void InitWindowDepthBuffer ()
{
	KillWindowDepthBuffer();
	
	g_windowDepthBufferSzBytes = sizeof (short) * GetScreenSizeX() * GetScreenSizeY();
	g_windowDepthBuffer = MmAllocateK(g_windowDepthBufferSzBytes);
}
void SetWindowDepthBuffer (int windowIndex, int x, int y)
{
	if (x < 0 || y < 0 || x >= GetScreenSizeX() || y >= GetScreenSizeY()) return;
	g_windowDepthBuffer[GetScreenSizeX() * y + x] = windowIndex;
}
short GetWindowIndexInDepthBuffer (int x, int y)
{
	if (x < 0 || y < 0 || x >= GetScreenSizeX() || y >= GetScreenSizeY()) return -1;
	short test = g_windowDepthBuffer[GetScreenSizeX() * y + x];
	return test;
}
//TODO: make this really work
void FillDepthBufferWithWindowIndex (Rectangle r, /*uint32_t* framebuffer, */int index)
{
	int hx = GetScreenSizeX(), hy = GetScreenSizeY();
	int idxl = r.left, idxr = r.right;
	if (idxl < 0) idxl = 0;
	if (idxr < 0) idxr = 0;
	if (idxl >= hx) return;//completely OOB
	if (idxr >= hx) idxr = hx;
	int gap = idxr - idxl;
	if (gap <= 0) return;//it will never be bigger than zero if it is now
	//int gapdiv2 = gap / 2;
	idxl += hx * r.top;
	for (int y = r.top; y < r.bottom; y++)
	{
		if (y >= hy) break;//no point.
		if (y < 0) continue;
		//memset_ints(&g_windowDepthBuffer[idxl], index<<16|index, gapdiv2);
		memset_shorts(&g_windowDepthBuffer[idxl], index, gap);
		g_windowDepthBuffer[idxr-1] = index;
		
		idxl += hx;
	}
}
void UpdateDepthBuffer (void)
{
	memset_ints (g_windowDepthBuffer, 0xFFFFFFFF, g_windowDepthBufferSzBytes/4);
	
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		short j = g_windowDrawOrder[i];
		if (j == -1) continue;
		if (g_windows[j].m_used)
			if (!g_windows[j].m_hidden)
			{
				//if (!g_windows[i].m_isSelected)
					FillDepthBufferWithWindowIndex (g_windows[j].m_rect, j);//g_windows[j].m_vbeData.m_framebuffer32, j);
			}
	}
}
#endif

// Window event processor
#if 1

#define OFFSET_FROM_WINDOW_POINTER(pWindow) (pWindow - g_windows)
typedef struct
{
    PWINDOW m_destWindow;
    int m_eventType, m_parm1, m_parm2;
}
WindowEventQueueItem;
#define MASTER_WIN_EVT_QUEUE_MAX 4096
WindowEventQueueItem g_windowEventQueue[MASTER_WIN_EVT_QUEUE_MAX];

int g_windowEventQueueHead = 0;
int g_windowEventQueueTails[WINDOWS_MAX];

void WindowAddEventToMasterQueue(PWINDOW pWindow, int eventType, int parm1, int parm2)
{
	if (pWindow->m_flags & WF_FROZEN)
		return ; // Can't send events to frozen objects!
	
    WindowEventQueueItem item;
    item.m_destWindow = pWindow;
    item.m_eventType = eventType;
    item.m_parm1 = parm1;
    item.m_parm2 = parm2;
    g_windowEventQueue[g_windowEventQueueHead] = item;

    // Allow infinite re-use of the queue by looping it around.
    g_windowEventQueueHead = (g_windowEventQueueHead + 1) % MASTER_WIN_EVT_QUEUE_MAX;
}
//This pops an event on the master queue with the window id.  If there isn't one, return false,
//otherwise, return true and fill in the pointers.
bool WindowPopEventFromQueue(PWINDOW pWindow, int *eventType, int *parm1, int *parm2)
{
    // Start from the window event queue tail.  Go through the queue until you hit the same point you started at.
    int offset = OFFSET_FROM_WINDOW_POINTER(pWindow);
    int backup = g_windowEventQueueTails[offset];

    // First of all, do we have an event right now right in front of us?
    if (g_windowEventQueue[backup].m_destWindow == pWindow)
    {
        // Yes! Advance the tail and return
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;

        g_windowEventQueue[backup].m_destWindow = NULL;
        *eventType = g_windowEventQueue[backup].m_eventType;
        *parm1     = g_windowEventQueue[backup].m_parm1;
        *parm2     = g_windowEventQueue[backup].m_parm2;
        return true;
    }

    // No.  Look through the entire queue array
    for (
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;
        g_windowEventQueueTails[offset] != backup;
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX
    )
    {
        int tail = g_windowEventQueueTails[offset];
        if (g_windowEventQueue[tail].m_destWindow == pWindow)
        {
            // Yes! Advance the tail and return
            g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;

            g_windowEventQueue[tail].m_destWindow = NULL;
            *eventType = g_windowEventQueue[tail].m_eventType;
            *parm1 = g_windowEventQueue[tail].m_parm1;
            *parm2 = g_windowEventQueue[tail].m_parm2;
            return true;
        }
    }

    // No, we still have not found it.  Return false, to signify that there are no more elements on the queue for now.
    return false;
}

//Registers an event to a window.  Not recommended for use.
void WindowRegisterEventUnsafe (Window* pWindow, short eventType, int parm1, int parm2)
{
	if (pWindow->m_flags & WF_FROZEN)
	{
		//return.  Do not queue up events (it can overflow)
		return;
	}
	if (pWindow->m_eventQueueSize < EVENT_QUEUE_MAX - 1)
	{
		pWindow->m_eventQueue[pWindow->m_eventQueueSize] = eventType;
		pWindow->m_eventQueueParm1[pWindow->m_eventQueueSize] = parm1;
		pWindow->m_eventQueueParm2[pWindow->m_eventQueueSize] = parm2;
		
		pWindow->m_eventQueueSize++;
	}
	else
		DebugLogMsg("Could not register event %d for window %x", eventType, pWindow);
}

//This is what you should use in most cases.
void WindowRegisterEvent (Window* pWindow, short eventType, int parm1, int parm2)
{
	//ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	
	int queue_timeout = 100000;
	while (pWindow->m_eventQueueLock)
	{
		queue_timeout--;
		KeTaskDone();
		
		if (queue_timeout == 0)
		{
			SLogMsg("Window with address %x (title: %s) is frozen/taking a long time to process events!", pWindow, pWindow->m_title);
			
			//pWindow->m_flags |= WF_FROZEN;
			
			return;
		}
	}
	
	pWindow->m_eventQueueLock = true;
	
	WindowRegisterEventUnsafe (pWindow, eventType, parm1, parm2);
	
	FREE_LOCK (pWindow->m_eventQueueLock);
}
#endif

// Window utilitary functions:
#if 1

extern Cursor* g_currentCursor, g_defaultCursor, g_waitCursor, g_iBeamCursor, g_crossCursor, g_pencilCursor;

Cursor* const g_CursorLUT[] =
{
	&g_defaultCursor,
	&g_waitCursor,
	&g_iBeamCursor,
	&g_crossCursor,
	&g_pencilCursor,
};

Cursor* GetCursorBasedOnID(int m_cursorID)
{
	if (m_cursorID < CURSOR_DEFAULT || m_cursorID >= CURSOR_COUNT) return &g_defaultCursor;
	
	return g_CursorLUT[m_cursorID];
}

void ChangeCursor (Window* pWindow, int cursorID)
{
	pWindow->m_cursorID = cursorID;
}

void WindowManagerShutdown()
{
	g_shutdownRequest = true;
}

void UndrawWindow (Window* pWindow)
{
	VBEData* backup = g_vbeData;
	VidSetVBEData(NULL);
	
	UpdateDepthBuffer();
	
	ACQUIRE_LOCK(g_backgdLock);
	
	//redraw the background and all the things underneath:
	RedrawBackground(pWindow->m_rect);
	
	FREE_LOCK(g_backgdLock);
	
	// draw the windows below it
	int sz=0; Window* windowDrawList[WINDOWS_MAX];
	
	//higher = faster, but may miss some smaller windows
	for (int y = pWindow->m_rect.top; y < pWindow->m_rect.bottom; y += 1) {
		for (int x = pWindow->m_rect.left; x <= pWindow->m_rect.right; x += 1) {
			short h = GetWindowIndexInDepthBuffer(x,y);
			if (h == -1) continue;
			//check if it's present in the windowDrawList
			Window* pWindowToCheck = GetWindowFromIndex(h);
			bool exists = false;
			for (int i = 0; i < sz; i++) {
				if (windowDrawList[i] == pWindowToCheck) {
					exists = true; break;
				}
			}
			if (!exists) 
			{
				windowDrawList[sz++] = pWindowToCheck;
			}
		}
	}
	
	// We've added the windows to the list, so draw them. We don't need to worry
	// about windows above them, as the way we're drawing them makes it so pixels
	// over the window aren't overwritten.
	//DebugLogMsg("Drawing %d windows below this one", sz);
	for (int i=0; i<sz; i++) 
	{
		//WindowRegisterEvent (windowDrawList[i], EVENT_PAINT, 0, 0);
		//windowDrawList[i]->m_vbeData.m_dirty = true;
		windowDrawList[i]->m_renderFinished = true;
	}
	
	//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
	g_vbeData = backup;
}

static void HideWindowUnsafe (Window* pWindow)
{
	pWindow->m_hidden = true;
	UndrawWindow(pWindow);
}

static void ShowWindowUnsafe (Window* pWindow)
{
	pWindow->m_hidden = false;
	UpdateDepthBuffer();
	//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
	pWindow->m_vbeData.m_dirty = true;
	pWindow->m_renderFinished = true;
}

void HideWindow (Window* pWindow)
{
	if (!KeGetRunningTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		HideWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_HIDE;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void ShowWindow (Window* pWindow)
{
	if (!KeGetRunningTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		ShowWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_SHOW;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

int g_TaskbarHeight = 0;

static void ResizeWindowInternal (Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	if (newPosX != -1)
	{
		if (newPosX < 0) newPosX = 0;
		if (newPosY < 0) newPosY = 0;
		if (newPosX >= GetScreenWidth ()) newPosX = GetScreenWidth ();
		if (newPosY >= GetScreenHeight()) newPosY = GetScreenHeight();
	}
	else
	{
		newPosX = pWindow->m_rect.left;
		newPosY = pWindow->m_rect.top;
	}
	if (newWidth < WINDOW_MIN_WIDTH)
		newWidth = WINDOW_MIN_WIDTH;
	if (newHeight< WINDOW_MIN_HEIGHT)
		newHeight= WINDOW_MIN_HEIGHT;
	uint32_t* pNewFb = (uint32_t*)MmAllocateK(newWidth * newHeight * sizeof(uint32_t));
	if (!pNewFb)
	{
		SLogMsg("Cannot resize window to %dx%d, out of memory", newWidth, newHeight);
		return;
	}
	
	// Copy the entire framebuffer's contents from old to new.
	int oldWidth = pWindow->m_vbeData.m_width, oldHeight = pWindow->m_vbeData.m_height;
	int minWidth = newWidth, minHeight = newHeight;
	if (minWidth > oldWidth)
		minWidth = oldWidth;
	if (minHeight > oldHeight)
		minHeight = oldHeight;
	
	for (int i = 0; i < minHeight; i++)
	{
		memcpy_ints (&pNewFb[i * newWidth], &pWindow->m_vbeData.m_framebuffer32[i * oldWidth], minWidth);
	}
	
	// Free the old framebuffer.  This action should be done atomically.
	// TODO: If I ever decide to add locks to mmfree etc, then fix this so that it can't cause deadlocks!!
	cli;
	MmFree(pWindow->m_vbeData.m_framebuffer32);
	pWindow->m_vbeData.m_framebuffer32 = pNewFb;
	pWindow->m_vbeData.m_width   = newWidth;
	pWindow->m_vbeData.m_pitch32 = newWidth;
	pWindow->m_vbeData.m_pitch16 = newWidth*2;
	pWindow->m_vbeData.m_pitch   = newWidth*4;
	pWindow->m_vbeData.m_height  = newHeight;
	sti;
	
	pWindow->m_rect.left   = newPosX;
	pWindow->m_rect.top    = newPosY;
	pWindow->m_rect.right  = pWindow->m_rect.left + newWidth;
	pWindow->m_rect.bottom = pWindow->m_rect.top  + newHeight;
	
	// Mark as dirty.
	pWindow->m_vbeData.m_dirty = true;
	
	// Send window events: EVENT_SIZE, EVENT_PAINT.
	WindowAddEventToMasterQueue(pWindow, EVENT_SIZE,  MAKE_MOUSE_PARM(newWidth, newHeight), MAKE_MOUSE_PARM(oldWidth, oldHeight));
	WindowAddEventToMasterQueue(pWindow, EVENT_PAINT, 0, 0);
}

static void ResizeWindowUnsafe(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	HideWindowUnsafe (pWindow);
	
	ResizeWindowInternal (pWindow, newPosX, newPosY, newWidth, newHeight);
	
	ShowWindowUnsafe (pWindow);
}

void ResizeWindow(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight)
{
	if (!KeGetRunningTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		ResizeWindowUnsafe(pWindow, newPosX, newPosY, newWidth, newHeight);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_RESIZE;
	action.rect.left   = newPosX;
	action.rect.top    = newPosY;
	action.rect.right  = newPosX + newWidth;
	action.rect.bottom = newPosY + newHeight;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

extern Heap* g_pHeap;

void NukeWindowUnsafe (Window* pWindow)
{
	HideWindowUnsafe (pWindow);
	
	Heap *pHeapBackup = g_pHeap;
	ResetToKernelHeap ();
	
	if (pWindow->m_vbeData.m_framebuffer32)
	{
		MmFreeK (pWindow->m_vbeData.m_framebuffer32);
		pWindow->m_vbeData.m_available     = 0;
		pWindow->m_vbeData.m_framebuffer32 = NULL;
	}
	if (pWindow->m_pControlArray)
	{
		MmFreeK(pWindow->m_pControlArray);
		pWindow->m_controlArrayLen = 0;
	}
	memset (pWindow, 0, sizeof (*pWindow));
	UseHeap (pHeapBackup);
	
	int et, p1, p2;
	while (WindowPopEventFromQueue(pWindow, &et, &p1, &p2));//flush queue
}

void NukeWindow (Window* pWindow)
{
	if (!KeGetRunningTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		NukeWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_DESTROY;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}

void DestroyWindow (Window* pWindow)
{
	if (!pWindow)
	{
		SLogMsg("Tried to destroy nullptr window?");
		return;
	}
	if (!pWindow->m_used)
		return;
	
	WindowAddEventToMasterQueue(pWindow, EVENT_DESTROY, 0, 0);
	// the task's last WindowCheckMessages call will see this and go
	// "ah yeah they want my window gone", then the WindowCallback will reply and say
	// "yeah you're good to go" and call ReadyToDestroyWindow().
}

void RequestRepaint (Window* pWindow)
{
	WindowRegisterEventUnsafe(pWindow, EVENT_PAINT, 0, 0);
}

extern void SetFocusedConsole(Console* console);
void SelectWindowUnsafe(Window* pWindow)
{
	bool wasSelectedBefore = pWindow->m_isSelected;
	if (!wasSelectedBefore) {
		SetFocusedConsole (NULL);
		g_focusedOnWindow = NULL;
		for (int i = 0; i < WINDOWS_MAX; i++) {
			if (g_windows[i].m_used) {
				if (g_windows[i].m_isSelected)
				{
					g_windows[i].m_isSelected = false;
					WindowRegisterEventUnsafe(&g_windows[i], EVENT_KILLFOCUS, 0, 0);
					WindowRegisterEventUnsafe(&g_windows[i], EVENT_PAINT, 0, 0);
					g_windows[i].m_vbeData.m_dirty = true;
					g_windows[i].m_renderFinished = true;
				}
			}
		}
		
		MovePreExistingWindowToFront (pWindow - g_windows);
		pWindow->m_isSelected = true;
		UpdateDepthBuffer();
		WindowRegisterEventUnsafe(pWindow, EVENT_SETFOCUS, 0, 0);
		WindowRegisterEventUnsafe(pWindow, EVENT_PAINT, 0, 0);
		pWindow->m_vbeData.m_dirty = true;
		pWindow->m_renderFinished = true;
		SetFocusedConsole (pWindow->m_consoleToFocusKeyInputsTo);
		g_focusedOnWindow = pWindow;
	}
}
void SelectWindow(Window* pWindow)
{
	if (!KeGetRunningTask())
	{
		// Automatically resort to unsafe versions because we're running in the wm task already
		SelectWindowUnsafe(pWindow);
		return;
	}
	
	WindowAction action;
	action.bInProgress = true;
	action.pWindow     = pWindow;
	action.nActionType = WACT_SELECT;
	
	WindowAction* ptr = ActionQueueAdd(action);
	
	while (ptr->bInProgress)
		KeTaskDone(); //Spinlock: pass execution off to other threads immediately
}
#endif

// Window creation
#if 1

int g_NewWindowX = 10, g_NewWindowY = 10;

Window* CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags)
{
	ACQUIRE_LOCK(g_createLock);
	
	if (xSize > GetScreenWidth())
		xSize = GetScreenWidth();
	if (ySize > GetScreenHeight())
		ySize = GetScreenHeight();
	
	if (xPos < 0 || yPos < 0)
	{
		g_NewWindowX += 10;
		g_NewWindowY += 10;
		if((g_NewWindowX + xSize + 50) >= GetScreenWidth())
			g_NewWindowX = GetScreenWidth() - xSize - 50;
		if((g_NewWindowY + ySize + 50) >= GetScreenHeight())
			g_NewWindowY = GetScreenHeight() - ySize - 50;
		xPos = g_NewWindowX;
		yPos = g_NewWindowY;
	}
	
	if (!(flags & WF_EXACTPOS))
	{
		if (xPos >= GetScreenWidth () - xSize)
			xPos  = GetScreenWidth () - xSize-1;
		if (yPos >= GetScreenHeight() - ySize)
			yPos  = GetScreenHeight() - ySize-1;
	}
	
	int freeArea = -1;
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (!g_windows[i].m_used)
		{
			freeArea = i; break;
		}
	}
	if (freeArea == -1) return NULL;//can't create the window.
	
	Window* pWnd = &g_windows[freeArea];
	memset (pWnd, 0, sizeof *pWnd);
	
	pWnd->m_used = true;
	int strl = strlen (title) + 1;
	if (strl >= WINDOW_TITLE_MAX) strl = WINDOW_TITLE_MAX - 1;
	memcpy (pWnd->m_title, title, strl + 1);
	
	Heap *pHeapBackup  = g_pHeap;
	ResetToKernelHeap ();
	
	pWnd->m_renderFinished = false;
	pWnd->m_hidden         = true;//false;
	pWnd->m_isBeingDragged = false;
	pWnd->m_isSelected     = false;
	pWnd->m_minimized      = false;
	pWnd->m_maximized      = false;
	pWnd->m_clickedInside  = false;
	pWnd->m_eventQueueLock = false;
	pWnd->m_flags          = flags;// | WF_FLATBORD;
	
	pWnd->m_rect.left = xPos;
	pWnd->m_rect.top  = yPos;
	pWnd->m_rect.right  = xPos + xSize;
	pWnd->m_rect.bottom = yPos + ySize;
	pWnd->m_eventQueueSize = 0;
	pWnd->m_eventQueueLock = false;
	pWnd->m_markedForDeletion = false;
	pWnd->m_callback = proc; 
	
	pWnd->m_consoleToFocusKeyInputsTo = NULL;
	
	pWnd->m_vbeData.m_available     = true;
	pWnd->m_vbeData.m_framebuffer32 = MmAllocateK (sizeof (uint32_t) * xSize * ySize);
	if (!pWnd->m_vbeData.m_framebuffer32)
	{
		LogMsg("Cannot allocate window buffer for '%s', out of memory!!!", pWnd->m_title);
		pWnd->m_used = false;
		return NULL;
	}
	ZeroMemory (pWnd->m_vbeData.m_framebuffer32,  sizeof (uint32_t) * xSize * ySize);
	pWnd->m_vbeData.m_width         = xSize;
	pWnd->m_vbeData.m_height        = ySize;
	pWnd->m_vbeData.m_pitch32       = xSize;
	pWnd->m_vbeData.m_bitdepth      = 2;     // 32 bit :)
	
	pWnd->m_iconID   = ICON_APPLICATION;
	
	pWnd->m_cursorID = CURSOR_DEFAULT;
	
	pWnd->m_eventQueueSize = 0;
	
	//give the window a starting point of 10 controls:
	pWnd->m_controlArrayLen = 10;
	size_t controlArraySize = sizeof(Control) * pWnd->m_controlArrayLen;
	pWnd->m_pControlArray   = (Control*)MmAllocateK(controlArraySize);
	
	if (!pWnd->m_pControlArray)
	{
		// The framebuffer fit, but this didn't
		LogMsg("Couldn't allocate pControlArray for window, out of memory!");
		MmFreeK(pWnd->m_vbeData.m_framebuffer32);
		pWnd->m_used = false;
		return NULL;
	}
	
	memset(pWnd->m_pControlArray, 0, controlArraySize);
	
	WindowRegisterEvent(pWnd, EVENT_CREATE, 0, 0);
	
	UseHeap (pHeapBackup);
	
	FREE_LOCK(g_createLock);
	return pWnd;
}
#endif 

// Mouse event handlers
#if 1
int g_currentlyClickedWindow = -1;
int g_prevMouseX, g_prevMouseY;

void OnUILeftClick (int mouseX, int mouseY)
{
	if (!g_windowManagerRunning) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	
	short idx = GetWindowIndexInDepthBuffer(mouseX, mouseY);
	
	if (idx > -1)
	{
		Window* window = GetWindowFromIndex(idx);
		if (!(window->m_flags & WF_FROZEN))
		{
			SelectWindow (window);
			
			//bool wasSelectedBefore = g_currentlyClickedWindow == idx;
			g_currentlyClickedWindow = idx;
			
			//are we in the title bar region? TODO
			Rectangle recta = window->m_rect;
			if (!window->m_minimized)
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
				recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.left++; recta.right--; recta.top++; recta.bottom = recta.top + TITLE_BAR_HEIGHT;
			}
			else
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
			}
			
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			Point mousePoint = {x, y};
			
			bool t = RectangleContains(&recta, &mousePoint);
			if (window->m_flags & WF_NOTITLE)
				t = false;
			
			if (!window->m_maximized && (t || window->m_minimized))
			{
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 1);
			}
			else if (!window->m_minimized)
			{
				int x = mouseX - window->m_rect.left;
				int y = mouseY - window->m_rect.top;
				
				window->m_clickedInside = true;
				
				//WindowRegisterEvent (window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
	else
		g_currentlyClickedWindow = -1;
	
	//FREE_LOCK(g_windowLock);
}
bool g_heldAlt = false;
Cursor g_windowDragCursor;
void OnUILeftClickDrag (int mouseX, int mouseY)
{
	if (!g_windowManagerRunning) return;
	if (g_currentlyClickedWindow == -1) return;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	Window* window = GetWindowFromIndex(g_currentlyClickedWindow);
	
	// if we're not frozen AND we have a title to drag on
	if (window->m_minimized || !(window->m_flags & (WF_FROZEN | WF_NOTITLE)))
	{
		if (!window->m_isBeingDragged)
		{
			//are we in the title bar region? TODO
			Rectangle recta = window->m_rect;
			if (!window->m_minimized)
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
				recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS;
				recta.left++; recta.right--; recta.top++; recta.bottom = recta.top + TITLE_BAR_HEIGHT;
			}
			else
			{
				recta.right  -= recta.left; recta.left = 0;
				recta.bottom -= recta.top;  recta.top  = 0;
			}
			
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			Point mousePoint = {x, y};
			
			if (!window->m_maximized && (RectangleContains(&recta, &mousePoint) || window->m_minimized))
			{
				window->m_isBeingDragged = true;
				
				if (g_RenderWindowContents)
				{
					HideWindow(window);
				}
				
				//change cursor:
				if (window->m_minimized)
				{
					Image* p = GetIconImage(window->m_iconID, 32);
					g_windowDragCursor.width    = p->width;
					g_windowDragCursor.height   = p->height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = p->framebuffer;
					g_windowDragCursor.m_transparency = true;
					g_windowDragCursor.m_resizeMode   = false;
					g_windowDragCursor.boundsWidth  = p->width;
					g_windowDragCursor.boundsHeight = p->height;
				}
				else if (g_heldAlt && (window->m_flags & WF_ALWRESIZ))
				{
					g_windowDragCursor.width    = window->m_vbeData.m_width;
					g_windowDragCursor.height   = window->m_vbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = window->m_vbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
					g_windowDragCursor.m_resizeMode   = true;
					g_windowDragCursor.boundsWidth  = window->m_vbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_vbeData.m_height;
				}
				else
				{
					g_windowDragCursor.width    = window->m_vbeData.m_width;
					g_windowDragCursor.height   = window->m_vbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = window->m_vbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
					g_windowDragCursor.m_resizeMode   = false;
					g_windowDragCursor.boundsWidth  = window->m_vbeData.m_width;
					g_windowDragCursor.boundsHeight = window->m_vbeData.m_height;
				}
				
				SetCursor (&g_windowDragCursor);
			}
			if (!window->m_minimized && !window->m_isBeingDragged)
			{
				window->m_clickedInside = true;
				
				WindowAddEventToMasterQueue(window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
	
	//FREE_LOCK(g_windowLock);
}

extern int g_mouseX, g_mouseY;//video.c
void RenderWindow (Window* pWindow);
void OnUILeftClickRelease (int mouseX, int mouseY)
{
	if (!g_windowManagerRunning) return;
	if (g_currentlyClickedWindow == -1) return;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	mouseX = g_mouseX;
	mouseY = g_mouseY;
	
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
//	short idx = GetWindowIndexInDepthBuffer(mouseX, mouseY);
	
	Window* pWindow = GetWindowFromIndex(g_currentlyClickedWindow);
	if (pWindow->m_isBeingDragged)
	{
		if (!g_RenderWindowContents)
		{
			HideWindowUnsafe(pWindow);
		}
		
		if (GetCurrentCursor()->m_resizeMode)
		{
			int newWidth = GetCurrentCursor()->boundsWidth, newHeight = GetCurrentCursor()->boundsHeight;
			
			//note that we resize the window this way here because we're running inside the wm task
			ResizeWindowInternal (pWindow, -1, -1, newWidth, newHeight);
		}
		else
		{
			Rectangle newWndRect;
			newWndRect.left   = mouseX - g_windowDragCursor.leftOffs;
			newWndRect.top    = mouseY - g_windowDragCursor.topOffs;
			if (newWndRect.top < 0)
				newWndRect.top = 0;
			newWndRect.right  = newWndRect.left + GetWidth (&pWindow->m_rect);
			newWndRect.bottom = newWndRect.top  + GetHeight(&pWindow->m_rect);
			pWindow->m_rect = newWndRect;
		}
		
		if (GetCurrentCursor() == &g_windowDragCursor)
		{
			SetCursor(NULL);
		}
		//WindowRegisterEvent(window, EVENT_PAINT, 0, 0);
		pWindow->m_vbeData.m_dirty = true;
		pWindow->m_renderFinished = true;
		pWindow->m_isBeingDragged = false;
		ShowWindowUnsafe(pWindow);
	}
	
	if (pWindow->m_minimized) return;
	
	int x = mouseX - pWindow->m_rect.left;
	int y = mouseY - pWindow->m_rect.top;
	
	if (pWindow->m_clickedInside)
	{
		pWindow->m_clickedInside = false;
		WindowAddEventToMasterQueue(pWindow, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (x, y), 0);
	}
	else
	{
		WindowAddEventToMasterQueue(pWindow, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (x, y), 1);
	}
	//FREE_LOCK(g_windowLock);
}
void OnUIRightClick (int mouseX, int mouseY)
{
	if (!g_windowManagerRunning) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	short idx = GetWindowIndexInDepthBuffer(mouseX, mouseY);
	
	if (idx > -1)
	{
		Window* window = GetWindowFromIndex(idx);
		
		if (window)
		{
			if (!window->m_minimized)
			{
				int x = mouseX - window->m_rect.left;
				int y = mouseY - window->m_rect.top;
				WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICK, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
}
void OnUIRightClickRelease (int mouseX, int mouseY)
{
	if (!g_windowManagerRunning) return;
	g_prevMouseX = (int)mouseX;
	g_prevMouseY = (int)mouseY;
	
	//ACQUIRE_LOCK (g_windowLock); -- NOTE: No need to lock anymore.  We're 'cli'ing anyway.
	short idx = GetWindowIndexInDepthBuffer(mouseX, mouseY);
	
	if (idx > -1)
	{
		Window* window = GetWindowFromIndex(idx);
		
		if (window)
		{
			if (window->m_minimized)
			{
				WindowRegisterEvent (window, EVENT_UNMINIMIZE, 0, 0);
			}
			else
			{
				int x = mouseX - window->m_rect.left;
				int y = mouseY - window->m_rect.top;
				
				WindowAddEventToMasterQueue (window, EVENT_RIGHTCLICKRELEASE, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
}

#endif

// Main loop thread.
#if 1

void RedrawEverything()
{
	VBEData* pBkp = g_vbeData;
	VidSetVBEData(NULL);
	UpdateDepthBuffer();
	
	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	RedrawBackground (r);
	
	//for each window, send it a EVENT_PAINT:
	for (int p = 0; p < WINDOWS_MAX; p++)
	{
		Window* pWindow = &g_windows [p];
		if (!pWindow->m_used) continue;
		
		int prm = MAKE_MOUSE_PARM(pWindow->m_rect.right - pWindow->m_rect.left, pWindow->m_rect.bottom - pWindow->m_rect.top);
		WindowAddEventToMasterQueue(pWindow, EVENT_SIZE,  prm, prm);
		WindowAddEventToMasterQueue(pWindow, EVENT_PAINT, 0,   0);
		//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
		pWindow->m_renderFinished = true;
	}
	VidSetVBEData(pBkp);
}

bool HandleMessages(Window* pWindow);
void TerminalHostTask(int arg);
void RefreshMouse(void);
void RenderCursor(void);

static Window* g_pShutdownMessage = NULL;

void WindowManagerOnShutdownTask (__attribute__((unused)) int useless)
{
	if (MessageBox (NULL, "It is now safe to shut down your computer.", "Shutdown Computer", MB_RESTART | ICON_SHUTDOWN << 16) == MBID_OK)
	{
		KeRestartSystem();
	}
}

void WindowManagerOnShutdown(void)
{
	//create a task
	UNUSED int useless = 0;
	KeStartTask(WindowManagerOnShutdownTask, 0, &useless);
}
void SetupWindowManager()
{
	if (g_windowManagerRunning)
	{
		LogMsg("Cannot start up window manager again.");
		return;
	}
	
	LogMsg("Please wait...");
	
	g_debugConsole.curY = g_debugConsole.height / 2;
	g_clickQueueSize = 0;
	// load background?
	memset (&g_windows, 0, sizeof (g_windows));
	InitWindowDepthBuffer();
	//CoClearScreen (&g_debugConsole);
	g_debugConsole.curX = g_debugConsole.curY = 0;
	g_debugConsole.pushOrWrap = 1;
	
	g_windowManagerRunning = true;
	
	g_pShutdownMessage = NULL;
	
	g_shutdownSentDestroySignals = false;
	g_shutdownWaiting			 = false;
	
	UpdateDepthBuffer();
	
	LoadDefaultThemingParms ();
	//VidFillScreen(BACKGROUND_COLOR);
	SetDefaultBackground ();
	
	//redraw background?
	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	RedrawBackground (r);
	
	//CreateTestWindows();
	UpdateDepthBuffer();
	
	VidSetFont(FONT_BASIC);
	//VidSetFont(FONT_TAMSYN_BOLD);
	//VidSetFont(FONT_TAMSYN_REGULAR);
	//VidSetFont(FONT_FAMISANS);
	//VidSetFont(FONT_GLCD);
	//VidSetFont(FONT_BIGTEST);
	
	LogMsg("\n\n\n");
	
	WindowCallInitialize ();
	
	//test:
#if !THREADING_ENABLED
	LogMsgNoCr("Huh!?? This shouldn't be on");
	LauncherEntry(0);
#else
	int errorCode = 0;
	Task* pTask;
	
	//create the taskbar task.
	errorCode = 0;
	pTask = KeStartTask(TaskbarEntry, 0, &errorCode);
	DebugLogMsg("Created taskbar task. pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	/*
	//create the debug monitor task.
	errorCode = 0;
	char* pb = MmAllocate(512);
	strcpy(pb, "--HookDebugConsole");
	pTask = KeStartTask(TerminalHostTask, (int)pb, &errorCode);
	DebugLogMsg("Created taskbar task. pointer returned:%x, errorcode:%x", pTask, errorCode);
	*/
#endif
}

void HandleKeypressOnWindow(unsigned char key)
{
	if (key == KEY_ALT)
	{
		g_heldAlt = true;
	}
	else if (key == (KEY_ALT | SCANCODE_RELEASE))
	{
		g_heldAlt = false;
		KillAltTab();
	}
	else if (key == KEY_TAB && g_heldAlt)
		OnPressAltTabOnce();
}

int g_oldMouseX = -1, g_oldMouseY = -1;
void WindowManagerTask(__attribute__((unused)) int useless_argument)
{
	SetupWindowManager();
	
	int timeout = 10;
	#define UPDATE_TIMEOUT 50
	int UpdateTimeout = UPDATE_TIMEOUT, shutdownTimeout = 500;
	
	while (true)
	{
		bool handled = false, hasRedrawnThem = false;
		UpdateFPSCounter();
		CrashReporterCheck();
		bool updated = false;
		
		while (!ActionQueueEmpty())
		{
			WindowAction *pFront = ActionQueueGetFront();
			
			SLogMsg("Executing action %d on window %x", pFront->nActionType, pFront->pWindow);
			
			switch (pFront->nActionType)
			{
				case WACT_DESTROY:
					NukeWindowUnsafe (pFront->pWindow);
					break;
				case WACT_HIDE:
					HideWindowUnsafe (pFront->pWindow);
					break;
				case WACT_SHOW:
					ShowWindowUnsafe (pFront->pWindow);
					break;
				case WACT_SELECT:
					SelectWindowUnsafe (pFront->pWindow);
					break;
				case WACT_RESIZE:
					ResizeWindowUnsafe (pFront->pWindow, pFront->rect.left, pFront->rect.top, GetWidth(&pFront->rect), GetHeight(&pFront->rect));
					break;
			}
			
			pFront->bInProgress = false;
			ActionQueuePop();
		}
		
		for (int p = 0; p < WINDOWS_MAX; p++)
		{
			Window* pWindow = &g_windows [p];
			if (!pWindow->m_used) continue;
			
			if (UpdateTimeout == 0 || updated)
			{
				//WindowAddEventToMasterQueue (pWindow, EVENT_UPDATE2, 0, 0);
				UpdateTimeout = UPDATE_TIMEOUT;
				updated = true;
			}
			
			//TODO: This method misses a lot of key inputs.  Perhaps make a way to route keyboard inputs directly
			//into a window's input buffer and read that instead of doing this hacky method right here?
			if (pWindow->m_isSelected)
			{
				if (updated)
				{
					//Also send an EVENT_MOVECURSOR
					int posX = g_mouseX - pWindow->m_rect.left;
					int posY = g_mouseY - pWindow->m_rect.top;
					if (g_oldMouseX != posX || g_oldMouseY != posY)
					{
						if (posX >= 0 && posY >= 0 && posX < (int)pWindow->m_vbeData.m_width && posY < (int)pWindow->m_vbeData.m_height)
						{
							WindowAddEventToMasterQueue(pWindow, EVENT_MOVECURSOR, MAKE_MOUSE_PARM(posX, posY), 0);
						}
					}
					g_oldMouseX = g_mouseX;
					g_oldMouseY = g_mouseY;
				}
			}
			
			if (pWindow->m_bWindowManagerUpdated)
			{
				HandleMessages(pWindow);
			}
			
		#if !THREADING_ENABLED
			if (pWindow == g_pShutdownMessage)
				if (!HandleMessages (pWindow))
				{
					//ReadyToDestroyWindow(pWindow);
					KeStopSystem();
					continue;
				}
		#endif
			if (!pWindow->m_hidden)
			{
				//cli;
				if (pWindow->m_renderFinished && !g_backgdLock)
				{
					if (!hasRedrawnThem)
					{
						hasRedrawnThem = true;
						RedrawBackgdDetails();
					}
					pWindow->m_renderFinished = false;
					
					//ACQUIRE_LOCK(g_backgdLock);
					cli;
					
					RenderWindow(pWindow);
					
					sti;
					
					//FREE_LOCK(g_backgdLock);
					
					Point p = { g_mouseX, g_mouseY };
					if (RectangleContains (&pWindow->m_rect, &p))
						RenderCursor ();
				}
				//sti;
			}
			
			if (pWindow->m_markedForDeletion)
			{
				//turn it off, because DestroyWindow sends an event here, 
				//and we don't want it to stack overflow. Stack overflows
				//go pretty ugly in this OS, so we need to be careful.
				pWindow->m_markedForDeletion = false;
				DestroyWindow (pWindow);
			}
		}
		UpdateTimeout--;
		
		RunOneEffectFrame ();
		/*if (!g_EffectRunning)
		{
			Rectangle dest = { 100, 100, 200, 200 };
			Rectangle src  = { 600, 600, 800, 800 };
			
			CreateMovingRectangleEffect(src, dest, "Hello");
		}*/
		
		// Get the window we're over:
		short windowOver = GetWindowIndexInDepthBuffer (g_mouseX, g_mouseY);
		
		if (windowOver >= 0)
		{
			Window* pWindow = &g_windows [windowOver];
			if (g_currentCursor != &g_windowDragCursor && g_currentCursor != GetCursorBasedOnID(pWindow->m_cursorID))
				SetCursor(GetCursorBasedOnID(pWindow->m_cursorID));
		}
		else if (g_currentCursor != &g_windowDragCursor && g_currentCursor != &g_defaultCursor)
			SetCursor(&g_defaultCursor);
		
		if (!handled)
		{
			unsigned char key = KbGetKeyFromRawBuffer();
			HandleKeypressOnWindow(key);
		}
		UpdateAltTabWindow();
		//cli;
		ACQUIRE_LOCK (g_clickQueueLock);
		
		RefreshMouse();
		//ACQUIRE_LOCK (g_screenLock);
		for (int i = 0; i < g_clickQueueSize; i++)
		{
			switch (g_clickQueue[i].clickType)
			{
				case CLICK_LEFT:   OnUILeftClick        (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_LEFTD:  OnUILeftClickDrag    (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_LEFTR:  OnUILeftClickRelease (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_RIGHT:  OnUIRightClick       (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_RIGHTR: OnUIRightClickRelease(g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
			}
		}
		g_clickQueueSize = 0;
		//FREE_LOCK (g_screenLock);
		FREE_LOCK (g_clickQueueLock);
		//sti;
		
		timeout--;
		
		if (g_shutdownRequest && !g_shutdownWaiting)
		{
			g_shutdownRequest = false;
			g_shutdownWaiting = true;
			
			//VidSetFont(FONT_TAMSYN_REGULAR);
			
			//LogMsg("Sending kill messages to windows...");
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				WindowRegisterEvent (g_windows + i, EVENT_DESTROY, 0, 0);
			}
			
			shutdownTimeout = 2500;
		}
		if (g_shutdownWaiting)
		{
			shutdownTimeout--;
			//LogMsgNoCr("\r(Waiting for all windows to shut down... -- %d ticks left.)", shutdownTimeout);
			bool noMoreWindows = true;
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (g_windows[i].m_used)
				{
					noMoreWindows = false;
					break;
				}
			}
			if (noMoreWindows)
			{
				//LogMsg("\nAll windows have shutdown gracefully?  Quitting...");
				//LogMsg("STATUS: We survived!  Exitting in a brief moment.");
				//g_windowManagerRunning = false;
				
				// On Shutdown:
				g_shutdownWaiting = false;
				WindowManagerOnShutdown ();
				continue;
			}
			//Shutdown timeout equals zero.  If there are any windows still up, force-kill them.
			if (shutdownTimeout <= 0)
			{
				LogMsg("\nWindow TIMEOUT (no response, all tasks dead/froze due to crash?)! Forcing *EMERGENCY EXIT* now! (Applying defibrillator)");
				for (int i = 0; i < WINDOWS_MAX; i++)
				{
					if (g_windows[i].m_used)
					{
						if (g_windows[i].m_pOwnerThread)
							KeKillTask (g_windows[i].m_pOwnerThread);
						if (g_windows[i].m_pSubThread)
							KeKillTask (g_windows[i].m_pSubThread);
						
						NukeWindow (&g_windows[i]);
					}
				}
				
				//g_windowManagerRunning = false;
				g_shutdownWaiting = false;
				WindowManagerOnShutdown ();
				continue;
			}
		}
		
		//for (int i = 0; i < 2; i++)
		//hlt;
		KeTaskDone();
	}
	WindowCallDeinitialize ();
	KillWindowDepthBuffer();
	g_debugConsole.pushOrWrap = 0;
	VidSetFont (FONT_TAMSYN_REGULAR);
}
#endif

// Control creation and management
#if 1

Control* GetControlByComboID(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_active)
		{
			if (pWindow->m_pControlArray[i].m_comboID == comboID)
				return &pWindow->m_pControlArray[i];
		}
	}
	return NULL;
}

//Returns an index, because we might want to relocate the m_pControlArray later.
int AddControlEx(Window* pWindow, int type, int anchoringMode, Rectangle rect, const char* text, int comboID, int p1, int p2)
{
	if (!pWindow->m_pControlArray)
	{
		VidSetVBEData(NULL);
		LogMsg("No pControlArray?");
		return -1;
	}
	int index = -1;
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (!pWindow->m_pControlArray[i].m_active)
		{
			index = i;
			break;
		}
	}
	if (index <= -1)
	{
		//Couldn't find a spot in the currently allocated thing.
		//Perhaps we need to expand the array.
		int cal = pWindow->m_controlArrayLen;
		if (cal < 2) cal = 2;
		
		cal += cal / 2;
		//series: 2, 3, 4, 6, 9, 13, 19, 28, 42, ...
		
		size_t newSize = sizeof(Control) * cal;
		Control* newCtlArray = (Control*)MmAllocateK(newSize);
		if (!newCtlArray)
		{
			SLogMsg("Cannot add control %d to window %x", type, pWindow);
			return -1;
		}
		memset(newCtlArray, 0, newSize);
		
		// copy stuff into the new control array:
		memcpy(newCtlArray, pWindow->m_pControlArray, sizeof(Control) * pWindow->m_controlArrayLen);
		
		// free the previous array:
		MmFreeK(pWindow->m_pControlArray);
		
		// then assign the new one
		pWindow->m_pControlArray   = newCtlArray;
		pWindow->m_controlArrayLen = cal;
		
		// last, re-search the thing
		index = -1;
		for (int i = 0; i < pWindow->m_controlArrayLen; i++)
		{
			if (!pWindow->m_pControlArray[i].m_active)
			{
				index = i;
				break;
			}
		}
	}
	if (index <= -1)
	{
		return -1;
	}
	
	// add the control itself:
	Control *pControl = &pWindow->m_pControlArray[index];
	pControl->m_active  = true;
	pControl->m_type    = type;
	pControl->m_dataPtr = NULL;
	pControl->m_rect    = pControl->m_triedRect = rect;
	pControl->m_comboID = comboID;
	pControl->m_parm1   = p1;
	pControl->m_parm2   = p2;
	pControl->m_bMarkedForDeletion = false;
	pControl->m_anchorMode = anchoringMode;
	
	if (text)
		strcpy (pControl->m_text, text);
	else
		pControl->m_text[0] = '\0';
	
	pControl->OnEvent = GetWidgetOnEventFunction(type);
	
	if (type == CONTROL_VSCROLLBAR || type == CONTROL_HSCROLLBAR)
	{
		pControl->m_scrollBarData.m_min = (pControl->m_parm1   >>  16);
		pControl->m_scrollBarData.m_max = (pControl->m_parm1 & 0xFFFF);
		pControl->m_scrollBarData.m_pos = (pControl->m_parm2 & 0xFFFF);
		pControl->m_scrollBarData.m_dbi = (pControl->m_parm2   >>  16);
		
		if (pControl->m_scrollBarData.m_dbi == 0)
			pControl->m_scrollBarData.m_dbi  = 1;
		
		if (pControl->m_scrollBarData.m_pos < pControl->m_scrollBarData.m_min)
			pControl->m_scrollBarData.m_pos = pControl->m_scrollBarData.m_min;
		if (pControl->m_scrollBarData.m_pos >= pControl->m_scrollBarData.m_max)
			pControl->m_scrollBarData.m_pos =  pControl->m_scrollBarData.m_max - 1;
	}
	else if (type == CONTROL_TEXTINPUT)
	{
		//by default you have single line
		CtlTextInputUpdateMode (pControl);
	}
	else if (type == CONTROL_CHECKBOX)
	{
		//by default you have single line
		pControl->m_checkBoxData.m_checked = p1 != 0;
		pControl->m_checkBoxData.m_clicked = 0;
	}
	
	//register an event for the window:
	//WindowRegisterEvent(pWindow, EVENT_PAINT, 0, 0);
	
	//call EVENT_CREATE to let the ctl initialize its data
	pControl->OnEvent(pControl, EVENT_CREATE, 0, 0, pWindow);
	
	//The control should be able to adjust its starting rect when created.
	pControl->m_triedRect = pControl->m_rect;
	
	return index;
}
int AddControl(Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2)
{
	return
	AddControlEx(pWindow, type, 0, rect, text, comboID, p1, p2);
}

void RemoveControl (Window* pWindow, int controlIndex)
{
	if (controlIndex >= pWindow->m_controlArrayLen || controlIndex < 0) return;
	
	ACQUIRE_LOCK(pWindow->m_eventQueueLock);
	Control* pControl = &pWindow->m_pControlArray[controlIndex];
	if (pControl->m_dataPtr)
	{
		//TODO
	}
	pControl->m_active = false;
	pControl->m_bMarkedForDeletion = false;
	pControl->OnEvent = NULL;
	
	FREE_LOCK(pWindow->m_eventQueueLock);
}

void ControlProcessEvent (Window* pWindow, int eventType, int parm1, int parm2)
{
	// Go backwards, because some controls might spawn other controls
	// They may want to be checked AFTER their children controls, so
	// we just go backwards.
	
	//Prioritise menu bar, as it's always at the top
	Control* pMenuBar = NULL;
	
	WidgetEventHandler pHandler = GetWidgetOnEventFunction(CONTROL_MENUBAR);
	for (int i = pWindow->m_controlArrayLen - 1; i != -1; i--)
	{
		if (pWindow->m_pControlArray[i].m_active)
		{
			Control* p = &pWindow->m_pControlArray[i];
			if (p->OnEvent == pHandler)
			{
				pMenuBar = &pWindow->m_pControlArray[i];
				break;
			}
		}
	}
	
	if (eventType != EVENT_PAINT && eventType != EVENT_CLICKCURSOR)
		if (pMenuBar)
			if (pMenuBar->OnEvent)
			{
				if (pMenuBar->OnEvent(pMenuBar, eventType, parm1, parm2, pWindow))
					return;
				if (eventType == EVENT_CREATE)
				{
					// Let the control adjust itself
					pMenuBar->m_triedRect = pMenuBar->m_rect;
				}
			}
	
	for (int i = pWindow->m_controlArrayLen - 1; i != -1; i--)
	{
		if (&pWindow->m_pControlArray[i] == pMenuBar) continue; // Skip over the menu bar.
		
		if (pWindow->m_pControlArray[i].m_active)
		{
			Control* p = &pWindow->m_pControlArray[i];
			if (p->OnEvent)
			{
				if (p->OnEvent(p, eventType, parm1, parm2, pWindow))
					return;
				if (eventType == EVENT_CREATE)
				{
					// Let the control adjust itself
					p->m_triedRect = p->m_rect;
				}
			}
		}
	}
	
	if (eventType == EVENT_PAINT || eventType == EVENT_CLICKCURSOR)
		if (pMenuBar)
			if (pMenuBar->OnEvent)
			{
				if (pMenuBar->OnEvent(pMenuBar, eventType, parm1, parm2, pWindow))
					return;
				if (eventType == EVENT_CREATE)
				{
					// Let the control adjust itself
					pMenuBar->m_triedRect = pMenuBar->m_rect;
				}
			}
}

#endif

#include "modals.h"

// Event processors called by user processes.
#if 1

//copied the VidPlotPixelInline code from video.c for speed:
extern uint32_t* g_framebufferCopy;
__attribute__((always_inline))
inline void blpx2cp(unsigned x, unsigned y, unsigned color)
{
	if (g_vbeData == &g_mainScreenVBEData)
		g_framebufferCopy[x + y * g_vbeData->m_width] = color;
}
__attribute__((always_inline))
inline void blpx2ver (unsigned x, unsigned y, unsigned color)
{
	g_vbeData->m_dirty = 1;
	g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32] = color;
}

__attribute__((always_inline))
inline void blpxinl(unsigned x, unsigned y, unsigned color)
{
	//if (!((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()))
	{
		blpx2cp (x, y, color);
		blpx2ver(x, y, color);
	}
}

//extern void VidPlotPixelCheckCursor(unsigned x, unsigned y, unsigned color);
void RenderWindow (Window* pWindow)
{
	if (pWindow->m_minimized)
	{
		// Draw as icon
		RenderIconForceSize(pWindow->m_iconID, pWindow->m_rect.left, pWindow->m_rect.top, 32);
		
		return;
	}
	
	//ACQUIRE_LOCK(g_screenLock);
	g_vbeData = &g_mainScreenVBEData;
	int sx = GetScreenWidth(), sy = GetScreenHeight();
	
	int windIndex = pWindow - g_windows;
	int x = pWindow->m_rect.left,  y = pWindow->m_rect.top;
	int tw = pWindow->m_vbeData.m_width, th = pWindow->m_vbeData.m_height;
	uint32_t *texture = pWindow->m_vbeData.m_framebuffer32;
	
	int o = 0;
	int x2 = x + tw, y2 = y + th;
	
	/*while (y <= -1)
	{
		o += pWindow->m_vbeData.m_width;
		y++;
	}*/
	if (y < 0)
	{
		o += -y * pWindow->m_vbeData.m_width;
		y = 0;
	}
	short n = GetWindowIndexInDepthBuffer (x, y);
	if (n == -1)
	{
		UpdateDepthBuffer();
	}
	bool isAboveEverything = true;
	
	// we still gotta decide...
	if (!pWindow->m_isSelected)
	{
		for (int j = y; j < y2; j += WINDOW_MIN_HEIGHT-1)
		{
			if (j >= sy) break;
			for (int i = x; i < x2; i += WINDOW_MIN_WIDTH-1)
			{
				short n = GetWindowIndexInDepthBuffer (i, j);
				if (n != windIndex)
				{
					isAboveEverything = false;
					break;
				}
			}
			short n = GetWindowIndexInDepthBuffer (x2 - 1, j);
			if (n != windIndex)
			{
				isAboveEverything = false;
			}
		}
		for (int i = x; i < x2; i += WINDOW_MIN_WIDTH-1)
		{
			short n = GetWindowIndexInDepthBuffer (i, y2 - 1);
			if (n != windIndex)
			{
				isAboveEverything = false;
				break;
			}
		}
		short n = GetWindowIndexInDepthBuffer (x2 - 1, y2 - 1);
		if (n != windIndex)
		{
			isAboveEverything = false;
		}
	}
	
	if (isAboveEverything)
	{
		//optimization
		//TODO FIXME: Crash when placing at the top right of the screen so that:
		//1) The y top position < 0
		//2) The x right position > ScreenWidth.
		//The crappy fix I did "for the moment" is just to disallow placement of the window at y<0.
		int ys = pWindow->m_rect.top;
		int ye = ys + pWindow->m_vbeData.m_height;
		int kys = 0, kzs = 0;
		if (ys < 0)
		{
			kys -= ys * pWindow->m_vbeData.m_width;
			kzs -= ys;
			ys = 0;
		}
		if (ye > GetScreenHeight())
			ye = GetScreenHeight();
		int xs = pWindow->m_rect.left;
		int xe = xs + pWindow->m_vbeData.m_width;
		int off = 0;
		if (xs < 0)
		{
			off = -xs;
			xs = 0;
		}
		if (xe >= GetScreenWidth())
			xe =  GetScreenWidth();
		
		int xd = (xe - xs);
		int oms = ys * g_mainScreenVBEData.m_pitch32 + xs,
		    omc = ys * g_mainScreenVBEData.m_width + xs;
		for (int y = ys, ky = kys, kz = kzs; y != ye; y++, kz++)
		{
			ky = kz * pWindow->m_vbeData.m_width + off;
			//just memcpy stuff
			memcpy_ints(&g_mainScreenVBEData.m_framebuffer32[oms], &pWindow->m_vbeData.m_framebuffer32[ky], xd);
			memcpy_ints(&g_framebufferCopy[omc], &pWindow->m_vbeData.m_framebuffer32[ky], xd);
			oms += g_mainScreenVBEData.m_pitch32;
			omc += g_mainScreenVBEData.m_width;
		}
	}
	else
	{
		int pitch  = g_vbeData->m_pitch32, width  = g_vbeData->m_width;
		int offfb,                         offcp;
		for (int j = y; j != y2; j++)
		{
			if (j >= sy) break;
			offfb = j * pitch, offcp = j * width;
			if (x > 0) offfb += x, offcp += x;
			for (int i = x; i != x2; i++)
			{
				if (i < sx && i >= 0)
				{
					short n = g_windowDepthBuffer [offcp];
					if (n == windIndex)
					{
						g_framebufferCopy         [offcp] = texture[o];
						g_vbeData->m_framebuffer32[offfb] = texture[o];
					}
					offcp++;
					offfb++;
				}
				o++;
			}
		}
	}
}
extern const unsigned char* g_pCurrentFont;
void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow)
{
	Rectangle recta = pWindow->m_rect;
	recta.right  -= recta.left; recta.left = 0;
	recta.bottom -= recta.top;  recta.top  = 0;
	
	Rectangle rectb = recta;
	
	if (!(pWindow->m_flags & WF_NOBORDER))
	{
		if (pWindow->m_flags & WF_FLATBORD)
		{
			VidDrawRect(WINDOW_TEXT_COLOR, rectb.left, rectb.top, rectb.right - 1, rectb.bottom - 1);
			/*rectb.left++;
			rectb.top++;
			rectb.right--;
			rectb.bottom--;*/
		}
		else
		{
			VidDrawHLine(0x000000, 0, rectb.right,  rectb.bottom-1);
			VidDrawVLine(0x000000, 0, rectb.bottom, rectb.right-1);
			
			rectb.left++;
			rectb.top++;
			rectb.right--;
			rectb.bottom--;
			
			VidDrawHLine(0x808080, 0, rectb.right-1,  rectb.bottom-1);
			VidDrawVLine(0x808080, 0, rectb.bottom-1, rectb.right-1);
			
			VidDrawHLine(0xFFFFFF, 1, rectb.right-2,  1);
			VidDrawVLine(0xFFFFFF, 1, rectb.bottom-2, 1);
			
			rectb.left++;
			rectb.top++;
			rectb.right--;
			rectb.bottom--;
		}
	}
	if (!(pWindow->m_flags & WF_NOTITLE))
	{
		Rectangle rectc = rectb;
		rectc.left++;
		rectc.top += TITLE_BAR_HEIGHT-2;
		rectc.right--;
		rectc.bottom--;
		
		//Cut out the gap stuff so that the animation looks good
		int iconGap = 0;
		
		//draw the window title:
		rectb.left++;
		rectb.top ++;
		rectb.right -= 2;
		rectb.bottom = rectb.top + TITLE_BAR_HEIGHT - 2;
		
		VidFillRectHGradient(
			pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR   : WINDOW_TITLE_INACTIVE_COLOR, 
			pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR_B : WINDOW_TITLE_INACTIVE_COLOR_B, 
			rectb.left,
			rectb.top,
			rectb.right,
			rectb.bottom
		);
	
		int textwidth, height;
		VidSetFont(TITLE_BAR_FONT);
		VidTextOutInternal(pWindow->m_title, 0, 0, 0, 0, true, &textwidth, &height);
		
		int MinimizAndCloseGap = 0;
		
		int offset = -5 + iconGap + (rectb.right - rectb.left - textwidth - MinimizAndCloseGap - iconGap) / 2;//-iconGap-textwidth-MinimizAndCloseGap)/2;
		
		int textOffset = (TITLE_BAR_HEIGHT) / 2 - height + 1;
		int iconOffset = (TITLE_BAR_HEIGHT) / 2 - 10;
		
		uint32_t flags = TEXT_RENDER_BOLD;
		if (TITLE_BAR_FONT != SYSTEM_FONT)
			flags = 0;
		
		VidTextOut(pWindow->m_title, rectb.left + offset + 1, rectb.top + 2 + 3 + textOffset, FLAGS_TOO(flags, WINDOW_TITLE_TEXT_COLOR_SHADOW), TRANSPARENT);
		VidTextOut(pWindow->m_title, rectb.left + offset + 0, rectb.top + 1 + 3 + textOffset, FLAGS_TOO(flags, WINDOW_TITLE_TEXT_COLOR       ), TRANSPARENT);
		VidSetFont(SYSTEM_FONT);
		
		if (pWindow->m_iconID != ICON_NULL)
			RenderIconForceSize(pWindow->m_iconID, rectb.left+1, rectb.top+1+iconOffset, 16);
	}
	
#undef X
}
void PaintWindowBorder(Window* pWindow)
{
	Rectangle recta = pWindow->m_rect;
	recta.right  -= recta.left; recta.left = 0;
	recta.bottom -= recta.top;  recta.top  = 0;
	
	//! X adjusts the size of the dropshadow on the window.
	recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS+1;
	recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS+1;
	
	VidFillRectangle(WINDOW_BACKGD_COLOR, recta);
	PaintWindowBorderNoBackgroundOverpaint (pWindow);
}
void PaintWindowBackgroundAndBorder(Window* pWindow)
{
	//VidFillScreen(TRANSPARENT);
	PaintWindowBorder(pWindow);
}

void RequestRepaintNew (Window* pWindow)
{
	//paint the window background:
	PaintWindowBackgroundAndBorder (pWindow);
	
	CallWindowCallbackAndControls  (pWindow, EVENT_PAINT, 0, 0);
}
bool IsEventDestinedForControlsToo(int type)
{
	switch (type)
	{
		case EVENT_DESTROY:
		case EVENT_IMAGE_REFRESH:
		case EVENT_PAINT:
		case EVENT_MOVECURSOR:
		case EVENT_CLICKCURSOR:
		case EVENT_RELEASECURSOR:
		case EVENT_RIGHTCLICK:
		case EVENT_RIGHTCLICKRELEASE:
		case EVENT_KEYPRESS:
		case EVENT_KEYRAW:
		case EVENT_SIZE:
		case EVENT_MENU_CLOSE:
			return true;
	}
	return false;
}

//ugly hax to make calling window callback not need to preserve edi, esi, ebx
//this was not an issue with no optimization but is now
int __attribute__((noinline)) CallWindowCallback(Window* pWindow, int eq, int eqp1, int eqp2)
{
	pWindow->m_callback(pWindow, eq, eqp1, eqp2);
	return eq * eqp1 * eqp2;
}
int __attribute__((noinline)) CallWindowCallbackAndControls(Window* pWindow, int eq, int eqp1, int eqp2)
{
	if ((eq != EVENT_CLICKCURSOR && eq != EVENT_RELEASECURSOR) || eqp2 != 1)
	{
		pWindow->m_callback(pWindow, eq, eqp1, eqp2);
	}
	
	if (IsEventDestinedForControlsToo(eq))
	{
		ControlProcessEvent(pWindow, eq, eqp1, eqp2);
	}
	
	return eq * eqp1 * eqp2;
}

int someValue = 0;

void UpdateControlsBasedOnAnchoringModes(UNUSED Window* pWindow, int oldSizeParm, int newSizeParm)
{
	//SLogMsg("TODO!");
	int oldSizeX = GET_X_PARM(oldSizeParm), oldSizeY = GET_Y_PARM(oldSizeParm);
	int newSizeX = GET_X_PARM(newSizeParm), newSizeY = GET_Y_PARM(newSizeParm);
	
	int sizeDifX = oldSizeX - newSizeX;
	int sizeDifY = oldSizeY - newSizeY;
	
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_active)
		{
			Control *pControl = &pWindow->m_pControlArray[i];
			
			if (pControl->m_anchorMode & ANCHOR_LEFT_TO_RIGHT)
				pControl->m_triedRect.left   += sizeDifX;
			if (pControl->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				pControl->m_triedRect.right  += sizeDifX;
			if (pControl->m_anchorMode & ANCHOR_TOP_TO_BOTTOM)
				pControl->m_triedRect.top    += sizeDifY;
			if (pControl->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				pControl->m_triedRect.bottom += sizeDifY;
			
			pControl->m_rect = pControl->m_triedRect;
			if (pControl->m_rect.right  - pControl->m_rect.left < 10)
				pControl->m_rect.right  = pControl->m_rect.left + 10;
			if (pControl->m_rect.bottom - pControl->m_rect.top  < 10)
				pControl->m_rect.bottom = pControl->m_rect.top  + 10;
		}
	}
}

void WinAddToInputQueue (Window* this, char input)
{
	if (!input) return;
	
	this->m_inputBuffer[this->m_inputBufferEnd++] = input;
	while
	   (this->m_inputBufferEnd >= KB_BUF_SIZE)
		this->m_inputBufferEnd -= KB_BUF_SIZE;
}
bool WinAnythingOnInputQueue (Window* this)
{
	return this->m_inputBufferBeg != this->m_inputBufferEnd;
}
char WinReadFromInputQueue (Window* this)
{
	if (WinAnythingOnInputQueue(this))
	{
		char k = this->m_inputBuffer[this->m_inputBufferBeg++];
		while
		   (this->m_inputBufferBeg >= KB_BUF_SIZE)
			this->m_inputBufferBeg -= KB_BUF_SIZE;
		return k;
	}
	else return 0;
}

static bool OnProcessOneEvent(Window* pWindow, int eventType, int parm1, int parm2)
{
	//setup paint stuff so the window can only paint in their little box
	VidSetVBEData (&pWindow->m_vbeData);
	VidSetFont(SYSTEM_FONT);
	pWindow->m_vbeData.m_dirty = 0;
	//pWindow->m_renderFinished = false;
	//todo: switch case much?
	if (eventType == EVENT_MINIMIZE)
	{
		Rectangle old_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
		
		VidSetVBEData (NULL);
		HideWindow (pWindow);
		if (!pWindow->m_minimized)
		{
			pWindow->m_minimized   = true;
			pWindow->m_rectBackup  = pWindow->m_rect;
			
			pWindow->m_rect.left += (pWindow->m_rect.right  - pWindow->m_rect.left - 32) / 2;
			pWindow->m_rect.top  += (pWindow->m_rect.bottom - pWindow->m_rect.top  - 32) / 2;
			pWindow->m_rect.right  = pWindow->m_rect.left + 32;
			pWindow->m_rect.bottom = pWindow->m_rect.top  + 32;
		}
		//pWindow->m_hidden = false;
		//UpdateDepthBuffer();
		
		ShowWindow (pWindow);
		
		VidSetVBEData (&pWindow->m_vbeData);
		
		Rectangle new_title_rect = pWindow->m_rect;
		
		CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
	}
	else if (eventType == EVENT_UNMINIMIZE)
	{
		Rectangle old_title_rect = pWindow->m_rect;
		
		VidSetVBEData (NULL);
		HideWindow (pWindow);
		
		pWindow->m_minimized   = false;
		pWindow->m_rect = pWindow->m_rectBackup;
		
		ShowWindow (pWindow);
		
		UpdateDepthBuffer();
		VidSetVBEData (&pWindow->m_vbeData);
		PaintWindowBackgroundAndBorder(pWindow);
		
		OnProcessOneEvent(pWindow, EVENT_PAINT, 0, 0);
		
		pWindow->m_renderFinished = true;
		
		Rectangle new_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
		
		CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
	}
	else if (eventType == EVENT_SIZE)
	{
		PaintWindowBackgroundAndBorder(pWindow);
		
		// Update controls based on their anchoring modes.
		UpdateControlsBasedOnAnchoringModes (pWindow, parm1, parm2);
	}
	else if (eventType == EVENT_MAXIMIZE)
	{
		Rectangle old_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
		
		if (!pWindow->m_maximized)
			pWindow->m_rectBackup = pWindow->m_rect;
		pWindow->m_maximized  = true;
		pWindow->m_rect.left = -2;
		pWindow->m_rect.top  = -2+g_TaskbarHeight;
		ResizeWindow(pWindow, -1, -1, GetScreenWidth() + 2, GetScreenHeight() - g_TaskbarHeight + 2);
		
		SetLabelText(pWindow, 0xFFFF0002, "\x1F");//TODO: 0xA technically has the restore icon, but that's literally '\n', so we'll use \x1F for now
		SetIcon     (pWindow, 0xFFFF0002, EVENT_UNMAXIMIZE);
		
		pWindow->m_renderFinished = true;
		
		Rectangle new_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
		
		CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
	}
	else if (eventType == EVENT_UNMAXIMIZE)
	{
		Rectangle old_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
		
		if (pWindow->m_maximized)
			ResizeWindow(pWindow, pWindow->m_rectBackup.left, pWindow->m_rectBackup.top, pWindow->m_rectBackup.right - pWindow->m_rectBackup.left, pWindow->m_rectBackup.bottom - pWindow->m_rectBackup.top);
		pWindow->m_maximized = false;
		
		Rectangle new_title_rect = { pWindow->m_rect.left + 3, pWindow->m_rect.top + 3, pWindow->m_rect.right - 3, pWindow->m_rect.top + 3 + TITLE_BAR_HEIGHT };
		
		SetLabelText(pWindow, 0xFFFF0002, "\x08");
		SetIcon     (pWindow, 0xFFFF0002, EVENT_MAXIMIZE);
		
		pWindow->m_renderFinished = true;
		
		CreateMovingRectangleEffect(old_title_rect, new_title_rect, pWindow->m_title);
	}
	else if (eventType == EVENT_CREATE)
	{
		PaintWindowBackgroundAndBorder(pWindow);
		DefaultWindowProc (pWindow, EVENT_CREATE, 0, 0);
	}
	else if (eventType == EVENT_PAINT)
	{
		PaintWindowBorderNoBackgroundOverpaint(pWindow);
	}
	
	someValue = CallWindowCallbackAndControls(pWindow, eventType, parm1, parm2);
	
	//reset to main screen
	VidSetVBEData (NULL);
	if (!pWindow->m_minimized)
	{
		if (pWindow->m_vbeData.m_dirty)
		{
			pWindow->m_renderFinished = true;
		}
	}
	else
		pWindow->m_renderFinished = true;
	
	//if the contents of this window have been modified, redraw them:
	//if (pWindow->m_vbeData.m_dirty && !pWindow->m_hidden)
	//	RenderWindow(pWindow);
	
	if (eventType == EVENT_SIZE)
	{
		OnProcessOneEvent(pWindow, EVENT_PAINT, 0, 0);
		
		pWindow->m_renderFinished = true;
	}
	else if (eventType == EVENT_CREATE)
	{
		AddWindowToDrawOrder (pWindow - g_windows);
		ShowWindow(pWindow);
		SelectWindow(pWindow);
	}
	else if (eventType == EVENT_DESTROY)
	{
		pWindow->m_eventQueueSize = 0;
		
		FREE_LOCK (pWindow->m_eventQueueLock);
		KeTaskDone();
		
		NukeWindow(pWindow);
		
		return false;
	}
	return true;
}

bool HandleMessages(Window* pWindow)
{
	// grab the lock
	ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	
	// While we have events in the master queue...
	int et = 0, p1 = 0, p2 = 0;
	while (WindowPopEventFromQueue(pWindow, &et, &p1, &p2))
	{
		if (!OnProcessOneEvent(pWindow, et, p1, p2))
			return false;
	}
	
	// While we have events in our own queue...
	for (int i = 0; i < pWindow->m_eventQueueSize; i++)
	{
		if (!OnProcessOneEvent(pWindow, pWindow->m_eventQueue[i], pWindow->m_eventQueueParm1[i], pWindow->m_eventQueueParm2[i]))
			return false;
	}
	pWindow->m_eventQueueSize = 0;
	
	// Keyboard events are handled separately, in games you may miss input otherwise...

	while (WinAnythingOnInputQueue(pWindow))
	{
		unsigned char out = WinReadFromInputQueue(pWindow);
		
		OnProcessOneEvent(pWindow, EVENT_KEYRAW, out, 0);
		
		// if the key was just pressed:
		if ((out & 0x80) == 0)
		{
			// convert it to a standard char
			char sensible = KbMapAtCodeToChar (out & 0x7F);
			
			if (sensible)
				OnProcessOneEvent(pWindow, EVENT_KEYPRESS, sensible, 0);
		}
	}
	
	FREE_LOCK (pWindow->m_eventQueueLock);
	KeTaskDone();//hlt; //give it a good halt
	return true;
}
void DefaultWindowProc (Window* pWindow, int messageType, UNUSED int parm1, UNUSED int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			// Add a default QUIT button control.
			
			if (pWindow->m_flags & WI_INITGOOD) break;
			
			if (!(pWindow->m_flags & WF_NOCLOSE))
			{
				bool has3dBorder = !(pWindow->m_flags & WF_FLATBORD);
				
				#define ACTION_BUTTON_WIDTH 18
				
				Rectangle rect;
				rect.right = pWindow->m_vbeData.m_width - 3 - WINDOW_RIGHT_SIDE_THICKNESS - has3dBorder * 2;
				rect.left  = rect.right - ACTION_BUTTON_WIDTH+2;
				rect.top   = 2 + has3dBorder * 2;
				rect.bottom= rect.top + TITLE_BAR_HEIGHT-4;
				AddControlEx (pWindow, CONTROL_BUTTON_EVENT, ANCHOR_LEFT_TO_RIGHT | ANCHOR_RIGHT_TO_RIGHT, rect, "\x09", 0xFFFF0000, EVENT_CLOSE, 0);
				
				#ifdef ENABLE_MAXIMIZE
				if (!(pWindow->m_flags & WF_NOMAXIMZ))
				{
					rect.left -= ACTION_BUTTON_WIDTH;
					rect.right -= ACTION_BUTTON_WIDTH;
					AddControlEx (pWindow, CONTROL_BUTTON_EVENT, ANCHOR_LEFT_TO_RIGHT | ANCHOR_RIGHT_TO_RIGHT, rect, "\x08", 0xFFFF0002, EVENT_MAXIMIZE, 0);
				}
				#endif
				
				if (!(pWindow->m_flags & WF_NOMINIMZ))
				{
					rect.left -= ACTION_BUTTON_WIDTH;
					rect.right -= ACTION_BUTTON_WIDTH;
					AddControlEx (pWindow, CONTROL_BUTTON_EVENT, ANCHOR_LEFT_TO_RIGHT | ANCHOR_RIGHT_TO_RIGHT, rect, "\x07", 0xFFFF0001, EVENT_MINIMIZE, 0);
				}
			}
			
			pWindow->m_flags |= WI_INITGOOD;
			
			break;
		}
		case EVENT_PAINT:
			//nope, user should handle this themselves
			//Actually EVENT_PAINT just requests a paint event,
			//so just mark this as dirty
			pWindow->m_vbeData.m_dirty = 1;
			break;
		case EVENT_SETFOCUS:
		case EVENT_KILLFOCUS:
			PaintWindowBorder(pWindow);
			break;
		case EVENT_CLOSE:
			DestroyWindow(pWindow);
			break;
		case EVENT_DESTROY:
			//NukeWindow(pWindow);//exits
			break;
		default:
			break;
	}
}
#endif