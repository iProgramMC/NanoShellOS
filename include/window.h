/*****************************************
		NanoShell Operating System
		(C)2021-2022 iProgramInCpp

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

#define WINDOWS_MAX 256
#define WINDOW_TITLE_MAX 250
#define EVENT_QUEUE_MAX 256

//TODO
//#define ENABLE_MAXIMIZE

//Optional window border.  Was going to have a 3D effect, but I scrapped it.
#define WINDOW_RIGHT_SIDE_THICKNESS 0

//Theming
#define DEFAULT_BACKGROUND_COLOR                0x00007f7f
#define DEFAULT_BUTTON_MIDDLE_COLOR             0x00CCCCCC
#define DEFAULT_WINDOW_BACKGD_COLOR             0x00AAAAAA
#define DEFAULT_WINDOW_EDGE_COLOR               0x00000000
#define DEFAULT_WINDOW_TITLE_ACTIVE_COLOR       0x0000003F
#define DEFAULT_WINDOW_TITLE_INACTIVE_COLOR     0x007F7F7F
#define DEFAULT_WINDOW_TITLE_ACTIVE_COLOR_B     0x000000FF
#define DEFAULT_WINDOW_TITLE_INACTIVE_COLOR_B   0x00EEEEEE
#define DEFAULT_WINDOW_TITLE_TEXT_COLOR_SHADOW  0x0000003F
#define DEFAULT_WINDOW_TITLE_TEXT_COLOR         0x00FFFFFF
#define DEFAULT_WINDOW_TEXT_COLOR               0x00000000
#define DEFAULT_WINDOW_TEXT_COLOR_LIGHT         0x00FFFFFF
#define DEFAULT_SYSTEM_FONT                     FONT_BASIC
#define DEFAULT_TITLE_BAR_HEIGHT                18
#define DEFAULT_TITLE_BAR_FONT                  FONT_BASIC

//#define HARDCODE_EVERYTHING

#ifdef HARDCODE_EVERYTHING

#define BACKGROUND_COLOR                DEFAULT_BACKGROUND_COLOR              
#define BUTTON_MIDDLE_COLOR             DEFAULT_BUTTON_MIDDLE_COLOR           
#define WINDOW_BACKGD_COLOR             DEFAULT_WINDOW_BACKGD_COLOR           
#define WINDOW_EDGE_COLOR               DEFAULT_WINDOW_EDGE_COLOR             
#define WINDOW_TITLE_ACTIVE_COLOR       DEFAULT_WINDOW_TITLE_ACTIVE_COLOR     
#define WINDOW_TITLE_INACTIVE_COLOR     DEFAULT_WINDOW_TITLE_INACTIVE_COLOR   
#define WINDOW_TITLE_ACTIVE_COLOR_B     DEFAULT_WINDOW_TITLE_ACTIVE_COLOR_B   
#define WINDOW_TITLE_INACTIVE_COLOR_B   DEFAULT_WINDOW_TITLE_INACTIVE_COLOR_B 
#define WINDOW_TITLE_TEXT_COLOR_SHADOW  DEFAULT_WINDOW_TITLE_TEXT_COLOR_SHADOW
#define WINDOW_TITLE_TEXT_COLOR         DEFAULT_WINDOW_TITLE_TEXT_COLOR       
#define WINDOW_TEXT_COLOR               DEFAULT_WINDOW_TEXT_COLOR       
#define WINDOW_TEXT_COLOR_LIGHT         DEFAULT_WINDOW_TEXT_COLOR_LIGHT
#define SYSTEM_FONT                     FONT_BASIC
#define TITLE_BAR_HEIGHT 18
#define TITLE_BAR_FONT   FONT_BASIC

#else

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
uint32_t GetThemingParameter(int type);
void     SetThemingParameter(int type, uint32_t);

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

#endif

// This flag tells the operating system that it may choose where to place a window.
// If the xPos and yPos are bigger than or equal to zero, the application tells the OS where it should place the window.
// The OS will use this as a guideline, for example, if an application wants to go off the screen, the OS
// will reposition its window to be fully inside the screen boundaries.
#define CW_AUTOPOSITION   (-1)

#define WINDOW_MIN_WIDTH  (32) //that's already very small.
#define WINDOW_MIN_HEIGHT (3+TITLE_BAR_HEIGHT)

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
	EVENT_UPDATE2,
	EVENT_MENU_CLOSE,
	EVENT_CLICK_CHAR,
	EVENT_MAXIMIZE,
	EVENT_UNMAXIMIZE,
	EVENT_IMAGE_REFRESH,
	EVENT_RIGHTCLICK,
	EVENT_RIGHTCLICKRELEASE,
	EVENT_CHECKBOX,
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
	bool  m_enableStyling, m_enableSyntaxHilite;
}
TextInputData;

typedef struct
{
	bool m_clicked;
	bool m_checked;
}
CheckBoxData;

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
	bool      m_bFocused;
	
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

enum CURSORTYPE
{
	CURSOR_DEFAULT,
	CURSOR_WAIT,
	CURSOR_IBEAM,
	CURSOR_CROSS,
	CURSOR_PENCIL,
	CURSOR_COUNT,
};

#define WF_NOCLOSE  0x00000001//Disable close button
#define WF_FROZEN   0x00000002//Freeze window
#define WF_NOTITLE  0x00000004//Disable title
#define WF_NOBORDER 0x00000008//Disable border
#define WF_NOMINIMZ 0x00000010//Disable minimize button
#define WF_ALWRESIZ 0x00000020//Allow resize
#define WF_EXACTPOS 0x00000040//Exact position.  Only kernel may use this
#define WF_NOMAXIMZ 0x00000080//Disable maximize button
#define WF_FLATBORD 0x00000100//Use a flat border instead of the regular border
#define WF_FLBRDFRC 0x80000000//Internal flag: Remove the flat border when removing maximization


#define WI_INITGOOD 0x40000000//If the initialization process succeeded

#define WIN_KB_BUF_SIZE  512
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
	
	bool       m_eventQueueLockUnused; // left to keep compatibity with old ELFs that modify the window structure directly
	short      m_eventQueue[EVENT_QUEUE_MAX];
	int        m_eventQueueParm1[EVENT_QUEUE_MAX];
	int        m_eventQueueParm2[EVENT_QUEUE_MAX];
	int        m_eventQueueSize;
	
	int        m_minWidth, m_minHeight;
	
	bool       m_markedForDeletion;
	
	Control*   m_pControlArray;
	int        m_controlArrayLen;
	
	void*      m_data; //user data
	
	Task      *m_pOwnerThread, 
	          *m_pSubThread;//in case you ever want to use this
	
	Console*   m_consoleToFocusKeyInputsTo;
	
	bool       m_bWindowManagerUpdated;
	
	int        m_cursorID;
	
	bool       m_maximized;
	
	// Raw input buffer.
	char       m_inputBuffer[WIN_KB_BUF_SIZE];
	int        m_inputBufferBeg, m_inputBufferEnd;
	
	bool       m_clickedInside;
	
	SafeLock   m_EventQueueLock;
} Window;

/**
 * Define that does nothing... yet.
 * Mark your callbacks with this anyway!
 */
#define CALLBACK 

#define MAKE_MOUSE_PARM(x, y) ((x)<<16|(y))
#define GET_X_PARM(parm1)  (parm1>>16)
#define GET_Y_PARM(parm1)  (parm1&0xFFFF)

typedef Window* PWINDOW;

//For internal use only - actions that may not be safe to perform outside the main window manager task
enum {
	WACT_NONE,
	WACT_RESIZE,
	WACT_HIDE,
	WACT_SHOW,
	WACT_DESTROY,
	WACT_SELECT,
};

typedef struct
{
	bool bInProgress;
	
	Window *pWindow;
	
	int  nActionType;
	
	union {
		int vars[16];
		int var;
		
		Rectangle rects[4];
		Rectangle rect;
	};
}
WindowAction;


/**
 * Check if a rectangle contains a point.
 */
bool RectangleContains(Rectangle*r, Point*p);

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
 * Pops up a modal dialog box requesting a file name, and returns a MmAllocate'd
 * region of memory with the text inside.  Make sure to free the result, if it's non-null.
 *
 * Returns NULL if the user clicks "Cancel".
 *
 * pDefaultText can be NULL. If it isn't, the text box will be initialized with the default value passed in.
 */
char* FilePickerBox(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText);

/**
 * Allows the user to select a color.  Returns 0xffffffff if they did not pick.
 */
uint32_t ColorInputBox(Window* pWindow, const char* pPrompt, const char* pCaption);

/**
 * This works almost exactly the same as CreateWindow, but with a parent window which
 * gets frozen until this window gets destroyed.
 *
 * All the interfacing between parent and child must be done manually.
 */
void PopupWindow(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags);
void PopupWindowEx(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags, void* newData);

/**
 * Adds a control to the window.
 * AddControlEx is an expansion to AddControl which allows caller to set the control's anchoring mode too.
 */
int AddControl  (Window* pWindow, int type,                    Rectangle rect, const char* text, int comboID, int p1, int p2);
int AddControlEx(Window* pWindow, int type, int anchoringMode, Rectangle rect, const char* text, int comboID, int p1, int p2);

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

/**
 * Requests an event for that window in the master queue.  The window will still get it at some point.
 */
void WindowAddEventToMasterQueue(PWINDOW pWindow, int eventType, int parm1, int parm2);

/**
 * Changes the cursor of a window.
 *
 * cursorID can be any value inside the CURSORTYPE enum.
 */
void ChangeCursor (Window* pWindow, int cursorID);
#define OnBusy(pWindow) ChangeCursor (pWindow, CURSOR_WAIT);
#define OnNotBusy(pWindow) ChangeCursor (pWindow, CURSOR_DEFAULT);

/**
 * Internal usage.
 *
 * If you add other controls, the pointer to the control may or may not get invalidated,
 * due to the internal control array resizing itself.
 */
Control* GetControlByComboID(Window* pWindow, int comboID);

#endif//_WINDOW_H