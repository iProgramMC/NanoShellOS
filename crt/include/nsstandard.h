//
// nsstandard.h
//
// Copyright (C) 2022 iProgramInCpp.
//
// The standard NanoShell library
//
#ifndef _NSSTANDARD_H
#define _NSSTANDARD_H

#include <nsstructs.h>

// Formatting
void vsprintf(char* OutBuffer, const char* FormatType, va_list list);
void sprintf (char* OutBuffer, const char* FormatType, ...);

// Threading
void sleep(int ms);
void exit (int errcode);

// Execution control
__attribute__((returns_twice)) int  SetJump (JumpBuffer env);
__attribute__((noreturn))      void LongJump(JumpBuffer env, int value);

// Miscellanea
int GetRandom();

// Logging
void LogMsg    (const char* Format, ...);
void LogMsgNoCr(const char* Format, ...);
#define printf LogMsgNoCr

// Memory management
void* malloc (size_t size);
void  free   (void*  ptr);
void* realloc(void*  ptr, size_t sz);

// Memory and string operations
int    memcmp     (const void* ap, const void* bp, size_t size);
void*  memcpy     (void* restrict dstptr, const void* restrict srcptr, size_t size);
void*  memmove    (void* restrict dstptr, const void* restrict srcptr, size_t size);
void*  memset     (void* bufptr, BYTE val, size_t size);
size_t strlen     (const char* str);
void*  strcpy     (const char* ds, const char* ss);
int    strcmp     (const char* as, const char* bs);
void   strcat     (char* dest, const char* after);
void   strtolower (char* as);
void   strtoupper (char* as);
void   memtolower (char* as, int w);
void   memtoupper (char* as, int w);
size_t strgetlento(const char* str, char chr);
int    atoi       (const char* str);
char*  strdup     (const char *pText);

// Optimized memory operations to word width

//NOTE: size must be 4 byte aligned!!
void  ZeroMemory (void* bufptr1, size_t size);
void  fmemcpy32 (void* restrict dest, const void* restrict src, size_t size);
void* fast_memset(void* bufptr, BYTE val, size_t size);

// File management
int     open  (const char* path, int oflag);
int     close (int fd);
size_t  read  (int fd,       void* buf,    unsigned int nbyte);
size_t  write (int fd, const void* buf,    unsigned int nbyte);
int     lseek (int fd,       int   offset,          int whence);
int     tellf (int fd);
int     tellsz(int fd);

int     FiOpenDir  (const char* pFileName);
int     FiCloseDir (int dd);
DirEnt* FiReadDir  (int dd);
int     FiSeekDir  (int dd,          int loc);
int     FiRewindDir(int dd);
int     FiTellDir  (int dd);
int     FiStatAt   (int dd,         const char*pfn,  StatResult* pres);
int     FiStat     (const char*pfn, StatResult* pres);
int     FiChDir    (const char*pfn);
const char* FiGetCwd();
const char* ErrNoStr(int errno);

// C Standard I/O

FILE*  fopen (const char* file, const char* mode);
int    fclose(FILE* file);
size_t fread (      void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int    fseek (FILE* file, int offset, int whence);
int    ftell (FILE* file);

// File manip. API
int remove (const char* filename);

// Graphics Interface

int  GetScreenSizeX();
int  GetScreenSizeY();
int  GetWidth(Rectangle* rect);
int  GetHeight(Rectangle* rect);
void VidPlotPixel(unsigned x, unsigned y, unsigned color);
void VidFillScreen(unsigned color);
void VidDrawVLine(unsigned color, int top, int bottom, int x);
void VidDrawHLine(unsigned color, int left, int right, int y);
void VidDrawLine(unsigned p, int x1, int y1, int x2, int y2);
void VidSetFont(unsigned fontType);
void VidPlotChar (char c, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/);
void VidBlitImage(Image* pImage, int x, int y);
void VidBlitImageResize(Image* pImage, int x, int y, int w, int h);
void VidTextOut(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/);
void VidTextOutInternal(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx);
void VidDrawText(const char* pText, Rectangle rect, unsigned drawFlags, unsigned colorFg, unsigned colorBg);
void VidShiftScreen (int amount);
void VidFillRect(unsigned color, int left, int top, int right, int bottom);
void VidDrawRect(unsigned color, int left, int top, int right, int bottom);
void VidFillRectangle(unsigned color, Rectangle rect);
void VidFillRectHGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom);
void VidFillRectVGradient(unsigned colorU, unsigned colorD, int left, int top, int right, int bottom);
void VidDrawRectangle(unsigned color, Rectangle rect);
void SetMousePos (unsigned pX, unsigned pY);
void VidSetVbeData (VBEData* pData);
void RenderIcon(int type, int x, int y);
void RenderIconOutline(int type, int x, int y, uint32_t color);
void RenderIconForceSize(int type, int x, int y, int size);
void RenderIconForceSizeOutline(int type, int x, int y, int size, uint32_t color);

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
char*       InputBox (Window *pWindow, const char *pPrompt, const char *pCaption, const char *pDefaultText);
uint32_t    GetThemingParameter (int type);
void        SetThemingParameter (int type, uint32_t parm);
void        SetWidgetEventHandler(Window *pWindow, int comboID, WidgetEventHandler handler);

// Internal C Compiler
int CcRunCCode(const char* pCode, int length);

// NanoShell Versioning
int NsGetVersion ();
const char* GetVersionString();

// Time
TimeStruct *GetTime ();
int GetTickCount();// Time since OS has started

// Shell
int ShellExecute        (const char *pCommand);    //for instance, "e <your favorite executable>"
int ShellExecuteResource(const char *pResourceID); //for instance, shell:stuff

#endif//_NSSTANDARD_H