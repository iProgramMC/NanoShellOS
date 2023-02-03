// nanoshell/types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _NANOSHELL_TYPES__H
#define _NANOSHELL_TYPES__H

// Basic includes everyone should have
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// Include other type files.
#include <nanoshell/stdio_types.h>
#include <nanoshell/stdlib_types.h>
#include <nanoshell/error_nums.h>
#include <nanoshell/setjmp_types.h>
#include <nanoshell/time_types.h>
#include <nanoshell/unistd_types.h>
#include <nanoshell/mman_types.h>
#include <nanoshell/graphics_types.h>
#include <nanoshell/keyboard.h>
#include <sys/types.h>

#define false 0
#define true 1

#define ARRAY_COUNT(array) (sizeof(array)/sizeof(*array))

#define UNUSED __attribute__((unused))

// Defines
#define WIN_KB_BUF_SIZE  512

//#define TITLE_BAR_HEIGHT 18

#define BACKGROUND_COLOR               	(GetThemingParameter(P_BACKGROUND_COLOR              ))
#define BUTTON_MIDDLE_COLOR             (GetThemingParameter(P_BUTTON_MIDDLE_COLOR           ))
#define WINDOW_BACKGD_COLOR             (GetThemingParameter(P_WINDOW_BACKGD_COLOR           ))
#define WINDOW_EDGE_COLOR               (GetThemingParameter(P_WINDOW_EDGE_COLOR             ))
#define WINDOW_TITLE_ACTIVE_COLOR       (GetThemingParameter(P_WINDOW_TITLE_ACTIVE_COLOR     ))
#define WINDOW_TITLE_INACTIVE_COLOR     (GetThemingParameter(P_WINDOW_TITLE_INACTIVE_COLOR   ))
#define WINDOW_TITLE_ACTIVE_COLOR_B     (GetThemingParameter(P_WINDOW_TITLE_ACTIVE_COLOR_B   ))
#define WINDOW_TITLE_INACTIVE_COLOR_B   (GetThemingParameter(P_WINDOW_TITLE_INACTIVE_COLOR_B ))
#define WINDOW_TITLE_TEXT_COLOR_SHADOW  (GetThemingParameter(P_WINDOW_TITLE_TEXT_COLOR_SHADOW))
#define WINDOW_TITLE_TEXT_COLOR         (GetThemingParameter(P_WINDOW_TITLE_TEXT_COLOR       ))
#define WINDOW_TEXT_COLOR               (GetThemingParameter(P_WINDOW_TEXT_COLOR             ))
#define WINDOW_TEXT_COLOR_LIGHT         (GetThemingParameter(P_WINDOW_TEXT_COLOR_LIGHT       ))
#define SYSTEM_FONT                     ((int)GetThemingParameter(P_SYSTEM_FONT              ))
#define TITLE_BAR_HEIGHT                ((int)GetThemingParameter(P_TITLE_BAR_HEIGHT         ))
#define TITLE_BAR_FONT                  ((int)GetThemingParameter(P_TITLE_BAR_FONT           ))
#define SELECTED_ITEM_COLOR             (GetThemingParameter(P_SELECTED_ITEM_COLOR           ))
#define SELECTED_ITEM_COLOR_B           (GetThemingParameter(P_SELECTED_ITEM_COLOR_B         ))

// Mark your system callbacks with this anyway!!!
#define CALLBACK

#define MAKE_MOUSE_PARM(x, y) ((x)<<16|(y))
#define GET_X_PARM(parm1)  (parm1>>16)
#define GET_Y_PARM(parm1)  (parm1&0xFFFF)

#define TEXTEDIT_MULTILINE (1)
#define TEXTEDIT_LINENUMS  (2)
#define TEXTEDIT_READONLY  (4)
#define TEXTEDIT_STYLING   (8)
#define TEXTEDIT_SYNTHILT  (16)

#define IMAGECTL_PAN  (1)
#define IMAGECTL_ZOOM (2)
#define IMAGECTL_PEN  (4)
#define IMAGECTL_FILL (8)

// By default, the control's anchoring mode is:
// ANCHOR_LEFT_TO_LEFT | ANCHOR_RIGHT_TO_LEFT | ANCHOR_TOP_TO_TOP | ANCHOR_BOTTOM_TO_TOP

// If the control's left edge anchors to the window's right edge.
// If this bit isn't set, the control's left edge anchors to the window's left edge.
#define ANCHOR_LEFT_TO_RIGHT 1
// If the control's right edge anchors to the window's right edge.
// If this bit isn't set, the control's right edge anchors to the window's left edge.
#define ANCHOR_RIGHT_TO_RIGHT 2
// If the control's top edge anchors to the window's bottom edge.
// If this bit isn't set, the control's top edge anchors to the window's top edge.
#define ANCHOR_TOP_TO_BOTTOM 4
// If the control's bottom edge anchors to the window's bottom edge.
// If this bit isn't set, the control's bottom edge anchors to the window's top edge.
#define ANCHOR_BOTTOM_TO_BOTTOM 8

#define WF_NOCLOSE  0x00000001//Disable close button
#define WF_FROZEN   0x00000002//Freeze window
#define WF_NOTITLE  0x00000004//Disable title
#define WF_NOBORDER 0x00000008//Disable border
#define WF_NOMINIMZ 0x00000010//Disable minimize button
#define WF_ALWRESIZ 0x00000020//Allow resize
#define WF_NOMAXIMZ 0x00000080//Disable maximize button
#define WF_FLATBORD 0x00000100//Use a flat border instead of the regular border
#define WF_NOWAITWM 0x00000200//Prevent waiting for the window manager to update. Useful for games (1)
#define WF_BACKGRND 0x00000400//The window is on a separate 'background' layer, behind normal windows.
#define WF_FOREGRND 0x00000800//The window is on a separate 'foreground' layer, in front of normal windows.
#define WF_SYSPOPUP 0x10000000//System Popup (omit from taskbar)

#define WINDOWS_MAX 256
#define WINDOW_TITLE_MAX 250
#define EVENT_QUEUE_MAX 256

#define KB_BUF_SIZE 512

#define LIST_ITEM_HEIGHT 16
#define ICON_ITEM_WIDTH  90
#define ICON_ITEM_HEIGHT 60

#define MB_OK                 0x00000000 //The message box contains one push button: OK.  This is the default.
#define MB_OKCANCEL           0x00000001 //The message box contains two push buttons: OK and Cancel.
#define MB_ABORTRETRYIGNORE   0x00000002 //The message box contains three push buttons: Abort, Retry and Ignore.
#define MB_YESNOCANCEL        0x00000003 //The message box contains three push buttons: Yes, No, and Cancel.
#define MB_YESNO              0x00000004 //The message box contains two push buttons: Yes, and No.
#define MB_RETRYCANCEL        0x00000005 //The message box contains two push buttons: Retry and Cancel.
#define MB_CANCELTRYCONTINUE  0x00000006 //The message box contains three push buttons: Cancel, Retry, and Continue.
#define MB_RESTART            0x00000007 //The message box contains one push button: Restart.

// This flag tells the operating system that it may choose where to place a window.
// If the xPos and yPos are bigger than or equal to zero, the application tells the OS where it should place the window.
// The OS will use this as a guideline, for example, if an application wants to go off the screen, the OS
// will reposition its window to be fully inside the screen boundaries.
#define CW_AUTOPOSITION   (-1)

#define WINDOW_MIN_WIDTH  (32) //that's already very small.
#define WINDOW_MIN_HEIGHT (3+TITLE_BAR_HEIGHT)

#define WINDOW_MINIMIZED_WIDTH  (160)
#define WINDOW_MINIMIZED_HEIGHT (3+TITLE_BAR_HEIGHT)

// Structs and enums

#define DefaultConsoleColor 0x0F

enum ConsoleType
{
	CONSOLE_TYPE_NONE, // uninitialized
	CONSOLE_TYPE_TEXT, // always full screen
	CONSOLE_TYPE_FRAMEBUFFER, // can either be the entire screen or just a portion of it. TODO
	CONSOLE_TYPE_SERIAL, // just plain old serial
	CONSOLE_TYPE_E9HACK, // Port E9 hack - qemu and bochs support this.
	CONSOLE_TYPE_WINDOW,
};

enum
{
	EVENT_NULL,
	EVENT_CREATE,  // Shall be only called once, when a window or widget is created.
	EVENT_DESTROY, // Shall be only called once, when a window or widget is destroyed.
	EVENT_PAINT,
	EVENT_MOVE,
	EVENT_SIZE,
	EVENT_ACTIVATE,
	EVENT_SETFOCUS,
	EVENT_KILLFOCUS,
	EVENT_UPDATE,
	EVENT_MOVECURSOR,
	EVENT_CLICKCURSOR,
	EVENT_RELEASECURSOR,
	EVENT_COMMAND,
	EVENT_KEYPRESS,
	EVENT_CLOSE,
	EVENT_KEYRAW,
	EVENT_MINIMIZE,//do not call this normally.
	EVENT_UNMINIMIZE,
	EVENT_UPDATE2,
	EVENT_MENU_CLOSE,
	EVENT_CLICK_CHAR,
	EVENT_MAXIMIZE,
	EVENT_UNMAXIMIZE,
	EVENT_IMAGE_REFRESH,
	EVENT_RIGHTCLICK,
	EVENT_RIGHTCLICKRELEASE,
	EVENT_CHECKBOX,
	EVENT_SCROLLDONE,
	EVENT_MAX,
	
	EVENT_USER = 0x1000,
};

//NOTE WHEN WORKING WITH CONTROLS:
//While yes, the window manager technically supports negative comboIDs, you're not supposed
//to use them.  They are used internally by other controls (for example list views and text input views).

enum
{
	//A null control.  Does nothing.
	CONTROL_NONE,
	//A text control printing text in its top-left corner.
	CONTROL_TEXT,
	//A control displaying an icon in the center of the rectangle.
	CONTROL_ICON,
	//A clickable button which triggers an EVENT_COMMAND with its comboID
	//as its first parm.
	CONTROL_BUTTON,
	//A text input field.  Not Finished
	CONTROL_TEXTINPUT,
	//A checkbox.  Not Finished.
	CONTROL_CHECKBOX,
	//A clickable label, which renders its text in the center-left.
	//Does the same as the CONTROL_BUTTON.
	CONTROL_CLICKLABEL,
	//A text control printing text in the center of the rectangle.
	CONTROL_TEXTCENTER,
	//A clickable button which triggers an event based on this->m_parm1
	//with its comboID as its first parm.
	CONTROL_BUTTON_EVENT,
	//A list view.  Complicated.
	CONTROL_LISTVIEW,
	//A vertical scroll bar.
	CONTROL_VSCROLLBAR,
	//A horizontal scroll bar.
	CONTROL_HSCROLLBAR,
	//A menu bar attached to the top of a window.
	//Adding more than one control is considered UB
	CONTROL_MENUBAR,
	//A text control printing big text (>127 chars)
	CONTROL_TEXTHUGE,
	//Same as CONTROL_LISTVIEW but with bigger icons.
	CONTROL_ICONVIEW,
	//Does nothing except surround other controls with a rectangle.  Useful for grouping settings.
	CONTROL_SURROUND_RECT,
	//Button with a colored background (parm2)
	CONTROL_BUTTON_COLORED,
	//Button as part of a list
	CONTROL_BUTTON_LIST,
	//Button with an icon on top.  Parm1= icon type, Parm2= icon size (16 or 32)
	CONTROL_BUTTON_ICON,
	//Button with an icon on top.  Parm1= icon type, Parm2= icon size (16 or 32)
	CONTROL_BUTTON_ICON_BAR,
	//A simple line control
	CONTROL_SIMPLE_HLINE,
	//Image control.  A _valid_ pointer to an Image structure must be passed into parm1.
	//When creating the control, the image gets duplicated, so the caller may free/dispose of
	//the old image.  The system will get rid of its own copy when the control gets destroyed.
	CONTROL_IMAGE,
	//Task list control
	CONTROL_TASKLIST,
	//Same as CONTROL_ICONVIEW but with draggable icons.
	CONTROL_ICONVIEWDRAG,
	//A table view. Even more complicated.
	CONTROL_TABLEVIEW,
	//Checkable icon button.
	CONTROL_BUTTON_ICON_CHECKABLE,
	//This control is purely to identify how many controls we support
	//currently.  This control is unsupported and will crash your application
	//if you use this.
	CONTROL_COUNT,
	
	CONTROL_SIMPLE_VLINE = -CONTROL_SIMPLE_HLINE, // macro for CONTROL_SIMPLE_HLINE with parm1 = 1
};

enum
{
	MBID_OK = 0x10010,
	MBID_CANCEL,
	MBID_ABORT,
	MBID_RETRY,
	MBID_IGNORE,
	MBID_YES,
	MBID_NO,
	MBID_TRY_AGAIN,
	MBID_CONTINUE,
	MBID_COUNT,
};

enum {
	P_BLACK,
	P_BACKGROUND_COLOR,
	P_BUTTON_MIDDLE_COLOR,
	P_WINDOW_BACKGD_COLOR,
	P_WINDOW_EDGE_COLOR,
	P_WINDOW_TITLE_ACTIVE_COLOR,
	P_WINDOW_TITLE_INACTIVE_COLOR,
	P_WINDOW_TITLE_ACTIVE_COLOR_B,
	P_WINDOW_TITLE_INACTIVE_COLOR_B,
	P_WINDOW_TITLE_TEXT_COLOR_SHADOW,
	P_WINDOW_TITLE_TEXT_COLOR,
	P_WINDOW_TEXT_COLOR,
	P_WINDOW_TEXT_COLOR_LIGHT,
	P_SYSTEM_FONT,
	P_TITLE_BAR_HEIGHT,
	P_TITLE_BAR_FONT,
	P_THEME_PARM_COUNT
};

enum
{
	/*0x80*/TIST_BOLD = '\x80',
	/*0x81*/TIST_UNDERLINE,
	/*0x82*/TIST_ITALIC,
	/*0x83*/TIST_RED,
	/*0x84*/TIST_BLUE,
	/*0x85*/TIST_GREEN,
	/*0x86*/TIST_LINK,
	/*0x87*/TIST_UNFORMAT,
	/*0x88*/TIST_UNBOLD,
	/*0x89*/TIST_UNITALIC,
	/*0x8A*/TIST_UNUNDERLINE,
	/*0x8B*/TIST_UNCOLOR,
	/*0x8C*/TIST_UNLINK,
	/*0x8D*/TIST_COUNT,
};

enum
{
	CLIPBOARD_DATA_NONE,
	CLIPBOARD_DATA_INTEGER,
	CLIPBOARD_DATA_BINARY,
	CLIPBOARD_DATA_TEXT,
	CLIPBOARD_DATA_LARGE_TEXT,
	
	// Add more clipboard data types here.  Unknown clipboard data types will be treated as generic binaries
};

//Console
typedef struct ConsoleStruct
{
	int  type; // ConsoleType enum
	int  width, height; // width and height
	uint16_t *textBuffer; // unused in fb mode
	uint16_t color; // colors
	int  curX, curY; // cursor X and Y positions
	bool pushOrWrap;// check if we should push whole screen up, or clear&wrap
	VBEData* m_vbeData;//vbe data to switch to when drawing, ONLY APPLIES TO CONSOLE_TYPE_WINDOW!!
	int  offX, offY;
	int  font;
	int  cwidth, cheight;
	bool m_dirty;
	char m_inputBuffer[KB_BUF_SIZE];
	int  m_inputBufferBeg, m_inputBufferEnd;
	int  m_cursorFlashTimer, m_cursorFlashState;
}
Console;


struct WindowStruct;
struct ControlStruct;
typedef bool (*WidgetEventHandler) (struct ControlStruct*, int eventType, int parm1, int parm2, struct WindowStruct* parentWindow);
typedef void (*WindowProc)         (struct WindowStruct*, int, int, int);

typedef struct
{
	int  m_icon;//can be blank
	char m_contents [128];
}
ListItem;

typedef struct
{
	bool m_hasIcons;
	int  m_elementCount, m_capacity;
	int  m_scrollY;
	int  m_highlightedElementIdx;
	ListItem *m_pItems;
}
ListViewData;

typedef struct
{
	bool m_isBeingDragged, m_clickedBefore;
	bool m_yMinButton, m_yMaxButton;
	int  m_min, m_max, m_pos, m_dbi;
}
ScrollBarData;

typedef struct tagMenuBarTreeItem
{
	int  m_comboID;//can be searchable
	int  m_childrenCount,
	     m_childrenCapacity;//if childrenCount reaches this and we need to add another, double this
	struct tagMenuBarTreeItem* m_childrenArray;
	char m_text [104];
	//if this value is set, it gets drawn if this is an item part of the root tree, or the parent is open too.
	bool m_isOpen;
}
MenuBarTreeItem;

typedef struct
{
	bool m_clicked,
	     m_hovered;
}
ButtonData;

typedef struct
{
	MenuBarTreeItem m_root;
}
MenuBarData;

typedef struct
{
	Image    *pImage;
	uint32_t  nCurrentColor;
	//if parm2 flag IMAGECTL_PAN  is set, this has an effect.  By default it is 0
	int       nCurPosX, nCurPosY;
	//to track cursor movement delta
	int       nLastXGot, nLastYGot;
	//if parm2 flag IMAGECTL_ZOOM is set, this has an effect on the resulting image.  By default it is the same as nWidth.
	//to get height, you do (int)(((long long)nHeight * nZoomedWidth) / nWidth)
	int       nZoomedWidth;
}
ImageCtlData;

typedef struct
{
	bool  m_focused;
	bool  m_dirty;//Has it been changed since the dirty flag was set to false?
	bool  m_onlyOneLine, m_showLineNumbers;//note that these are mutually exclusive, but both can be turned off
	int   m_textCapacity, m_textLength;//The text length needs to be 1 less than the text capacity.
	                                   //If the text capacity is 65, for example, the textLength may not be bigger than 64.
	int   m_textCursorIndex, m_textCursorSelStart, m_textCursorSelEnd,
	      m_scrollY;
	char* m_pText;
	bool  m_readOnly;
	bool  m_enableStyling;
}
TextInputData;

typedef struct
{
	bool m_clicked;
	bool m_checked;
}
CheckBoxData;

typedef struct ControlStruct
{
	bool      m_active;
	int       m_type;//CONTROL_XXX
	int       m_parm1, m_parm2;
	int       m_comboID;
	char      m_text[128];
	void*     m_dataPtr;
	Rectangle m_rect;
	bool      m_bMarkedForDeletion;
	
	//data for controls:
	union
	{
		ListViewData  m_listViewData;
		ScrollBarData m_scrollBarData;
		ButtonData    m_buttonData;
		MenuBarData   m_menuBarData;
		TextInputData m_textInputData;
		CheckBoxData  m_checkBoxData;
		ImageCtlData  m_imageCtlData;
	};
	
	int m_anchorMode;
	
	//event handler
	WidgetEventHandler OnEvent;
	
	// A rect that was tried.  This is what the control's size _should_ be,
	// but due to some limitation m_triedRect may not match m_rect.
	Rectangle m_triedRect;
	
	// The smallest rectangle a control can occupy is 10x10.
}
Control;

typedef struct
{
	bool  m_held;
	void* m_task_owning_it;
}
SafeLock;

// DON'T rely on this!!! This is an internal kernel struct and can be changed.

typedef struct WindowStruct
{
	bool       m_used;
	bool       m_minimized;
	bool       m_hidden;
	bool       m_isBeingDragged;
	bool       m_isSelected;
	
	bool       m_renderFinished;
	
	char*      m_title;
	
	int 	   m_flags;
	
	WindowProc m_callback;
	Rectangle  m_rect;
	Rectangle  m_rectBackup;
	VBEData    m_vbeData;
	
	int        m_iconID;
	
	bool       m_eventQueueLockUnused; // left to keep compatibity with old ELFs that modify the window structure directly
	short*     m_eventQueue;
	int*       m_eventQueueParm1;
	int*       m_eventQueueParm2;
	//short      m_eventQueue[EVENT_QUEUE_MAX];
	//int        m_eventQueueParm1[EVENT_QUEUE_MAX];
	//int        m_eventQueueParm2[EVENT_QUEUE_MAX];
	int        m_eventQueueSize;
	
	int        m_minWidth, m_minHeight;
	
	bool       m_markedForDeletion;
	
	Control*   m_pControlArray;
	int        m_controlArrayLen;
	
	void*      m_data; //user data
	
	void      *m_pOwnerThread, 
	          *m_pSubThread;//in case you ever want to use this
	
	Console*   m_consoleToFocusKeyInputsTo;
	
	bool       m_bWindowManagerUpdated;
	
	int        m_cursorID;
	
	bool       m_maximized;
	
	// Raw input buffer.
	char*      m_inputBuffer;
	int        m_inputBufferBeg, m_inputBufferEnd;
	
	bool       m_clickedInside;
	
	SafeLock   m_EventQueueLock;
	
	Rectangle  m_taskbarRect;
	
	Cursor     m_customCursor;
	
	int        m_frequentWindowRenders;
	int        m_lastSentPaintEventExternallyWhen;
	
	int        m_cursorID_backup;
	
	int        m_lastHandledMessagesWhen;
	
	//these two booleans are updated by UpdateDepthBuffer() internally
	//if none of the window's pixels are visible
	bool       m_bObscured;
	//if all of the window's pixels are visible at the same time
	//(we can optimize drawing by just VidBitBlitting it directly
	//to the screen, instead of taking occlusion into account)
	bool       m_bForemost;
	
	//avoid a data race while resizing the screen
	SafeLock   m_screenLock;
} Window;

typedef Window* PWINDOW;


//BetterStrTok: https://github.com/iProgramMC/BetterStrTok
typedef struct {
    bool m_bInitted;
    char*m_pContinuation;
    char*m_pReturnValue;
} TokenState;

typedef struct
{
	SafeLock m_lock;
	
	int  m_type;
	char m_short_str[256];
	int  m_blob_size;
	
	union {
		int   m_int_data;
		void *m_generic_data_ptr;
		char *m_char_str;
	};
}
ClipboardVariant;

#endif//_NANOSHELL_TYPES__H