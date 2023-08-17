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

#define WINDOWS_MAX 64
#define WINDOW_TITLE_MAX 4096
#define EVENT_QUEUE_MAX 256

//TODO
#define ENABLE_MAXIMIZE

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
#define DEFAULT_MENU_BAR_HEIGHT                 15
#define DEFAULT_MENU_ITEM_HEIGHT                18
#define DEFAULT_SCROLL_BAR_SIZE                 16
#define DEFAULT_TITLE_BAR_FONT                  FONT_BASIC
#define DEFAULT_SELECTED_ITEM_COLOR             0x00316AC5
#define DEFAULT_SELECTED_ITEM_COLOR_B           0x00C1D2EE
#define DEFAULT_BORDER_SIZE                     10
#define DEFAULT_BORDER_SIZE_NORESIZE            3
#define DEFAULT_WINDOW_BORDER_COLOR             0x00B0B0B0

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
#define SELECTED_ITEM_COLOR             DEFAULT_SELECTED_ITEM_COLOR
#define SELECTED_ITEM_COLOR_B           DEFAULT_SELECTED_ITEM_COLOR_B
#define SYSTEM_FONT                     FONT_BASIC
#define TITLE_BAR_HEIGHT                18
#define MENU_BAR_HEIGHT                 18
#define SCROLL_BAR_SIZE                 16
#define TITLE_BAR_FONT                  FONT_BASIC
#define BORDER_SIZE                     DEFAULT_BORDER_SIZE
#define BORDER_SIZE_NORESIZE            DEFAULT_BORDER_SIZE_NORESIZE
#define WINDOW_BORDER_COLOR             DEFAULT_WINDOW_BORDER_COLOR

#else

#define ARROW_SIZE (TITLE_BAR_HEIGHT / 4) + 3

enum {
P_BLACK,
P_BACKGROUND_COLOR,
P_BUTTON_MIDDLE_COLOR,
P_WINDOW_BACKGD_COLOR,
P_WINDOW_BORDER_COLOR,
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
P_SELECTED_MENU_ITEM_COLOR,
P_SELECTED_MENU_ITEM_COLOR_B,
P_MENU_BAR_HEIGHT,
P_BORDER_SIZE,
P_BUTTON_HILITE_COLOR,
P_BUTTON_SHADOW_COLOR,
P_BUTTON_EDGE_COLOR,
P_BUTTON_HOVER_COLOR,
P_SCROLL_BAR_SIZE,
P_MENU_BAR_SELECTION_COLOR,
P_SELECTED_ITEM_COLOR,
P_SELECTED_TEXT_COLOR,
P_DESELECTED_TEXT_COLOR,
P_LIST_BACKGD_COLOR,
P_TOOLTIP_BACKGD_COLOR,
P_TOOLTIP_TEXT_COLOR,
P_SCROLL_BAR_BACKGD_COLOR,
P_SELECTED_MENU_ITEM_TEXT_COLOR,
P_DESELECTED_MENU_ITEM_TEXT_COLOR,
P_TABLE_VIEW_ALT_ROW_COLOR,
P_MENU_ITEM_HEIGHT,
P_BUTTON_XSHADOW_COLOR,
P_CAPTION_BUTTON_ICON_COLOR,
P_BORDER_SIZE_NORESIZE,
P_THEME_PARM_COUNT
};
uint32_t GetThemingParameter(int type);
void     SetThemingParameter(int type, uint32_t);

#define BACKGROUND_COLOR               	(GetThemingParameter(P_BACKGROUND_COLOR                ))
#define BUTTON_MIDDLE_COLOR             (GetThemingParameter(P_BUTTON_MIDDLE_COLOR             ))
#define WINDOW_BACKGD_COLOR             (GetThemingParameter(P_WINDOW_BACKGD_COLOR             ))
#define WINDOW_TITLE_ACTIVE_COLOR       (GetThemingParameter(P_WINDOW_TITLE_ACTIVE_COLOR       ))
#define WINDOW_TITLE_INACTIVE_COLOR     (GetThemingParameter(P_WINDOW_TITLE_INACTIVE_COLOR     ))
#define WINDOW_TITLE_ACTIVE_COLOR_B     (GetThemingParameter(P_WINDOW_TITLE_ACTIVE_COLOR_B     ))
#define WINDOW_TITLE_INACTIVE_COLOR_B   (GetThemingParameter(P_WINDOW_TITLE_INACTIVE_COLOR_B   ))
#define WINDOW_TITLE_TEXT_COLOR_SHADOW  (GetThemingParameter(P_WINDOW_TITLE_TEXT_COLOR_SHADOW  ))
#define WINDOW_TITLE_TEXT_COLOR         (GetThemingParameter(P_WINDOW_TITLE_TEXT_COLOR         ))
#define WINDOW_TEXT_COLOR               (GetThemingParameter(P_WINDOW_TEXT_COLOR               ))
#define WINDOW_TEXT_COLOR_LIGHT         (GetThemingParameter(P_WINDOW_TEXT_COLOR_LIGHT         ))
#define SYSTEM_FONT                     ((int)GetThemingParameter(P_SYSTEM_FONT                ))
#define TITLE_BAR_HEIGHT                ((int)GetThemingParameter(P_TITLE_BAR_HEIGHT           ))
#define TITLE_BAR_FONT                  ((int)GetThemingParameter(P_TITLE_BAR_FONT             ))
#define SELECTED_MENU_ITEM_COLOR        (GetThemingParameter(P_SELECTED_MENU_ITEM_COLOR        ))
#define SELECTED_MENU_ITEM_COLOR_B      (GetThemingParameter(P_SELECTED_MENU_ITEM_COLOR_B      ))
#define WINDOW_BORDER_COLOR             (GetThemingParameter(P_WINDOW_BORDER_COLOR             ))
#define MENU_BAR_HEIGHT                 ((int)GetThemingParameter(P_MENU_BAR_HEIGHT            ))
#define MENU_ITEM_HEIGHT                ((int)GetThemingParameter(P_MENU_ITEM_HEIGHT           ))
#define BORDER_SIZE                     ((int)GetThemingParameter(P_BORDER_SIZE                ))
#define BUTTON_HILITE_COLOR             (GetThemingParameter(P_BUTTON_HILITE_COLOR             ))
#define BUTTON_SHADOW_COLOR             (GetThemingParameter(P_BUTTON_SHADOW_COLOR             ))
#define BUTTON_EDGE_COLOR               (GetThemingParameter(P_BUTTON_EDGE_COLOR               ))
#define BUTTON_HOVER_COLOR              (GetThemingParameter(P_BUTTON_HOVER_COLOR              ))
#define SCROLL_BAR_SIZE                 ((int)GetThemingParameter(P_SCROLL_BAR_SIZE            ))
#define MENU_BAR_SELECTION_COLOR        (GetThemingParameter(P_MENU_BAR_SELECTION_COLOR        ))
#define SELECTED_ITEM_COLOR             (GetThemingParameter(P_SELECTED_ITEM_COLOR             ))
#define SELECTED_TEXT_COLOR             (GetThemingParameter(P_SELECTED_TEXT_COLOR             ))
#define DESELECTED_TEXT_COLOR           (GetThemingParameter(P_DESELECTED_TEXT_COLOR           ))
#define LIST_BACKGD_COLOR               (GetThemingParameter(P_LIST_BACKGD_COLOR               ))
#define TOOLTIP_BACKGD_COLOR            (GetThemingParameter(P_TOOLTIP_BACKGD_COLOR            ))
#define TOOLTIP_TEXT_COLOR              (GetThemingParameter(P_TOOLTIP_TEXT_COLOR              ))
#define SCROLL_BAR_BACKGD_COLOR         (GetThemingParameter(P_SCROLL_BAR_BACKGD_COLOR         ))
#define SELECTED_MENU_ITEM_TEXT_COLOR   (GetThemingParameter(P_SELECTED_MENU_ITEM_TEXT_COLOR   ))
#define DESELECTED_MENU_ITEM_TEXT_COLOR (GetThemingParameter(P_DESELECTED_MENU_ITEM_TEXT_COLOR ))
#define TABLE_VIEW_ALT_ROW_COLOR        (GetThemingParameter(P_TABLE_VIEW_ALT_ROW_COLOR        ))
#define BUTTON_XSHADOW_COLOR            (GetThemingParameter(P_BUTTON_XSHADOW_COLOR            ))
#define CAPTION_BUTTON_ICON_COLOR       (GetThemingParameter(P_CAPTION_BUTTON_ICON_COLOR       ))
#define BORDER_SIZE_NORESIZE            ((int)GetThemingParameter(P_BORDER_SIZE_NORESIZE       ))

#endif

#define COOLBAR_BUTTON_HEIGHT (TITLE_BAR_HEIGHT - 6 + 9)

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
	EVENT_SCROLLDONE,
	EVENT_BGREPAINT,
	EVENT_CTLREPAINT,//parm1 represents the comboID of the control to repaint.
	EVENT_TABCHANGED,//CONTROL_TAB_PICKER
	EVENT_CTLUPDATEVISIBLE,
	EVENT_TICK, // used for blinking text controls.
	EVENT_SMARTSNAP,
	EVENT_COMBOSELCHANGED,
	EVENT_COMBOSELCHANGED_PTE, // the message that the sub window will send
	EVENT_COMBOSUBGONE,
	EVENT_MAX,
	
	EVENT_PRIVATE_START = 0xF00,
	EVENT_RIGHTCLICKRELEASE_PRIVATE,
	EVENT_COMMAND_PRIVATE,
	EVENT_SET_WINDOW_TITLE_PRIVATE,
	EVENT_REPAINT_TITLE_PRIVATE,
	EVENT_SHOW_MENU_PRIVATE,
	EVENT_REQUEST_RESIZE_PRIVATE,
	EVENT_BORDER_SIZE_UPDATE_PRIVATE,
	EVENT_HIDE_WINDOW_PRIVATE,
	EVENT_SHOW_WINDOW_PRIVATE,
	EVENT_REPAINT_PRIVATE,
	EVENT_SET_WINDOW_ICON_PRIVATE,
	
	EVENT_USER = 0x1000,
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
	//A text input field.
	CONTROL_TEXTINPUT,
	//A checkbox.
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
	//Tab picker control.
	CONTROL_TAB_PICKER,
	//Progress bar control.
	CONTROL_PROGRESS_BAR,
	//Combo box control.
	CONTROL_COMBOBOX,
	//Color picker control.
	CONTROL_COLORPICKER,
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
	int  m_posX, m_posY; //used for CONTROL_ICONVIEWDRAG
	char m_contents [128];
	char m_contentsShown [64]; // padded in the end with '...'
}
ListItem;

#define MAX_COLUMN_LENGTH      (32)
#define MAX_ROW_CONTENT_LENGTH (128)

typedef enum
{
	TABLE_SORT_NEUTRAL,    // No effect
	TABLE_SORT_ASCENDING,
	TABLE_SORT_DESCENDING,
}
eTableSortMode;

typedef struct
{
	char   m_text[MAX_COLUMN_LENGTH];
	int    m_sort_order;
	int    m_width;
	Rectangle m_rect;
	bool   m_hovered;
	bool   m_clicked;
	bool   m_bNumbers; // if this column handles numbers. Useful when sorting.
	eTableSortMode m_sortMode;
}
TableViewColumn;

typedef struct
{
	char m_text[MAX_ROW_CONTENT_LENGTH];
}
TableViewItem;

typedef struct
{
	int m_icon;
	TableViewItem* m_items;
}
TableViewRow;

typedef struct
{
	int m_column_count;
	int m_column_capacity;
	TableViewColumn *m_pColumnData;
	
	int m_row_count;
	int m_row_capacity;
	TableViewRow    *m_pRowData;
	
	int m_row_scroll;
	int m_selected_row;
}
TableViewData;

typedef struct
{
	bool m_hasIcons;
	int  m_elementCount, m_capacity;
	int  m_scrollX, m_scrollY;
	int  m_highlightedElementIdx;
	ListItem *m_pItems;
	int  m_trackedListItem;
	bool m_bIsDraggingIt;
	int  m_extentX, m_extentY;
	int  m_startDragX, m_startDragY;
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
	      m_scrollX, m_scrollY, m_textCursorX, m_textCursorY;
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

typedef struct
{
	bool m_used;
	int  m_frequency;
	int  m_nextTickAt;
	int  m_firedEvent;
}
WindowTimer;

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
	bool      m_bVisible;
	
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
		TableViewData m_tableViewData;
	};
	
	int m_anchorMode;
	
	//event handler
	WidgetEventHandler OnEvent;
	
	// A rect that was tried.  This is what the control's size _should_ be,
	// but due to some limitation m_triedRect may not match m_rect.
	// The smallest rectangle a control can occupy is 10x10.
	Rectangle m_triedRect;
	
	bool      m_bDisabled;
}
Control;

enum CURSORTYPE
{
	CURSOR_DEFAULT,
	CURSOR_WAIT,
	CURSOR_IBEAM,
	CURSOR_CROSS,
	CURSOR_PENCIL,
	CURSOR_SIZE_NS,
	CURSOR_SIZE_WE,
	CURSOR_SIZE_NWSE,
	CURSOR_SIZE_NESW,
	CURSOR_SIZE_ALL,
	CURSOR_COUNT,
	CURSOR_CUSTOM,
};

//m_privFlags
#define WPF_MINIMIZEHOVERED 0x00000001
#define WPF_MAXIMIZEHOVERED 0x00000002
#define WPF_CLOSEBTNHOVERED 0x00000004
#define WPF_ICONBUTNHOVERED 0x00000008
#define WPF_MINIMIZECLICKED 0x00000010
#define WPF_MAXIMIZECLICKED 0x00000020
#define WPF_CLOSEBTNCLICKED 0x00000040
#define WPF_ICONBUTNCLICKED 0x00000080
#define WPF_OPENMENU        0x00000100//Internal flag: A menu is open from this window's menu bar -- don't open another.
#define WPF_NOHIDDEN        0x00000200//Internal flag: This window wasn't hidden at the time of resizing.
#define WPF_FROZENRM        0x00000400//Internal flag: Remove the 'frozen' flag when the window is no longer hung
#define WPF_HUNGWIND        0x00000800//Internal flag: The window is hung (won't respond to events)
#define WPF_INITGOOD        0x00001000//If the initialization process succeeded
#define WPF_FLBRDFRC        0x00002000//Internal flag: Remove the flat border when removing maximization

//m_flags
#define WF_NOCLOSE  0x00000001//Disable close button
#define WF_FROZEN   0x00000002//Freeze window
#define WF_NOTITLE  0x00000004//Disable title
#define WF_NOBORDER 0x00000008//Disable border
#define WF_NOMINIMZ 0x00000010//Disable minimize button
#define WF_ALWRESIZ 0x00000020//Allow resize
#define WF_EXACTPOS 0x00000040//Exact position.  Only kernel may use this
#define WF_NOMAXIMZ 0x00000080//Disable maximize button
#define WF_FLATBORD 0x00000100//Use a flat border instead of the regular border
#define WF_NOWAITWM 0x00000200//Prevent waiting for the window manager to update. Useful for games (1)
#define WF_BACKGRND 0x00000400//The window is on a separate 'background' layer, behind normal windows.
#define WF_FOREGRND 0x00000800//The window is on a separate 'foreground' layer, in front of normal windows.
#define WF_MAXIMIZE 0x00001000//The window is maximized.
#define WF_MINIMIZE 0x00002000//The window is minimized.
#define WF_NOIFOCUS 0x00004000//The window won't be focused on creation.
#define WF_SYSPOPUP 0x00008000//Internal flag: System Popup (omit from taskbar)
#define WF_MENUITEM 0x00010000//Internal flag: Menu Item
#define WI_NEVERSEL 0x00020000//Internal flag: Never select this window.
#define WI_MESSGBOX 0x00040000//Internal flag: This is a message box. Wait for it
#define WF_BACKGND2 0x00080000//The window is on a separate 'background2' layer, behind normal and background windows.


#define WI_INTEMASK 0x00000000//Internal flag mask that CreateWindow will filter

//Internal flag mask that SetWindowFlags will check for differences. If there are any found,
//we will call `WmOnChangedBorderParms`.
#define WI_BORDMASK (WF_NOBORDER | WF_ALWRESIZ | WF_FLATBORD | WF_NOTITLE)

// (1.) This should actually be enabled automatically if the process is seen rendering, like, a lot

#define WIN_KB_BUF_SIZE  4096
#define C_MAX_WIN_TIMER  8

typedef struct WindowStruct
{
	bool       m_used;
	uint8_t    m_resize_flags;
	bool       m_hidden;
	bool       m_isBeingDragged;
	bool       m_isSelected;
	
	bool       m_renderFinished;
	
	char*      m_title;
	
	int 	   m_flags;
	
	WindowProc m_callback;
	Rectangle  m_rect;
	Rectangle  m_rectBackup;
	// Now only includes the client area of the window.
	VBEData    m_vbeData;
	
	int        m_iconID;
	
	bool       m_eventQueueLockUnused; // left to keep compatibity with old ELFs that modify the window structure directly
	short*     m_unused0;
	int*       m_unused1;
	int*       m_unused2;
	//short      m_eventQueue[EVENT_QUEUE_MAX];
	//int        m_eventQueueParm1[EVENT_QUEUE_MAX];
	//int        m_eventQueueParm2[EVENT_QUEUE_MAX];
	int        m_unused3;
	
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
	
	bool       m_reserved1;
	
	// Raw input buffer.
	char*      m_inputBuffer;
	int        m_inputBufferBeg, m_inputBufferEnd;
	
	bool       m_clickedInside;
	
	SafeLock   m_unusedLock;
	
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
	
	// a list of timers
	WindowTimer m_timers[C_MAX_WIN_TIMER];
	int        m_timer_count;
	
	MenuBarData* m_pMenuBar; // The menu bar's data.
	
	// Private flags. These aren't modifiable by the window's controller.
	uint32_t   m_privFlags;
	
	// Includes everything, including window decorations and stuff.
	VBEData    m_fullVbeData;
	
	// The full rectangle. For backwards compatibility, the m_rect will now refer only
	// to the client rect offset by the position of the top-leftmost pixel of said area.
	Rectangle  m_fullRect;
	
	// The known border size. This is used in ResizeWindow when the border size changes.
	int        m_knownBorderSize;
	
	// The last window flags, before a border size recalculation
	int        m_lastWindowFlags;
	
	// The number of controls that require a ticking timer that sends
	// an EVENT_TICK to function properly. There are only two right now:
	// CONTROL_TEXTINPUT and CONTROL_COMBOBOX.
	int       m_nTickingCtls;
	
	// The ID of said timer.
	int       m_nTickTimerID;
	
	// The tick count of when we last sent a message.
	int       m_lastSentMessageTime;
}
Window;

/**
 * Define that does nothing... yet.
 * Mark your callbacks with this anyway!
 */
#define CALLBACK 

#define MAKE_MOUSE_PARM(x, y) ((unsigned short)(x)<<16|(unsigned short)(y))
#define GET_X_PARM(parm1)  (parm1>>16)
#define GET_Y_PARM(parm1)  ((short)(unsigned short)(parm1&0xFFFF))

typedef Window* PWINDOW;

//For internal use only - actions that may not be safe to perform outside the main window manager task
enum {
	WACT_NONE,
	WACT_UNDRAW_RECT,
	WACT_HIDE,
	WACT_SHOW,
	WACT_DESTROY,
	WACT_SELECT,
	WACT_UPDATEALL,
	WACT_STARTDRAG,
	WACT_STOPDRAG,
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
bool RectangleContains(Rectangle *r, Point *p);

/**
 * Check if a rectangle overlaps another rectangle.
 */
bool RectangleOverlap(Rectangle *r1, Rectangle *r2);

/**
 * Register an event to a certain window.
 */
void WindowRegisterEvent(Window* pWindow, short eventType, int parm1, int parm2);
void WindowRegisterEventUnsafe(Window* pWindow, short eventType, int parm1, int parm2);

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
void RequestRepaint(Window* pWindow);
void RequestRepaintNew(Window* pWindow);

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
char* FilePickerBoxEx(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText, const char* pInitPath /* = "/" */);
char* FilePickerBox  (Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText);

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
 * Removes a control with the specified comboID from the window.
 */
void RemoveControl(Window *pWindow, int comboID);

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
 * Call the ControlCallback of a specific control inside a window
 */
void CallControlCallback(Window* pWindow, int comboID, int eventType, int parm1, int parm2);

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

/**
 * Check if the window manager is running right now.
 */
bool IsWindowManagerRunning(void);

/**
 * Create a tooltip with the specified text.
 */
void TooltipShow(const char* text, int x, int y);

/**
 * Remove an active tooltip, if needed.
 */
void TooltipDismiss();

/**
 * Render a tooltip's text if needed.
 */
void TooltipDraw();

/**
 * Get the tooltip's rectangle.
 */
Rectangle* TooltipGetRect();

/**
 * Check if we're below a certain resolution that iProgramInCpp decided (right now) should be 800x600.
 */
bool IsLowResolutionMode();

/**
 * Adds a timer to the window and returns its ID.
 */
int AddTimer(Window* pWindow, int frequency, int event /* = EVENT_UPDATE */);

/**
 * Disarms a timer from the window (ie. removes it)
 */
void DisarmTimer(Window* pWindow, int timerID);

/**
 * Changes a timer's frequency.
 */
void ChangeTimer(Window* pWindow, int timerID, int newFrequency, int newTimer);

/**
 * Loads a cursor from a specified image and returns the ID that
 * may be passed on later to ChangeCursor().
 * Release this resource with ReleaseCursor().
 */
int UploadCursor(Image * pImage, int xOff, int yOff);

/**
 * Release a cursor resource uploaded using UploadCursor().
 */
void ReleaseCursor(int cursorID);

/**
 * The struct definition for the function below.
 */
typedef struct
{
	int     windowID;
	int     flags;
	char   *titleOut;
	size_t  titleOutSize; // The size of the 'titleOut' buffer. 
	int     iconID;
}
WindowQuery;

/**
 * Get a list of the windows on the system in `table`.
 */
void QueryWindows(WindowQuery* table, size_t tableSize, size_t* numWindows);

/**
 * Set the title of a window.
 */
void SetWindowTitle(Window* pWindow, const char* pText);

/**
 * Set the icon of a window.
 */
void SetWindowIcon(Window* pWindow, int icon);

/**
 * Set the visibility of a control.
 * 
 * If a control is being hidden, the background behind it will be repainted, but the
 * controls in front of this one won't - the window will have to repaint them by itself.
 * 
 * By default, controls are visible. That may be changeable in a future version.
 */
void SetControlVisibility(Window* pWindow, int comboID, bool bVisible);

/**
 * Get the border size of a window, respectively a set of window flags.
 */
int GetWindowBorderSize(Window* pWindow);
int GetBorderSize(uint32_t flags);

/**
 * Get the margins of a window. This isn't an actual rectangle, but rather a quad of 4 offsets.
 */
Rectangle GetWindowMargins(Window* pWindow);

/**
 * Gets the client rectangle of the window.
 *
 * bOffset = If the rectangle should be a screen coordinate, rather than a window coordinate.
 */
Rectangle GetWindowClientRect(Window* pWindow, bool offset);

/**
 * Get the composition flags of a window.
 */
int GetWindowFlags(Window* pWindow);

/**
 * Set the composition flags of a window. If any border flags are modified (WF_NOBORDER for instance),
 * the window will be automatically adjusted to fit the new border parameters, but without moving the
 * client area of the window.
 */
void SetWindowFlags(Window* pWindow, int flags);

/**
 * Draws one or more edges of a rectangle.
 *
 * Available flags are below this function prototype.
 *
 * If DRE_FILLED is not specified, the 'bg' parameter is ignored.
 */
void DrawEdge(Rectangle rect, int style, unsigned bg);

#define DRE_RAISEDINNER (1 << 0)
#define DRE_SUNKENINNER (1 << 1)
#define DRE_RAISEDOUTER (1 << 2)
#define DRE_SUNKENOUTER (1 << 3)
#define DRE_BLACKOUTER  (1 << 4) // takes priority over all these

#define DRE_OUTER (DRE_RAISEDOUTER | DRE_SUNKENOUTER)
#define DRE_INNER (DRE_RAISEDINNER | DRE_SUNKENINNER)

#define DRE_RAISED (DRE_RAISEDINNER | DRE_RAISEDOUTER)
#define DRE_SUNKEN (DRE_SUNKENINNER | DRE_SUNKENOUTER)

#define DRE_FILLED (1 << 24) // 'bg' is ignored if this is not set
#define DRE_FLAT   (1 << 25) // flat border.
#define DRE_HOT    (1 << 26) // the button is hovered

#define DRE_LEFT   (1  << 27)
#define DRE_TOP    (1  << 28)
#define DRE_RIGHT  (1  << 29)
#define DRE_BOTTOM (1  << 30)
#define DRE_RECT   (15 << 27)

/**
 * Draws an arrow of a specified type.
 */
typedef enum eArrowType
{
	DRA_UP,
	DRA_DOWN,
	DRA_LEFT,
	DRA_RIGHT,
}
eArrowType;

#define DRA_IGNOREXSIZE (1 << 0) // Ignores the width of the rectangle.
#define DRA_IGNOREYSIZE (1 << 1) // Ignores the height of the rectangle.

#define DRA_CENTERX     (1 << 2) // Center along the X axis.
#define DRA_CENTERY     (1 << 3) // Center along the Y axis.

// DRA_IGNOREXSIZE | DRA_IGNOREYSIZE means that both sizes are ignored, so the default size of ARROW_SIZE will be used.
#define DRA_IGNORESIZE (DRA_IGNOREXSIZE | DRA_IGNOREYSIZE)
#define DRA_CENTERALL  (DRA_CENTERX | DRA_CENTERY)

void DrawArrow(Rectangle rect, eArrowType arrowType, int flags, unsigned color);

#endif//_WINDOW_H