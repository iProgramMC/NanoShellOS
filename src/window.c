/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

           Window Manager module
******************************************/

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

#undef cli
#undef sti
#define cli
#define sti

//fps counter:
#if 1

int g_FPS, g_FPSThisSecond, g_FPSLastCounted;

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

//background code:
#if 1

//#define CHECKER_PATTERN
#ifdef CHECKER_PATTERN
	#define BG_WHITE 0xFF007F7F
	#define BG_BLACK 0xFF005C5C
	const uint32_t g_placeholderBackground[] = {
		BACKGROUND_COLOR,
		/*00*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*01*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*02*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*03*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*04*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*05*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*06*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*07*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*08*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*09*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*0A*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*0B*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*0C*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*0D*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*0E*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*0F*/BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,
		/*10*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*11*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*12*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*13*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*14*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*15*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*16*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*17*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*18*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*19*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*1A*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*1B*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*1C*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*1D*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*1E*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
		/*1F*/BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_WHITE,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,BG_BLACK,
	};
	
	Image* g_background, g_defaultBackground = {
		32, 32, g_placeholderBackground
	};
#else
	const uint32_t g_placeholderBackground[] = {
		BACKGROUND_COLOR,
	};
	
	Image* g_background, g_defaultBackground = {
		1, 1, g_placeholderBackground
	};
#endif

void RedrawBackground (Rectangle rect)
{
	/*rect.bottom++, rect.right++;
	for (int y = rect.top; y != rect.bottom; y++)
	{
		for (int x = rect.left; x != rect.right; x++)
		{
			//TODO: is a z-buffer check necessary?
			if (true/ * && ...* /)
			{
				VidPlotPixel (x, y, g_background->framebuffer[(x % g_background->width) + g_background->width * (y % g_background->height)]);
			}
		}
	}*/
	//simple background:
	VidFillRectangle (BACKGROUND_COLOR, rect);
}

void SetDefaultBackground()
{
	g_background = &g_defaultBackground;
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
	int gapdiv2 = gap / 2;
	idxl += hx * r.top;
	for (int y = r.top; y < r.bottom; y++)
	{
		if (y >= hy) break;//no point.
		if (y < 0) continue;
		memset_ints(&g_windowDepthBuffer[idxl], index<<16|index, gapdiv2);
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
	ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	
	WindowRegisterEventUnsafe (pWindow, eventType, parm1, parm2);
	
	FREE_LOCK (pWindow->m_eventQueueLock);
}
#endif

// Window utilitary functions:
#if 1

void WindowManagerShutdown()
{
	g_shutdownRequest = true;
}

void UndrawWindow (Window* pWindow)
{
	UpdateDepthBuffer();
	
	//redraw the background and all the things underneath:
	RedrawBackground(pWindow->m_rect);
	
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
		windowDrawList[i]->m_vbeData.m_dirty = true;
		windowDrawList[i]->m_renderFinished = true;
	}
	
	//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
}

void HideWindow (Window* pWindow)
{
	pWindow->m_hidden = true;
	UndrawWindow(pWindow);
}

void ShowWindow (Window* pWindow)
{
	pWindow->m_hidden = false;
	UpdateDepthBuffer();
	//WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
	pWindow->m_vbeData.m_dirty = true;
	pWindow->m_renderFinished = true;
}
extern VBEData* g_vbeData, g_mainScreenVBEData;
extern Heap* g_pHeap;

void ReadyToDestroyWindow (Window* pWindow)
{
	HideWindow (pWindow);
	
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
}

void DestroyWindow (Window* pWindow)
{
	WindowRegisterEventUnsafe(pWindow, EVENT_DESTROY, 0, 0);
	// the task's last WindowCheckMessages call will see this and go
	// "ah yeah they want my window gone", then the WindowCallback will reply and say
	// "yeah you're good to go" and call ReadyToDestroyWindow().
}

void RequestRepaint (Window* pWindow)
{
	WindowRegisterEventUnsafe(pWindow, EVENT_PAINT, 0, 0);
}

extern void SetFocusedConsole(Console* console);
void SelectThisWindowAndUnselectOthers(Window* pWindow)
{
	bool wasSelectedBefore = pWindow->m_isSelected;
	if (!wasSelectedBefore) {
		SetFocusedConsole (NULL);
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
	}
}
#endif

// Window creation
#if 1
Window* CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags)
{
	ACQUIRE_LOCK(g_createLock);
	
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
	
	pWnd->m_used = true;
	int strl = strlen (title) + 1;
	if (strl >= WINDOW_TITLE_MAX) strl = WINDOW_TITLE_MAX - 1;
	memcpy (pWnd->m_title, title, strl + 1);
	
	Heap *pHeapBackup  = g_pHeap;
	ResetToKernelHeap ();
	
	pWnd->m_renderFinished = false;
	pWnd->m_hidden = true;//false;
	pWnd->m_isBeingDragged = false;
	pWnd->m_isSelected = false;
	pWnd->m_eventQueueLock = false;
	pWnd->m_flags = flags;
	
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
	ZeroMemory (pWnd->m_vbeData.m_framebuffer32,  sizeof (uint32_t) * xSize * ySize);
	pWnd->m_vbeData.m_width         = xSize;
	pWnd->m_vbeData.m_height        = ySize;
	pWnd->m_vbeData.m_pitch32       = xSize;
	pWnd->m_vbeData.m_bitdepth      = 2;     // 32 bit :)
	
	pWnd->m_iconID = ICON_APPLICATION;
	
	//give the window a starting point of 10 controls:
	pWnd->m_controlArrayLen = 10;
	size_t controlArraySize = sizeof(Control) * pWnd->m_controlArrayLen;
	pWnd->m_pControlArray   = (Control*)MmAllocateK(controlArraySize);
	memset(pWnd->m_pControlArray, 0, controlArraySize);
	
	WindowRegisterEvent(pWnd, EVENT_CREATE, 0, 0);
	/*UpdateDepthBuffer();
	
	
	AddWindowToDrawOrder (pWnd - g_windows);
	
	SelectThisWindowAndUnselectOthers(pWnd);
	
	cli;
	UpdateDepthBuffer();
	sti;
	*/
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
			SelectThisWindowAndUnselectOthers (window);
			
			//bool wasSelectedBefore = g_currentlyClickedWindow == idx;
			g_currentlyClickedWindow = idx;
			
			if (!window->m_minimized)
			{
				int x = mouseX - window->m_rect.left;
				int y = mouseY - window->m_rect.top;
				WindowRegisterEvent (window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
	else
		g_currentlyClickedWindow = -1;
	
	//FREE_LOCK(g_windowLock);
}
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
			
			int x = mouseX - window->m_rect.left;
			int y = mouseY - window->m_rect.top;
			Point mousePoint = {x, y};
			
			if (RectangleContains(&recta, &mousePoint) || window->m_minimized)
			{
				window->m_isBeingDragged = true;
				
				HideWindow(window);
				
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
				}
				else
				{
					g_windowDragCursor.width    = window->m_vbeData.m_width;
					g_windowDragCursor.height   = window->m_vbeData.m_height;
					g_windowDragCursor.leftOffs = mouseX - window->m_rect.left;
					g_windowDragCursor.topOffs  = mouseY - window->m_rect.top;
					g_windowDragCursor.bitmap   = window->m_vbeData.m_framebuffer32;
					g_windowDragCursor.m_transparency = false;
				}
				
				SetCursor (&g_windowDragCursor);
			}
			else if (!window->m_minimized)
			{
				WindowRegisterEvent (window, EVENT_CLICKCURSOR, MAKE_MOUSE_PARM (x, y), 0);
			}
		}
	}
	
	//FREE_LOCK(g_windowLock);
}
extern int g_mouseX, g_mouseY;//video.c
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
	
	Window* window = GetWindowFromIndex(g_currentlyClickedWindow);
	if (window->m_isBeingDragged)
	{
		Rectangle newWndRect;
		newWndRect.left   = mouseX - g_windowDragCursor.leftOffs;
		newWndRect.top    = mouseY - g_windowDragCursor.topOffs;
		if (newWndRect.top < 0)
			newWndRect.top = 0;
		newWndRect.right  = newWndRect.left + GetWidth(&window->m_rect);
		newWndRect.bottom = newWndRect.top  + GetHeight(&window->m_rect);
		window->m_rect = newWndRect;
		
		ShowWindow(window);
		
		if (GetCurrentCursor() == &g_windowDragCursor)
		{
			SetCursor(NULL);
		}
		//WindowRegisterEvent(window, EVENT_PAINT, 0, 0);
		window->m_vbeData.m_dirty = true;
		window->m_renderFinished = true;
		window->m_isBeingDragged = false;
	}
	int x = mouseX - window->m_rect.left;
	int y = mouseY - window->m_rect.top;
	WindowRegisterEvent (window, EVENT_RELEASECURSOR, MAKE_MOUSE_PARM (x, y), 0);
	
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
			if (window->m_minimized)
				WindowRegisterEvent (window, EVENT_UNMINIMIZE, 0, 0);
		
		//hide this window:
		//HideWindow(window);
		
		//TODO
	}
	//FREE_LOCK(g_windowLock);
}

#endif

// Main loop thread.
#if 1

void RedrawEverything()
{
	//cli;
//	Rectangle r = {0, 0, GetScreenSizeX(), GetScreenSizeY() };
	/*VidFillScreen(BACKGROUND_COLOR);
	
	//wait for apps to fully setup their windows:
	sti;
	for (int i = 0; i < 50000; i++)
		hlt;
	cli;*/
	
	UpdateDepthBuffer();
	
	//sti;
	
	//for each window, send it a EVENT_PAINT:
	for (int p = 0; p < WINDOWS_MAX; p++)
	{
		Window* pWindow = &g_windows [p];
		if (!pWindow->m_used) continue;
		
		WindowRegisterEvent (pWindow, EVENT_PAINT, 0, 0);
	}
}

bool HandleMessages(Window* pWindow);
void RenderWindow (Window* pWindow);
void TerminalHostTask(int arg);
void RefreshMouse(void);
void RenderCursor(void);

static Window* g_pShutdownMessage = NULL;

void WindowManagerOnShutdownTask (__attribute__((unused)) int useless)
{
	if (MessageBox (NULL, "It is now safe to shut down your computer.", "Shutdown Computer", MB_RESTART | ICON_NULL << 16) == MBID_OK)
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
	
	g_debugConsole.curY = g_debugConsole.height / 2;
	g_clickQueueSize = 0;
	// load background?
	memset (&g_windows, 0, sizeof (g_windows));
	InitWindowDepthBuffer();
	CoClearScreen (&g_debugConsole);
	g_debugConsole.curX = g_debugConsole.curY = 0;
	g_debugConsole.pushOrWrap = 1;
	
	g_windowManagerRunning = true;
	
	g_pShutdownMessage = NULL;
	
	g_shutdownSentDestroySignals = false;
	g_shutdownWaiting			 = false;
	
	UpdateDepthBuffer();
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
#endif
}

bool g_heldAlt = false;
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
	else if (key == KEY_TAB)
		OnPressAltTabOnce();
}
void WindowManagerTask(__attribute__((unused)) int useless_argument)
{
	SetupWindowManager();
	
	int timeout = 10;
	int UpdateTimeout = 100, shutdownTimeout = 500;
	
	while (true)
	{
		bool handled = false;
		UpdateFPSCounter();
		CrashReporterCheck();
		for (int p = 0; p < WINDOWS_MAX; p++)
		{
			Window* pWindow = &g_windows [p];
			if (!pWindow->m_used) continue;
			
			if (UpdateTimeout == 0)
			{
				WindowRegisterEvent (pWindow, EVENT_UPDATE, 0, 0);
				UpdateTimeout = 100;
			}
			
			if (pWindow->m_isSelected)
			{
				while (!KbIsBufferEmpty())
					WindowRegisterEvent (pWindow, EVENT_KEYPRESS, KbGetKeyFromBuffer(), 0);
				
				while (!KbIsRawBufferEmpty())
				{
					unsigned char key = KbGetKeyFromRawBuffer();
					WindowRegisterEvent(pWindow, EVENT_KEYRAW, key, 0);
					
					HandleKeypressOnWindow(key);
					handled = true;
				}
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
				if (pWindow->m_renderFinished)
				{
					pWindow->m_renderFinished = false;
					RenderWindow(pWindow);
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
				case CLICK_LEFT:   OnUILeftClick       (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_LEFTD:  OnUILeftClickDrag   (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_LEFTR:  OnUILeftClickRelease(g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
				case CLICK_RIGHT:  OnUIRightClick      (g_clickQueue[i].clickedAtX, g_clickQueue[i].clickedAtY); break;
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
			LogMsgNoCr("\r(Waiting for all windows to shut down... -- %d ticks left.)", shutdownTimeout);
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
				LogMsg("\nAll windows have shutdown gracefully?  Quitting...");
				LogMsg("STATUS: We survived!  Exitting in a brief moment.");
				g_windowManagerRunning = false;
				
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
						
						ReadyToDestroyWindow (&g_windows[i]);
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

//Returns an index, because we might want to relocate the m_pControlArray later.
int AddControl(Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2)
{
	if (!pWindow->m_pControlArray)
	{
		VidSetVBEData(NULL);
		LogMsg("No pControlArray!?");
		KeStopSystem();
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
	pControl->m_rect    = rect;
	pControl->m_comboID = comboID;
	pControl->m_parm1   = p1;
	pControl->m_parm2   = p2;
	pControl->m_bMarkedForDeletion = false;
	
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
	
	//register an event for the window:
	//WindowRegisterEvent(pWindow, EVENT_PAINT, 0, 0);
	
	//call EVENT_CREATE to let the ctl initialize its data
	pControl->OnEvent(pControl, EVENT_CREATE, 0, 0, pWindow);
	
	return index;
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
				if (pMenuBar->OnEvent(pMenuBar, eventType, parm1, parm2, pWindow))
					return;
	
	for (int i = pWindow->m_controlArrayLen - 1; i != -1; i--)
	{
		if (&pWindow->m_pControlArray[i] == pMenuBar) continue; // Skip over the menu bar.
		
		if (pWindow->m_pControlArray[i].m_active)
		{
			Control* p = &pWindow->m_pControlArray[i];
			if (p->OnEvent)
				if (p->OnEvent(p, eventType, parm1, parm2, pWindow))
					return;
		}
	}
	
	if (eventType == EVENT_PAINT || eventType == EVENT_CLICKCURSOR)
		if (pMenuBar)
			if (pMenuBar->OnEvent)
				if (pMenuBar->OnEvent(pMenuBar, eventType, parm1, parm2, pWindow))
					return;
}

#endif

// Modal dialog box code.
#if 1

//Forward declaration
void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow);

void CALLBACK MessageBoxWindowLightCallback (Window* pWindow, int messageType, int parm1, int parm2)
{
	DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

void CALLBACK MessageBoxCallback (Window* pWindow, int messageType, int parm1, int parm2)
{
	if (messageType == EVENT_COMMAND)
	{
		//Which button did we click?
		if (parm1 >= MBID_OK && parm1 < MBID_COUNT)
		{
			//We clicked a valid button.  Return.
			pWindow->m_data = (void*)parm1;
		}
	}
	else
		DefaultWindowProc (pWindow, messageType, parm1, parm2);
}

int MessageBox (Window* pWindow, const char* pText, const char* pCaption, uint32_t style)
{
	// Free the locks that have been acquired.
	bool wnLock = g_windowLock, scLock = g_screenLock, eqLock = false;
	if  (wnLock) FREE_LOCK (g_windowLock);
	if  (scLock) FREE_LOCK (g_screenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_eventQueueLock;
		if (eqLock) FREE_LOCK (pWindow->m_eventQueueLock);
	
		wasSelectedBefore = pWindow->m_isSelected;
		if (wasSelectedBefore)
		{
			pWindow->m_isSelected = false;
			PaintWindowBorderNoBackgroundOverpaint (pWindow);
		}
	}
	
	VBEData* pBackup = g_vbeData;
	
	VidSetVBEData(NULL);
	// Freeze the current window.
	int old_flags = 0;
	WindowProc pProc;
	if (pWindow)
	{
		pProc = pWindow->m_callback;
		old_flags = pWindow->m_flags;
		pWindow->m_callback = MessageBoxWindowLightCallback;
		pWindow->m_flags |= WF_FROZEN;//Do not respond to user attempts to move/other
	}
	
	int szX, szY;
	
	char* test = MmAllocateK(strlen(pText)+5);
	WrapText(test, pText, GetScreenWidth() * 2 / 3);
	
	// Measure the pText text.
	VidTextOutInternal (test, 0, 0, 0, 0, true, &szX, &szY);
	
	szY += 12;
	
	int  iconID = style >> 16;
	bool iconAvailable = iconID != ICON_NULL;
	
	if (iconAvailable)
		if (szY < 50)
			szY = 50;
	
	int buttonWidth  = 70;
	int buttonWidthG = 76;
	int buttonHeight = 20;
	
	// We now have the text's size in szX and szY.  Get the window size.
	int wSzX = szX + 
			   40 + //X padding on both sides
			   10 + //Gap between icon and text.
			   32 * iconAvailable + //Icon's size.
			   5 +
			   WINDOW_RIGHT_SIDE_THICKNESS;//End.
	int wSzY = szY + 
			   20 + //Y padding on both sides
			   buttonHeight + //Button's size.
			   TITLE_BAR_HEIGHT +
			   5 + 
			   WINDOW_RIGHT_SIDE_THICKNESS;
	
	int wPosX = (GetScreenSizeX() - wSzX) / 2,
		wPosY = (GetScreenSizeY() - wSzY) / 2;
	
	// Spawn a new window.
	Window* pBox = CreateWindow (pCaption, wPosX, wPosY, wSzX, wSzY, MessageBoxCallback, WF_NOCLOSE | WF_NOMINIMZ);
	
	// Add the basic controls required.
	Rectangle rect;
	rect.left   = 20 + iconAvailable*32 + 10;
	rect.top    = 20;
	rect.right  = wSzX - 20;
	rect.bottom = wSzY - buttonHeight - 20;
	AddControl (pBox, CONTROL_TEXTHUGE, rect, NULL, 0x10000, 0, TEXTSTYLE_VCENTERED);
	SetHugeLabelText(pBox, 0x10000, test);
	
	MmFreeK(test);
	
	if (iconAvailable)
	{
		rect.left = 20;
		rect.top  = 20 + (szY - 32) / 2;
		rect.right = rect.left + 32;
		rect.bottom= rect.top  + 32;
		AddControl (pBox, CONTROL_ICON, rect, NULL, 0x10001, iconID, 0);
	}
	
	int buttonStyle = style & 0x7;
	switch (buttonStyle)
	{
		case MB_OK:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "OK", MBID_OK, 0, 0);
			break;
		}
		case MB_RESTART:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Restart", MBID_OK, 0, 0);
			break;
		}
		case MB_YESNOCANCEL:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "No", MBID_NO, 0, 0);
			rect.right -= buttonWidthG;
			rect.left  -= buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Yes", MBID_YES, 0, 0);
			rect.right += 2 * buttonWidthG;
			rect.left  += 2 * buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			break;
		}
		case MB_ABORTRETRYIGNORE:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Retry", MBID_RETRY, 0, 0);
			rect.right -= buttonWidthG;
			rect.left  -= buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Abort", MBID_ABORT, 0, 0);
			rect.right += 2 * buttonWidthG;
			rect.left  += 2 * buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Ignore", MBID_IGNORE, 0, 0);
			break;
		}
		case MB_CANCELTRYCONTINUE:
		{
			rect.left = (wSzX - buttonWidth) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Try again", MBID_TRY_AGAIN, 0, 0);
			rect.right -= buttonWidthG;
			rect.left  -= buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			rect.right += 2 * buttonWidthG;
			rect.left  += 2 * buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Continue", MBID_CONTINUE, 0, 0);
			break;
		}
		case MB_YESNO:
		{
			rect.left = (wSzX - buttonWidthG * 2) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Yes", MBID_YES, 0, 0);
			rect.right += buttonWidthG;
			rect.left  += buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "No", MBID_NO, 0, 0);
			break;
		}
		case MB_OKCANCEL:
		{
			rect.left = (wSzX - buttonWidthG * 2) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "OK", MBID_OK, 0, 0);
			rect.right += buttonWidthG;
			rect.left  += buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			break;
		}
		case MB_RETRYCANCEL:
		{
			rect.left = (wSzX - buttonWidthG * 2) / 2;
			rect.top  = (wSzY - buttonHeight - 10);
			rect.right  = rect.left + buttonWidth;
			rect.bottom = rect.top  + buttonHeight;
			AddControl (pBox, CONTROL_BUTTON, rect, "Retry", MBID_RETRY, 0, 0);
			rect.right += buttonWidthG;
			rect.left  += buttonWidthG;
			AddControl (pBox, CONTROL_BUTTON, rect, "Cancel", MBID_CANCEL, 0, 0);
			break;
		}
	}
	
	pBox->m_iconID = ICON_NULL;
	
	// Handle messages for this modal dialog window.
	while (HandleMessages(pBox))
	{
		if (pBox->m_data)
		{
			break;//we're done.
		}
		//hlt;
		KeTaskDone();
	}
	
	int dataReturned = (int)pBox->m_data;
	
	DestroyWindow(pBox);
	while (HandleMessages(pBox));
	
	if (pWindow)
	{
		pWindow->m_callback = pProc;
		pWindow->m_flags    = old_flags;
	}
	g_vbeData = pBackup;
	
	//NB: No null dereference, because if pWindow is null, wasSelectedBefore would be false anyway
	if (wasSelectedBefore)
	{
		pWindow->m_isSelected = true;
		PaintWindowBorderNoBackgroundOverpaint (pWindow);
	}
	
	// Re-acquire the locks that have been freed before.
	if (pWindow)
	{
		if (eqLock) ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	}
	if (wnLock) ACQUIRE_LOCK (g_windowLock);
	if (scLock) ACQUIRE_LOCK (g_screenLock);
	return dataReturned;
}

//TODO FIXME: Why does this freeze the OS when clicking on the main controlpanel window
//when I put this in kapp/cpanel.c??
void Cpl$WindowPopup(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags)
{
	// Free the locks that have been acquired.
	bool wnLock = g_windowLock, scLock = g_screenLock, eqLock = false;
	if  (wnLock) FREE_LOCK (g_windowLock);
	if  (scLock) FREE_LOCK (g_screenLock);
	
	bool wasSelectedBefore = false;
	if (pWindow)
	{
		eqLock = pWindow->m_eventQueueLock;
		if (eqLock) FREE_LOCK (pWindow->m_eventQueueLock);
	
		wasSelectedBefore = pWindow->m_isSelected;
		if (wasSelectedBefore)
		{
			pWindow->m_isSelected = false;
			PaintWindowBorderNoBackgroundOverpaint (pWindow);
		}
	}
	
	VBEData* pBackup = g_vbeData;
	
	VidSetVBEData(NULL);
	// Freeze the current window.
	int old_flags = 0;
	WindowProc pProc;
	if (pWindow)
	{
		pProc = pWindow->m_callback;
		old_flags = pWindow->m_flags;
		//pWindow->m_callback = MessageBoxWindowLightCallback;
		pWindow->m_flags |= WF_FROZEN;//Do not respond to user attempts to move/other
	}
	
	Window* pSubWindow = CreateWindow(newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags);
	if (pSubWindow)
	{
		while (HandleMessages(pSubWindow))
		{
			KeTaskDone();
		}
	}
	
	if (pWindow)
	{
		pWindow->m_callback = pProc;
		pWindow->m_flags    = old_flags;
	}
	g_vbeData = pBackup;
	
	//NB: No null dereference, because if pWindow is null, wasSelectedBefore would be false anyway
	if (wasSelectedBefore)
	{
		pWindow->m_isSelected = true;
		PaintWindowBorderNoBackgroundOverpaint (pWindow);
	}
	
	// Re-acquire the locks that have been freed before.
	if (pWindow)
	{
		if (eqLock) ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	}
	if (wnLock) ACQUIRE_LOCK (g_windowLock);
	if (scLock) ACQUIRE_LOCK (g_screenLock);
}

#endif

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
	
	while (y <= -1)
	{
		o += pWindow->m_vbeData.m_width;
		y++;
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
			//just memcpy shit
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
			offfb = j * pitch + x, offcp = j * width + x;
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

void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow)
{
	Rectangle recta = pWindow->m_rect;
	recta.right  -= recta.left; recta.left = 0;
	recta.bottom -= recta.top;  recta.top  = 0;
	
	Rectangle rectb = recta;
	
	if (!(pWindow->m_flags & WF_NOBORDER))
	{
		//! X adjusts the size of the dropshadow on the window.
		recta.right  -= WINDOW_RIGHT_SIDE_THICKNESS+1;
		recta.bottom -= WINDOW_RIGHT_SIDE_THICKNESS+1;
		
		rectb = recta;
		
		//VidFillRectangle(WINDOW_BACKGD_COLOR, recta);
		VidDrawRectangle(WINDOW_EDGE_COLOR, recta);
		
		for (int i = 0; i < WINDOW_RIGHT_SIDE_THICKNESS; i++) {
			//recta.left++;  recta.top++;
			recta.right++; recta.bottom++;
			VidDrawHLine(WINDOW_EDGE_COLOR, recta.left, recta.right, recta.bottom);
			VidDrawVLine(WINDOW_EDGE_COLOR, recta.top, recta.bottom, recta.right);
		}
		
		//draw a white border thing:
		rectb.left++;
		rectb.top ++;
		rectb.right--;
		rectb.bottom--;
		//VidDrawRectangle(WINDOW_TITLE_TEXT_COLOR, rectb);
		VidDrawHLine (WINDOW_TITLE_TEXT_COLOR,     rectb.left, rectb.right, rectb.top);
		VidDrawHLine (WINDOW_TITLE_INACTIVE_COLOR, rectb.left, rectb.right, rectb.bottom);
		VidDrawVLine (WINDOW_TITLE_TEXT_COLOR,     rectb.top, rectb.bottom, rectb.left);
		VidDrawVLine (WINDOW_TITLE_INACTIVE_COLOR, rectb.top, rectb.bottom, rectb.right);
	}
	/*
	rectc.left++;
	rectc.top++;
	rectc.right--;
	rectc.bottom--;
	VidDrawRectangle(pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR_B : WINDOW_TITLE_INACTIVE_COLOR_B, rectc);*/
	
	if (!(pWindow->m_flags & WF_NOTITLE))
	{
		Rectangle rectc = rectb;
		rectc.left++;
		rectc.top += TITLE_BAR_HEIGHT-2;
		rectc.right--;
		rectc.bottom--;
		
		int iconGap = 16 * (pWindow->m_iconID != ICON_NULL);
		
		VidDrawRectangle(pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR_B : WINDOW_TITLE_INACTIVE_COLOR_B, rectc);
		
		//draw the window title:
		rectb.left++;
		rectb.top ++;
		rectb.right--;
		rectb.bottom = rectb.top + TITLE_BAR_HEIGHT - 1;
		
		//todo: gradients?
		//VidFillRectangle(pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR : WINDOW_TITLE_INACTIVE_COLOR, rectb);
		VidFillRectHGradient(
			pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR   : WINDOW_TITLE_INACTIVE_COLOR, 
			pWindow->m_isSelected ? WINDOW_TITLE_ACTIVE_COLOR_B : WINDOW_TITLE_INACTIVE_COLOR_B, 
			rectb.left,
			rectb.top,
			rectb.right,
			rectb.bottom
		);
	
		int textwidth, __attribute__((unused)) height;
		VidTextOutInternal(pWindow->m_title, 0, 0, 0, 0, true, &textwidth, &height);
		
		int MinimizAndCloseGap = ((pWindow->m_flags & WF_NOMINIMZ) ? 0:16) + ((pWindow->m_flags & WF_NOCLOSE) ? 0:16);
		
		int offset = (rectb.right-rectb.left-iconGap*2-textwidth-MinimizAndCloseGap)/2;
	
		VidTextOut(pWindow->m_title, rectb.left + offset + 1 + iconGap, rectb.top + 2 + 3, FLAGS_TOO(TEXT_RENDER_BOLD, WINDOW_TITLE_TEXT_COLOR_SHADOW), TRANSPARENT);
		VidTextOut(pWindow->m_title, rectb.left + offset + 0 + iconGap, rectb.top + 1 + 3, FLAGS_TOO(TEXT_RENDER_BOLD, WINDOW_TITLE_TEXT_COLOR       ), TRANSPARENT);
		
		if (pWindow->m_iconID != ICON_NULL)
			RenderIconForceSize(pWindow->m_iconID, rectb.left+1, rectb.top+1, 16);
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
		case EVENT_PAINT:
		case EVENT_MOVECURSOR:
		case EVENT_CLICKCURSOR:
		case EVENT_RELEASECURSOR:
		case EVENT_KEYPRESS:
		case EVENT_KEYRAW:
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
	pWindow->m_callback(pWindow, eq, eqp1, eqp2);
	
	if (IsEventDestinedForControlsToo(eq))
		ControlProcessEvent(pWindow, eq, eqp1, eqp2);
	
	return eq * eqp1 * eqp2;
}

int someValue = 0;
bool HandleMessages(Window* pWindow)
{
	// grab the lock
	ACQUIRE_LOCK (pWindow->m_eventQueueLock);
	
	for (int i = 0; i < pWindow->m_eventQueueSize; i++)
	{
		//setup paint stuff so the window can only paint in their little box
		VidSetVBEData (&pWindow->m_vbeData);
		pWindow->m_vbeData.m_dirty = 0;
		pWindow->m_renderFinished = false;
		if (pWindow->m_eventQueue[i] == EVENT_MINIMIZE)
		{
			VidSetVBEData (NULL);
			HideWindow(pWindow);
			if (!pWindow->m_minimized)
			{
				pWindow->m_minimized   = true;
				pWindow->m_rectBackup  = pWindow->m_rect;
				
				pWindow->m_rect.left += (pWindow->m_rect.right  - pWindow->m_rect.left - 32) / 2;
				pWindow->m_rect.top  += (pWindow->m_rect.bottom - pWindow->m_rect.top  - 32) / 2;
				pWindow->m_rect.right  = pWindow->m_rect.left + 32;
				pWindow->m_rect.bottom = pWindow->m_rect.top  + 32;
			}
			pWindow->m_hidden = false;
			UpdateDepthBuffer();
			VidSetVBEData (&pWindow->m_vbeData);
		}
		else if (pWindow->m_eventQueue[i] == EVENT_UNMINIMIZE)
		{
			VidSetVBEData (NULL);
			HideWindow(pWindow);
			pWindow->m_minimized   = false;
			pWindow->m_rect = pWindow->m_rectBackup;
			pWindow->m_hidden = false;
			//pWindow->m_rect.right  = pWindow->m_rect.left + pWindow->m_vbeData.m_width;
			//pWindow->m_rect.bottom = pWindow->m_rect.top  + pWindow->m_vbeData.m_height;
			UpdateDepthBuffer();
			VidSetVBEData (&pWindow->m_vbeData);
			PaintWindowBackgroundAndBorder(pWindow);
			pWindow->m_eventQueue[pWindow->m_eventQueueSize++] = EVENT_PAINT;
			pWindow->m_renderFinished = true;
		}
		if (pWindow->m_eventQueue[i] == EVENT_CREATE)
		{
			PaintWindowBackgroundAndBorder(pWindow);
			DefaultWindowProc (pWindow, EVENT_CREATE, 0, 0);
		}
		if (pWindow->m_eventQueue[i] == EVENT_PAINT)
		{
			PaintWindowBorderNoBackgroundOverpaint(pWindow);
		}
		
		someValue = CallWindowCallbackAndControls(pWindow, pWindow->m_eventQueue[i], pWindow->m_eventQueueParm1[i], pWindow->m_eventQueueParm2[i]);
		
		//reset to main screen
		VidSetVBEData (NULL);
		if (!pWindow->m_minimized)
		{
			if (pWindow->m_vbeData.m_dirty)
				pWindow->m_renderFinished = true;
		}
		else
			pWindow->m_renderFinished = true;
		
		//if the contents of this window have been modified, redraw them:
		//if (pWindow->m_vbeData.m_dirty && !pWindow->m_hidden)
		//	RenderWindow(pWindow);
		
		if (pWindow->m_eventQueue[i] == EVENT_CREATE)
		{
			AddWindowToDrawOrder (pWindow - g_windows);
			ShowWindow(pWindow);
			SelectThisWindowAndUnselectOthers(pWindow);
		}
		if (pWindow->m_eventQueue[i] == EVENT_DESTROY)
		{
			pWindow->m_eventQueueSize = 0;
			
			FREE_LOCK (pWindow->m_eventQueueLock);
			KeTaskDone();
			
			ReadyToDestroyWindow(pWindow);
			
			return false;
		}
	}
	pWindow->m_eventQueueSize = 0;
	
	FREE_LOCK (pWindow->m_eventQueueLock);
	KeTaskDone();//hlt; //give it a good halt
	return true;
}
/*
void PostQuitMessage (Window* pWindow)
{
	VBEData* backup = g_vbeData;
	g_vbeData = &g_mainScreenVBEData;
	LogMsg("ReadyToDestroyWindow");
	ReadyToDestroyWindow(pWindow);
	
	#if THREADING_ENABLED
	LogMsg("KeExit");
	KeExit();
	LogMsg("KeExit done?");
	#endif
	g_vbeData = backup;
}
*/
void DefaultWindowProc (Window* pWindow, int messageType, UNUSED int parm1, UNUSED int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			// Add a default QUIT button control.
			
			if (!(pWindow->m_flags & WF_NOCLOSE))
			{
				Rectangle rect;
				rect.right = pWindow->m_vbeData.m_width - 4 - WINDOW_RIGHT_SIDE_THICKNESS;
				rect.left  = rect.right - TITLE_BAR_HEIGHT+2;
				rect.top   = 4;
				rect.bottom= rect.top + TITLE_BAR_HEIGHT-4;
				AddControl (pWindow, CONTROL_BUTTON_EVENT, rect, "\x09", 0xFFFF0000, EVENT_CLOSE, 0);
				
				if (!(pWindow->m_flags & WF_NOMINIMZ))
				{
					rect.left -= TITLE_BAR_HEIGHT;
					rect.right -= TITLE_BAR_HEIGHT;
					AddControl (pWindow, CONTROL_BUTTON_EVENT, rect, "\x07", 0xFFFF0000, EVENT_MINIMIZE, 0);
				}
			}
			
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
			//ReadyToDestroyWindow(pWindow);//exits
			break;
		default:
			break;
	}
}
#endif
