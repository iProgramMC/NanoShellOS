/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

             VBE Video module
******************************************/
#include <main.h>
#include <vga.h>
#include <video.h>
#include <memory.h>
#include <string.h>
#include <icon.h>

//! TODO: perhaps merge vga.c with this?
#include "extra/fonts.h"

// Basic definitions for video
#if 1
bool g_isVideoMode = false;
uint32_t g_framebufferPhysical = 0;

extern uint32_t *g_curPageDir;

//! TODO: if planning to extend this beyond 1920x1080x32, extend THIS variable.
#define MAX_VIDEO_PAGES 2048
#define FRAMEBUFFER_MAPPED_ADDR 0xE0000000
uint32_t g_vbePageEntries[MAX_VIDEO_PAGES] __attribute__((aligned(4096))); 
uint32_t* g_framebufferCopy = NULL;

VBEData g_mainScreenVBEData;

VBEData* g_vbeData = NULL;
#endif

// Mouse graphics stuff
#if 1

int g_mouseX = 0, g_mouseY = 0;


#define X 0X00FFFFFF,
#define B 0XFF000000,
#define o TRANSPARENT,

uint32_t g_cursorColors[] = 
{
	B o o o o o o o o o o o
	B B o o o o o o o o o o
	B X B o o o o o o o o o
	B X X B o o o o o o o o
	B X X X B o o o o o o o
	B X X X X B o o o o o o
	B X X X X X B o o o o o
	B X X X X X X B o o o o
	B X X X X X X X B o o o
	B X X X X X X X X B o o
	B X X X X X X X X X B o
	B X X X X X X B B B B B
	B X X X B X X B o o o o
	B X X B B X X B o o o o
	B X B o o B X X B o o o
	B B o o o B X X B o o o
	B o o o o o B X X B o o
	o o o o o o B X X B o o
	o o o o o o o B X X B o
	o o o o o o o B X X B o
	o o o o o o o o B B o o
};
uint32_t g_waitCursorColors[] = 
{
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

Cursor g_defaultCursor = {
	12, 21, 0, 0, 
	g_cursorColors,
	true
};
Cursor g_waitCursor = {
	14, 22, 0, 0, 
	g_waitCursorColors,
	true
};
#undef X
#undef B
#undef o
Cursor* g_currentCursor = NULL;
Cursor* g_pDefaultCursor = NULL; // pointer for easy checking
Cursor* g_pWaitCursor = NULL; // pointer for easy checking

bool g_isMouseVisible = false;

//forward decls to stuff
unsigned VidReadPixel (unsigned x, unsigned y);
static void VidPlotPixelIgnoreCursorChecksChecked(unsigned x, unsigned y, unsigned color);

//Click queue to handle clicks later (window manager)
ClickInfo g_clickQueue [CLICK_INFO_MAX];
int       g_clickQueueSize = 0;
bool      g_clickQueueLock = false;

typedef struct
{
	int newX, newY;
	bool updated;
}
MouseMoveQueue;

MouseMoveQueue g_queueMouseUpdateTo;

void RefreshMouse()
{
	if (g_queueMouseUpdateTo.updated)
	{
		g_queueMouseUpdateTo.updated = false;
		SetMousePos(g_queueMouseUpdateTo.newX, g_queueMouseUpdateTo.newY);
	}
	hlt;
}

void AddClickInfoToQueue(const ClickInfo* info)
{
	ACQUIRE_LOCK (g_clickQueueLock);
	
	if (g_clickQueueSize >= CLICK_INFO_MAX)
	{
		//only handle the next clicks now
		g_clickQueueSize = 0;
	}
	g_clickQueue[g_clickQueueSize++] = *info;
	
	FREE_LOCK (g_clickQueueLock);
}
void OnLeftClick()
{
	ClickInfo info;
	info.clickType = CLICK_LEFT;
	info.clickedAtX = g_mouseX;
	info.clickedAtY = g_mouseY;
	AddClickInfoToQueue (&info);
}
void OnLeftClickRelease()
{
	ClickInfo info;
	info.clickType = CLICK_LEFTR;
	info.clickedAtX = g_mouseX;
	info.clickedAtY = g_mouseY;
	AddClickInfoToQueue (&info);
}
void OnLeftClickDrag()
{
	ClickInfo info;
	info.clickType = CLICK_LEFTD;
	info.clickedAtX = g_mouseX;
	info.clickedAtY = g_mouseY;
	AddClickInfoToQueue (&info);
}
void OnRightClick()
{
	ClickInfo info;
	info.clickType = CLICK_RIGHT;
	info.clickedAtX = g_mouseX;
	info.clickedAtY = g_mouseY;
	AddClickInfoToQueue (&info);
}

uint8_t g_previousFlags = 0;
void ForceKernelTaskToRunNext();
void OnUpdateMouse(uint8_t flags, uint8_t Dx, uint8_t Dy, __attribute__((unused)) uint8_t Dz)
{
	int dx, dy;
	dx = (flags & (1 << 4)) ? (int8_t)Dx : Dx;
	dy = (flags & (1 << 5)) ? (int8_t)Dy : Dy;
	
	//move the cursor:
	int newX = g_mouseX + dx;
	int newY = g_mouseY - dy;
	if (newX < 0) newX = 0;
	if (newY < 0) newY = 0;
	//SetMousePos (newX, newY);
	
	g_queueMouseUpdateTo.newX = newX;
	g_queueMouseUpdateTo.newY = newY;
	g_queueMouseUpdateTo.updated = true;
	
	if (flags & MOUSE_FLAG_R_BUTTON)
	{
		if (!(g_previousFlags & MOUSE_FLAG_R_BUTTON))
			OnRightClick();
		ForceKernelTaskToRunNext (); //window manager likes this
	}
	if (flags & MOUSE_FLAG_L_BUTTON)
	{
		if (!(g_previousFlags & MOUSE_FLAG_L_BUTTON))
			OnLeftClick();
		else
			OnLeftClickDrag();
		ForceKernelTaskToRunNext (); //window manager likes this
	}
	else if (g_previousFlags & MOUSE_FLAG_L_BUTTON)
	{
		OnLeftClickRelease();
		ForceKernelTaskToRunNext (); //window manager likes this
	}
	
	g_previousFlags = flags & 7;
}

Cursor* GetCurrentCursor()
{
	return g_currentCursor;
}

void SetCursor(Cursor* pCursor)
{
	if (!pCursor) pCursor = g_pDefaultCursor;
	if (g_currentCursor == pCursor) return;
	
	cli;
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	
	int mx = g_mouseX, my = g_mouseY;
	
	//undraw the old cursor:
	if (g_currentCursor)
	{
		for (int i = -2; i <= g_currentCursor->height + 1; i++)
		{
			for (int j = -2; j <= g_currentCursor->width + 1; j++)
			{
				int x = mx + j - g_currentCursor->leftOffs;
				int y = my + i - g_currentCursor->topOffs;
				VidPlotPixelIgnoreCursorChecksChecked (x, y, VidReadPixel(x, y));
			}
		}
	}
	
	//draw the new cursor:
	g_currentCursor = pCursor;
	for (int i = 0; i < g_currentCursor->height; i++)
	{
		for (int j = 0; j < g_currentCursor->width; j++)
		{
			int id = i * g_currentCursor->width + j;
			if (g_currentCursor->bitmap[id] != TRANSPARENT)
			{
				VidPlotPixelIgnoreCursorChecksChecked(
					j + mx - g_currentCursor->leftOffs,
					i + my - g_currentCursor->topOffs,
					g_currentCursor->bitmap[id]
				);
			}
		}
	}
	
	
	
	g_vbeData = backup;
	
	sti;
}

void SetMouseVisible (bool b)
{
	g_isMouseVisible = b;
	if (!g_isMouseVisible)
	{
		for (int i = 0; i < g_currentCursor->height; i++)
		{
			if (i + g_mouseY - g_currentCursor->topOffs >= GetScreenSizeY()) break;
			for (int j = 0; j < g_currentCursor->width; j++)
			{
				if (j + g_mouseX - g_currentCursor->leftOffs >= GetScreenSizeX()) break;
				int id = i * g_currentCursor->width + j;
				if (g_currentCursor->bitmap[id] != TRANSPARENT)
				{
					int kx = j + g_mouseX - g_currentCursor->leftOffs,
						ky = i + g_mouseY - g_currentCursor->topOffs;
					VidPlotPixelIgnoreCursorChecksChecked (
						kx, ky, VidReadPixel (kx, ky)
					);
				}
			}
		}
	}
	else if (g_vbeData->m_bitdepth == 2)
	{
		//NEW: Optimization
		int ys =                         - g_currentCursor->topOffs + g_mouseY;
		int ye = g_currentCursor->height - g_currentCursor->topOffs + g_mouseY;
		int kys = 0, kzs = 0;
		if (ys < 0)
		{
			kys -= ys * g_currentCursor->width;
			kzs -= ys;
			ys = 0;
		}
		int xs =                         - g_currentCursor->leftOffs+ g_mouseX;
		int xe = g_currentCursor->width  - g_currentCursor->leftOffs+ g_mouseX;
		int off = 0;
		if (xs < 0)
		{
			off = -xs;
			xs = 0;
		}
		if (xe >= GetScreenSizeX())
			xe = GetScreenSizeX() - 1;
		//int xd = (xe - xs) * sizeof(uint32_t);
		for (int y = ys, ky = kys, kz = kzs; y < ye; y++, kz++)
		{
			ky = kz * g_currentCursor->width + off;
			//just memcpy shit
			//memcpy (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + xs], &g_currentCursor->bitmap[ky], xd);
			for (int x = xs; x < xe; x++)
			{
				if (g_currentCursor->bitmap[ky] != TRANSPARENT)
					g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + x] = g_currentCursor->bitmap[ky];
				ky++;
			}
		}
	}
	else
	{
		for (int i = 0; i < g_currentCursor->height; i++)
		{
			if (i + g_mouseY - g_currentCursor->topOffs >= GetScreenSizeY()) break;
			for (int j = 0; j < g_currentCursor->width; j++)
			{
				if (j + g_mouseX - g_currentCursor->leftOffs >= GetScreenSizeX()) break;
				int id = i * g_currentCursor->width + j;
				if (g_currentCursor->bitmap[id] != TRANSPARENT)
				{
					int kx = j + g_mouseX - g_currentCursor->leftOffs,
						ky = i + g_mouseY - g_currentCursor->topOffs;
					VidPlotPixelIgnoreCursorChecksChecked (
						kx, ky, g_currentCursor->bitmap[id]
					);
				}
			}
		}
	}
}

void SetDefaultCursor ()
{
	g_currentCursor = &g_defaultCursor;
	g_pDefaultCursor = &g_defaultCursor;
	g_pWaitCursor = &g_waitCursor;
}
#endif

// Getters
#if 1
bool VidIsAvailable()
{
	if (!g_vbeData) return false;
	return g_vbeData->m_available;
}
int GetScreenSizeX()
{
	if (!g_vbeData) return 0;
	return g_vbeData->m_width;
}
int GetScreenSizeY()
{
	if (!g_vbeData) return 0;
	return g_vbeData->m_height;
}
int GetWidth (Rectangle* rect)
{
	return rect->right - rect->left;
}
int GetHeight (Rectangle* rect)
{
	return rect->bottom - rect->top;
}
#endif

// Graphical drawing routines
#if 1
typedef void (*PlotPixelFunction)(unsigned x, unsigned y, unsigned color);

//! Temporary placeholder functions:
char VidColor32to8(unsigned color)
{
	return color & 0xFF;
}
short VidColor32to16(unsigned color)
{
	return color & 0xFF;
}

void VidPlotPixelRaw8  (unsigned x, unsigned y, unsigned color)
{
	g_vbeData->m_dirty = 1;
	g_vbeData->m_framebuffer8 [x + y * g_vbeData->m_pitch  ] = VidColor32to8 (color);
}
void VidPlotPixelRaw16 (unsigned x, unsigned y, unsigned color)
{
	g_vbeData->m_dirty = 1;
	g_vbeData->m_framebuffer16[x + y * g_vbeData->m_pitch16] = VidColor32to16(color);
}
void VidPlotPixelRaw32 (unsigned x, unsigned y, unsigned color)
{
	g_vbeData->m_dirty = 1;
	g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32] = color;
}
__attribute__((always_inline))
inline void VidPlotPixelRaw32I (unsigned x, unsigned y, unsigned color)
{
	g_vbeData->m_dirty = 1;
	g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32] = color;
}
inline void VidPlotPixelIgnoreCursorChecksChecked(unsigned x, unsigned y, unsigned color)
{
	if ((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()) return;
	VidPlotPixelRaw32I(x, y, color);
}
__attribute__((always_inline))
inline void VidPlotPixelToCopyInlineUnsafe(unsigned x, unsigned y, unsigned color)
{
	if (g_vbeData == &g_mainScreenVBEData)
		g_framebufferCopy[x + y * g_vbeData->m_width] = color;
}
void VidPlotPixel(unsigned x, unsigned y, unsigned color)
{
	if ((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()) return;
	VidPlotPixelToCopyInlineUnsafe(x, y, color);
	VidPlotPixelRaw32I (x, y, color);
}
__attribute__((always_inline))
inline void VidPlotPixelInline(unsigned x, unsigned y, unsigned color)
{
	if (!((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()))
	{
		VidPlotPixelToCopyInlineUnsafe(x, y, color);
		VidPlotPixelRaw32I (x, y, color);
	}
}
void VidPlotPixelCheckCursor(unsigned x, unsigned y, unsigned color)
{
	if ((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()) return;
	VidPlotPixelToCopyInlineUnsafe(x, y, color);
	
	// if inside the cursor area, don't display this pixel on the screen:
	if (g_vbeData == &g_mainScreenVBEData)
	{
		if (g_currentCursor && g_isMouseVisible)
		{
			if ((int)x >= g_mouseX - g_currentCursor->leftOffs &&
				(int)y >= g_mouseY - g_currentCursor->topOffs  &&
				(int)x <  g_mouseX + g_currentCursor->width  - g_currentCursor->leftOffs &&
				(int)y <  g_mouseY + g_currentCursor->height - g_currentCursor->topOffs)
			{
				int mx = x - g_mouseX + g_currentCursor->leftOffs;
				int my = y - g_mouseY + g_currentCursor->topOffs;
				int index = my * g_currentCursor->width + mx;
				if (g_currentCursor->bitmap[index] != TRANSPARENT)
				{
					return;
				}
			}
		}
	}
	VidPlotPixelRaw32I (x, y, color);
}
void VidPrintTestingPattern()
{
	for (int y = 0; y < GetScreenSizeY(); y++) 
	{
		for (int x = 0; x < GetScreenSizeX(); x++)
		{
			int pixel = (x + y) * 0x010101;
			VidPlotPixelInline(x, y, pixel);
		}
	}
}
void VidFillScreen(unsigned color)
{
	g_vbeData->m_dirty = 1;
	int color2 = color;
	bool alsoToTheCopy = (g_vbeData == &g_mainScreenVBEData);
	switch (g_vbeData->m_bitdepth)
	{
		case 0:
			color2 = VidColor32to8(color);
			for (int y = 0; y < GetScreenSizeY(); y++) 
				for (int x = 0; x < GetScreenSizeX(); x++)
				{
					g_vbeData->m_framebuffer8 [x + y * g_vbeData->m_pitch  ] = color2;
					if (alsoToTheCopy) g_framebufferCopy[x + y * g_vbeData->m_width] = color;
				}
			break;
		case 1:
			color2 = VidColor32to16(color);
			for (int y = 0; y < GetScreenSizeY(); y++) 
				for (int x = 0; x < GetScreenSizeX(); x++)
				{
					g_vbeData->m_framebuffer16[x + y * g_vbeData->m_pitch16] = color2;
					if (alsoToTheCopy) g_framebufferCopy[x + y * g_vbeData->m_width] = color;
				}
			break;
		case 2:
			for (int y = 0; y < GetScreenSizeY(); y++) 
				for (int x = 0; x < GetScreenSizeX(); x++)
				{
					g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32] = color;
					if (alsoToTheCopy) g_framebufferCopy[x + y * g_vbeData->m_width] = color;
				}
			break;
	}
}
void VidFillRect(unsigned color, int left, int top, int right, int bottom)
{
	//basic clipping:
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= GetScreenSizeX()) right = GetScreenSizeX() - 1;
	if (bottom >= GetScreenSizeY()) bottom = GetScreenSizeY() - 1;
	
	for (int y = top; y <= bottom; y++)
	{
		for (int x = left; x <= right; x++)
			VidPlotPixelInline(x, y, color);
	}
}
void VidFillRectHGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom)
{
	//basic clipping:
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= GetScreenSizeX()) right = GetScreenSizeX() - 1;
	if (bottom >= GetScreenSizeY()) bottom = GetScreenSizeY() - 1;
	
	int rwidth = right - left;
	if (rwidth <= 1) return;
	
	for (int x = left; x <= right; x++)
	{
		int r1 = (colorL >> 16) & 0xFF, r2 = (colorR >> 16) & 0xFF;
		int g1 = (colorL >>  8) & 0xFF, g2 = (colorR >>  8) & 0xFF;
		int b1 = (colorL >>  0) & 0xFF, b2 = (colorR >>  0) & 0xFF;
		int a  = 0x00;
		int xa = x - left;
		int rc = (r2 * xa + r1 * (rwidth - xa)) / (rwidth); rc &= 0xFF;//don't exceed 255, we will
		int gc = (g2 * xa + g1 * (rwidth - xa)) / (rwidth); gc &= 0xFF;//bleed into the next color
		int bc = (b2 * xa + b1 * (rwidth - xa)) / (rwidth); bc &= 0xFF;
		
		unsigned color = a << 24 | rc << 16 | gc << 8 | bc;
		
		for (int y = top; y <= bottom; y++)
			VidPlotPixelInline(x, y, color);
	}
}
void VidFillRectVGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom)
{
	//basic clipping:
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= GetScreenSizeX()) right = GetScreenSizeX() - 1;
	if (bottom >= GetScreenSizeY()) bottom = GetScreenSizeY() - 1;
	
	int rheight = bottom - top;
	if (rheight <= 1) return;
	
	for (int y = top; y <= bottom; y++)
	{
		int r1 = (colorL >> 16) & 0xFF, r2 = (colorR >> 16) & 0xFF;
		int g1 = (colorL >>  8) & 0xFF, g2 = (colorR >>  8) & 0xFF;
		int b1 = (colorL >>  0) & 0xFF, b2 = (colorR >>  0) & 0xFF;
		int a  = 0x00;
		int ya = y - top;
		int rc = (r2 * ya + r1 * (rheight - ya)) / (rheight); rc &= 0xFF;//don't exceed 255, we will
		int gc = (g2 * ya + g1 * (rheight - ya)) / (rheight); gc &= 0xFF;//bleed into the next color
		int bc = (b2 * ya + b1 * (rheight - ya)) / (rheight); bc &= 0xFF;
		
		unsigned color = a << 24 | rc << 16 | gc << 8 | bc;
		
		for (int x = left; x <= right; x++)
			VidPlotPixelInline(x, y, color);
	}
}
void VidDrawHLine(unsigned color, int left, int right, int y)
{
	//basic clipping:
	if (left < 0) left = 0;
	if (y < 0) y = 0;
	if (right >= GetScreenSizeX()) right = GetScreenSizeX() - 1;
	if (y >= GetScreenSizeY()) y = GetScreenSizeY() - 1;
	
	for (int x = left; x <= right; x++)
	{
		VidPlotPixelInline(x, y, color);
		VidPlotPixelInline(x, y, color);
	}
}
void VidDrawVLine(unsigned color, int top, int bottom, int x)
{
	//basic clipping:
	if (x < 0) x = 0;
	if (top < 0) top = 0;
	if (x >= GetScreenSizeX()) x = GetScreenSizeX() - 1;
	if (bottom >= GetScreenSizeY()) bottom = GetScreenSizeY() - 1;
	
	for (int y = top; y <= bottom; y++)
	{
		VidPlotPixelInline(x, y, color);
		VidPlotPixelInline(x, y, color);
	}
}
int absinl(int i)
{
	if (i < 0) return -i; 
	return i;
}

//shamelessly stolen from https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/olcPixelGameEngine.h#L1866
void VidDrawLine(unsigned p, int x1, int y1, int x2, int y2)
{
	//is line vertical?
	int aux = 0;
	if (x1 == x2)
	{
		//we have a more optimized version, use that
		if (y1 > y2)
		{
			aux = y1; y1 = y2; y2 = aux;
		}
		VidDrawVLine(p, y1, y2, x1);
		return;
	}
	//is line horizontal?
	if (y1 == y2)
	{
		if (x1 > x2)
		{
			aux = x1; x1 = x2; x2 = aux;
		}
		//we have a more optimized version, use that
		VidDrawHLine(p, x1, x2, y1);
		return;
	}
	
	int dx = x2 - x1, dy = y2 - y1;
	int dx1 = absinl(dx), dy1 = absinl(dy), xe, ye, x, y;
	int px = 2 * dy1 - dx1, py = 2 * dx1 - dy1;
	
	if (dy1 <= dx1)
	{
		if (dx >= 0)
		{
			x = x1, y = y1, xe = x2;
		}
		else
		{
			x = x2, y = y2, xe = x1;
		}
		
		VidPlotPixelInline(x, y, p);
		
		for (int i = 0; x < xe; i++)
		{
			x++;
			if (px < 0)
				px += 2 * dy1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) y++; else y--;
				px += 2 * (dy1 - dx1);
			}
			VidPlotPixelInline(x, y, p);
		}
	}
	else
	{
		if (dy >= 0)
		{
			x = x1, y = y1, ye = y2;
		}
		else
		{
			x = x2, y = y2, ye = y1;
		}
		
		VidPlotPixelInline(x, y, p);
		
		for (int i = 0; y < ye; i++)
		{
			y++;
			if (py <= 0)
				py += 2 * dx1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) x++; else x--;
				py += 2 * (dx1 - dy1);
			}
			VidPlotPixelInline(x, y, p);
		}
	}
}
void VidBlitImage(Image* pImage, int x, int y)
{
	const uint32_t* fb = pImage->framebuffer;
	
	int ixe = x + pImage->width, iye = y + pImage->height;
	for (int iy = y; iy < iye; iy++)
		for (int ix = x; ix < ixe; ix++)
		{
			if (*fb != TRANSPARENT)
				VidPlotPixelInline(ix, iy, *fb);
			fb++;
		}
}
void VidBlitImageResize(Image* p, int gx, int gy, int width, int height)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int xgrab = x * p->width / width;
			int ygrab = y * p->height/ height;
			
			uint32_t pixel = p->framebuffer[xgrab + p->width * ygrab];
			if (pixel != TRANSPARENT)
				VidPlotPixel (gx+x, gy+y, pixel);
		}
	}
}
void VidDrawRect(unsigned color, int left, int top, int right, int bottom)
{
	//basic clipping:
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= GetScreenSizeX()) right = GetScreenSizeX() - 1;
	if (bottom >= GetScreenSizeY()) bottom = GetScreenSizeY() - 1;
	
	for (int x = left; x <= right; x++)
	{
		VidPlotPixelInline(x, top,    color);
		VidPlotPixelInline(x, bottom, color);
	}
	for (int y = top; y <= bottom; y++)
	{
		VidPlotPixelInline(left,  y, color);
		VidPlotPixelInline(right, y, color);
	}
}
void VidFillRectangle(unsigned color, Rectangle rect)
{
	VidFillRect (color, rect.left, rect.top, rect.right, rect.bottom);
}
void VidDrawRectangle(unsigned color, Rectangle rect)
{
	VidDrawRect (color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidSetVBEData(VBEData* pData)
{
	if (pData)
		g_vbeData = pData;
	else
		g_vbeData = &g_mainScreenVBEData;
}


// Font rendering
bool g_uses8by16Font = 0;

const unsigned char* g_fontIDToData[] = {
	g_TamsynRegu8x16,
	g_TamsynBold8x16,
	g_PaperMFont8x16,
	g_FamiSans8x8,
	g_BasicFontData,
	g_GlcdData,
};
const unsigned char* g_pCurrentFont = NULL;
void VidSetFont(unsigned fontType)
{
	if (fontType >= FONT_LAST) 
	{
		LogMsg("Can't set the font to that! (%d)", fontType);
		return;
	}
	g_pCurrentFont  = g_fontIDToData[fontType];
	g_uses8by16Font = (g_pCurrentFont[1] != 8);
}
void VidPlotChar (char c, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/)
{
	if (!g_pCurrentFont) {
		SLogMsg("FUCK!");
		return;
	}
	int width = g_pCurrentFont[0], height = g_pCurrentFont[1];
	const unsigned char* test = g_pCurrentFont + 3;
	if (g_pCurrentFont[2] == 2)
	{
		int x = 0;
		for (x = 0; x < width; x++)
		{
			for (int y = 0, bitmask = 1; y < height; y++, bitmask <<= 1)
			{
				if (test[c * width + x] & bitmask)
					VidPlotPixelInline(ox + x, oy + y, colorFg);
				else if (colorBg != TRANSPARENT)
					VidPlotPixelInline(ox + x, oy + y, colorBg);
			}
		}
		for (int y = 0; y < height; y++)
		{
			if (colorBg != TRANSPARENT)
				VidPlotPixelInline(ox + x, oy + y, colorBg);
		}
		
		return;
	}
	else
	{
		for (int y = 0; y < height; y++)
		{
			for (int x = 0, bitmask = (1 << (width - 1)); x < width; x++, bitmask >>= 1)
			{
				if (test[c * height + y] & bitmask)
					VidPlotPixelInline(ox + x, oy + y, colorFg);
				else if (colorBg != TRANSPARENT)
					VidPlotPixelInline(ox + x, oy + y, colorBg);
			}
		}
	}
}
void VidTextOutInternal(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx)
{
	int x = ox, y = oy;
	int lineHeight = g_pCurrentFont[1], charWidth = g_pCurrentFont[0];
	bool hasVariableCharWidth = g_pCurrentFont[2] == 1;
	if (g_pCurrentFont[2] == 2)
		charWidth++;
	
	int width = 0;
	int cwidth = 0, height = lineHeight;
	
	while (*pText)
	{
		//print this character:
		char c = *pText;
		if (c == '\n')
		{
			y += lineHeight;
			height += lineHeight;
			x = ox;
			if (cwidth < width)
				cwidth = width;
			width = 0;
		}
		else
		{
			int cw = charWidth;
			if (hasVariableCharWidth) cw = g_pCurrentFont[3 + 256 * lineHeight + c];
			
			if (!doNotActuallyDraw)
				VidPlotChar(c, x, y, colorFg, colorBg);
			
			x += cw;
			width += cw;
		}
		pText++;
	}
	if (cwidth < width)
		cwidth = width;
	
	*widthx  = cwidth;
	*heightx = height;
}
void VidTextOut(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg)
{
	UNUSED int a, b;
	VidTextOutInternal (pText, ox, oy, colorFg, colorBg, false, &a, &b);
}

void VidDrawText(const char* pText, Rectangle rect, unsigned drawFlags, unsigned colorFg, unsigned colorBg)
{
	//TODO: Fix this function
	int w, h;
	VidTextOutInternal(pText, 0, 0, 0, 0, true, &w, &h);
	
	int xStart = rect.left, yStart = rect.top;
	if (drawFlags & TEXTSTYLE_HCENTERED)
		xStart = rect.left + ((rect.right - rect.left - w) / 2);
	if (drawFlags & TEXTSTYLE_VCENTERED)
		yStart = rect.top  + ((rect.bottom- rect.top  - h) / 2);
	
	VidTextOutInternal(pText, xStart, yStart, colorFg, colorBg, false, &w, &h);
}

//! DO NOT use this on non-main-screen framebuffers!
unsigned VidReadPixel (unsigned x, unsigned y)
{
	if (x >= (unsigned)GetScreenSizeX()) return 0;
	if (y >= (unsigned)GetScreenSizeY()) return 0;
	return g_framebufferCopy[x + y * GetScreenSizeX()];
}
__attribute__((always_inline))
inline unsigned VidReadPixelInline (unsigned x, unsigned y)
{
	return g_framebufferCopy[x + y * g_vbeData->m_width];
}

//! DO NOT use this on non-main-screen framebuffers!
void VidShiftScreen (int howMuch)
{
	if (howMuch >= GetScreenSizeY())
		return;
	/*for (int i = howMuch; i < GetScreenSizeY(); i++) {
		for (int k = 0; k < GetScreenSizeX(); k++) {
			VidPlotPixelInline (k, i-howMuch, VidReadPixel (k, i));
		}
	}*/
	
	if (g_vbeData->m_bitdepth == 2)
	{
		if (g_vbeData == &g_mainScreenVBEData)
		{
			int a = g_vbeData->m_width * 4;
			for (int i = howMuch, j = 0, k = 0; i < GetScreenSizeY(); i++, j += g_vbeData->m_pitch, k += a)
			{
				fast_memcpy(((uint8_t*)g_vbeData->m_framebuffer32 + j), &g_framebufferCopy[i * g_vbeData->m_width], a);
				fast_memcpy(((uint8_t*)g_framebufferCopy          + k), &g_framebufferCopy[i * g_vbeData->m_width], a);
			}
		}
		else
		{
			int sz = g_vbeData->m_width * (g_vbeData->m_height - howMuch);
			fast_memcpy(g_vbeData->m_framebuffer32, &g_vbeData->m_framebuffer32[g_vbeData->m_width * howMuch], sz);
		}
	}
	else
	{
		;//unhandled
	}
}
#endif

// Stuff
#if 1

void RenderCursor(void)
{
	if (g_vbeData->m_bitdepth == 2)
	{
		//NEW: Optimization
		int ys =                         - g_currentCursor->topOffs + g_mouseY;
		int ye = g_currentCursor->height - g_currentCursor->topOffs + g_mouseY;
		int kys = 0, kzs = 0;
		if (ys < 0)
		{
			kys -= ys * g_currentCursor->width;
			kzs -= ys;
			ys = 0;
		}
		int xs =                         - g_currentCursor->leftOffs+ g_mouseX;
		int xe = g_currentCursor->width  - g_currentCursor->leftOffs+ g_mouseX;
		int off = 0;
		if (xs < 0)
		{
			off = -xs;
			xs = 0;
		}
		if (xe >= GetScreenSizeX())
			xe = GetScreenSizeX() - 1;
		//int xd = (xe - xs) * sizeof(uint32_t);
		for (int y = ys, ky = kys, kz = kzs; y < ye; y++, kz++)
		{
			ky = kz * g_currentCursor->width + off;
			//just memcpy shit
			//memcpy (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + xs], &g_currentCursor->bitmap[ky], xd);
			for (int x = xs; x < xe; x++)
			{
				if (g_currentCursor->bitmap[ky] != TRANSPARENT)
					g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + x] = g_currentCursor->bitmap[ky];
				ky++;
			}
		}
	}
	else//use the slower but more reliable method:
	{
		for (int i = 0, ky=g_mouseY - g_currentCursor->topOffs; i < g_currentCursor->height; i++, ky++)
		{
			if (ky < 0) {
				i += ky;
				ky = 0;
			}
			for (int j = 0, kx=g_mouseX - g_currentCursor->leftOffs; j < g_currentCursor->width; j++, kx++)
			{
				if (kx < 0) {
					j += kx;
					kx = 0;
				}
				int id = i * g_currentCursor->width + j;
				if (g_currentCursor->bitmap[id] != TRANSPARENT)
				{
					//int kx = j + g_mouseX - g_currentCursor->leftOffs,
					//	ky = i + g_mouseY - g_currentCursor->topOffs;
					if (kx < 0 || ky < 0 || kx >= GetScreenSizeX() || ky >= GetScreenSizeY()) continue;
					VidPlotPixelIgnoreCursorChecksChecked (
						kx,
						ky,
						g_currentCursor->bitmap[id]
					);
				}
			}
		}
	}
}

void SetMousePos (unsigned newX, unsigned newY)
{
	//NOTE: As this is called in an interrupt too, a call here might end up coming right
	//while we we're drawing a window or something.  Keep a backup of the previous settings.
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	
	int oldX = g_mouseX, oldY = g_mouseY;
	
	if (newX >= (unsigned)GetScreenSizeX()) newX = GetScreenSizeX() - 1;
	if (newY >= (unsigned)GetScreenSizeY()) newY = GetScreenSizeY() - 1;
	
	g_mouseX = newX, g_mouseY = newY;
	
	//--uncomment if you want one pixel cursor (This is very useless and hard to use)
	//VidPlotPixelIgnoreCursorChecks (g_mouseX, g_mouseY, 0xFF);
	//VidPlotPixel (oldX, oldY, VidReadPixel(oldX, oldY));
	
	//Draw the cursor image at the new position:
	RenderCursor();
	
	//Redraw all the pixels under where the cursor was previously:
	for (int i = 0; i < g_currentCursor->height; i++)
	{
		for (int j = 0; j < g_currentCursor->width; j++)
		{
			bool condition = true;
			if (g_currentCursor->m_transparency)
			{
				int id = i * g_currentCursor->width + j;
				condition = g_currentCursor->bitmap[id] != TRANSPARENT;
			}
			if (condition)
			{
				int kx = j + oldX - g_currentCursor->leftOffs,
					ky = i + oldY - g_currentCursor->topOffs;
				if (kx < 0 || ky < 0 || kx >= GetScreenSizeX() || ky >= GetScreenSizeY()) continue;
				VidPlotPixelCheckCursor (
				//VidPlotPixelInline(
					kx, ky, VidReadPixelInline (kx, ky)
				);
			}
		}
	}
	//TODO: check flags here
	
	g_vbeData = backup;
}

#endif

// Video initialization
#if 1
void VidInitializeVBEData(multiboot_info_t* pInfo)
{
	int bpp = pInfo->framebuffer_bpp;
	g_vbeData->m_available = true;
	g_vbeData->m_width     = pInfo->framebuffer_width;
	g_vbeData->m_height    = pInfo->framebuffer_height;
	if (bpp == 32)
		g_vbeData->m_bitdepth  = 2;
	else if (bpp == 16)
		g_vbeData->m_bitdepth  = 1;
	else
		g_vbeData->m_bitdepth  = 0;
	g_vbeData->m_pitch     = pInfo->framebuffer_pitch;
	g_vbeData->m_pitch32   = pInfo->framebuffer_pitch / sizeof (int);
	g_vbeData->m_pitch16   = pInfo->framebuffer_pitch / sizeof (short);
	
	g_vbeData->m_framebuffer32 = (uint32_t*)FRAMEBUFFER_MAPPED_ADDR;
	g_vbeData->m_framebuffer16 = (uint16_t*)FRAMEBUFFER_MAPPED_ADDR;
	g_vbeData->m_framebuffer8  = (uint8_t *)FRAMEBUFFER_MAPPED_ADDR;
}
//present, read/write, user/supervisor, writethrough
#define VBE_PAGE_BITS (1 | 2 | 4 | 8)
void VidInitialize(multiboot_info_t* pInfo)
{
	cli;
	g_vbeData = &g_mainScreenVBEData;
	
	g_vbeData->m_available = false;
	if (pInfo->flags & MULTIBOOT_INFO_VIDEO_INFO)
	{
		if (pInfo->framebuffer_type != 1)
		{
			LogMsg("Need direct RGB framebuffer!");
			sti;
			return;
		}
		// map shit to 0xE0000000 or above
		int index = FRAMEBUFFER_MAPPED_ADDR >> 22;
		uint32_t pointer = pInfo->framebuffer_addr;
		uint32_t final_address = 0xE0000000;
		final_address += pointer & 0xFFF;
		pointer &= ~0xFFF;
		
		//LogMsg("VBE Pointer: %x", pointer);
		//LogMsg("Bitdepth: %d", pInfo->framebuffer_bpp);
		for (int i = 0; i < MAX_VIDEO_PAGES; i++)
		{
			g_vbePageEntries[i] = pointer | VBE_PAGE_BITS;
			pointer += 0x1000;
		}
		for (int i = 0; i < MAX_VIDEO_PAGES; i += 1024)
		{
			//LogMsg("Mapping %d-%d to %dth page table pointer", i,i+1023, index);
			uint32_t pageTable = ((uint32_t)&g_vbePageEntries[i]) - BASE_ADDRESS;
			
			g_curPageDir[index] = pageTable | VBE_PAGE_BITS;
			index++;
		}
		
		g_framebufferCopy = MmAllocate (pInfo->framebuffer_width * pInfo->framebuffer_height * 4);
		
		MmTlbInvalidate();
		
		// initialize the VBE data:
		VidInitializeVBEData (pInfo);
		VidPrintTestingPattern();
		
		// initialize the console:
		//LogMsg("Setting font.");
		VidSetFont (FONT_TAMSYN_REGULAR);
		//VidSetFont (FONT_BASIC);
		//LogMsg("Re-initializing debug console with graphics");
		CoInitAsGraphics(&g_debugConsole);
		//sti;
	}
	else
	{
		SwitchMode (1);
		CoInitAsText(&g_debugConsole);
		//LogMsg("Warning: no VBE mode specified.");
		//sti;
	}
}
#endif
