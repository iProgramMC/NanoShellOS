/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

     Window Manager Call Array module
******************************************/

#include <window.h>
#include <widget.h>
#include <video.h>
#include <print.h>
#include <console.h>
#include <memory.h>
#include <vfs.h>
#include <misc.h>
#include <idt.h>

/*****************************************************
 * These calls are different from the Syscalls,
 * in that they aren't called using `int 0x7F`
 * syntax.  Instead they're called by calling
 * into a function at 0xC0007C00 (phys 0x7C00).
 *
 * Note that 0x7C00 is used here, because it's
 * pretty out-of-the-way of pretty much
 * everything else in the kernel.
 *
 * System Calls prevent interrupts from executing
 * while they execute. Since our Window Manager is 
 * multithreaded based (actually overly so),
 * and abuses locks heavily, we HAVE to not block
 * task switches.
 *
 * This is why we use this crappy method instead.
 *
 * The method works like this:
 *  (call the user wrapper function)
 *    (move the call type to 0x7CFC)
 *    (call 0x7C00 as if it were a pointer to a 
 *     function of the same type as our user wrapper)
 *  (done)
 *
 * If (or when, rather) I switch away from this method
 * (and use the int instruction), I'll leave 
 * 0xc0007000 mapped for user programs, and I'll
 * shove in a stub which calls that, for backwards
 * compatibility.
 *
 * Of course this will be difficult since arguments
 * are passed on the stack, so I just want to punch
 * my old self for going this way. :^)
*****************************************************/

enum
{
	// System Calls V1.0
		CALL_NOTHING,
		//Video Driver calls:
		VID_GET_SCREEN_WIDTH,
		VID_GET_SCREEN_HEIGHT,
		VID_PLOT_PIXEL,
		VID_FILL_SCREEN, //Actually fills the context, not the screen
		VID_DRAW_H_LINE,
		VID_DRAW_V_LINE,
		VID_DRAW_LINE,
		VID_SET_FONT,
		VID_PLOT_CHAR,
		VID_BLIT_IMAGE,
		VID_TEXT_OUT,
		VID_TEXT_OUT_INT,
		VID_DRAW_TEXT,
		VID_SHIFT_SCREEN,
		VID_FILL_RECT,
		VID_DRAW_RECT,
		VID_FILL_RECT_H_GRADIENT,
		VID_FILL_RECT_V_GRADIENT,
		
		//Window Manager calls:
		WIN_CREATE,
		WIN_HANDLE_MESSAGES,
		WIN_DEFAULT_PROC,
		WIN_DESTROY,//don't use
		WIN_MESSAGE_BOX,
		WIN_ADD_CONTROL,
	
	// System Calls V1.1
		WIN_REQUEST_REPAINT,
		WIN_SET_LABEL_TEXT,
		WIN_ADD_MENUBAR_ITEM,
		WIN_SET_SCROLL_BAR_MIN,
		WIN_SET_SCROLL_BAR_MAX,
		WIN_SET_SCROLL_BAR_POS,
		WIN_GET_SCROLL_BAR_POS,
		WIN_ADD_ELEM_TO_LIST,
		WIN_REM_ELEM_FROM_LIST,
		WIN_GET_ELEM_STR_FROM_LIST,
		WIN_CLEAR_LIST,
		
		CON_PUTSTRING,
		CON_READCHAR,
		CON_READSTR,
		
		MM_ALLOCATE_D,
		MM_FREE,
		MM_DEBUG_DUMP,
		
		FI_OPEN_D,
		FI_CLOSE,
		FI_READ,
		FI_WRITE,
		FI_TELL,
		FI_TELLSIZE,
		FI_SEEK,
	
	// System Calls V1.2
		WIN_SET_HUGE_LABEL_TEXT,
		WIN_SET_INPUT_TEXT_TEXT,
		WIN_SET_WINDOW_ICON,
		WIN_SET_WINDOW_TITLE,
		WIN_REGISTER_EVENT,
		WIN_REGISTER_EVENT2,
		
		TM_GET_TICK_COUNT,
		TM_GET_TIME,
		
		CPU_GET_TYPE,
		CPU_GET_NAME,
		
		CON_GET_CURRENT_CONSOLE,
		
	// System Calls V1.21
		VID_BLIT_IMAGE_RESIZE,
		
		TM_SLEEP,
		
	// System Calls V1.22
		WIN_SET_ICON,
		
	// System Calls V1.23
		NS_GET_VERSION,
		
	// System Calls V1.24
		WIN_GET_THEME_PARM,
		WIN_SET_THEME_PARM,
};

void LogString(const char* pText)
{
	LogMsgNoCr("%s", pText);
}

void SetWindowIcon (Window* pWindow, int icon)
{
	pWindow->m_iconID = icon;
}
extern VBEData * g_vbeData;
void SetWindowTitle(Window* pWindow, const char* pTitle)
{
	int len = strlen (pTitle);
	if (len < WINDOW_TITLE_MAX-1)
		len = WINDOW_TITLE_MAX-1;
	
	memcpy (pWindow->m_title, pTitle, len);
	pWindow->m_title[len] = 0;
	
	VBEData * backup = g_vbeData;
	VidSetVBEData (&pWindow->m_vbeData);
	RequestRepaintNew(pWindow);
	g_vbeData = backup;
}
int GetVersionNumber()
{
	return VersionNumber;
}
extern Console* g_currentConsole;
Console* GetCurrentConsole()
{
	return g_currentConsole;
}

void *WindowCall[] = {
	// System Calls V1.0 -- 14/01/2022
		NULL,
		//Video Driver calls:
		GetScreenSizeX,
		GetScreenSizeY,
		VidPlotPixel,
		VidFillScreen,
		VidDrawHLine,
		VidDrawVLine,
		VidDrawLine,
		VidSetFont,
		VidPlotChar,
		VidBlitImage,
		VidTextOut,
		VidTextOutInternal,
		VidDrawText,
		VidShiftScreen,
		VidFillRect,
		VidDrawRect,
		VidFillRectHGradient,
		VidFillRectVGradient,
		
		//Window Manager calls:
		CreateWindow,
		HandleMessages,
		DefaultWindowProc,
		DestroyWindow,
		MessageBox,
		AddControl,
	
	// System Calls V1.1 -- 25/01/2022
		RequestRepaint,
		SetLabelText,
		AddMenuBarItem,
		SetScrollBarMin,
		SetScrollBarMax,
		SetScrollBarPos,
		GetScrollBarPos,
		AddElementToList,
		RemoveElementFromList,
		GetElementStringFromList,
		ResetList,
		
		LogString,
		CoGetChar,
		CoGetString,
		
		MmAllocateD,
		MmFree,
		MmDebugDump,
		
		FiOpenD,
		FiClose,
		FiRead,
		FiWrite,
		FiTell,
		FiTellSize,
		FiSeek,
		
	// System Calls V1.2 - 29/01/2022
		SetHugeLabelText,
		SetTextInputText,
		SetWindowIcon,
		SetWindowTitle,
		WindowRegisterEvent,
		WindowRegisterEventUnsafe,
		
		GetTickCount,
		TmReadTime,
		
		GetCPUType,
		GetCPUName,
		
		GetCurrentConsole,
		
	// System Calls V1.21- 31/01/2022
		VidBlitImageResize,
		
		WaitMS,
	
	// System Calls V1.22- 07/01/2022
		SetIcon,
	
	// System Calls V1.23- 10/01/2022
		GetVersionNumber,
		
	// System Calls V1.24- 11/03/2022
		GetThemingParameter,
		SetThemingParameter,
};

void UserCallStuffNotSupportedC(void)
{
	if (!VidIsAvailable())
		LogMsg("This program requires a VBE graphical display to run.");
	else
		LogMsg("This program requires NanoShell Window Manager to run.");
}

extern char UserCallStuff[], UserCallStuffEnd[];
extern char UserCallStuffNotSupported[], UserCallStuffNotSupportedEnd[];
void WindowCallInitialize()
{
	memcpy ((void*)0xC0007C00, UserCallStuff, UserCallStuffEnd - UserCallStuff);
}
void WindowCallDeinitialize()
{
	memcpy ((void*)0xC0007C00, UserCallStuffNotSupported, UserCallStuffNotSupportedEnd - UserCallStuffNotSupported);
}

