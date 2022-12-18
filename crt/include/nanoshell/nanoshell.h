// nanoshell/nanoshell.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library

// This non-portable include file includes everything from NanoShell.

#ifndef _NANOSHELL___H
#define _NANOSHELL___H

#include <nanoshell/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <alloca.h>
#include <unistd.h>
#include <time.h>

// NanoShell specifics
#include <nanoshell/graphics.h>

// Threading
void sleep(int ms);      //not actually standard I don't think
void exit (int errcode);

// Assertion


// Optimized memory operations to word width

//NOTE: size must be 4 byte aligned!!
void  ZeroMemory (void* bufptr1, size_t size);
void  fmemcpy32 (void* restrict dest, const void* restrict src, size_t size);
void* fast_memset(void* bufptr, BYTE val, size_t size);

// File management

// Graphics Interface

// Window API
Window*     CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags);
bool        HandleMessages(Window* pWindow);
void        DefaultWindowProc (Window* pWindow, int messageType, int parm1, int parm2);
void        DestroyWindow (Window* pWindow);
int         MessageBox (Window* pWindow, const char* pText, const char* pCaption, uint32_t type);
int         AddControl(Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2);
void        SetScrollBarMin (Window *pWindow, int comboID, int min);
void        SetScrollBarMax (Window *pWindow, int comboID, int max);
void        SetScrollBarPos (Window *pWindow, int comboID, int pos);
int         GetScrollBarPos (Window *pWindow, int comboID);
void        AddElementToList (Window* pWindow, int comboID, const char* pText, int optionalIcon);
const char* GetElementStringFromList (Window* pWindow, int comboID, int index);
void        RemoveElementFromList (Window* pWindow, int comboID, int elemIndex);
void        ResetList (Window* pWindow, int comboID);
void        SetLabelText (Window *pWindow, int comboID, const char* pText);
void        AddMenuBarItem (Window* pWindow, int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText);
void        SetHugeLabelText (Window *pWindow, int comboID, const char* pText);
void        SetTextInputText(Window* pWindow, int comboID, const char* pText);
void        SetWindowIcon (Window* pWindow, int icon);
void        SetIcon (Window* pWindow, int comboID, int icon);
void        SetWindowTitle(Window* pWindow, const char* pTitle);
void        RegisterEvent(Window* pWindow, short evType, int parm1, int parm2);
void        RegisterEventInsideWndProc(Window* pWindow, short evType, int parm1, int parm2);
int         AddControlEx(Window* pWindow, int type, int anchor_mode, Rectangle rect, const char* text, int comboID, int p1, int p2);
bool        TextInputQueryDirtyFlag(Window* pWindow, int comboID);
void        TextInputClearDirtyFlag(Window* pWindow, int comboID);
const char* TextInputGetRawText(Window* pWindow, int comboID);
bool        CheckboxGetChecked(Window* pWindow, int comboID);
void        CheckboxSetChecked(Window* pWindow, int comboID, bool checked);
void        SetTextInputText(Window* pWindow, int comboID, const char* pText);
void        ShellAbout (const char* pText, int icon);
void        RequestRepaint (Window *pWindow);
void        RequestRepaintNew (Window *pWindow);
void        PopupWindow (Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags);
uint32_t    ColorInputBox (Window *pWindow, const char *pPrompt, const char *pCaption);
uint32_t    GetThemingParameter (int type);
void        SetThemingParameter (int type, uint32_t parm);
void        SetWidgetEventHandler(Window *pWindow, int comboID, WidgetEventHandler handler);
// Returns a kernel memory region. Use MmKernelFree() instead of free() to free it.
char*       InputBox (Window *pWindow, const char *pPrompt, const char *pCaption, const char *pDefaultText);

// Internal C Compiler
int CcRunCCode(const char* pCode, int length);

// NanoShell Versioning
int NsGetVersion ();
const char* GetVersionString();

// Shell
int ShellExecute        (const char *pCommand);    //for instance, "e <your favorite executable>"
int ShellExecuteResource(const char *pResourceID); //for instance, shell:stuff

// Errors
int  SetErrorNumber(int errno);
int  GetErrorNumber();
int* GetErrorNumberPointer();
#define errno (*GetErrorNumberPointer())

// Kernel memory resource management.
// NOTE: Improper management of kernel memory resources will cause a leak that
// will persist over the rest of the OS' runtime, so use carefully!!!!
void* MmKernelAllocate(size_t sz);

//note: Do not feed this function addresses from malloc().
void MmKernelFree(void *pData);


#endif//_NANOSHELL___H