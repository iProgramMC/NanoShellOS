//  ***************************************************************
//  crtlib.h - Creation date: 21/04/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _CRTLIB_H
#define _CRTLIB_H

#include "../include/nsstructs.h"

#define WCALL_VERSION 26
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
		WIN_DESTROY,
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
		
		//V1.21
		VID_BLIT_IMAGE_RESIZE,
		TM_SLEEP,
		
		//V1.22
		WIN_SET_ICON,
		
		//V1.23
		NS_GET_VERSION,
		
		//V1.24
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
		WIN_FILE_CHOOSE_BOX,//TODO
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
		CB_GET_CURRENT_VARIANT,//danger!!
		CB_RELEASE,
	
	// System Calls V1.5
		VID_RENDER_ICON,
		VID_RENDER_ICON_OUTLINE,
		VID_RENDER_ICON_SIZE,
		VID_RENDER_ICON_SIZE_OUTLINE,
		TM_GET_RANDOM,
	
	// System Calls V1.6
		MM_REALLOCATE_D,
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
};

__attribute__((noreturn))
void   abort      ();

int    memcmp     (const void* ap, const void* bp, size_t size);
void*  memcpy     (void* restrict dstptr, const void* restrict srcptr, size_t size);
void*  memmove    (void* restrict dstptr, const void* restrict srcptr, size_t size);
void*  memset     (void* bufptr, int val, size_t size);
size_t strlen     (const char* str);
void*  strcpy     (const char* ds, const char* ss);
char*  strncpy    (char *dst, const char *src, size_t n);
int    strcmp     (const char* as, const char* bs);
char*  strcat     (char* dest, const char* after);
void   strtolower (char* as);
void   strtoupper (char* as);
void   memtolower (char* as, int w);
void   memtoupper (char* as, int w);
size_t strgetlento(const char* str, char chr);
int    atoi       (const char* str);
char*  Tokenize   (TokenState* pState, char* pString, char* separator);

void*  malloc     (size_t size);
void*  calloc     (size_t nmemb, size_t size);
void   free       (void*  ptr);

void   exit       (int num);

void LogMsg     ( const char *pfmt, ... );
void LogMsgNoCr ( const char *pfmt, ... );

int  SetErrorNumber(int errno);
int  GetErrorNumber();
int* GetErrorNumberPointer();

int sprintf(char* buf, const char* fmt, ...);

void OnAssertionFail(const char *cond_msg, const char *file, int line);
#define assert(cond) do { if (!(cond)) OnAssertionFail(#cond, __FILE__, __LINE__); } while (0)
#define ASSERT assert

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#endif//_CRTLIB_H