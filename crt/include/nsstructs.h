#ifndef _NSSTRUCTS_H
#define _NSSTRUCTS_H

// Basic includes everyone should have
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// https://github.com/jezze/subc
typedef struct
{
	void *esp, *eax, *ebp;
}
JumpBufferTag;

typedef JumpBufferTag JumpBuffer[1], jmp_buf[1];

//TODO

typedef uint8_t BYTE;

#define false 0
#define true 1

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

#define EOF (-1)

// mmap() flags
#define PROT_NONE  (0 << 0)
#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2) //not applicable here

#define MAP_FAILED ((void*) -1) //not NULL

#define MAP_FILE      (0 << 0) //retroactive, TODO
#define MAP_SHARED    (1 << 0) //means changes in the mmapped region will be written back to the file on unmap/close
#define MAP_PRIVATE   (1 << 1) //means changes won't be committed back to the source file
#define MAP_FIXED     (1 << 4) //fixed memory mapping means that we really want it at 'addr'.
#define MAP_ANONYMOUS (1 << 5) //anonymous mapping, means that there's no file backing this mapping :)
#define MAP_ANON      (1 << 5) //synonymous with "MAP_ANONYMOUS"
#define MAP_NORESERVE (0 << 0) //don't reserve swap space, irrelevent here

#define MAP_DONTREPLACE (1 << 30) //don't clobber preexisting fixed mappings there. Used with MAP_FIXED to create...
#define MAP_FIXED_NOREPLACE (MAP_DONTREPLACE | MAP_FIXED)

#define ARRAY_COUNT(array) (sizeof(array)/sizeof(*array))

#define UNUSED __attribute__((unused))

// Defines
#define TRANSPARENT 0xFFFFFFFF

#define WIN_KB_BUF_SIZE  512

#define FLAGS_TOO(flags, color) (flags | (color & 0XFFFFFF))

#define TEXT_RENDER_TRANSPARENT 0xFFFFFFFF
#define TEXT_RENDER_BOLD        0x01000000

#define TEXTSTYLE_HCENTERED   (1)
#define TEXTSTYLE_VCENTERED   (2)
#define TEXTSTYLE_WORDWRAPPED (4)
#define TEXTSTYLE_RJUSTIFY    (8)
#define TEXTSTYLE_FORCEBGCOL  (16)//VidDrawText does nothing to prevent that, but it's useful for CONTROL_TEXTCENTER.

//#define TITLE_BAR_HEIGHT 18

#define WINDOW_BACKGD_COLOR (GetThemingParameter(P_WINDOW_BACKGD_COLOR))
#define WINDOW_TEXT_COLOR   (GetThemingParameter(P_WINDOW_TEXT_COLOR))
#define TITLE_BAR_HEIGHT    (GetThemingParameter(P_TITLE_BAR_HEIGHT))

#define RECT(rect,x,y,w,h) do {\
	rect.left = x, rect.top = y, rect.right = x+w, rect.bottom = y+h;\
} while (0)

// Mark your system callbacks with this anyway!!!
#define CALLBACK

#define MAKE_MOUSE_PARM(x, y) ((x)<<16|(y))
#define GET_X_PARM(parm1)  (parm1>>16)
#define GET_Y_PARM(parm1)  (parm1&0xFFFF)

#define TEXTEDIT_MULTILINE (1)
#define TEXTEDIT_LINENUMS  (2)
#define TEXTEDIT_READONLY  (4)
#define TEXTEDIT_STYLING   (8)

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

#define KEY_UNDEFINED_0 0
#define KEY_ESC 1
#define KEY_1 2
#define KEY_2 3
#define KEY_3 4
#define KEY_4 5
#define KEY_5 6
#define KEY_6 7
#define KEY_7 8
#define KEY_8 9
#define KEY_9 10
#define KEY_0 11
#define KEY_MINUS 12
#define KEY_HYPHEN KEY_MINUS
#define KEY_EQUALS 13
#define KEY_BACKSPACE 14
#define KEY_TAB 15
#define KEY_A 0x1e
#define KEY_B 0x30
#define KEY_C 0x2e
#define KEY_D 0x20
#define KEY_E 0x12
#define KEY_F 0x21
#define KEY_G 0x22
#define KEY_H 0x23
#define KEY_I 0x17
#define KEY_J 0x24
#define KEY_K 0x25
#define KEY_L 0x26
#define KEY_M 0x32
#define KEY_N 0x31
#define KEY_O 0x18
#define KEY_P 0x19
#define KEY_Q 0x10
#define KEY_R 0x13
#define KEY_S 0x1f
#define KEY_T 0x14
#define KEY_U 0x16
#define KEY_V 0x2f
#define KEY_W 0x11
#define KEY_X 0x2d
#define KEY_Y 0x15
#define KEY_Z 0x2c
#define KEY_BRACKET_LEFT 0x1a
#define KEY_BRACKET_RIGHT 0x1b
#define KEY_ENTER 0x1c
#define KEY_CONTROL 0x1d
#define KEY_CTRL KEY_CONTROL
#define KEY_SEMICOLON 0x27
#define KEY_APOSTROPHE 0x28
#define KEY_BACKTICK 0x29
#define KEY_LSHIFT 0x2a
#define KEY_BACKSLASH 0x2b
#define KEY_COMMA 0x33
#define KEY_DOT 0x34
#define KEY_SLASH 0x35
#define KEY_RSHIFT 0x36
#define KEY_PRINTSCREEN 0x37
#define KEY_ALT 0x38
#define KEY_SPACE 0x39
#define KEY_CAPSLOCK 0x3a
#define KEY_F1 0x3b
#define KEY_F2 0x3c
#define KEY_F3 0x3d
#define KEY_F4 0x3e
#define KEY_F5 0x3f
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_NUMLOCK 0x45
#define KEY_SCROLLLOCK 0x46
#define KEY_HOME 0x47
#define KEY_ARROW_UP 0x48
#define KEY_PAGEUP 0x49
#define KEY_NUMPAD_MINUS 0x4a
#define KEY_NUMPAD_HYPHEN KEY_NUMPAD_MINUS
#define KEY_ARROW_LEFT 0x4b
#define KEY_LEFT KEY_ARROW_LEFT
#define KEY_UNDEFINED_4C 0x4c
#define KEY_ARROW_RIGHT 0x4d
#define KEY_RIGHT KEY_ARROW_RIGHT
#define KEY_NUMPAD_PLUS 0x4e
#define KEY_END 0x4f
#define KEY_ARROW_DOWN 0x50
#define KEY_DOWN KEY_ARROW_DOWN
#define KEY_PAGEDOWN 0x51
#define KEY_INSERT 0x52
#define KEY_DELETE 0x53
#define KEY_UNDEFINED_54 0x54
#define KEY_UNDEFINED_55 0x55
#define KEY_UNDEFINED_56 0x56
#define KEY_F11 0x57
#define KEY_F12 0x58
#define KEY_UP KEY_ARROW_UP
#define KEY_MENU 0x5D

#define SCANCODE_RELEASE 0x80

#define WINDOWS_MAX 256
#define WINDOW_TITLE_MAX 250
#define EVENT_QUEUE_MAX 256

#define KB_BUF_SIZE 512

#define O_RDONLY (1)
#define O_WRONLY (2)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (4)
#define O_CREAT  (8)
#define O_EXEC   (1024)

//lseek whences
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define LIST_ITEM_HEIGHT 16
#define ICON_ITEM_WIDTH  90
#define ICON_ITEM_HEIGHT 60

#define PATH_MAX (260)
#define PATH_SEP ('/')
#define PATH_THISDIR (".")
#define PATH_PARENTDIR ("..")

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

enum
{
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FILE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16 //to be OR'd into the other flags
};

// use with the negative prefix
enum
{
	ENOTHING,
	EACCES,
	EEXIST,
	EINTR,
	EINVAL,
	EIO,
	EISDIR,
	ELOOP,
	EMFILE,
	ENAMETOOLONG,
	ENFILE,
	ENOENT,
	ENOSR,
	ENOSPC,
	ENOTDIR,
	ENXIO,
	EOVERFLOW,
	EROFS,
	EAGAIN,
	ENOMEM,
	ETXTBUSY,
	EBADF,
	ESPIPE,
};

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
	ICON_NULL,
	ICON_CABINET,
	ICON_CHIP,
	ICON_CHIP_SQ,
	ICON_COMPUTER,
	ICON_COMPUTER_SHUTDOWN,
	ICON_DESKTOP,
	ICON_DRAW,
	ICON_EARTH,
	ICON_ERROR,
	ICON_EXECUTE_FILE,
	ICON_FILE,
	ICON_FILES,
	ICON_FOLDER,
	ICON_FOLDER_BLANK,
	ICON_FOLDER_MOVE,
	ICON_FOLDER_PARENT,
	ICON_FOLDER16_CLOSED,
	ICON_FOLDER16_OPEN,
	ICON_GLOBE,
	ICON_GO,
	ICON_HAND,
	ICON_HELP,
	ICON_INFO,
	ICON_KEYBOARD,
	ICON_KEYBOARD2,
	ICON_LAPTOP,
	ICON_NOTES,
	ICON_PAINT,
	ICON_SERIAL,
	ICON_STOP,
	ICON_TEXT_FILE,
	ICON_WARNING,
	ICON_NANOSHELL_LETTERS,
	ICON_NANOSHELL_LETTERS16,
	ICON_NANOSHELL,
	ICON_NANOSHELL16,
	ICON_BOMB,
	ICON_BOMB_SPIKEY,
	ICON_FILE16,
	ICON_TEXT_FILE16,
	ICON_EXECUTE_FILE16,
	ICON_FOLDER_PARENT16,
	//icons V1.1
	ICON_FOLDER_SETTINGS,
	ICON_CABINET16,
	ICON_COMPUTER16,
	ICON_COMMAND,
	ICON_COMMAND16,
	ICON_ERROR16,
	//icons V1.2
	ICON_LOCK,
	ICON_DIRECTIONS,
	ICON_CERTIFICATE,
	ICON_FILE_WRITE,
	ICON_SCRAP_FILE,
	ICON_SCRAP_FILE16,
	ICON_RESMON,
	ICON_BILLBOARD,
	ICON_FILE_CSCRIPT,
	ICON_FILE_CSCRIPT16,
	ICON_FILE_CLICK,
	ICON_KEYS,
	ICON_RESTRICTED,
	ICON_HOME,
	ICON_HOME16,
	ICON_ADAPTER,
	ICON_CLOCK,
	ICON_CLOCK16,
	//icons V1.3
	ICON_APPLICATION,
	ICON_APPLICATION16,
	ICON_TASKBAR,
	ICON_APP_DEMO,
	ICON_COMPUTER_FLAT,
	ICON_CALCULATOR,
	ICON_CALCULATOR16,
	ICON_DESKTOP2,
	ICON_MOUSE,
    //Icons V1.31
	ICON_AMBULANCE,
	//icons V1.32
	ICON_FONTS,
	ICON_FONTS16,
	//icons V1.33
	ICON_RESMON16,
	ICON_NOTES16,
	ICON_FILE_NANO,
	//icons V1.34
	ICON_CLOCK_EMPTY,//Special case which draws more stuff
	//icons V1.35
	ICON_RUN,
	ICON_RUN16,
	//icons V1.4
	ICON_DEVTOOL,
	ICON_DEVTOOL_FILE,
	ICON_HEX_EDIT,
	ICON_CHAIN,
	ICON_CHAIN16,
	ICON_DEVTOOL16,
	ICON_TODO,
	ICON_FOLDER_DOCU,
	ICON_DLGEDIT,
	ICON_DESK_SETTINGS,
	ICON_SHUTDOWN,
	ICON_NOTEPAD,
	ICON_FILE_MKDOWN,
	ICON_FILE_MKDOWN16,
	ICON_COMPUTER_PANIC,
	ICON_EXPERIMENT,
	ICON_GRAPH,
	ICON_CABINET_COMBINE,
	ICON_REMOTE,
	ICON_CABINET_OLD,
	//icons V1.5
	ICON_DEVICE_CHAR,
	ICON_DEVICE_BLOCK,
	ICON_HARD_DRIVE,
	ICON_HARD_DRIVE_MOUNT,
	ICON_WINDOW,
	ICON_WINDOW_SNAP,
	ICON_WINDOW_OVERLAP,
	ICON_SWEEP_SMILE,
	ICON_SWEEP_CLICK,
	ICON_SWEEP_DEAD,
	ICON_SWEEP_CARET,
	ICON_DLGEDIT16,
	ICON_BOMB_SPIKEY16,
	ICON_MAGNIFY,
	ICON_MAGNIFY16,
	ICON_TAR_ARCHIVE,
	ICON_SYSMON,
	ICON_SYSMON16,
	ICON_COMPUTER_SHUTDOWN16,
	ICON_EXIT,
	ICON_KEY,
	ICON_KEYB_REP_SPEED,
	ICON_KEYB_REP_DELAY,
	ICON_MONITOR,
	//icons V1.6
	ICON_FILE_INI,
	ICON_WMENU,
	ICON_WMENU16,
	ICON_FILE_IMAGE,
	ICON_FILE_IMAGE16,
	ICON_FILE_LOG,
	ICON_STICKY_NOTES,
	ICON_STICKY_NOTES16,
	ICON_NOTE_YELLOW,
	ICON_NOTE_BLUE,
	ICON_NOTE_GREEN,
	ICON_NOTE_WHITE,
	ICON_FOLDER_OPEN,
	//icons V1.61
	ICON_EXPERIMENT2,
	ICON_FLOPPY, ICON_ACTION_SAVE = ICON_FLOPPY,
	ICON_ACTION_SAVE16,
	ICON_ACTION_OPEN,
	ICON_ACTION_OPEN16,
	ICON_PLUS,
	ICON_COUNT
};

enum CURSORTYPE
{
	CURSOR_DEFAULT,
	CURSOR_WAIT,
	CURSOR_IBEAM,
	CURSOR_CROSS,
	CURSOR_PENCIL,
	CURSOR_COUNT,
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

enum
{
	FONT_TAMSYN_REGULAR,
	FONT_TAMSYN_BOLD,
	FONT_PAPERM,
	FONT_FAMISANS,
	FONT_BASIC,
	FONT_GLCD,
	FONT_TAMSYN_MED_REGULAR,
	FONT_TAMSYN_MED_BOLD,
	FONT_TAMSYN_SMALL_REGULAR,
	FONT_TAMSYN_SMALL_BOLD,
	//FONT_BIGTEST,
	//FONT_BIGTEST2,
	FONT_LAST,
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


typedef struct
{
	int seconds,
		minutes,
		hours,
		weekday,
		day,
		month,
		year,
		statusA,
		statusB;
}
TimeStruct;

typedef struct
{
	int left, top, right, bottom;
}
Rectangle;

typedef struct
{
	int x, y;
}
Point;

typedef struct
{
	short width, height;
	const uint32_t *framebuffer;
}
Image;

typedef struct
{
	bool     m_available;			    //if the vbe display is available
	unsigned m_width, m_height, m_pitch;//bytes per row
	int      m_bitdepth;                //bits per pixel, only values we support: 0=8, 1=16, 2=32
	bool     m_dirty;					//useful if the framebuffer won't directly be pushed to the screen
	union {
		uint32_t* m_framebuffer32; //for ease of addressing
		uint16_t* m_framebuffer16;
		uint8_t * m_framebuffer8;
	};
	int m_pitch32, m_pitch16;      //uint32_t's and uint16_t's per row.
	Rectangle m_clipRect;
}
VBEData;

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
	
	char       m_title [250];
	
	int 	   m_flags;
	
	WindowProc m_callback;
	Rectangle  m_rect;
	Rectangle  m_rectBackup;
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
	
	// DO NOT TOUCH!
	void      *m_pOwnerThread, 
	          *m_pSubThread;
	
	Console*   m_consoleToFocusKeyInputsTo;
	
	bool       m_bWindowManagerUpdated;
	
	int        m_cursorID;
	
	bool       m_maximized;
	
	// Raw input buffer.
	char m_inputBuffer[WIN_KB_BUF_SIZE];
	int  m_inputBufferBeg, m_inputBufferEnd;
	
	bool       m_clickedInside;
	
	SafeLock   m_EventQueueLock;
} Window;

typedef Window* PWINDOW;


//BetterStrTok: https://github.com/iProgramMC/BetterStrTok
typedef struct {
    bool m_bInitted;
    char*m_pContinuation;
    char*m_pReturnValue;
} TokenState;


typedef struct DirEntS
{
	char     m_name[128]; //+nullterm, so 127 concrete chars
	uint32_t m_inode;     //device specific
	uint32_t m_type;
}
DirEnt;

typedef struct
{
	uint32_t m_type;
	uint32_t m_size;
	uint32_t m_blocks;
	uint32_t m_inode;
	
	uint32_t m_perms;
	uint32_t m_modifyTime;
	uint32_t m_createTime;
}
StatResult;

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

typedef struct
{
	int fd;
}
FILE;

#define stdout ((FILE*)1)
#define stdin  ((FILE*)2)
#define stderr ((FILE*)3)

#endif//_NSSTRUCTS_H