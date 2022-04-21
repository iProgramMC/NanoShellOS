/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

          Keyboard Driver module
******************************************/
#include <keyboard.h>
#include <string.h>
#include <window.h>
#include <clip.h>


#define inb(a) ReadPort(a)
#define outb(a,b) WritePort(a,b)

Console* g_focusedOnConsole = &g_debugConsole;
Window*  g_focusedOnWindow  = NULL;

// This changes the console that keypresses also go to.
void SetFocusedConsole(Console *pConsole)
{
	if (!pConsole)
		g_focusedOnConsole = &g_debugConsole;
	else
		g_focusedOnConsole = pConsole;
}

const unsigned char KeyboardMap[256] =
{
	// shift not pressed.
    0,  27, '1', '2',  '3',  '4', '5', '6',  '7', '8',
  '9', '0', '-', '=', '\b', '\t', 'q', 'w',  'e', 'r',
  't', 'y', 'u', 'i',  'o',  'p', '[', ']', '\n',   0,
  'a', 's', 'd', 'f',  'g',  'h', 'j', 'k',  'l', ';',
 '\'', '`',   0,'\\',  'z',  'x', 'c', 'v',  'b', 'n',
  'm', ',', '.', '/',   0,   '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,
	0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	
	// shift pressed.
	0,  0, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,
	0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};	
const unsigned char PrintableChars[256] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};	

KeyState keyboardState[128];

#define ENTER_KEY_CODE 0x1c

#define PIC1_DATA 0x21
#define PIC2_DATA 0xa1

#define KB_RAW_SIZE KB_BUF_SIZE*8

char KeyboardBuffer[KB_BUF_SIZE];
int  KeyboardBufferBeg = 0, KeyboardBufferEnd = 0;
char RawKeyboardBuffer[KB_RAW_SIZE];
int  RawKeyboardBufferBeg = 0, RawKeyboardBufferEnd = 0;
void KbAddKeyToBuffer(char key)
{
	if (!key) return;
	//LogMsg("Added key: ");LogIntDec(key);LogMsg("\n");
	KeyboardBuffer[KeyboardBufferEnd++] = key;
	while (KeyboardBufferEnd >= KB_BUF_SIZE)
		KeyboardBufferEnd -= KB_BUF_SIZE;
	
	CoAddToInputQueue (g_focusedOnConsole, key);
}
bool KbIsBufferEmpty()
{
	bool e = (KeyboardBufferBeg == KeyboardBufferEnd);
	return e;
}
char KbGetKeyFromBuffer()
{
	if (KeyboardBufferBeg != KeyboardBufferEnd)
	{
		char k = KeyboardBuffer[KeyboardBufferBeg++];
		while (KeyboardBufferBeg >= KB_BUF_SIZE)
			KeyboardBufferBeg -= KB_BUF_SIZE;
		return k;
	}
	else return 0;
}

void WinAddToInputQueue (Window* this, char input);
void KbAddRawKeyToBuffer(char key)
{
	if (!key) return;
	//LogMsg("Added key: ");LogIntDec(key);LogMsg("\n");
	RawKeyboardBuffer[RawKeyboardBufferEnd++] = key;
	while (RawKeyboardBufferEnd >= KB_RAW_SIZE)
		RawKeyboardBufferEnd -= KB_RAW_SIZE;
	if (g_focusedOnWindow)
		WinAddToInputQueue(g_focusedOnWindow, key);
}
bool KbIsRawBufferEmpty()
{
	bool e = (RawKeyboardBufferBeg == RawKeyboardBufferEnd);
	return e;
}
char KbGetKeyFromRawBuffer()
{
	if (RawKeyboardBufferBeg != RawKeyboardBufferEnd)
	{
		char k = RawKeyboardBuffer[RawKeyboardBufferBeg++];
		while (RawKeyboardBufferBeg >= KB_RAW_SIZE)
			RawKeyboardBufferBeg -= KB_RAW_SIZE;
		return k;
	}
	else return 0;
}


char KbWaitForKeyAndGet()
{
	while (KbIsBufferEmpty()) 
	{
		hlt; hlt; hlt; hlt;
	}
	return KbGetKeyFromBuffer();
}
void KbFlushBuffer()
{
	KeyboardBufferBeg = KeyboardBufferEnd = 0;
}

// max_size is not optional, contrary to popular belief :)
void KbGetString(char* buffer, int max_size)
{
	int index = 0, max_length = max_size - 1;
	//index represents where the next character we type would go
	while (index < max_length)
	{
		//! has to stall
		char k = KbWaitForKeyAndGet();
		if (k == '\n')
		{
			//return:
			LogMsgNoCr("%c", k);
			buffer[index++] = 0;
			return;
		}
		else if (k == '\b')
		{
			if (index > 0)
			{
				LogMsgNoCr("%c", k);
				index--;
				buffer[index] = 0;
			}
		}
		else
		{
			buffer[index++] = k;
			LogMsgNoCr("%c", k);
		}
	}
	LogMsg("");
	buffer[index] = 0;
}

KeyState KbGetKeyState(unsigned char keycode)
{
	if (keycode >= 128 && keycode <= 0) return KEY_RELEASED;
	return keyboardState[keycode];
}
bool ShiftPressed()
{
	return (KbGetKeyState(KEY_LSHIFT) == KEY_PRESSED ||
			KbGetKeyState(KEY_RSHIFT) == KEY_PRESSED);
}
bool g_virtualMouseEnabled = true;
int  g_virtualMouseSpeed = 5;
bool g_virtualMouseHadUpdatesBefore = false;

extern Cursor* g_currentCursor;
extern bool g_mouseInitted;
extern bool g_ps2MouseAvail;

void KbSetLedStatus(uint8_t status)
{
	status &= 0b111;
	while (ReadPort (0x64)  &  0x2) asm("pause");
	WritePort (0x60, 0xED);
	while (ReadPort (0x60) != 0xFA) asm("pause");
	WritePort (0x60, status);
	while (ReadPort (0x60) != 0xFA) asm("pause");
}
void KbEnableScanning()
{
	while (ReadPort (0x64)  &  0x2) asm("pause");
	WritePort (0x60, 0xF4);
	while (ReadPort (0x60) != 0xFA) asm("pause");
}
static void KbSetTypematicParmsByte(uint8_t status)
{
	while (ReadPort (0x64)  &  0x2) asm("pause");
	WritePort (0x60, 0xF3);
	while (ReadPort (0x60) != 0xFA) asm("pause");
	WritePort (0x60, status);
	while (ReadPort (0x60) != 0xFA) asm("pause");
}
static void KbSetUseScanCodeSet(uint8_t scs)
{
	while (ReadPort (0x64)  &  0x2) asm("pause");
	WritePort (0x60, 0xF0);
	while (ReadPort (0x60) != 0xFA) asm("pause");
	WritePort (0x60, scs);
	while (ReadPort (0x60) != 0xFA) asm("pause");
}

uint8_t g_typematicParms = 0b00110100;//default BIOS parms.
uint8_t g_newTypematicRepeatRate, g_newTypematicRepeatDelay;
void KbSetTypematicParms(uint8_t repeat_rate, uint8_t key_repeat_delay)
{
	g_typematicParms = (repeat_rate & 0b11111) | ((key_repeat_delay & 0b11) << 5);
	KbSetTypematicParmsByte(g_typematicParms);
}

uint8_t GetKeyboardProperty(int index)
{
	switch (index)
	{
		case KBPROPERTY_DELAY_BEFORE_REPEAT_MAX:
			return 0b11;
		case KBPROPERTY_REPEAT_FREQUENCY_MAX:
			return 0b11111;
		default:
			SLogMsg("GetKeyboardProperty: property number %d is not available");
			return 0;
		case KBPROPERTY_DELAY_BEFORE_REPEAT:
			return (g_typematicParms >> 5) & 0b11;
		case KBPROPERTY_REPEAT_FREQUENCY:
			return (g_typematicParms) & 0b11111;
	}
}
void SetKeyboardProperty(int index, uint8_t data)
{
	switch (index)
	{
		case KBPROPERTY_DELAY_BEFORE_REPEAT_MAX:
		case KBPROPERTY_REPEAT_FREQUENCY_MAX:
		default:
			SLogMsg("GetKeyboardProperty: property number %d is read-only or not available");
			break;
		case KBPROPERTY_DELAY_BEFORE_REPEAT:
			g_newTypematicRepeatRate = data;
			break;
		case KBPROPERTY_REPEAT_FREQUENCY:
			g_newTypematicRepeatDelay = data;
			break;
	}
}
void FlushKeyboardProperties()
{
	KbSetTypematicParms(g_newTypematicRepeatDelay, g_newTypematicRepeatRate);
}
void RevertKeyboardProperties()
{
}

void KbInitialize()
{
	//setup keyboard
	//this shows that everything is setup alright
	/*KbSetLedStatus(7);
	
	//enable scancodes.
	//This is not necessary as the BIOS already does this for you, but
	//what if it gets disabled before boot for whatever reason!?
	KbEnableScanning();
	
	KbSetUseScanCodeSet(1);//By default it's 1
	//^^TODO this does not work as scancodes don't mess up
	
	KbSetTypematicParms(0x14, 0x1);//10.9 chars/sec, 500ms delay before repeat.
	
	KbSetLedStatus(0);*/
}

//mappings: F11-Left Click, F12-Right Click, F9-make it slower, F10-make it faster
void UpdateFakeMouse()
{
	if (!g_currentCursor)
	{
		g_mouseInitted = true;
		SetDefaultCursor();
		SetMouseVisible (true);
		SetMousePos (GetScreenSizeX() / 2, GetScreenSizeY() / 2);
	}
	
	int mflags = 
		((keyboardState[KEY_F11] == KEY_PRESSED) * MOUSE_FLAG_L_BUTTON) | 
		((keyboardState[KEY_F12] == KEY_PRESSED) * MOUSE_FLAG_R_BUTTON);
	
	if (keyboardState[KEY_F9]  == KEY_PRESSED) g_virtualMouseSpeed--;
	if (keyboardState[KEY_F10] == KEY_PRESSED) g_virtualMouseSpeed++;
	
	//TODO: maybe some kind of OSD?
	
	int dirX = 0, dirY = 0;
	
	dirY += g_virtualMouseSpeed * (keyboardState[KEY_ARROW_UP]    == KEY_PRESSED);
	dirY -= g_virtualMouseSpeed * (keyboardState[KEY_ARROW_DOWN]  == KEY_PRESSED);
	dirX -= g_virtualMouseSpeed * (keyboardState[KEY_ARROW_LEFT]  == KEY_PRESSED);
	dirX += g_virtualMouseSpeed * (keyboardState[KEY_ARROW_RIGHT] == KEY_PRESSED);
	
	if (dirX < 0) mflags |= (1 << 4);
	if (dirY < 0) mflags |= (1 << 5);
	
	// If we have movement/clicks, or if we HAD them but don't (we still need to inform kernel about releases)
	if (dirX || dirY || mflags || g_virtualMouseHadUpdatesBefore)
	{
		OnUpdateMouse (mflags, dirX, dirY, 0);
		g_virtualMouseHadUpdatesBefore = false;
	}
	// If we have movement/clicks right now, tell our later selves to also send an update when releasing movements.
	if (dirX || dirY || mflags)
	{
		g_virtualMouseHadUpdatesBefore = true;
	}
}

char KbMapAtCodeToChar(char kc)
{
	return KeyboardMap[(kc) + (ShiftPressed() ? 0x80 : 0x00)];
}

void IrqKeyboard(UNUSED int e[50])
{
	// acknowledge interrupt:
	WritePort(0x20, 0x20);
	WritePort(0xA0, 0x20);
	
	// get status:
	unsigned char status, keycode;
	status = ReadPort(KEYBOARD_STATUS_PORT);
	
	// lowest bit of status _will_ be set if buffer is not empty
	if (status == 0xFA)
	{
		LogMsgNoCr("Got ack!");
	}
	if (status & 0x01)
	{
		keycode = ReadPort (KEYBOARD_DATA_PORT);
		KbAddRawKeyToBuffer (keycode);
		if (keycode & SCANCODE_RELEASE)
		{
			keyboardState[keycode & SCANCODE_NOTREL] = KEY_RELEASED;
		}
		else
		{
			//test
			int kc = keycode & SCANCODE_NOTREL;
			
			keyboardState[kc] = KEY_PRESSED;
			KbAddKeyToBuffer(KbMapAtCodeToChar(kc));
		}
		
		if (keycode == (SCANCODE_RELEASE | KEY_F12))
		{
			CbPushTextIntoBuffer();
		}
		
		if (g_virtualMouseEnabled && VidIsAvailable() && !g_ps2MouseAvail)
		{
			UpdateFakeMouse();
		}
	}
}
