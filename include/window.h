/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Window Manager module header file
******************************************/
#ifndef _WINDOW_H
#define _WINDOW_H

#include <main.h>
#include <string.h>
#include <video.h>
#include <console.h>
#include <memory.h>
#include <task.h>

#define THREADING_ENABLED 1 //0

#define WINDOWS_MAX 64
#define WINDOW_TITLE_MAX 250
#define EVENT_QUEUE_MAX 256

#define TITLE_BAR_HEIGHT 18

//Optional window border.  Was going to have a 3D effect, but I scrapped it.
#define WINDOW_RIGHT_SIDE_THICKNESS 0

#define BACKGROUND_COLOR                0xFF007f7f
#define BUTTON_MIDDLE_COLOR             0xFFCCCCCC
#define WINDOW_BACKGD_COLOR             0xFFAAAAAA
#define WINDOW_EDGE_COLOR               0xFF000000
#define WINDOW_TITLE_ACTIVE_COLOR       0xFF00003F
#define WINDOW_TITLE_INACTIVE_COLOR     0xFF7F7F7F
//#define WINDOW_TITLE_ACTIVE_COLOR_B   0xFF0000FF
//#define WINDOW_TITLE_INACTIVE_COLOR_B 0xFFAAAAAA
#define WINDOW_TITLE_ACTIVE_COLOR_B     0xFF0000FF
#define WINDOW_TITLE_INACTIVE_COLOR_B   0xFFEEEEEE
#define WINDOW_TITLE_TEXT_COLOR_SHADOW  0xFF00003F
#define WINDOW_TITLE_TEXT_COLOR         0x00FFFFFF

#define WINDOW_MIN_WIDTH  (32) //that's already very small.
#define WINDOW_MIN_HEIGHT (14)

#define WINDOW_MINIMIZED_WIDTH  (160)
#define WINDOW_MINIMIZED_HEIGHT (3+TITLE_BAR_HEIGHT)

enum {
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
	EVENT_MAX
};

//NOTE WHEN WORKING WITH CONTROLS:
//While yes, the window manager technically supports negative comboIDs, you're not supposed
//to use them.  They are used internally by other controls (for example list views and text input views).

enum {
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
	//This control is purely to identify how many controls we support
	//currently.  This control is unsupported and will crash your application
	//if you use this.
	CONTROL_COUNT
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

#define MB_OK                 0x00000000 //The message box contains one push button: OK.  This is the default.
#define MB_OKCANCEL           0x00000001 //The message box contains two push buttons: OK and Cancel.
#define MB_ABORTRETRYIGNORE   0x00000002 //The message box contains three push buttons: Abort, Retry and Ignore.
#define MB_YESNOCANCEL        0x00000003 //The message box contains three push buttons: Yes, No, and Cancel.
#define MB_YESNO              0x00000004 //The message box contains two push buttons: Yes, and No.
#define MB_RETRYCANCEL        0x00000005 //The message box contains two push buttons: Retry and Cancel.
#define MB_CANCELTRYCONTINUE  0x00000006 //The message box contains three push buttons: Cancel, Retry, and Continue.
#define MB_RESTART            0x00000007 //The message box contains one push button: Restart.

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

#define LIST_ITEM_HEIGHT 16
#define ICON_ITEM_WIDTH  90
#define ICON_ITEM_HEIGHT 60

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
	bool m_clicked;
}
ButtonData;

typedef struct
{
	MenuBarTreeItem m_root;
}
MenuBarData;

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
	};
	
	//event handler
	WidgetEventHandler OnEvent;
}
Control;

#define WF_NOCLOSE  0x00000001
#define WF_FROZEN   0x00000002
#define WF_NOTITLE  0x00000004
#define WF_NOBORDER 0x00000008
#define WF_NOMINIMZ 0x00000010

typedef struct WindowStruct
{
	bool       m_used;
	bool       m_minimized;
	bool       m_hidden;
	bool       m_isBeingDragged;
	bool       m_isSelected;
	
	bool       m_renderFinished;
	
	char       m_title [250];
	
	int 	   m_flags;
	
	WindowProc m_callback;
	Rectangle  m_rect;
	Rectangle  m_rectBackup;
	//uint32_t*  m_framebuffer;
	//int        m_fbWidth, m_fbHeight;
	VBEData    m_vbeData;
	
	int        m_iconID;
	
	bool       m_eventQueueLock;
	short      m_eventQueue[EVENT_QUEUE_MAX];
	int        m_eventQueueParm1[EVENT_QUEUE_MAX];
	int        m_eventQueueParm2[EVENT_QUEUE_MAX];
	int        m_eventQueueSize;
	
	bool       m_markedForDeletion;
	
	Control*   m_pControlArray;
	int        m_controlArrayLen;
	
	void*      m_data; //user data
	
	Task      *m_pOwnerThread, 
	          *m_pSubThread;//in case you ever want to use this
	
	Console*   m_consoleToFocusKeyInputsTo;
} Window;

/**
 * Define that does nothing... yet.
 * Mark your callbacks with this anyway!
 */
#define CALLBACK 

#define MAKE_MOUSE_PARM(x, y) ((x)<<16|(y))
#define GET_X_PARM(parm1)  (parm1>>16)
#define GET_Y_PARM(parm2)  (parm1&0xFFFF)

/**
 * Check if a rectangle contains a point.
 */
bool RectangleContains(Rectangle*r, Point*p) ;

/**
 * Register an event to a certain window.
 */
void WindowRegisterEvent (Window* pWindow, short eventType, int parm1, int parm2);
void WindowRegisterEventUnsafe (Window* pWindow, short eventType, int parm1, int parm2);

/**
 * Entry point of the window manager.
 *
 * For utility this can directly be put inside a KeStartTask.
 */
void WindowManagerTask(__attribute__((unused)) int useless_argument);


//Windowing API

/**
 * Creates a window, with its top left corner at (xPos, yPos), and its
 * bottom right corner at (xPos + xSize, yPos + ySize).
 *
 * WindowProc is the main event handler of the program, but it isn't called.
 * spontaneously. Instead, you use it like:
 *
 * while (HandleMessages(pWindow));
 */
Window* CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags);

/**
 * Updates the window, and handles its messages.
 */
bool HandleMessages(Window* pWindow);

/**
 * The default window event procedure.  Call this when you don't know
 * how to handle an event properly.
 */
void DefaultWindowProc (Window* pWindow, int messageType, UNUSED int parm1, UNUSED int parm2);

/**
 * Requests a safe window destruction from the window manager.
 */
void DestroyWindow (Window* pWindow);

/**
 * Requests a safe re-paint of the window from the window manager.
 */
void RequestRepaint (Window* pWindow);
void RequestRepaintNew (Window* pWindow);

/**
 * Displays a modal dialog box that contains a system icon, a set of buttons, and 
 * a brief application-specific message, such as status or error information.  The message
 * box returns an integer value that indicates which button the user clicked.
 */
int MessageBox (Window* pWindow, const char* pText, const char* pCaption, uint32_t type);

/**
 * Pops up a modal dialog box requesting an input string, and returns a MmAllocate'd
 * region of memory with the text inside.  Make sure to free the result, if it's non-null.
 *
 * Returns NULL if the user clicks "Cancel".
 *
 * pDefaultText can be NULL. If it isn't, the text box will be initialized with the default value passed in.
 */
char* InputBox(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText);

/**
 * TBA
 */
uint32_t ColorInputBox(Window* pWindow, const char* pPrompt, const char* pCaption);

/**
 * Adds a control to the window.
 */
int AddControl(Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2);

/**
 * Gets the updates per second the window manager could do.
 */
int GetWindowManagerFPS();

/**
 * Call the WindowCallback of a window.
 */
int CallWindowCallback(Window* pWindow, int eq, int eqp1, int eqp2);

/**
 * Call the WindowCallback of a window and its controls.
 */
int CallWindowCallbackAndControls(Window* pWindow, int eq, int eqp1, int eqp2);

#endif//_WINDOW_H