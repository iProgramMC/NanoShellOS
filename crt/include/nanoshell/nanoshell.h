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
#include <errno.h>
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
void* fast_memset(void* bufptr, uint8_t val, size_t size);

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
void        SetListItemText(Window* pWindow, int comboID, int index, int icon, const char * pText);
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
void        PopupWindow  (Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags);
void        PopupWindowEx(Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags, void* pData);
uint32_t    ColorInputBox (Window *pWindow, const char *pPrompt, const char *pCaption);
uint32_t    GetThemingParameter (int type);
void        SetThemingParameter (int type, uint32_t parm);
void        SetWidgetEventHandler(Window *pWindow, int comboID, WidgetEventHandler handler);
WidgetEventHandler GetWidgetEventHandler(Window* pWindow, int comboID);
// The input boxes that return strings return a kernel memory region. Use MmKernelFree() instead of free() to free it.
char*       InputBox (Window *pWindow, const char *pPrompt, const char *pCaption, const char *pDefaultText);
char*       FilePickerBox(Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText);
void        CallControlCallback(Window * pWindow, int comboID, int event, int parm1, int parm2);
void        TextInputSetMode (Window *pWindow, int comboID, int mode);
int         GetScrollBarMin (Window *pWindow, int comboID);
int         GetScrollBarMax (Window *pWindow, int comboID);
int         GetSelectedIndexList (Window* pWindow, int comboID);
void        SetSelectedIndexList (Window* pWindow, int comboID, int index);
int         GetSelectedIndexTable(Window* pWindow, int comboID);
void        SetSelectedIndexTable(Window* pWindow, int comboID, int selectedIndex);
int         GetScrollTable(Window* pWindow, int comboID);
void        SetScrollTable(Window* pWindow, int comboID, int scroll);
void        AddTableRow(Window* pWindow, int comboID, const char* pText[], int optionalIcon);
void        AddTableColumn(Window* pWindow, int comboID, const char* pText, int width);
bool        GetRowStringsFromTable(Window* pWindow, int comboID, int index, const char * output[]);
void        RemoveRowFromTable(Window* pWindow, int comboID, int elementIndex);
void        ResetTable(Window* pWindow, int comboID);
const char* GetWindowTitle(Window* pWindow);
void*       GetWindowData(Window* pWindow);
void        SetWindowData(Window* pWindow, void* pData);
void        GetWindowRect(Window* pWindow, Rectangle* pRectOut);
void        ChangeCursor(Window* pWindow, int cursorID);
void        SetImageCtlMode(Window* pWindow, int comboID, int mode);
void        CallWindowCallbackAndControls(Window* pWindow, int eventType, int parm1, int parm2);
int         GetWindowFlags(Window * pWindow);
void        SetWindowFlags(Window * pWindow, int flags);
int         AddTimer(Window* pWindow, int frequencyMs, int eventFired);
void        DisarmTimer(Window* pWindow, int timerID);
void        ChangeTimer(Window* pWindow, int newFrequencyMs /* = -1 */, int newEventFired /* = -1 */);
int         UploadCursor(Image* pImage, int xOff, int yOff);
void        ReleaseCursor(int cursorID);
Image*      GetIconImage(int iconType, int size /* = -1 */);
Resource*   GetResource(int resID);
Point       GetMousePos();
void        SetImageCtlMode(Window* pWindow, int comboID, int mode);
void        SetImageCtlColor(Window* pWindow, int comboID, uint32_t color);
void        SetImageCtlCurrentImage(Window* pWindow, int comboID, Image* pImage);
Image*      GetImageCtlCurrentImage(Window* pWindow, int comboID);
void        ImageCtlZoomToFill(Window* pWindow, int comboID);
void        SetControlDisabled(Window* pWindow, int comboID, bool flag);
void        SetControlFocused (Window* pWindow, int comboID, bool flag);
void        SetControlVisible (Window* pWindow, int comboID, bool flag);

// Internal C Compiler
int CcRunCCode(const char* pCode, int length);

// NanoShell Versioning
int NsGetVersion ();
const char* GetVersionString();

// Shell
int ShellExecute        (const char *pCommand);    //for instance, "e <your favorite executable>"
int ShellExecuteResource(const char *pResourceID); //for instance, shell:stuff

// Errors
int  SetErrorNumber(int en);
int  GetErrorNumber();
int* GetErrorNumberPointer();
#define ErrorNumber (*GetErrorNumberPointer())

// Kernel memory resource management.
// NOTE: Improper management of kernel memory resources will cause a leak that
// will persist over the rest of the OS' runtime, so use carefully!!!!
void* MmKernelAllocate(size_t sz);

//note: Do not feed this function addresses from malloc().
void MmKernelFree(void *pData);


#endif//_NANOSHELL___H