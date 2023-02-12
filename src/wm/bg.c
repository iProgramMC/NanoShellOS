/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

         Window Manager BG Module
******************************************/
#include "wi.h"

#define g_BackgroundSolidColor (GetThemingParameter(BACKGROUND_COLOR))
//uint32_t g_BackgroundSolidColor = BACKGROUND_COLOR;
bool     g_BackgroundSolidColorActive = true;

extern int g_DefaultCursorID;

typedef void (*DisposeBackgroundFunc)(Image* pImg);

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

// If you use the `g_defaultBackground`, you must use this.
void WmDisposeDefaultBackground(UNUSED Image* pImg)
{
}

// Note: The image MUST be created with BitmapDuplicate!
void WmDisposeImageBackground(Image* pImg)
{
	MmFree(pImg);
}

DisposeBackgroundFunc g_DisposeBackground = WmDisposeDefaultBackground;

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

__attribute__((always_inline))
inline void VidPlotPixelInlineRF(unsigned x, unsigned y, unsigned color)
{
	if (!((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()))
	{
		VidPlotPixelToCopyInlineUnsafeRF(x, y, color);
		VidPlotPixelRaw32IRF (x, y, color);
	}
}

void RedrawBackground (Rectangle rect)
{
	if (g_BackgroundSolidColorActive)
	{
		/*rect.right--, rect.bottom--;
		VidFillRectangle(GetThemingParameter(P_BACKGROUND_COLOR), rect);*/
		
		VidBitBlit(
			g_vbeData,
			rect.left, rect.top,
			rect.right  - rect.left,
			rect.bottom - rect.top,
			NULL,
			(int)GetThemingParameter(P_BACKGROUND_COLOR),
			0,
			BOP_DSTFILL
		);
		
		return;
	}
	
	if (rect.left < 0) rect.left = 0;
	if (rect.top  < 0) rect.top  = 0;
	if (rect.right  >= GetScreenWidth ()) rect.right  = GetScreenWidth ();
	if (rect.bottom >= GetScreenHeight()) rect.bottom = GetScreenHeight();
	
	// if the rectangle is FULLY inside the 0,0 tile:
	// (TODO: Make this work on any tile)
	// (Another TODO: If there's one horz seam or one vert seam, split the main rect into 2 rects across the seam
	//  and call RedrawBackground on them)
	int rlc = rect.left / g_background->width,  rrc = (rect.right  - 1) / g_background->width;
	int rtc = rect.top  / g_background->height, rbc = (rect.bottom - 1) / g_background->height;
	if (rlc == rrc && rtc == rbc && rlc == 0 && rtc == 0)
	{
		//just draw the clipped portion
		
		// TODO: More complete fill-in of the VBEData structure
		VBEData data;
		data.m_bitdepth = 2;
		data.m_width    = data.m_pitch32 = g_background->width;
		data.m_height   = g_background->height;
		data.m_framebuffer32 = (uint32_t*)g_background->framebuffer;
		
		VidBitBlit(g_vbeData,
			rect.left, rect.top,
			rect.right  - rect.left,
			rect.bottom - rect.top,
			&data,
			rect.left, rect.top,
			BOP_SRCCOPY
		);
		
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
			
			VidPlotPixelInlineRF (x, y, g_background->framebuffer[xa + g_background->width * ymod]);
		}
	}
	
	//simple background:
	/*VidFillRectangle (BACKGROUND_COLOR, rect);*/
}

void RefreshScreen()
{
	Rectangle rect = { 0, 0, GetScreenWidth(), GetScreenHeight() };
	RefreshRectangle(rect, NULL);
}

void RefreshEverything()
{
	for (int i = 0; i < WINDOWS_MAX; i++)
	{
		if (!g_windows[i].m_used) continue;
		if (g_windows[i].m_hidden) continue;
		
		WindowAddEventToMasterQueue(&g_windows[i], EVENT_REPAINT_PRIVATE, 0, 0);
	}
	
	Rectangle rect = { 0, 0, GetScreenWidth(), GetScreenHeight() };
	RefreshRectangle(rect, NULL);
}

void SetBackgroundSolidColor()
{
	LockAcquire(&g_BackgdLock);
	g_BackgroundSolidColorActive = true;
	
	g_DisposeBackground(g_background);
	g_background = &g_defaultBackground;
	g_DisposeBackground = WmDisposeDefaultBackground;
	
	LockFree(&g_BackgdLock);
	RefreshScreen();
}

// A safe function to set the background image.
void SetBackgroundImage(Image* pImage)
{
	if (!pImage)
	{
		SetBackgroundSolidColor();
		g_DefaultCursorID = CURSOR_DEFAULT;
		return;
	}
	
	LockAcquire(&g_BackgdLock);
	
	g_BackgroundSolidColorActive = true;
	
	Image* pOldBG = g_background;
	g_background = BitmapDuplicate(pImage);
	
	// we couldn't duplicate the bitmap? fine
	if (!g_background)
	{
		g_background = pOldBG;
		LockFree(&g_BackgdLock);
		return;
	}
	
	// Dispose of the old background
	g_DisposeBackground(pOldBG);
	
	// Set the new dispose function.
	g_DisposeBackground = WmDisposeImageBackground;
	
	g_BackgroundSolidColorActive = false;
	
	LockFree(&g_BackgdLock);
	
	RefreshScreen();
	
	g_DefaultCursorID = CURSOR_DEFAULT;
}

void VidPrintTestingPattern2(uint32_t or_mask, uint32_t y_shift)
{
	for (int y = 0, z = 0; y < GetScreenSizeY(); y++, z += g_vbeData->m_pitch32) 
	{
		uint32_t ty = (y * 255 / GetScreenSizeY());
		ty = (ty << y_shift) | ((ty << y_shift) << 8);
		for (int x = 0; x < GetScreenSizeX(); x++)
		{
			uint32_t pixel = or_mask | ty;
			g_vbeData->m_framebuffer32[z + x] = pixel;
		}
	}
}

void GenerateBackground()
{
	ConfigEntry *pEntryGradient = CfgGetEntry ("Theming::BackgroundGradient");
	if (!pEntryGradient) return;
	
	if (strcmp (pEntryGradient->value, "yes")) return;
	
	Image *pImage = BitmapAllocate (GetScreenWidth(), GetScreenHeight(), 0xFFFFFFFF);
	
	if (!pImage)
	{
		SetBackgroundSolidColor();
	}
	else
	{
		VBEData sData;
		BuildGraphCtxBasedOnImage (&sData, pImage);
		
		VidSetVBEData (&sData);
		VidPrintTestingPattern2(0x0000FF, 8);
		VidSetVBEData (NULL);
		
		SetBackgroundImage(pImage);
	}
	
	MmFree(pImage);
}

void WmBackgroundLoaderThread(UNUSED int parameter)
{
	ConfigEntry *pEntry = CfgGetEntry ("Theming::BackgroundFile");
	const char *pFileName = NULL;	
	if (pEntry)
	{
		pFileName = pEntry->value;
	}
	
	//Try to open a file now.
	if (!pFileName)
	{
		SLogMsg("There is no wallpaper. Using default one!");
		GenerateBackground();
		return;
	}
	
	int fd = FiOpen(pFileName, O_RDONLY);
	if (fd < 0)
	{
		SLogMsg("Could not open wallpaper file '%s'. Using default one!", pFileName);
		GenerateBackground();
		return;
	}
	
	int size = FiTellSize(fd);
	
	uint8_t* pData = MmAllocate(size);
	if (!pData)
	{
		SLogMsg("Could not allocate %d bytes for wallpaper data... Using default wallpaper!", pData);
		GenerateBackground();
		FiClose(fd);
		return;
	}
	
	FiRead(fd, pData, size);
	
	FiClose(fd);
	
	int errorCode = 0;
	Image* pImage = LoadImageFile(pData, &errorCode);
	MmFree(pData);
	
	if (pImage)
	{
		SetBackgroundImage(pImage);
		MmFree(pImage);
	}
	else
	{
		SLogMsg("Could not load wallpaper data (errorcode: %d). Using default one!", errorCode);
		GenerateBackground();
	}
}

void SetDefaultBackground()
{
	g_background = &g_defaultBackground;
	
	UNUSED int errorCode = 0;
	Task* task = KeStartTask(WmBackgroundLoaderThread, 0, &errorCode);
	KeUnsuspendTask(task);
}
