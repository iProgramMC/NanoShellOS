/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

       Keyboard module header file
******************************************/
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <main.h>

typedef uint8_t KeyState;
#define KEY_PRESSED ((KeyState) 1)
#define KEY_HELD ((KeyState) 2)
#define KEY_RELEASED ((KeyState) 0)

// Keycode definitions:
#if 1
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
#endif

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

void IrqKeyboard();
void KbAddKeyToBuffer(char key);
char KbGetKeyFromBuffer();
char KbWaitForKeyAndGet();
void KbFlushBuffer();
bool KbIsBufferEmpty();
KeyState KbGetKeyState(unsigned char keycode);
void SetFocusedConsole(Console *pConsole);

// max_size is not optional, contrary to popular belief :)
void KbGetString(char* buffer, int max_size);

#endif//_KEYBOARD_H
