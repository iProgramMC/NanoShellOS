/*****************************************
		NanoShell Operating System
	   (C) 2021-2023 iProgramInCpp

        Graphical Interface Module
******************************************/
#include <main.h>
#include <vga.h>
#include <video.h>
#include <string.h>
#include <icon.h>
#include <task.h>
#include <misc.h>

#define VISIBLE_DRAW_BORDER_THICKNESS 1

// -------------------------------------------------------------------------
//  Warning
//
//       The code you are about to see is a mess. However, it's optimized
//   to hell, and my (few) attempts to split it up and generalize it have
//   dealt a severe performance hit to features such as console scrolling,
//   and window dragging.
//       If you're up to the challenge of cleaning this up yourself, I
//   invite you to do so, and create a pull request, however, be prepared
//   for the challenges ahead.
//
//     -iProgramInCpp
// -------------------------------------------------------------------------

// Basic definitions for video
#if 1

//! TODO: if planning to extend this beyond 1920x1080x32, extend THIS variable.
#define MAX_VIDEO_PAGES 2048

uint32_t  g_vbePageEntries[MAX_VIDEO_PAGES] __attribute__((aligned(4096))); 
uint32_t* g_framebufferCopy = NULL;

bool      g_isVideoMode = false;
uint32_t  g_framebufferPhysical = 0;

extern uint32_t *g_curPageDir;

VBEData* g_vbeData = NULL, g_mainScreenVBEData;
#endif

extern bool g_RenderWindowContents;

// Mouse graphics stuff
#if 1

int g_mouseX = 0, g_mouseY = 0;

#define SEMI_TRANSPARENT TRANSPARENT//0x7F7F7F7F

#include "extra/cursors.h"

//const for now, TODO
//bool g_disableShadows = true;
#define g_disableShadows 0

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
SafeLock  g_ClickQueueLock;

typedef struct
{
	int newX, newY;
	bool updated;
}
MouseMoveQueue;

MouseMoveQueue g_queueMouseUpdateTo;

void RenderCursor(void);
void RedrawOldPixels(int oldX, int oldY);
void RedrawOldPixelsFull(int oldX, int oldY);

bool RefreshMouse()
{
	cli;
	MouseMoveQueue QueueMouseUpdateTo = g_queueMouseUpdateTo;
	g_queueMouseUpdateTo.updated = false;
	g_queueMouseUpdateTo.newX = g_queueMouseUpdateTo.newY = 0;
	
	int lockX, lockY;
	lockX = (int)(short)g_currentCursor->mouseLockX;
	lockY = (int)(short)g_currentCursor->mouseLockY;
	sti;
	
	if (QueueMouseUpdateTo.updated)
	{
		QueueMouseUpdateTo.updated = false;
		
		if (g_currentCursor->m_flags & CUR_RESIZE)
		{
			VBEData* backup = g_vbeData;
			g_vbeData = &g_mainScreenVBEData;
			
			RedrawOldPixels(g_mouseX, g_mouseY);
			
			int newX = QueueMouseUpdateTo.newX;
			int newY = QueueMouseUpdateTo.newY;
			
			const int minWidth = 200;
			const int minHeight = 50;//TODO: BORDER_SIZE * 2 + TITLE_BAR_HEIGHT + 1;
			
			if (~g_currentCursor->m_flags & CUR_LOCK_X)
			{
				g_mouseX += newX;
				if (g_currentCursor->m_flags & CUR_RESIZE_MOVE_X)
					g_currentCursor->boundsWidth -= newX;
				else
					g_currentCursor->boundsWidth += newX, g_currentCursor->leftOffs += newX;
				
				if (g_currentCursor->boundsWidth < minWidth && (g_currentCursor->m_flags & CUR_RESIZE_MOVE_X))
				{
					int diff = minWidth - g_currentCursor->boundsWidth;
					g_currentCursor->boundsWidth = minWidth;
					g_mouseX -= diff;
				}
			}
			if (~g_currentCursor->m_flags & CUR_LOCK_Y)
			{
				g_mouseY += newY;
				if (g_currentCursor->m_flags & CUR_RESIZE_MOVE_Y)
					g_currentCursor->boundsHeight -= newY;
				else
					g_currentCursor->boundsHeight += newY, g_currentCursor->topOffs += newY;
				
				if (g_currentCursor->boundsHeight < minHeight && (g_currentCursor->m_flags & CUR_RESIZE_MOVE_Y))
				{
					int diff = minHeight - g_currentCursor->boundsHeight;
					g_currentCursor->boundsHeight = minHeight;
					g_mouseY -= diff;
				}
			}
			
			if (g_currentCursor->boundsWidth  < minWidth)
				g_currentCursor->boundsWidth  = minWidth;
			if (g_currentCursor->boundsHeight < minHeight)
				g_currentCursor->boundsHeight = minHeight;
			if (g_currentCursor->boundsWidth  >= GetScreenSizeX())
				g_currentCursor->boundsWidth   = GetScreenSizeX();
			if (g_currentCursor->boundsHeight >= GetScreenSizeY())
				g_currentCursor->boundsHeight  = GetScreenSizeY();
			
			RenderCursor();
			
			g_vbeData = backup;
		}
		else
		{
			int newMouseX = g_mouseX;
			int newMouseY = g_mouseY;
			if (lockX == -1 || lockY == -1)
			{
				newMouseX += QueueMouseUpdateTo.newX;
				newMouseY += QueueMouseUpdateTo.newY;
			}
			
			SetMousePos(newMouseX, newMouseY);
		}
		
		QueueMouseUpdateTo.newX = 0;
		QueueMouseUpdateTo.newY = 0;
		return true;
	}
	
	return false;
}

bool RectangleContains(Rectangle*r, Point*p) 
{
	return (r->left <= p->x && r->right >= p->x && r->top <= p->y && r->bottom >= p->y);
}

bool RectangleOverlap(Rectangle *r1, Rectangle *r2)
{
	return (r1->left <= r2->right && r1->right >= r2->left && r1->top <= r2->bottom && r1->bottom >= r2->top);
}

bool RectangleIntersect(Rectangle* r, const Rectangle* r1, const Rectangle* r2)
{
	#define MIN(a,b) ((a)<(b)?(a):(b))
	#define MAX(a,b) ((a)>(b)?(a):(b))
	r->left   = MAX(r1->left,  r2->left);
	r->top    = MAX(r1->top,   r2->top);
	r->right  = MIN(r1->right, r2->right);
	r->bottom = MIN(r1->bottom,r2->bottom);
	return !(r->left > r->right || r->top > r->bottom);
	#undef MIN
	#undef MAX
}

void AddClickInfoToQueue(const ClickInfo* info)
{
	if (g_clickQueueSize >= CLICK_INFO_MAX)
	{
		//only handle the next clicks now
		g_clickQueueSize = 0;
	}
	g_clickQueue[g_clickQueueSize++] = *info;
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

void OnRightClickDrag()
{
	ClickInfo info;
	info.clickType = CLICK_RIGHTD;
	info.clickedAtX = g_mouseX;
	info.clickedAtY = g_mouseY;
	AddClickInfoToQueue (&info);
}

void OnRightClickRelease()
{
	ClickInfo info;
	info.clickType = CLICK_RIGHTR;
	info.clickedAtX = g_mouseX;
	info.clickedAtY = g_mouseY;
	AddClickInfoToQueue (&info);
}

uint8_t g_previousFlags = 0;
void ForceKernelTaskToRunNext();
bool IsWindowManagerRunning();

void OnUpdateMouse(uint8_t flags, uint8_t Dx, uint8_t Dy, __attribute__((unused)) uint8_t Dz)
{
	int dx, dy;
	dx = (flags & (1 << 4)) ? (int8_t)Dx : Dx;
	dy = (flags & (1 << 5)) ? (int8_t)Dy : Dy;
	
	if (!IsWindowManagerRunning())
		return;
	
	//move the cursor:
	g_queueMouseUpdateTo.newX += dx;
	g_queueMouseUpdateTo.newY += -dy;
	
	if (dx || dy)
		g_queueMouseUpdateTo.updated = true;
	
	// Left click
	if (flags & MOUSE_FLAG_L_BUTTON)
	{
		if (!(g_previousFlags & MOUSE_FLAG_L_BUTTON))
			OnLeftClick();
		else
			OnLeftClickDrag();
	}
	else if (g_previousFlags & MOUSE_FLAG_L_BUTTON)
	{
		OnLeftClickRelease();
	}
	
	// Right click
	if (flags & MOUSE_FLAG_R_BUTTON)
	{
		if (!(g_previousFlags & MOUSE_FLAG_R_BUTTON))
			OnRightClick();
		else
			OnRightClickDrag();
	}
	else if (g_previousFlags & MOUSE_FLAG_R_BUTTON)
	{
		OnRightClickRelease();
	}
	
	g_previousFlags = flags & 7;
}

Cursor* GetCurrentCursor()
{
	return g_currentCursor;
}

void RenderCursor(void);

void SetCursorInternal(Cursor* pCursor, bool bUndrawOldCursor)
{
	if (!pCursor) pCursor = g_pDefaultCursor;
	if (g_currentCursor == pCursor) return;
	
	KeVerifyInterruptsEnabled;
	cli;
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	
	//undraw the old cursor:
	if (g_currentCursor && bUndrawOldCursor)
	{
		if (g_currentCursor->m_flags & CUR_RESIZE)
		{
			if (g_currentCursor->width > g_currentCursor->boundsWidth || g_currentCursor->height > g_currentCursor->boundsHeight)
			{
				RedrawOldPixelsFull(g_mouseX, g_mouseY);
			}
			else
				RedrawOldPixels(g_mouseX, g_mouseY);
		}
		else
			RedrawOldPixelsFull(g_mouseX, g_mouseY);
	}
	
	//draw the new cursor:
	g_currentCursor = pCursor;
	
	if ((int)(short)pCursor->mouseLockX != -1)
	{
		g_mouseX = pCursor->mouseLockX;
		g_mouseY = pCursor->mouseLockY;
	}
	
	RenderCursor();
	
	g_vbeData = backup;
	
	KeVerifyInterruptsDisabled;
	sti;
}

void SetCursor(Cursor* pCursor)
{
	SetCursorInternal(pCursor, true);
}

void SetMouseVisible (bool b)
{
	g_isMouseVisible = b && IsWindowManagerRunning();
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
			xe = GetScreenSizeX();
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
	return g_vbeData->m_version != 0;
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
int GetScreenWidth()
{
	return g_mainScreenVBEData.m_width;
}
int GetScreenHeight()
{
	return g_mainScreenVBEData.m_height;
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
__attribute__((always_inline))
inline void VidPlotPixelInline(unsigned x, unsigned y, unsigned color)
{
	if (
		(int)x <  g_vbeData->m_clipRect.left ||
		(int)y <  g_vbeData->m_clipRect.top  ||
		(int)x >= g_vbeData->m_clipRect.right ||
		(int)y >= g_vbeData->m_clipRect.bottom
	)
		return;
	VidPlotPixelToCopyInlineUnsafe(x, y, color);
	VidPlotPixelRaw32I (x, y, color);
}
// The exposed vidplotpixel :)
void VidPlotPixel(unsigned x, unsigned y, unsigned color)
{
	VidPlotPixelInline(x, y, color);
	//DirtyRectLogger (x, y, 1, 1);
}
static void VidPlotPixelCheckCursor(unsigned x, unsigned y, unsigned color)
{
	if ((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()) return;
	VidPlotPixelToCopyInlineUnsafe(x, y, color);
	
	// if inside the cursor area, don't display this pixel on the screen:
	if (g_vbeData == &g_mainScreenVBEData)
	{
		if (g_currentCursor && g_isMouseVisible)
		{
			//inside the rectangle?
			if ((int)x >= g_mouseX - g_currentCursor->leftOffs &&
				(int)y >= g_mouseY - g_currentCursor->topOffs  &&
				(int)x <  g_mouseX + g_currentCursor->width  - g_currentCursor->leftOffs &&
				(int)y <  g_mouseY + g_currentCursor->height - g_currentCursor->topOffs)
			{
				if (!g_currentCursor->m_transparency) return;
				//check the pixel
				int mx = x - g_mouseX + g_currentCursor->leftOffs;
				int my = y - g_mouseY + g_currentCursor->topOffs;
				int index = my * g_currentCursor->width + mx;
				unsigned bru = g_currentCursor->bitmap[index];
				if (bru != TRANSPARENT)// && bru != SEMI_TRANSPARENT)
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
	VidFillRect (color, 0, 0, GetScreenSizeX() - 1, GetScreenSizeY() - 1);
}
void VidFillRect(unsigned color, int left, int top, int right, int bottom)
{
	if (left > right || top > bottom) return;
	
	// Is this rectangle fully outside of the clip rect ?
	if (left >= g_vbeData->m_clipRect.right)  return;
	if (top  >= g_vbeData->m_clipRect.bottom) return;
	if (right  < g_vbeData->m_clipRect.left)  return;
	if (bottom < g_vbeData->m_clipRect.top)   return;
	
	// Do other clipping stuff
	if (left   < g_vbeData->m_clipRect.left  ) left   = g_vbeData->m_clipRect.left  ;
	if (top    < g_vbeData->m_clipRect.top   ) top    = g_vbeData->m_clipRect.top   ;
	if (right >= g_vbeData->m_clipRect.right ) right  = g_vbeData->m_clipRect.right-1;
	if (bottom>= g_vbeData->m_clipRect.bottom) bottom = g_vbeData->m_clipRect.bottom-1;
	
	if (left > right || top > bottom) return;
	
	int yoffs = top * g_vbeData->m_pitch32;
	int start = yoffs + left;
	int xs = right-left+1;
	for (int y = top; y <= bottom; y++)
	{
		memset_ints (&g_vbeData->m_framebuffer32[start], color, xs);
		start += g_vbeData->m_pitch32;
	}
	if (g_vbeData == &g_mainScreenVBEData)
	{
		yoffs = top * g_vbeData->m_width;
		start = yoffs + left;
		xs = right-left+1;
		for (int y = top; y <= bottom; y++)
		{
			memset_ints (&g_framebufferCopy[start], color, xs);
			start += g_vbeData->m_width;
		}
	}
	
	DirtyRectLogger(left, top, right - left + 1, bottom - top + 1);
}
void VidFillRectHGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom)
{
	// Is this rectangle fully outside of the clip rect ?
	if (left >= g_vbeData->m_clipRect.right)  return;
	if (top  >= g_vbeData->m_clipRect.bottom) return;
	if (right  < g_vbeData->m_clipRect.left)  return;
	if (bottom < g_vbeData->m_clipRect.top)   return;
	
	// Do other clipping stuff
	if (left   < g_vbeData->m_clipRect.left  ) left   = g_vbeData->m_clipRect.left  ;
	if (top    < g_vbeData->m_clipRect.top   ) top    = g_vbeData->m_clipRect.top   ;
	if (right >= g_vbeData->m_clipRect.right ) right  = g_vbeData->m_clipRect.right-1;
	if (bottom>= g_vbeData->m_clipRect.bottom) bottom = g_vbeData->m_clipRect.bottom-1;
	
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
	
	DirtyRectLogger(left, top, right - left + 1, bottom - top + 1);
}
void VidFillRectVGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom)
{
	// Is this rectangle fully outside of the clip rect ?
	if (left >= g_vbeData->m_clipRect.right)  return;
	if (top  >= g_vbeData->m_clipRect.bottom) return;
	if (right  < g_vbeData->m_clipRect.left)  return;
	if (bottom < g_vbeData->m_clipRect.top)   return;
	
	// Do other clipping stuff
	if (left   < g_vbeData->m_clipRect.left  ) left   = g_vbeData->m_clipRect.left  ;
	if (top    < g_vbeData->m_clipRect.top   ) top    = g_vbeData->m_clipRect.top   ;
	if (right >= g_vbeData->m_clipRect.right ) right  = g_vbeData->m_clipRect.right-1;
	if (bottom>= g_vbeData->m_clipRect.bottom) bottom = g_vbeData->m_clipRect.bottom-1;
	
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
	
	DirtyRectLogger(left, top, right - left + 1, bottom - top + 1);
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
	
	DirtyRectLogger(left, y, right - left + 1, 1);
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
	
	DirtyRectLogger(x, top, 1, bottom - top + 1);
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
		DirtyRectLogger (x, y, 1, 1);
		
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
			DirtyRectLogger (x, y, 1, 1);
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
		DirtyRectLogger (x, y, 1, 1);
		
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
			DirtyRectLogger (x, y, 1, 1);
		}
	}
}

void VidBlitImageForceOpaque(Image* pImage, int x, int y)
{
	if (!pImage) return;
	
	if (pImage->width == 0 || pImage->height == 0)
		return;
	
	// TODO: More complete fill-in of the VBEData structure
	VBEData data;
	data.m_bitdepth = 2;
	data.m_width    = data.m_pitch32 = pImage->width;
	data.m_height   = pImage->height;
	data.m_framebuffer32 = (uint32_t*)pImage->framebuffer;
	
	VidBitBlit(
		g_vbeData,
		x, y,
		pImage->width, pImage->height,
		&data,
		0, 0,
		BOP_SRCCOPY
	);
	
	DirtyRectLogger(x, y, pImage->width, pImage->height);
}

void VidBlitImage(Image* pImage, int x, int y)
{
	if (!pImage) return;
	
	if (pImage->width == 0 || pImage->height == 0)
		return;
	
	const uint32_t* fb = pImage->framebuffer;
	
	int ixe = x + pImage->width, iye = y + pImage->height;
	for (int iy = y; iy < iye; iy++)
		for (int ix = x; ix < ixe; ix++)
		{
			if (*fb != TRANSPARENT)
				VidPlotPixelInline(ix, iy, *fb);
			fb++;
		}
	
	DirtyRectLogger(x, y, pImage->width, pImage->height);
}

void VidBlitImageOutline(Image* pImage, int x, int y, uint32_t color)
{
	if (!pImage) return;
	
	if (pImage->width == 0 || pImage->height == 0)
		return;
	
	const uint32_t* fb = pImage->framebuffer;
	
	int ixe = x + pImage->width, iye = y + pImage->height;
	for (int iy = y; iy < iye; iy++)
		for (int ix = x; ix < ixe; ix++)
		{
			if (*fb != TRANSPARENT)
				VidPlotPixelInline(ix, iy, color);
			fb++;
		}
	
	DirtyRectLogger(x, y, pImage->width, pImage->height);
}

void VidBlitImageResize(Image* p, int gx, int gy, int width, int height)
{
	if (!p) return;
	
	if (p->width == 0 || p->height == 0)
		return;
	
	if (width == p->width && height == p->height)
	{
		VidBlitImage (p, gx, gy);
		return;
	}
	
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int xgrab = x * p->width / width;
			int ygrab = y * p->height/ height;
			
			uint32_t pixel = p->framebuffer[xgrab + p->width * ygrab];
			if (pixel != TRANSPARENT)
				VidPlotPixelInline (gx+x, gy+y, pixel);
		}
	}
	
	DirtyRectLogger(gx, gy, width, height);
}

void VidBlitImageResizeForceOpaque(Image* p, int gx, int gy, int width, int height)
{
	if (!p) return;
	
	if (p->width == 0 || p->height == 0)
		return;
	
	if (width == p->width && height == p->height)
	{
		VidBlitImage (p, gx, gy);
		return;
	}
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int xgrab = x * p->width / width;
			int ygrab = y * p->height/ height;
			
			uint32_t pixel = p->framebuffer[xgrab + p->width * ygrab];
			VidPlotPixelInline (gx+x, gy+y, pixel);
		}
	}
	
	DirtyRectLogger(gx, gy, width, height);
}

void VidBlitImageResizeOutline(Image* p, int gx, int gy, int width, int height, uint32_t outline)
{
	if (!p) return;
	
	if (p->width == 0 || p->height == 0)
		return;
	
	if (width == p->width && height == p->height)
	{
		VidBlitImageOutline (p, gx, gy, outline);
		return;
	}
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int xgrab = x * p->width / width;
			int ygrab = y * p->height/ height;
			
			if (p->framebuffer[xgrab + p->width * ygrab] != TRANSPARENT)
				VidPlotPixelInline (gx+x, gy+y, outline);
		}
	}
	
	DirtyRectLogger(gx, gy, width, height);
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
	
	// TODO ensure these don't merge into the full area of the rect
	
	DirtyRectLogger(left, top, right-left+1, 1);  // top edge
	DirtyRectLogger(left, top, 1, bottom-top+1);  // left edge
	
	DirtyRectLogger(left,  bottom, right-left+1, 1);  // bottom edge
	DirtyRectLogger(right, top,    1, bottom-top+1);  // right edge
}

void VidFillRectangle(unsigned color, Rectangle rect)
{
	VidFillRect (color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidDrawRectangle(unsigned color, Rectangle rect)
{
	VidDrawRect (color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidSetClipRectEx(Rectangle* pOutRect, Rectangle *pRect)
{
	if (pOutRect)
		*pOutRect = g_vbeData->m_clipRect;
	
	if (pRect)
	{
		g_vbeData->m_clipRect = *pRect;
		
		if (g_vbeData->m_clipRect.left   < 0) g_vbeData->m_clipRect.left   = 0;
		if (g_vbeData->m_clipRect.top    < 0) g_vbeData->m_clipRect.top    = 0;
		if (g_vbeData->m_clipRect.right  >= (int)g_vbeData->m_width)  g_vbeData->m_clipRect.right  = g_vbeData->m_width;
		if (g_vbeData->m_clipRect.bottom >= (int)g_vbeData->m_height) g_vbeData->m_clipRect.bottom = g_vbeData->m_height;
	}
	else
	{
		Rectangle clipRect;
		clipRect.top    = clipRect.left = 0;
		clipRect.right  = GetScreenSizeX();
		clipRect.bottom = GetScreenSizeY();
		g_vbeData->m_clipRect = clipRect;
	}
}

void VidSetClipRect(Rectangle *pRect)
{
	VidSetClipRectEx(NULL, pRect);
}

VBEData* VidSetVBEData(VBEData* pData)
{
	VBEData* old = g_vbeData;
	if (pData)
		g_vbeData = pData;
	else
		g_vbeData = &g_mainScreenVBEData;
	
	VidSetClipRect(NULL);
	
	return old;
}

VBEData* VidGetVBEData()
{
	return g_vbeData;
}

// Font rendering

//! DO NOT use this on non-main-screen framebuffers!
unsigned VidReadPixel (unsigned x, unsigned y)
{
	if (x >= (unsigned)GetScreenWidth ()) return 0;
	if (y >= (unsigned)GetScreenHeight()) return 0;
	return g_framebufferCopy[x + y * GetScreenWidth()];
}
unsigned VidReadPixelU (unsigned x, unsigned y)
{
	if (x >= (unsigned)GetScreenSizeX()) return 0;
	if (y >= (unsigned)GetScreenSizeY()) return 0;
	if (g_vbeData == &g_mainScreenVBEData)
		return VidReadPixel (x, y);
	return g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + x];
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
	
	if (g_vbeData->m_bitdepth == 2)
	{
		if (g_vbeData == &g_mainScreenVBEData)
		{
			int a = g_vbeData->m_width * 4;
			for (int i = howMuch, j = 0, k = 0; i < GetScreenSizeY(); i++, j += g_vbeData->m_pitch, k += a)
			{
				memcpy_16_byte_aligned(((uint8_t*)g_vbeData->m_framebuffer32 + j), &g_framebufferCopy[i * g_vbeData->m_width], a);
				memcpy_16_byte_aligned(((uint8_t*)g_framebufferCopy          + k), &g_framebufferCopy[i * g_vbeData->m_width], a);
			}
		}
		else
		{
			int sz = g_vbeData->m_width * (g_vbeData->m_height - howMuch);
			memcpy_16_byte_aligned(g_vbeData->m_framebuffer32, &g_vbeData->m_framebuffer32[g_vbeData->m_width * howMuch], sz);
		}
	}
	else
	{
		;//unhandled
	}
}

void VidBitBlit(VBEData* pDest, int cx, int cy, int width, int height, VBEData* pSrc, int x1, int y1, uint32_t mode)
{
	//SLogMsg("VidBitBlitU(%x, %d, %d, %d, %d, %x, %d, %d, %x)",pDest,cx,cy,width,height,pSrc,x1,y1,mode);
	
	//TODO: more safety checks??!
	if (cx < 0)
	{
		width  += cx;
		if (mode == BOP_SRCCOPY)
			x1 -= cx;
		cx = 0;
	}
	if (cy < 0)
	{
		height += cy;
		if (mode == BOP_SRCCOPY)
			y1 -= cy;
		cy = 0;
	}
	
	if (mode == BOP_SRCCOPY)
	{
		if (x1 < 0) cx     -= x1, width  += x1, x1 = 0;
		if (y1 < 0) cy     -= y1, height += y1, y1 = 0;
	}
	
	if (width  > (int)pDest->m_width  - cx) width  = pDest->m_width  - cx;
	if (height > (int)pDest->m_height - cy) height = pDest->m_height - cy;
	
	if (pSrc)
	{
		if (width  > (int)pSrc->m_width  - x1) width  = pSrc->m_width  - x1;
		if (height > (int)pSrc->m_height - y1) height = pSrc->m_height - y1;
	}
	
	if (width  <= 0) return;
	if (height <= 0) return;
	
	// if the source and destination are the same, use memmove to ensure all the data is moved properly.
	// Otherwise, use the optimized memcpy_ints.
	void (*MemcpyInts)(void*, const void*, int) = pSrc == pDest ? memmove_ints : memcpy_ints;
	
	//SLogMsg("VidBitBlitR(%x, %d, %d, %d, %d, %x, %d, %d, %x)",pDest,cx,cy,width,height,pSrc,x1,y1,mode);
	
	if (mode == BOP_SRCCOPY)
	{
		if (pDest->m_bitdepth != 2 || pSrc->m_bitdepth != 2) //assertion
		{
			//usually this is not a problem unless multiboot couldn't give us a proper video mode
			//(86box is one notable example which gives me a 24-bit VBE instead of a 32-bit one)
			SLogMsg("TODO: VidBitBlit to differing bitdepths!!");
		}
		
		const bool isMainScreen = (pSrc == &g_mainScreenVBEData);
		
		uint32_t* const pSrcFB   = isMainScreen ? g_framebufferCopy : pSrc->m_framebuffer32;
		uint32_t  const srcPitch = isMainScreen ? pSrc->m_width     : pSrc->m_pitch32;
		
		//basic bit copy:
		bool reverseCheck = cy >= y1;
		for (int y = reverseCheck ? height - 1 : 0; reverseCheck ? (y >= 0) : (y < height); reverseCheck ? (--y) : (++y))
		{
			//copy one scanline from cy to y1
			int yDest = cy + y, ySrc = y1 + y;
			
			//determine the offset for each scanline:
			uint32_t* pdoffset = pDest->m_framebuffer32 + (yDest * pDest->m_pitch32) + cx;
			uint32_t* psoffset = pSrcFB                 + (ySrc  * srcPitch)         + x1;
			
			MemcpyInts(pdoffset, psoffset, width);
			if (pDest == &g_mainScreenVBEData)
			{
				// Hrm. Also draw to the copy, you never know.
				pdoffset = g_framebufferCopy + (yDest * pDest->m_width) + cx;
				MemcpyInts(pdoffset, psoffset, width);
			}
		}
		
		DirtyRectLogger(cx, cy, width, height);
	}
	else if (mode == BOP_DSTFILL)
	{
		for (int y = 0; y < height; y++)
		{
			//copy one scanline from cy to y1
			int yDest = cy + y;
			
			//determine the offset for each scanline:
			uint32_t* pdoffset = pDest->m_framebuffer32 + (yDest * pDest->m_pitch32) + cx;
			
			memset_ints(pdoffset, x1, width);
			if (pDest == &g_mainScreenVBEData)
			{
				// Hrm. Also draw to the copy, you never know.
				pdoffset = g_framebufferCopy + (yDest * pDest->m_width) + cx;
				memset_ints(pdoffset, x1, width);
			}
		}
		DirtyRectLogger(cx, cy, width, height);
	}
	else SLogMsg("TODO: VidBitBlit mode %x");
}

void ScrollRect(Rectangle* pRect, int amountX, int amountY)
{
	if (amountX == 0 && amountY == 0) return;
	
	int cx = pRect->left, cy = pRect->top, x1 = cx, y1 = cy;
	int width = GetWidth(pRect), height = GetHeight(pRect);
	
	if (amountX > 0) cx += amountX, width  -= amountX;
	else             x1 -= amountX, width  += amountX;
	if (amountY > 0) cy += amountY, height -= amountY;
	else             y1 -= amountY, height += amountY;
	
	VidBitBlit(VidGetVBEData(), cx, cy, width, height, VidGetVBEData(), x1, y1, BOP_SRCCOPY);
}

#endif

// Stuff
#if 1

__attribute__((always_inline))
static inline void RenderCursorTransparent(void)
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
			xe = GetScreenSizeX();
		if (ye >= GetScreenSizeY())
			ye = GetScreenSizeY();
		//int xd = (xe - xs) * sizeof(uint32_t);
		int off11 = 0;
		for (int y = ys, ky = kys, kz = kzs; y < ye; y++, kz++)
		{
			off11 = y * g_vbeData->m_pitch32 + xs;
			ky = kz * g_currentCursor->width + off;
			//just memcpy shit
			//memcpy (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + xs], &g_currentCursor->bitmap[ky], xd);
			for (int x = xs; x < xe; x++)
			{
				uint32_t test = g_currentCursor->bitmap[ky];
				if (test != TRANSPARENT)
				{
					if (test == SEMI_TRANSPARENT)
					{
						test = VidReadPixelInline(x, y);
						test = (((test & 0xFF0000)>>1) & 0xFF0000) | (((test & 0xFF00)>>1) & 0xFF00) | (((test & 0xFF)>>1) & 0xFF);
						//if (!g_disableShadows)
							g_vbeData->m_framebuffer32[off11] = test;
					}
					else
						g_vbeData->m_framebuffer32[off11] = test;
				}
				ky++;
				off11++;
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
					uint32_t test = g_currentCursor->bitmap[id];
					if (test == SEMI_TRANSPARENT)
					{
						test = VidReadPixelInline(kx, ky);
						test = ((test>>1) & 0xFF0000) | ((test>>1) & 0xFF00) | ((test>>1) & 0xFF);
					}
					//int kx = j + g_mouseX - g_currentCursor->leftOffs,
					//	ky = i + g_mouseY - g_currentCursor->topOffs;
					if (kx < 0 || ky < 0 || kx >= GetScreenSizeX() || ky >= GetScreenSizeY()) continue;
					VidPlotPixelIgnoreCursorChecksChecked (
						kx,
						ky,
						test
					);
				}
			}
		}
	}
}
__attribute__((always_inline))
static inline void RenderCursorOpaque(void)
{
	if (!g_RenderWindowContents)
	{
		//Just XOR the pixels around the window frame
		
		for (int i = 0, ky=g_mouseY - g_currentCursor->topOffs; i < g_currentCursor->height; i++, ky++)
		{
			if (ky < 0) i = -ky, ky = 0;
			if (ky >= GetScreenHeight()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(g_mouseX - g_currentCursor->leftOffs + i,                          ky, VidReadPixel(g_mouseX - g_currentCursor->leftOffs + i,                          ky) ^ 0xFFFFFFFF);
				VidPlotPixelIgnoreCursorChecksChecked(g_mouseX - g_currentCursor->leftOffs + g_currentCursor->width-1-i, ky, VidReadPixel(g_mouseX - g_currentCursor->leftOffs + g_currentCursor->width-1-i, ky) ^ 0xFFFFFFFF);
			}
		}
		for (int j = 0, kx=g_mouseX - g_currentCursor->leftOffs; j < g_currentCursor->width; j++, kx++)
		{
			if (kx < 0) j = -kx, kx = 0;
			if (kx >= GetScreenWidth()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(kx, g_mouseY - g_currentCursor->topOffs + i,                           VidReadPixel(kx, g_mouseY - g_currentCursor->topOffs + i                          ) ^ 0xFFFFFFFF);
				VidPlotPixelIgnoreCursorChecksChecked(kx, g_mouseY - g_currentCursor->topOffs + g_currentCursor->height-1-i, VidReadPixel(kx, g_mouseY - g_currentCursor->topOffs + g_currentCursor->height-1-i) ^ 0xFFFFFFFF);
			}
		}
		return;
	}
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
			xe =  GetScreenSizeX();
		if (ye >= GetScreenSizeY())
			ye =  GetScreenSizeY();
		int xd = (xe - xs) * sizeof(uint32_t);
		for (int y = ys, ky = kys, kz = kzs; y < ye; y++, kz++)
		{
			ky = kz * g_currentCursor->width + off;
			//just memcpy shit
			align4_memcpy (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + xs], &g_currentCursor->bitmap[ky], xd);
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

__attribute__((always_inline))
static inline void RenderCursorStretchy(void)
{
	//if (!g_RenderWindowContents)
	{
		//Just XOR the pixels around the window frame
		
		for (int i = 0, ky=g_mouseY - g_currentCursor->topOffs; i < g_currentCursor->boundsHeight; i++, ky++)
		{
			if (ky < 0) i = -ky, ky = 0;
			if (ky >= GetScreenHeight()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(g_mouseX - g_currentCursor->leftOffs + i,                                ky, VidReadPixel(g_mouseX - g_currentCursor->leftOffs + i,                                ky) ^ 0xFFFFFFFF);
				VidPlotPixelIgnoreCursorChecksChecked(g_mouseX - g_currentCursor->leftOffs + g_currentCursor->boundsWidth-1-i, ky, VidReadPixel(g_mouseX - g_currentCursor->leftOffs + g_currentCursor->boundsWidth-1-i, ky) ^ 0xFFFFFFFF);
			}
		}
		for (int j = 0, kx=g_mouseX - g_currentCursor->leftOffs; j < g_currentCursor->boundsWidth; j++, kx++)
		{
			if (kx < 0) j = -kx, kx = 0;
			if (kx >= GetScreenWidth()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(kx, g_mouseY - g_currentCursor->topOffs + i,                                 VidReadPixel(kx, g_mouseY - g_currentCursor->topOffs + i                                ) ^ 0xFFFFFFFF);
				VidPlotPixelIgnoreCursorChecksChecked(kx, g_mouseY - g_currentCursor->topOffs + g_currentCursor->boundsHeight-1-i, VidReadPixel(kx, g_mouseY - g_currentCursor->topOffs + g_currentCursor->boundsHeight-1-i) ^ 0xFFFFFFFF);
			}
		}
		return;
	}
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
		if (ye >= GetScreenHeight())
			ye =  GetScreenHeight();
		int xs =                         - g_currentCursor->leftOffs+ g_mouseX;
		int xe = g_currentCursor->width  - g_currentCursor->leftOffs+ g_mouseX;
		int off = 0;
		if (xs < 0)
		{
			off = -xs;
			xs = 0;
		}
		if (xe >= GetScreenSizeX())
			xe =  GetScreenSizeX();
		int xd = (xe - xs) * sizeof(uint32_t);
		for (int y = ys, ky = kys, kz = kzs; y < ye; y++, kz++)
		{
			ky = kz * g_currentCursor->width + off;
			//just memcpy shit
			align4_memcpy (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + xs], &g_currentCursor->bitmap[ky], xd);
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

void RenderCursor(void)
{
	if (!IsWindowManagerRunning())
		return;
	
	if (g_currentCursor->m_transparency)
		RenderCursorTransparent();
	else if (g_currentCursor->m_flags & CUR_RESIZE)
		RenderCursorStretchy();
	else
		RenderCursorOpaque();
}

__attribute__((always_inline))
static inline void RedrawOldPixelsTransparent(int oldX, int oldY)
{
	// We have no other choice but to do this.
	for (int i = 0; i < g_currentCursor->height; i++)
	{
		for (int j = 0; j < g_currentCursor->width; j++)
		{
			int id = i * g_currentCursor->width + j;
			if (g_currentCursor->bitmap[id] != TRANSPARENT)
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
}
__attribute__((always_inline))
static inline void RedrawOldPixelsOpaque(int oldX, int oldY)
{
	/*for (int i = 0; i < g_currentCursor->height; i++)
	{
		for (int j = 0; j < g_currentCursor->width; j++)
		{
			int kx = j + oldX - g_currentCursor->leftOffs,
				ky = i + oldY - g_currentCursor->topOffs;
			if (kx < 0 || ky < 0 || kx >= GetScreenSizeX() || ky >= GetScreenSizeY()) continue;
			VidPlotPixelCheckCursor (
			//VidPlotPixelInline(
				kx, ky, VidReadPixelInline (kx, ky)
			);
		}
	}*/
	if (!g_RenderWindowContents)
	{
		//Just XOR the pixels around the window frame
		
		for (int i = 0, ky=oldY - g_currentCursor->topOffs; i < g_currentCursor->height; i++, ky++)
		{
			if (ky < 0) i = -ky, ky = 0;
			if (ky >= GetScreenHeight()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(oldX - g_currentCursor->leftOffs + i,                          ky, VidReadPixel(oldX - g_currentCursor->leftOffs + i,                          ky));
				VidPlotPixelIgnoreCursorChecksChecked(oldX - g_currentCursor->leftOffs + g_currentCursor->width-1-i, ky, VidReadPixel(oldX - g_currentCursor->leftOffs + g_currentCursor->width-1-i, ky));
			}
		}
		for (int j = 0, kx=oldX - g_currentCursor->leftOffs; j < g_currentCursor->width; j++, kx++)
		{
			if (kx < 0) j = -kx, kx = 0;
			if (kx >= GetScreenWidth()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(kx, oldY - g_currentCursor->topOffs + i,                           VidReadPixel(kx, oldY - g_currentCursor->topOffs + i                          ));
				VidPlotPixelIgnoreCursorChecksChecked(kx, oldY - g_currentCursor->topOffs + g_currentCursor->height-1-i, VidReadPixel(kx, oldY - g_currentCursor->topOffs + g_currentCursor->height-1-i));
			}
		}
		return;
	}
	
	// Draw over the Y coordinate.
	
	int left = oldX - g_currentCursor->leftOffs, right = left + g_currentCursor->width;
	int top  = oldY - g_currentCursor->topOffs,  bottom= top  + g_currentCursor->height;
	int topUpTo = g_mouseY - g_currentCursor->topOffs;
	if (top < 0) top = 0;
	if (topUpTo < 0) topUpTo = 0;
	if (top >= GetScreenHeight()) top = GetScreenHeight();
	if (topUpTo >= GetScreenHeight()) topUpTo = GetScreenHeight();
	if (bottom < 0) bottom= 0;
	if (bottom >= GetScreenHeight()) bottom = GetScreenHeight();
	if (left < 0) left = 0;
	if (left >= GetScreenWidth()) left = GetScreenWidth();
	if (right < 0) right = 0;
	if (right >= GetScreenWidth()) right = GetScreenWidth();
	int xs = right-left+1;
	int yoffscp, yoffsfb;
	int startcp, startfb;
	if (xs > 0)
	{
		// Did we move down?
		if (oldY < g_mouseY)
		{
			yoffscp = g_vbeData->m_width * top, yoffsfb = g_vbeData->m_pitch32 * top;
			startcp = yoffscp + left,           startfb = yoffsfb + left;
			//SLogMsg("top:%d topUpTo:%d yoffscp:%d startcp:%d yoffsfb:%d startfb:%d xs:%d", top, topUpTo,yoffscp,startcp,yoffsfb,startfb,xs);
			for (int y = top; y < topUpTo; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
			
		}
		// Did we move up?
		else if (oldY > g_mouseY)
		{
			topUpTo = g_mouseY - g_currentCursor->topOffs;
			int bottomUpTo = topUpTo + g_currentCursor->height;
			if (bottomUpTo < 0) bottom= 0;
			if (bottomUpTo >= GetScreenHeight()) bottomUpTo = GetScreenHeight();
			yoffscp = g_vbeData->m_width * bottomUpTo, yoffsfb = g_vbeData->m_pitch32 * bottomUpTo;
			startcp = yoffscp + left,                  startfb = yoffsfb + left;
			for (int y = bottomUpTo; y < bottom; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
		}
	}
	
	// Did we move right?
	if (oldX < g_mouseX)
	{
		yoffscp = g_vbeData->m_width * top, yoffsfb = g_vbeData->m_pitch32 * top;
		int leftUpTo = g_mouseX - g_currentCursor->leftOffs;
		if (leftUpTo < 0) leftUpTo = 0;
		if (leftUpTo >= GetScreenWidth()) leftUpTo = GetScreenWidth();
		
		xs = leftUpTo-left;
		if (xs > 0)
		{
			startcp = yoffscp + left, startfb = yoffsfb + left;
			//SLogMsg("left:%d leftUpTo:%d yoffscp:%d startcp:%d yoffsfb:%d startfb:%d xs:%d", left, leftUpTo,yoffscp,startcp,yoffsfb,startfb,xs);
			for (int y = top; y < bottom; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
		}
	}
	// Did we move left?
	else if (oldX > g_mouseX)
	{
		yoffscp = g_vbeData->m_width * top, yoffsfb = g_vbeData->m_pitch32 * top;
		int leftUpTo = g_mouseX - g_currentCursor->leftOffs;
		int rightUpTo = leftUpTo +  g_currentCursor->width;
		if (rightUpTo < 0) rightUpTo = 0;
		if (rightUpTo >= GetScreenWidth()) rightUpTo = GetScreenWidth();
		
		xs = right-rightUpTo;
		if (xs > 0)
		{
			startcp = yoffscp + rightUpTo, startfb = yoffsfb + rightUpTo;
			//SLogMsg("left:%d leftUpTo:%d yoffscp:%d startcp:%d yoffsfb:%d startfb:%d xs:%d", left, leftUpTo,yoffscp,startcp,yoffsfb,startfb,xs);
			for (int y = top; y < bottom; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
		}
	}
}
__attribute__((always_inline))
static inline void RedrawOldPixelsStretchy(int oldX, int oldY)
{
	/*for (int i = 0; i < g_currentCursor->height; i++)
	{
		for (int j = 0; j < g_currentCursor->width; j++)
		{
			int kx = j + oldX - g_currentCursor->leftOffs,
				ky = i + oldY - g_currentCursor->topOffs;
			if (kx < 0 || ky < 0 || kx >= GetScreenSizeX() || ky >= GetScreenSizeY()) continue;
			VidPlotPixelCheckCursor (
			//VidPlotPixelInline(
				kx, ky, VidReadPixelInline (kx, ky)
			);
		}
	}*/
	//if (!g_RenderWindowContents)
	{
		//Just XOR the pixels around the window frame
		
		for (int i = 0, ky=oldY - g_currentCursor->topOffs; i < g_currentCursor->boundsHeight; i++, ky++)
		{
			if (ky < 0) i = -ky, ky = 0;
			if (ky >= GetScreenHeight()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(oldX - g_currentCursor->leftOffs + i,                                ky, VidReadPixel(oldX - g_currentCursor->leftOffs + i,                                ky));
				VidPlotPixelIgnoreCursorChecksChecked(oldX - g_currentCursor->leftOffs + g_currentCursor->boundsWidth-1-i, ky, VidReadPixel(oldX - g_currentCursor->leftOffs + g_currentCursor->boundsWidth-1-i, ky));
			}
		}
		for (int j = 0, kx=oldX - g_currentCursor->leftOffs; j < g_currentCursor->boundsWidth; j++, kx++)
		{
			if (kx < 0) j = -kx, kx = 0;
			if (kx >= GetScreenWidth()) break;
			for (int i = 0; i < VISIBLE_DRAW_BORDER_THICKNESS; i++)
			{
				VidPlotPixelIgnoreCursorChecksChecked(kx, oldY - g_currentCursor->topOffs + i,                                 VidReadPixel(kx, oldY - g_currentCursor->topOffs + i                                ));
				VidPlotPixelIgnoreCursorChecksChecked(kx, oldY - g_currentCursor->topOffs + g_currentCursor->boundsHeight-1-i, VidReadPixel(kx, oldY - g_currentCursor->topOffs + g_currentCursor->boundsHeight-1-i));
			}
		}
		return;
	}
	
	// Draw over the Y coordinate.
	
	int left = oldX - g_currentCursor->leftOffs, right = left + g_currentCursor->width;
	int top  = oldY - g_currentCursor->topOffs,  bottom= top  + g_currentCursor->height;
	int topUpTo = g_mouseY - g_currentCursor->topOffs;
	if (top < 0) top = 0;
	if (topUpTo < 0) topUpTo = 0;
	if (top >= GetScreenHeight()) top = GetScreenHeight();
	if (topUpTo >= GetScreenHeight()) topUpTo = GetScreenHeight();
	if (bottom < 0) bottom= 0;
	if (bottom >= GetScreenHeight()) bottom = GetScreenHeight();
	if (left < 0) left = 0;
	if (left >= GetScreenWidth()) left = GetScreenWidth();
	if (right < 0) right = 0;
	if (right >= GetScreenWidth()) right = GetScreenWidth();
	int xs = right-left+1;
	int yoffscp, yoffsfb;
	int startcp, startfb;
	if (xs > 0)
	{
		// Did we move down?
		if (oldY < g_mouseY)
		{
			yoffscp = g_vbeData->m_width * top, yoffsfb = g_vbeData->m_pitch32 * top;
			startcp = yoffscp + left,           startfb = yoffsfb + left;
			//SLogMsg("top:%d topUpTo:%d yoffscp:%d startcp:%d yoffsfb:%d startfb:%d xs:%d", top, topUpTo,yoffscp,startcp,yoffsfb,startfb,xs);
			for (int y = top; y < topUpTo; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
			
		}
		// Did we move up?
		else if (oldY > g_mouseY)
		{
			topUpTo = g_mouseY - g_currentCursor->topOffs;
			int bottomUpTo = topUpTo + g_currentCursor->height;
			if (bottomUpTo < 0) bottom= 0;
			if (bottomUpTo >= GetScreenHeight()) bottomUpTo = GetScreenHeight();
			yoffscp = g_vbeData->m_width * bottomUpTo, yoffsfb = g_vbeData->m_pitch32 * bottomUpTo;
			startcp = yoffscp + left,                  startfb = yoffsfb + left;
			for (int y = bottomUpTo; y < bottom; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
		}
	}
	
	// Did we move right?
	if (oldX < g_mouseX)
	{
		yoffscp = g_vbeData->m_width * top, yoffsfb = g_vbeData->m_pitch32 * top;
		int leftUpTo = g_mouseX - g_currentCursor->leftOffs;
		if (leftUpTo < 0) leftUpTo = 0;
		if (leftUpTo >= GetScreenWidth()) leftUpTo = GetScreenWidth();
		
		xs = leftUpTo-left;
		if (xs > 0)
		{
			startcp = yoffscp + left, startfb = yoffsfb + left;
			//SLogMsg("left:%d leftUpTo:%d yoffscp:%d startcp:%d yoffsfb:%d startfb:%d xs:%d", left, leftUpTo,yoffscp,startcp,yoffsfb,startfb,xs);
			for (int y = top; y < bottom; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
		}
	}
	// Did we move left?
	else if (oldX > g_mouseX)
	{
		yoffscp = g_vbeData->m_width * top, yoffsfb = g_vbeData->m_pitch32 * top;
		int leftUpTo = g_mouseX - g_currentCursor->leftOffs;
		int rightUpTo = leftUpTo +  g_currentCursor->width;
		if (rightUpTo < 0) rightUpTo = 0;
		if (rightUpTo >= GetScreenWidth()) rightUpTo = GetScreenWidth();
		
		xs = right-rightUpTo;
		if (xs > 0)
		{
			startcp = yoffscp + rightUpTo, startfb = yoffsfb + rightUpTo;
			//SLogMsg("left:%d leftUpTo:%d yoffscp:%d startcp:%d yoffsfb:%d startfb:%d xs:%d", left, leftUpTo,yoffscp,startcp,yoffsfb,startfb,xs);
			for (int y = top; y < bottom; y++)
			{
				//to get an awesome glitch effect, switch this out with memset_ints :D
				memcpy_ints(&g_vbeData->m_framebuffer32[startfb], &g_framebufferCopy[startcp], xs);
				startcp += g_vbeData->m_width;
				startfb += g_vbeData->m_pitch32;
			}
		}
	}
}

void RedrawOldPixels(int oldX, int oldY)
{
	if (g_currentCursor->m_transparency)
		RedrawOldPixelsTransparent(oldX, oldY);
	else if (g_currentCursor->m_flags & CUR_RESIZE)
		RedrawOldPixelsStretchy   (oldX, oldY);
	else
		RedrawOldPixelsOpaque     (oldX, oldY);
}
void RefreshPixels(int oldX, int oldY, int oldWidth, int oldHeight)
{
	if (oldX > GetScreenWidth ()) return;
	if (oldY > GetScreenHeight()) return;
	if (oldX + oldWidth  < 0) return;
	if (oldY + oldHeight < 0) return;
	
	//NEW: Optimization
	int ys =           + oldY;
	int ye = oldHeight + oldY;
	if (ys < 0)
		ys = 0;
	if (ye >= GetScreenHeight())
		ye =  GetScreenHeight();
	int xs =          + oldX;
	int xe = oldWidth + oldX;
	if (xs < 0)
		xs = 0;
	if (xe >= GetScreenSizeX())
		xe =  GetScreenSizeX();
	int xd = (xe - xs) * sizeof(uint32_t);
	if (xd == 0)
		return;
	for (int y = ys; y < ye; y++)
	{
		int ky = y * g_vbeData->m_width + xs;
		//just memcpy shit
		align4_memcpy (&g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32 + xs], &g_framebufferCopy[ky], xd);
	}
}
void RedrawOldPixelsFull(int oldX, int oldY)
{
	RefreshPixels(oldX - g_currentCursor->leftOffs, oldY - g_currentCursor->topOffs, g_currentCursor->width, g_currentCursor->height);
}

Point GetMousePos ()
{
	Point p = { g_mouseX, g_mouseY };
	return p;
}

void SetMousePosUnsafe(int newX, int newY)
{
	//NOTE: As this is called in an interrupt too, a call here might end up coming right
	//while we we're drawing a  or something.  Keep a backup of the previous settings.
	
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	
	int oldX = g_mouseX, oldY = g_mouseY;
	
	if (newX < 0) newX = 0;
	if (newY < 0) newY = 0;
	if (newX >= GetScreenSizeX()) newX = GetScreenSizeX() - 1;
	if (newY >= GetScreenSizeY()) newY = GetScreenSizeY() - 1;
	
	g_mouseX = newX, g_mouseY = newY;
	
	//--uncomment if you want one pixel cursor (This is very useless and hard to use)
	//VidPlotPixelIgnoreCursorChecks (g_mouseX, g_mouseY, 0xFF);
	//VidPlotPixel (oldX, oldY, VidReadPixel(oldX, oldY));
	
	//Redraw all the pixels under where the cursor was previously:
	RedrawOldPixels(oldX, oldY);
	
	//Draw the cursor image at the new position:
	RenderCursor();
	//TODO: check flags here
	
	g_vbeData = backup;
}

void SetMousePos(int x, int y)
{
	KeVerifyInterruptsEnabled;
	cli;
	SetMousePosUnsafe(x, y);
	sti;
}

#endif

// Video initialization
#if 1
void VidInitializeVBEData(multiboot_info_t* pInfo, void* address)
{
	int bpp = pInfo->framebuffer_bpp;
	g_vbeData->m_version   = VBEDATA_VERSION_1;
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
	
	g_vbeData->m_framebuffer32 = (uint32_t*)address;
	g_vbeData->m_framebuffer16 = (uint16_t*)address;
	g_vbeData->m_framebuffer8  = (uint8_t *)address;
	
	VidSetClipRect (NULL);
}

bool     g_IsBGADevicePresent;
uint32_t g_BGADeviceBAR0;
void BgaWriteRegister(unsigned short index, unsigned short data)
{
	WritePortW(VBE_DISPI_IOPORT_INDEX, index);
	WritePortW(VBE_DISPI_IOPORT_DATA,  data);
}
unsigned short BgaReadRegister(unsigned short index)
{
	WritePortW(VBE_DISPI_IOPORT_INDEX, index);
	return ReadPortW(VBE_DISPI_IOPORT_DATA);
}
bool BgaIsAvailable()
{
	return (BgaReadRegister(VBE_DISPI_INDEX_ID) & 0xFFF0) == VBE_DISPI_ID0;//can be 0xB0C0-0xB0CF or else
}
bool BgaChangeScreenResolution(int xSize, int ySize)
{
	if (!BgaIsAvailable())
	{
		SLogMsg("BGA device not available.");
		return false;
	}
	
	BgaWriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
	BgaWriteRegister(VBE_DISPI_INDEX_XRES,   xSize);
	BgaWriteRegister(VBE_DISPI_INDEX_YRES,   ySize);
	BgaWriteRegister(VBE_DISPI_INDEX_BPP,    32);
	BgaWriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED | VBE_DISPI_NOCLEARMEM);
	return true;
}
void* MmAllocateInternal(size_t size, uint32_t* physAddresses, bool bInterruptsEnabled);
bool VidChangeScreenResolution(int xSize, int ySize)
{
	if (!g_IsBGADevicePresent)
	{
		//may want to test it?
		if (BgaIsAvailable())
		{
			g_IsBGADevicePresent = true;
			SLogMsg("Found BGA device.");
			return VidChangeScreenResolution(xSize, ySize);
		}
	}
	else
	{
		if (g_mainScreenVBEData.m_framebuffer32 != (uint32_t*)0xE0000000)
		{
			SLogMsg("Attempt to VidChangeScreenResolution may fail!");
		}
		
		if (!BgaChangeScreenResolution(xSize, ySize))
		{
			g_IsBGADevicePresent = false;
			return false;
		}
		
		cli;
		
		//Assume that everything went ok, and set our main screen VBE data to have that:
		g_mainScreenVBEData.m_version   = VBEDATA_VERSION_1;
		g_mainScreenVBEData.m_width     = xSize;
		g_mainScreenVBEData.m_height    = ySize;
		g_mainScreenVBEData.m_pitch     = xSize * 4;//TODO: Hack
		g_mainScreenVBEData.m_pitch16   = xSize * 2;
		g_mainScreenVBEData.m_pitch32   = xSize * 1;
		g_mainScreenVBEData.m_bitdepth  = 2;
		g_mainScreenVBEData.m_dirty     = 1;
		//else, preserve the address
		
		//if  we have a g_framebufferCopy allocated yet, free it and replace it with something new:
		if (g_framebufferCopy)
		{
			MmFreeID(g_framebufferCopy);
		}
		g_framebufferCopy = (uint32_t*)MmAllocateInternal(xSize * ySize * 32, ALLOCATE_BUT_DONT_WRITE_PHYS, false);
		
		sti;
		
		return true;
	}
	return false;
}

void BgaInitIfApplicable()
{
	if (!VidIsAvailable())
	{
		multiboot_info_t* mbi = KiGetMultibootInfo();
		SLogMsg("BGA device BAR0:%x", g_BGADeviceBAR0);
		
		// try this:
		#define DEFAULT_WIDTH 1024
		#define DEFAULT_HEIGHT 768
		
		BgaChangeScreenResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);
		mbi->flags |= MULTIBOOT_INFO_FRAMEBUFFER_INFO;
		mbi->framebuffer_type = 1;
		// TODO FIXME: We assume this is the address, but what if it isn't?
		// Furthermore, what if the pitch doesn't match the width*4?
		mbi->framebuffer_addr   = g_BGADeviceBAR0;
		mbi->framebuffer_width  = DEFAULT_WIDTH;
		mbi->framebuffer_pitch  = DEFAULT_WIDTH * 4;
		mbi->framebuffer_height = DEFAULT_HEIGHT;
		mbi->framebuffer_bpp = 32;

		// and re-attempt init:
		VidInit();
	}
}

//present, read/write, user/supervisor, writethrough
void * MhMapPhysicalMemory(uintptr_t physMem, size_t numPages, bool bReadWrite);

void VidInit()
{
	multiboot_info_t* pInfo = KiGetMultibootInfo();
	
	g_vbeData = &g_mainScreenVBEData;
	g_vbeData->m_version = 0;
	
	if (pInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
	{
		if (pInfo->framebuffer_type != 1)
		{
			SLogMsg("Need direct RGB framebuffer!");
			sti;
			return;
		}
		
		uint32_t pointer = pInfo->framebuffer_addr;
		size_t p = pInfo->framebuffer_pitch * pInfo->framebuffer_height;
		void *final_address = MhMapPhysicalMemory(pointer, (p + 4095) / 4096, true);
		
		g_framebufferCopy = MmAllocateInternal(p, ALLOCATE_BUT_DONT_WRITE_PHYS, false);
		
		// initialize the VBE data:
		VidInitializeVBEData (pInfo, final_address);
		
		VidPrintTestingPattern();
		
		VidSetFont (FONT_TAMSYN_REGULAR);
		
		CoInitAsGraphics(&g_debugConsole);
	}
	else
	{
		SLogMsg("Sorry, didn't pass in VBE info.");
		SwitchMode (1);
		CoInitAsText(&g_debugConsole);
	}
}
#endif

// Dirty rect logger
#if 1

bool RectangleOverlapTolerant(Rectangle* r1, Rectangle* r2)
{
	return (r1->left - 3 <= r2->right + 3 && r1->right + 3 >= r2->left - 3 && r1->top - 3 <= r2->bottom + 3 && r1->bottom + 3 >= r2->top - 3);
}

// if r2 is FULLY inside r1
bool RectangleFullyInside(Rectangle* r1, Rectangle* r2)
{
	// r2 is fully contained inside r1
	return (
	r1->left <= r2->left && r2->right  <= r1->right  &&
	r1->top  <= r2->top  && r2->bottom <= r1->bottom
	);
}
void DisjointRectSetClear (DsjRectSet *pSet)
{
	if (!pSet) return;
	pSet->m_rectCount = 0;
	pSet->m_bIgnoreAndDrawAll = false;
}

void DisjointRectSetAdd (DsjRectSet* pSet, Rectangle rect)
{
	if (!pSet) return;
	
#ifdef DIRTY_RECT_TRACK
	int ox = 0, oy = 0;
	if (g_vbeData->m_version >= VBEDATA_VERSION_3)
		ox = g_vbeData->m_offsetX, oy = g_vbeData->m_offsetY;
	
	if (rect.left < 0) rect.left = 0;
	if (rect.top  < 0) rect.top  = 0;
	if (rect.right  >= GetScreenSizeX()) rect.right  = GetScreenSizeX();
	if (rect.bottom >= GetScreenSizeY()) rect.bottom = GetScreenSizeY();
	
	if (rect.left >= rect.right || rect.top >= rect.bottom) return;
	
	rect.left += ox; rect.right += ox;
	rect.top += oy; rect.bottom += oy;
	
	bool bIntsEnabled = !KeCheckInterruptsDisabled();
	
	if (bIntsEnabled) cli;
	
	if (pSet->m_bIgnoreAndDrawAll)
	{
		if (bIntsEnabled) sti;
		return;
	}
	
	for (int i = 0; i < pSet->m_rectCount; i++)
	{
		Rectangle* arect = &pSet->m_rects[i];
		if (RectangleFullyInside(arect, &rect))
		{
			if (bIntsEnabled) sti;
			return; //nothing changed
		}

		//check if the rectangles intersect
		if (RectangleOverlapTolerant (arect, &rect))
		{
			//yes, let's just merge them both
			int left   = arect->left;   if (left   > rect.left)   left   = rect.left;
			int top    = arect->top;    if (top    > rect.top)    top    = rect.top;
			int right  = arect->right;  if (right  < rect.right)  right  = rect.right;
			int bottom = arect->bottom; if (bottom < rect.bottom) bottom = rect.bottom;
			
			arect->left   = left;
			arect->top    = top;
			arect->right  = right;
			arect->bottom = bottom;
			
			if (bIntsEnabled) sti;
			
			return;
		}
	}
	
	if (pSet->m_rectCount >= 100)
	{
		// All, hell naw!
		// There's another check for this, but why not check here too? Maybe this'll get called
		// from something by itself instead of just calling DisjointRectSetAdd.
		// Bail and draw everything.
		pSet->m_bIgnoreAndDrawAll = true;
		DisjointRectSetClear(pSet);
		
		if (bIntsEnabled) sti;
		
		return;
	}
	
	//merging will be done by the window manager itself
	pSet->m_rects[pSet->m_rectCount++] = rect;
	
	if (bIntsEnabled) sti;
	
#endif
}

// The function a vbedata will use to let us know of changes
void DirtyRectLogger (int x, int y, int width, int height)
{
#ifdef DIRTY_RECT_TRACK
	if (g_vbeData->m_version < VBEDATA_VERSION_2)
		return;
	
	Rectangle rect = { x, y, x + width, y + height };
	DisjointRectSetAdd(g_vbeData->m_drs, rect);
#endif
}

void VidCorruptScreenForTesting()
{
	VBEData* backup = g_vbeData;
	if (g_vbeData != &g_mainScreenVBEData)
	{
		g_vbeData = &g_mainScreenVBEData;
	}
	
	VidPrintTestingPattern();
	
	g_vbeData = backup;
}
void DirtyRectInvalidateAll()
{
#ifdef DIRTY_RECT_TRACK
	g_vbeData->m_drs->m_bIgnoreAndDrawAll = true;
	DisjointRectSetClear(g_vbeData->m_drs);
#endif
}
#endif
