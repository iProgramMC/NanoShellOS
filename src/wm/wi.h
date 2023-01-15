/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

      Window Manager internal header
******************************************/
#ifndef INT_WI_H
#define INT_WI_H

#include <window.h>
#include <widget.h>
#include <wmenu.h>
#include <misc.h>
#include <icon.h>
#include <print.h>
#include <task.h>
#include <widget.h>
#include <keyboard.h>
#include <wbuiltin.h>
#include <wcall.h>
#include <vfs.h>
#include <image.h>
#include <config.h>

#define SAFE_DELETE(x) do { if (x) { MmFree(x); x = NULL; } } while (0)

#define WINDOW_ACTION_MENU_ORIG_CID (0x12345678)

enum
{
	CID_RESTORE = 1,
	CID_MINIMIZE,
	CID_MAXIMIZE,
	CID_SPACER,
	CID_CLOSE,
};

typedef struct Tooltip
{
	bool m_shown;
	char m_text[512];
	Rectangle m_rect;
}
Tooltip;

extern Cursor   g_windowDragCursor;
extern Cursor*  g_currentCursor, g_defaultCursor, g_waitCursor, g_iBeamCursor, g_crossCursor, g_pencilCursor;
extern Window   g_windows [WINDOWS_MAX];
extern Window*  g_focusedOnWindow;
extern Window*  g_pShutdownMessage;
extern Window*  g_currentlyClickedWindow;
extern Tooltip  g_tooltip;
extern VBEData* g_vbeData, g_mainScreenVBEData;
extern SafeLock g_WindowLock, g_ScreenLock, g_BufferLock, g_CreateLock, g_BackgdLock; 
extern int      g_TaskbarHeight;
extern int      g_mouseX, g_mouseY;
extern bool     g_RenderWindowContents;
extern bool     g_GlowOnHover, g_heldAlt;
extern short    g_windowDrawOrder[WINDOWS_MAX];
extern short*   g_windowDepthBuffer;
extern uint32_t* g_framebufferCopy;
extern int      g_windowDrawOrderSize;

extern ClickInfo g_clickQueue [CLICK_INFO_MAX];
extern int       g_clickQueueSize;
extern SafeLock  g_ClickQueueLock;
extern bool
g_shutdownSentDestroySignals,
g_shutdownWaiting,
g_shutdownRequest,
g_shutdownWantReb,
g_shutdownSentCloseSignals,
g_shutdownProcessing,
g_shutdownDoneAll;

extern Cursor* const g_CursorLUT[];

void* WmCAllocate(size_t sz);
void KeTaskDone(void);
WindowAction* ActionQueueAdd(WindowAction action);
WindowAction* ActionQueueGetFront(void);
SafeLock* ActionQueueGetSafeLock(void);
Window* GetWindowFromIndex(int i);
Cursor* GetCursorBasedOnID(int m_cursorID, Window *pWindow);
Window* ShootRayAndGetWindow (int x, int y);
bool IsWindowManagerTask();
bool ActionQueueWouldOverflow(void);
void ActionQueuePop(void);
void ActionQueueWaitForFrontToFinish(void);
bool ActionQueueEmpty(void);
void RunOneEffectFrame(void);
void CreateMovingRectangleEffect(Rectangle src, Rectangle dest, const char* text);
void LoadDefaultThemingParms(void);
void RedrawBackground (Rectangle rect);
void SetDefaultBackground(void);
void VidBlitImageForceOpaque(Image* pImage, int x, int y);
void RefreshRectangle(Rectangle rect, Window* pWindowToExclude);
//void UpdateDepthBuffer(void);
void RemoveWindowFromDrawOrder(int windowIndex);
void MovePreExistingWindowToFront(short windowIndex);
void ResetWindowDrawOrder();
//void InitWindowDepthBuffer(void);
//void KillWindowDepthBuffer(void);
void AddWindowToDrawOrder(short windowIndex);
void WindowAddEventToMasterQueue(PWINDOW pWindow, int eventType, int parm1, int parm2);
bool WindowPopEventFromQueue(PWINDOW pWindow, int *eventType, int *parm1, int *parm2);
void NukeWindow (Window* pWindow);
void ShutdownProcessing(int parameter);
void PaintWindowBorderNoBackgroundOverpaint(Window* pWindow);
void HideWindow(Window* pWindow);
void ShowWindow(Window* pWindow);
void SelectWindow(Window* pWindow);
void RenderWindow(Window* pWindow);
void PaintWindowBackgroundAndBorder(Window* pWindow);
void PaintWindowBorder(Window* pWindow);
void UpdateControlsBasedOnAnchoringModes(Window* pWindow, int oldSizeParm, int newSizeParm);
void ResizeWindow(Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight);
int  CallWindowCallback(Window* pWindow, int eq, int eqp1, int eqp2);
int  CallWindowCallbackAndControls(Window* pWindow, int eq, int eqp1, int eqp2);
void WinAddToInputQueue(Window* this, char input);
bool WinAnythingOnInputQueue(Window* this);
char WinReadFromInputQueue(Window* this);
void OnUILeftClick (int mouseX, int mouseY);
void OnUILeftClickDrag (int mouseX, int mouseY);
void OnUILeftClickRelease (int mouseX, int mouseY);
void OnUIRightClick (int mouseX, int mouseY);
void OnUIRightClickRelease (int mouseX, int mouseY);
void WindowManagerOnShutdown(void);
void ResizeWindowInternal (Window* pWindow, int newPosX, int newPosY, int newWidth, int newHeight);
void SetFocusedConsole(Console* console);
void RequestTaskbarUpdate();
void SetCursorInternal(Cursor* pCursor, bool bUndrawOldCursor);
void OnRightClickShowMenu(Window * pWindow, int parm1);
void WmCreateRectangleStack();
void WmFreeRectangleStack();
int  WmAddRectangleToStack(Rectangle* rect);
void WmSplitRectangleStackByWindow(Window* pWindow);
void WmSplitRectangle(Rectangle ogRect, const Window* pExcept, Rectangle** pStartOut, Rectangle** pEndOut);

#endif//INT_WI_H