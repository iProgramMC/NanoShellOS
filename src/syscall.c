/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

     Window Manager Call Array module
******************************************/

#include <window.h>
#include <wbuiltin.h>
#include <wmenu.h>
#include <widget.h>
#include <video.h>
#include <print.h>
#include <console.h>
#include <memory.h>
#include <cinterp.h>
#include <keyboard.h>
#include <vfs.h>
#include <clip.h>
#include <misc.h>
#include <time.h>
#include <config.h>
#include <idt.h>
#include <process.h>
#include <resource.h>

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
 * are passed on the stack.
*****************************************************/

extern VBEData * g_vbeData;
extern Console* g_currentConsole;

// Miscellaneous utils
void UserRequestRepaint(Window* pWindow)
{
	RequestRepaint(pWindow);
}

void UserRequestRepaintNew(Window* pWindow)
{
	RequestRepaintNew(pWindow);
}

void * UserMmAllocateDebug(size_t sz, UNUSED const char* a, UNUSED int b)
{
	return MmAllocate(sz);
}

void * UserMmReAllocateDebug(void* ptr, size_t sz, UNUSED const char* a, UNUSED int b)
{
	return MmReAllocate(ptr, sz);
}

int GetVersionNumber()
{
	return VersionNumber;
}

void VidSetClipRectP (Rectangle rect)
{
	// A safer version that's exposed to user space
	if (rect.left < 0 || rect.top < 0 || rect.right < 0 || rect.bottom < 0)
		VidSetClipRect(NULL);
	else
		VidSetClipRect(&rect);
}

void LogString(const char* pText)
{
	LogMsgNoCr("%s", pText);
}

int FiUnlinkFile2(UNUSED const char* pText)
{
	// TODO
	return  -EXDEV;
}

const char * GetWindowTitle(Window* pWindow)
{
	return pWindow->m_title;
}

void* GetWindowData(Window* pWindow)
{
	return pWindow->m_data;
}

void SetWindowData(Window* pWindow, void* pData)
{
	pWindow->m_data = pData;
}

void GetWindowRect(Window* pWindow, Rectangle * pRectOut)
{
	*pRectOut = pWindow->m_rect;
}

void ShellExecuteCommand(char* p, bool* pbExit); // shell.c

int ShellExecute(const char *pCommand)
{
	char buffer[256];
	
	size_t len = strlen(pCommand);
	
	if (len > 255)
		return -ENAMETOOLONG;
	
	UNUSED bool bExit = false;
	
	strcpy(buffer, pCommand);
	ShellExecuteCommand(buffer, &bExit);
	
	return 0;
}

int ShellExecuteResource(const char *pResource)
{
	RESOURCE_STATUS status = LaunchFileOrResource(pResource);
	return status;
}

// System call interface
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
		
		MM_ALLOCATE_D, //legacy
		MM_FREE,       //legacy
		MM_DEBUG_DUMP, //legacy
		
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
	
	// System Calls V1.3
		WIN_ADD_CONTROL_EX,
		WIN_TEXT_INPUT_QUERY_DIRTY_FLAG,
		WIN_TEXT_INPUT_CLEAR_DIRTY_FLAG,
		WIN_TEXT_INPUT_GET_RAW_TEXT,
		WIN_CHECKBOX_GET_CHECKED,
		WIN_CHECKBOX_SET_CHECKED,
		
		CC_RUN_C_CODE,
		
		FI_REMOVE_FILE,
		
		WIN_REQUEST_REPAINT_NEW,
		WIN_SHELL_ABOUT,
		WIN_INPUT_BOX,
		WIN_COLOR_BOX,
		WIN_FILE_CHOOSE_BOX,
		WIN_POPUP_WINDOW,
		
	// System Calls V1.4
		VID_SET_VBE_DATA,//dangerous!! be careful!!
		
		FI_OPEN_DIR_D,
		FI_CLOSE_DIR,
		FI_READ_DIR_LEGACY,
		FI_SEEK_DIR,
		FI_REWIND_DIR,
		FI_TELL_DIR,
		FI_STAT_AT,
		FI_STAT,
		FI_GET_CWD,
		FI_CHANGE_DIR,
		
		WIN_GET_WIDGET_EVENT_HANDLER,
		WIN_SET_WIDGET_EVENT_HANDLER,
		WIN_SET_IMAGE_CTL_MODE,
		WIN_SET_IMAGE_CTL_COLOR,
		WIN_SET_IMAGE_CTL_IMAGE,
		WIN_GET_IMAGE_CTL_IMAGE,
		WIN_IMAGE_CTL_ZOOM_TO_FILL,
		WIN_SET_FOCUSED_CONTROL,
		WIN_POPUP_WINDOW_EX,
		
		ERR_GET_STRING,
		
		VID_GET_MOUSE_POS,
		VID_SET_CLIP_RECT,
		
		CB_CLEAR,
		CB_COPY_TEXT,
		CB_COPY_BLOB,
		CB_GET_CURRENT_VARIANT,
		CB_RELEASE,
	
	// System Calls V1.5
		VID_RENDER_ICON,
		VID_RENDER_ICON_OUTLINE,
		VID_RENDER_ICON_SIZE,
		VID_RENDER_ICON_SIZE_OUTLINE,
		TM_GET_RANDOM,
		
	// System Calls V1.6
		MM_REALLOCATE_D, //legacy
		SH_EXECUTE,
		SH_EXECUTE_RESOURCE,
		
	// System Calls V1.7
		MM_MAP_MEMORY_USER,
		MM_UNMAP_MEMORY_USER,
		
	// System Calls V1.8
		FI_RENAME,
		FI_MAKE_DIR,
		FI_REMOVE_DIR,
		FI_CREATE_PIPE,
		FI_IO_CONTROL,
		WIN_CALL_CTL_CALLBACK,
		WIN_TEXT_INPUT_SET_MODE,
		
	// System Calls V1.9
		WIN_GET_SCROLL_BAR_MIN,
		WIN_GET_SCROLL_BAR_MAX,
		WIN_GET_SEL_INDEX_LIST,
		WIN_SET_SEL_INDEX_LIST,
		WIN_GET_SEL_INDEX_TABLE,
		WIN_SET_SEL_INDEX_TABLE,
		WIN_GET_SCROLL_TABLE,
		WIN_SET_SCROLL_TABLE,
		WIN_ADD_TABLE_ROW,
		WIN_ADD_TABLE_COLUMN,
		WIN_GET_ROW_STRINGS_FROM_TABLE,
		WIN_REMOVE_ROW_FROM_TABLE,
		WIN_RESET_TABLE,
		
	// System Calls V2.0
		VID_READ_PIXEL,
		CFG_GET_STRING,
		VID_GET_VBE_DATA,
		
	// System Calls V2.1
		WIN_GET_WINDOW_TITLE,
		WIN_GET_WINDOW_DATA,
		WIN_SET_WINDOW_DATA,
		WIN_GET_WINDOW_RECT,
		WIN_CALL_CALLBACK_AND_CTLS,
		WIN_CHANGE_CURSOR,
		WIN_SET_FLAGS,
		WIN_GET_FLAGS,
		
	// System Calls V2.2
		VID_SET_MOUSE_POS,
		WIN_ADD_TIMER,
		WIN_DISARM_TIMER,
		WIN_CHANGE_TIMER,
		
	// System Calls V2.3
		WIN_UPLOAD_CURSOR,
		WIN_RELEASE_CURSOR,
		WIN_SET_LIST_ITEM_TEXT,
		WIN_GET_ICON_IMAGE,
		RST_LOOK_UP_RESOURCE,
		WIN_SET_CONTROL_DISABLED,
		WIN_SET_CONTROL_FOCUSED,
		WIN_SET_CONTROL_VISIBLE,
		WIN_PROG_BAR_SET_PROGRESS,
		WIN_PROG_BAR_SET_MAX_PROG,
		
	// System Calls V2.4
		WIN_COMBO_BOX_ADD_ITEM,
		WIN_COMBO_BOX_GET_SELECTED_ITEM,
		WIN_COMBO_BOX_SET_SELECTED_ITEM,
		WIN_COMBO_BOX_CLEAR_ITEMS,
		WIN_IS_CONTROL_FOCUSED,
		WIN_IS_CONTROL_DISABLED,
		WIN_TEXT_INPUT_SET_FONT,
		WIN_TEXT_INPUT_REQUEST_COMMAND,
		
	// System Calls V2.5
		WIN_DRAW_EDGE,
		WIN_DRAW_ARROW,
		WIN_TAB_VIEW_ADD_TAB,
		WIN_TAB_VIEW_REMOVE_TAB,
		WIN_TAB_VIEW_CLEAR_TABS,
		WIN_SPAWN_MENU,
		KB_GET_KEY_STATE,
		LCK_ACQUIRE,
		LCK_FREE,
		
	// System Calls V2.6
		FI_READ_DIR,
		FI_STAT_LINK,
		
	// System Calls V2.7
		FI_STAT_FD,
		FI_CHANGE_MODE,
		FI_CHANGE_TIME,
		FI_CHANGE_MODE_FD,
		FI_CHANGE_TIME_FD,
		FI_CHANGE_DIR_FD,
		
	// System Calls V2.8
		VID_SET_CLIP_RECT_EX,
		VID_TEXT_OUT_INT_EX,
		VID_WRAP_TEXT,
		VID_GET_CHAR_WIDTH,
		VID_GET_LINE_HEIGHT,
		
		SYSTEM_CALL_COUNT,
};

const void *WindowCall[] = {
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
		UserRequestRepaint,
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
		
		UserMmAllocateDebug,
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
		WindowAddEventToMasterQueue,
		WindowAddEventToMasterQueue,
		
		GetTickCount,
		TmReadTime,
		
		GetCPUType,
		GetCPUName,
		
		GetCurrentConsole,
		
	// System Calls V1.21- 31/01/2022
		VidBlitImageResize,
		
		WaitMS,
	
	// System Calls V1.22- 07/02/2022
		SetIcon,
	
	// System Calls V1.23- 10/02/2022
		GetVersionNumber,
		
	// System Calls V1.24- 11/03/2022
		GetThemingParameter,
		SetThemingParameter,
		
	// System Calls V1.3 - 11/03/2022
		AddControlEx,
		TextInputQueryDirtyFlag,
		TextInputClearDirtyFlag,
		TextInputGetRawText,
		CheckboxGetChecked,
		CheckboxSetChecked,
		CcRunCCode,
		FiUnlinkFile2,
		UserRequestRepaintNew,
		ShellAbout,
		InputBox,
		ColorInputBox,
		FilePickerBox,
		PopupWindow,
		
	// System Calls V1.4 - 20/04/2022
		VidSetVBEData,
		FiOpenDirD,
		FiCloseDir,
		FiReadDirLegacy,
		FiSeekDir,
		FiRewindDir,
		FiTellDir,
		FiStatAt,
		FiStat,
		FiGetCwd,
		FiChangeDir,
		GetWidgetOnEventFunction,
		SetWidgetEventHandler,
		SetImageCtlMode,
		SetImageCtlColor,
		SetImageCtlCurrentImage,
		GetImageCtlCurrentImage,
		ImageCtlZoomToFill,
		SetFocusedControl,
		PopupWindowEx,
		GetErrNoString,
		GetMousePos,
		VidSetClipRectP,
		CbClear,
		CbCopyText,
		CbCopyBlob,
		CbGetCurrentVariant,
		CbRelease,
		
	// System Calls V1.5 - 21/05/2022
		RenderIcon,
		RenderIconOutline,
		RenderIconForceSize,
		RenderIconForceSizeOutline,
		GetRandom,
		
	// System Calls V1.6 - 14/07/2022
		UserMmReAllocateDebug,
		ShellExecute,
		ShellExecuteResource,
		
	// System Calls V1.7 - 31/08/2022
		MmMapMemoryUser,
		MmUnMapMemoryUser,
		
	// System Calls V1.8 - 18/12/2022
		FiRename,
		FiMakeDir,
		FiRemoveDir,
		FiCreatePipe,
		FiIoControl,
		CallControlCallback,
		TextInputSetMode,
		
	// System Calls V1.9 - 28/12/2022
		GetScrollBarMin,
		GetScrollBarMax,
		GetSelectedIndexList,
		SetSelectedIndexList,
		GetSelectedIndexTable,
		SetSelectedIndexTable,
		GetScrollTable,
		SetScrollTable,
		AddTableRow,
		AddTableColumn,
		GetRowStringsFromTable,
		RemoveRowFromTable,
		ResetTable,
		
	// System Calls V2.0 - 31/12/2022
		VidReadPixel,
		CfgGetEntryValue,
		VidGetVBEData,
		
	// System Calls V2.1 - 21/01/2023
		GetWindowTitle,
		GetWindowData,
		SetWindowData,
		GetWindowRect,
		CallWindowCallbackAndControls,
		ChangeCursor,
		SetWindowFlags,
		GetWindowFlags,
		
	// System Calls V2.2
		SetMousePos,
		AddTimer,
		DisarmTimer,
		ChangeTimer,
		
	// System Calls V2.3 - 14/02/2023
		UploadCursor,
		ReleaseCursor,
		SetListItemText,
		GetIconImage,
		ExLookUpResource,
		SetDisabledControl,
		SetFocusedControl,
		SetControlVisibility,
		ProgBarSetProgress,
		ProgBarSetMaxProg,
		
	// System Calls V2.4 - 14/05/2023
		ComboBoxAddItem,
		ComboBoxGetSelectedItemID,
		ComboBoxSetSelectedItemID,
		ComboBoxClearItems,
		IsControlFocused,
		IsControlDisabled,
		TextInputSetFont,
		TextInputRequestCommand,
		
	// System Calls V2.5 - 24/05/2023
		DrawEdge,
		DrawArrow,
		TabViewAddTab,
		TabViewRemoveTab,
		TabViewClearTabs,
		SpawnMenu,
		KbGetKeyState,
		LockAcquire,
		LockFree,
		
	// System Calls V2.6 - 05/06/2023
		FiReadDir,
		FiLinkStat,
		
	// System Calls V2.7 - 10/06/2023
		FiFileDesStat,
		FiChangeMode,
		FiChangeTime,
		FiFileDesChangeMode,
		FiFileDesChangeTime,
		FiFileDesChangeDir,
		
	// System Calls V2.8 - 12/06/2023
		VidSetClipRectEx,
		VidTextOutInternalEx,
		WrapText,
		GetCharWidth,
		GetLineHeight,
};

STATIC_ASSERT(ARRAY_COUNT(WindowCall) == SYSTEM_CALL_COUNT, "These should be the same size!");

// Other utils
void UserCallStuffNotSupportedC(void)
{
	if (!VidIsAvailable())
		LogMsg("This program requires a VBE graphical display to run.");
	else
		LogMsg("This program requires NanoShell Window Manager to run.");
}

extern char UserCallStuff[], UserCallStuffEnd[];
extern char UserCallStuffNotSupported[], UserCallStuffNotSupportedEnd[];
void WindowCallInit()
{
	memcpy ((void*)0xC0007C00, UserCallStuff, UserCallStuffEnd - UserCallStuff);
}
void WindowCallDeinitialize()
{
	memcpy ((void*)0xC0007C00, UserCallStuffNotSupported, UserCallStuffNotSupportedEnd - UserCallStuffNotSupported);
}

// This function gets called when an interrupt 0x80 is received. Such types of interrupts aren't actually supported.
void OnSyscallReceived(UNUSED Registers* regs)
{
	SLogMsg("WARNING: This system call protocol is unsupported for now.");
}

