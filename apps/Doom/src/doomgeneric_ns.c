#include <nsstandard.h>
#include "i_system.h"

//INPUT
#if 1
#define DKEY_RIGHTARROW	0xae
#define DKEY_LEFTARROW	0xac
#define DKEY_UPARROW	0xad
#define DKEY_DOWNARROW	0xaf
#define DKEY_STRAFE_L	0xa0
#define DKEY_STRAFE_R	0xa1
#define DKEY_USE		0xa2
#define DKEY_FIRE		0xa3
#define DKEY_ESCAPE		27
#define DKEY_ENTER		13
#define DKEY_TAB		9
#define DKEY_F1			(0x80+0x3b)
#define DKEY_F2			(0x80+0x3c)
#define DKEY_F3			(0x80+0x3d)
#define DKEY_F4			(0x80+0x3e)
#define DKEY_F5			(0x80+0x3f)
#define DKEY_F6			(0x80+0x40)
#define DKEY_F7			(0x80+0x41)
#define DKEY_F8			(0x80+0x42)
#define DKEY_F9			(0x80+0x43)
#define DKEY_F10		(0x80+0x44)
#define DKEY_F11		(0x80+0x57)
#define DKEY_F12		(0x80+0x58)

#define DKEY_BACKSPACE	0x7f
#define DKEY_PAUSE	0xff

#define DKEY_EQUALS	0x3d
#define DKEY_MINUS	0x2d

#define DKEY_RSHIFT	(0x80+0x36)
#define DKEY_RCTRL	(0x80+0x1d)
#define DKEY_RALT	(0x80+0x38)

#define DKEY_LALT	DKEY_RALT

// new keys:

#define DKEY_CAPSLOCK    (0x80+0x3a)
#define DKEY_NUMLOCK     (0x80+0x45)
#define DKEY_SCRLCK      (0x80+0x46)
#define DKEY_PRTSCR      (0x80+0x59)

#define DKEY_HOME        (0x80+0x47)
#define DKEY_END         (0x80+0x4f)
#define DKEY_PGUP        (0x80+0x49)
#define DKEY_PGDN        (0x80+0x51)
#define DKEY_INS         (0x80+0x52)
#define DKEY_DEL         (0x80+0x53)

#define DKEYP_0          0
#define DKEYP_1          DKEY_END
#define DKEYP_2          DKEY_DOWNARROW
#define DKEYP_3          DKEY_PGDN
#define DKEYP_4          DKEY_LEFTARROW
#define DKEYP_5          '5'
#define DKEYP_6          DKEY_RIGHTARROW
#define DKEYP_7          DKEY_HOME
#define DKEYP_8          DKEY_UPARROW
#define DKEYP_9          DKEY_PGUP

#define DKEYP_DIVIDE     '/'
#define DKEYP_PLUS       '+'
#define DKEYP_MINUS      '-'
#define DKEYP_MULTIPLY   '*'
#define DKEYP_PERIOD     0
#define DKEYP_EQUALS     DKEY_EQUALS
#define DKEYP_ENTER      DKEY_ENTER

static __attribute__((unused)) const char at_to_doom[] =
{
    /* 0x00 */ 0x00,
    /* 0x01 */ DKEY_ESCAPE,
    /* 0x02 */ '1',
    /* 0x03 */ '2',
    /* 0x04 */ '3',
    /* 0x05 */ '4',
    /* 0x06 */ '5',
    /* 0x07 */ '6',
    /* 0x08 */ '7',
    /* 0x09 */ '8',
    /* 0x0a */ '9',
    /* 0x0b */ '0',
    /* 0x0c */ '-',
    /* 0x0d */ '=',
    /* 0x0e */ DKEY_BACKSPACE,
    /* 0x0f */ DKEY_TAB,
    /* 0x10 */ 'q',
    /* 0x11 */ 'w',
    /* 0x12 */ 'e',
    /* 0x13 */ 'r',
    /* 0x14 */ 't',
    /* 0x15 */ 'y',
    /* 0x16 */ 'u',
    /* 0x17 */ 'i',
    /* 0x18 */ 'o',
    /* 0x19 */ 'p',
    /* 0x1a */ '[',
    /* 0x1b */ ']',
    /* 0x1c */ DKEY_ENTER,
    /* 0x1d */ DKEY_FIRE, /* KEY_RCTRL, */
    /* 0x1e */ 'a',
    /* 0x1f */ 's',
    /* 0x20 */ 'd',
    /* 0x21 */ 'f',
    /* 0x22 */ 'g',
    /* 0x23 */ 'h',
    /* 0x24 */ 'j',
    /* 0x25 */ 'k',
    /* 0x26 */ 'l',
    /* 0x27 */ ';',
    /* 0x28 */ '\'',
    /* 0x29 */ '`',
    /* 0x2a */ DKEY_RSHIFT,
    /* 0x2b */ '\\',
    /* 0x2c */ 'z',
    /* 0x2d */ 'x',
    /* 0x2e */ 'c',
    /* 0x2f */ 'v',
    /* 0x30 */ 'b',
    /* 0x31 */ 'n',
    /* 0x32 */ 'm',
    /* 0x33 */ ',',
    /* 0x34 */ '.',
    /* 0x35 */ '/',
    /* 0x36 */ DKEY_RSHIFT,
    /* 0x37 */ DKEYP_MULTIPLY,
    /* 0x38 */ DKEY_LALT,
    /* 0x39 */ DKEY_USE,
    /* 0x3a */ DKEY_CAPSLOCK,
    /* 0x3b */ DKEY_F1,
    /* 0x3c */ DKEY_F2,
    /* 0x3d */ DKEY_F3,
    /* 0x3e */ DKEY_F4,
    /* 0x3f */ DKEY_F5,
    /* 0x40 */ DKEY_F6,
    /* 0x41 */ DKEY_F7,
    /* 0x42 */ DKEY_F8,
    /* 0x43 */ DKEY_F9,
    /* 0x44 */ DKEY_F10,
    /* 0x45 */ DKEY_NUMLOCK,
    /* 0x46 */ 0x0,
    /* 0x47 */ 0x0, /* 47 (Keypad-7/Home) */
    /* 0x48 */ 0x0, /* 48 (Keypad-8/Up) */
    /* 0x49 */ 0x0, /* 49 (Keypad-9/PgUp) */
    /* 0x4a */ 0x0, /* 4a (Keypad--) */
    /* 0x4b */ 0x0, /* 4b (Keypad-4/Left) */
    /* 0x4c */ 0x0, /* 4c (Keypad-5) */
    /* 0x4d */ 0x0, /* 4d (Keypad-6/Right) */
    /* 0x4e */ 0x0, /* 4e (Keypad-+) */
    /* 0x4f */ 0x0, /* 4f (Keypad-1/End) */
    /* 0x50 */ 0x0, /* 50 (Keypad-2/Down) */
    /* 0x51 */ 0x0, /* 51 (Keypad-3/PgDn) */
    /* 0x52 */ 0x0, /* 52 (Keypad-0/Ins) */
    /* 0x53 */ 0x0, /* 53 (Keypad-./Del) */
    /* 0x54 */ 0x0, /* 54 (Alt-SysRq) on a 84+ key keyboard */
    /* 0x55 */ 0x0,
    /* 0x56 */ 0x0,
    /* 0x57 */ 0x0,
    /* 0x58 */ 0x0,
    /* 0x59 */ 0x0,
    /* 0x5a */ 0x0,
    /* 0x5b */ 0x0,
    /* 0x5c */ 0x0,
    /* 0x5d */ 0x0,
    /* 0x5e */ 0x0,
    /* 0x5f */ 0x0,
    /* 0x60 */ 0x0,
    /* 0x61 */ 0x0,
    /* 0x62 */ 0x0,
    /* 0x63 */ 0x0,
    /* 0x64 */ 0x0,
    /* 0x65 */ 0x0,
    /* 0x66 */ 0x0,
    /* 0x67 */ DKEY_UPARROW,
    /* 0x68 */ 0x0,
    /* 0x69 */ DKEY_LEFTARROW,
    /* 0x6a */ DKEY_RIGHTARROW,
    /* 0x6b */ 0x0,
    /* 0x6c */ DKEY_DOWNARROW,
    /* 0x6d */ 0x0,
    /* 0x6e */ 0x0,
    /* 0x6f */ 0x0,
    /* 0x70 */ 0x0,
    /* 0x71 */ 0x0,
    /* 0x72 */ 0x0,
    /* 0x73 */ 0x0,
    /* 0x74 */ 0x0,
    /* 0x75 */ 0x0,
    /* 0x76 */ 0x0,
    /* 0x77 */ 0x0,
    /* 0x78 */ 0x0,
    /* 0x79 */ 0x0,
    /* 0x7a */ 0x0,
    /* 0x7b */ 0x0,
    /* 0x7c */ 0x0,
    /* 0x7d */ 0x0,
    /* 0x7e */ 0x0,
    /* 0x7f */ DKEY_FIRE, //KEY_RCTRL,
};

#include "doomgeneric.h"

extern uint32_t* DG_ScreenBuffer;
bool drawFrameNow = false;
Window* g_pWindow;

#define KEYQUEUE_SIZE 128

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

//same as what a PS/2 keyboard would send. Convenient!

static unsigned char CvtToDoom(unsigned short kc)
{
	if (kc & 0xE000)
	{
		switch ((unsigned char)(kc) & 0x7F)
		{
			case 0x48: return DKEY_UPARROW;
			case 0x4B: return DKEY_LEFTARROW;
			case 0x4D: return DKEY_RIGHTARROW;
			case 0x50: return DKEY_DOWNARROW;
			default:
				return at_to_doom[(unsigned char)(kc) & 0x7F];
		}
	}
	else
		return at_to_doom[(unsigned char)(kc & 0x7F)];
}

static void AddKeyToQueue(bool pressed, unsigned short keycode)
{
	s_KeyQueue[s_KeyQueueWriteIndex] = (pressed << 12) | CvtToDoom(keycode);
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

int DG_GetKey(__attribute__((unused)) int* pressed, __attribute__((unused)) unsigned char* doomKey)
{
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
	{
		//key queue is empty

		return 0;
	}
	else
	{
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = (keyData >> 12) != 0;
		*doomKey = keyData & 0xFF;//TODO

		return 1;
	}
}
#endif

void I_Quit(void);
void DG_DrawFrame()
{
	drawFrameNow = true;
	
	if (!g_pWindow)
	{
		LogMsg("Quit?");
		I_Quit();
	}
	RegisterEventInsideWndProc(g_pWindow, EVENT_PAINT, 0, 0);
	HandleMessages (g_pWindow);
}

uint32_t DG_GetTicksMs()
{
	return GetTickCount();
}

void DG_SleepMs(uint32_t ms)
{
	sleep(ms);
}

void DG_SetWindowTitle(const char* title)
{
	if (g_pWindow)
	{
		SetWindowTitle(g_pWindow, title);
	}
}

int g_nextUpdateIn, timer;
Image image;
bool gotHexE0 = false;

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_KEYRAW:
		{
			if ((unsigned char)parm1 != 0xE0)//Extended:
			{
				short prototype = gotHexE0 * 0xE000 | (unsigned char) parm1;
				gotHexE0 = false;
				
				bool press = (parm1 & 0x80) == 0;
				AddKeyToQueue(press, prototype);
			}
			else
			{
				gotHexE0 = true;
			}
			break;
		}
		case EVENT_CREATE:
		{
			g_pWindow = pWindow;
			image.framebuffer = (const uint32_t*)DG_ScreenBuffer;
			image.width       = DOOMGENERIC_RESX;
			image.height      = DOOMGENERIC_RESY;
			break;
		}
		case EVENT_DESTROY:
		{
			//call I_Quit
			LogMsg("I destroy.");
			//I_Quit();
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			g_pWindow = NULL;
			//I_Quit();
			break;
		}
		case EVENT_PAINT:
		{
			if (drawFrameNow)
			{
				if (DG_ScreenBuffer)
				{
					//draw frame:
					VidBlitImage(&image, 0, 0);
				}
			}
			drawFrameNow = false;
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void WindowDeconstruct()//called AtExit
{
	LogMsg("WindowDeconstruct!");
	if (g_pWindow)
	{
		DestroyWindow(g_pWindow);
		
		//The EVENT_DESTROY handler makes HandleMessages return false.
		while (HandleMessages(g_pWindow));
		
		g_pWindow = NULL;
	}
}

bool WindowInit()
{
	Window* pWindow = CreateWindow ("doom", 150, 150, DOOMGENERIC_RESX, DOOMGENERIC_RESY, WndProc, 0);
	
	if (!pWindow)
	{
		LogMsg("Window creation failed!");
		return false;
	}
	LogMsg("WindowInit! %x", pWindow);
	
	g_pWindow = pWindow;
	
	LogMsg("I_AtExit(WindowDeconstruct)!");
	I_AtExit(WindowDeconstruct, true);
	
	return true;
}

bool DG_Init()
{
	return true;
}


