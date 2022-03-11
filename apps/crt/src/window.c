//
// crt.c
//
// Copyright (C) 2022 iProgramInCpp.
//
// The standard NanoShell library internal implementation
// -- Video and Window API
//

#include "crtlib.h"
#include "crtinternal.h"

int GetScreenSizeX()
{
	return _I_GetScreenSizeX();
}

int GetScreenSizeY()
{
	return _I_GetScreenSizeY();
}

int GetWidth(Rectangle* rect)
{
	return rect->right - rect->left;
}

int GetHeight(Rectangle* rect)
{
	return rect->bottom - rect->top;
}

void VidPlotPixel(unsigned x, unsigned y, unsigned color)
{
	_I_VidPlotPixel(x, y, color);
}

void VidFillScreen(unsigned color)
{
	_I_VidFillScreen(color);
}

void VidDrawVLine(unsigned color, int top, int bottom, int x)
{
	_I_VidDrawVLine(color, top, bottom, x);
}

void VidDrawHLine(unsigned color, int left, int right, int y)
{
	_I_VidDrawHLine(color, left, right, y);
}

void VidDrawLine(unsigned p, int x1, int y1, int x2, int y2)
{
	_I_VidDrawLine(p, x1, y1, x2, y2);
}

void VidSetFont(unsigned fontType)
{
	_I_VidSetFont(fontType);
}

void VidPlotChar (char c, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/)
{
	_I_VidPlotChar(c, ox, oy, colorFg, colorBg);
}

void VidBlitImage(Image* pImage, int x, int y)
{
	_I_VidBlitImage(pImage, x, y);
}

void VidBlitImageResize(Image* pImage, int x, int y, int w, int h)
{
	_I_VidBlitImageResize(pImage, x, y, w, h);
}

void VidTextOut(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/)
{
	_I_VidTextOut(pText, ox, oy, colorFg, colorBg);
}

void VidTextOutInternal(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx)
{
	_I_VidTextOutInternal(pText, ox, oy, colorFg, colorBg, doNotActuallyDraw, widthx, heightx);
}

void VidDrawText(const char* pText, Rectangle rect, unsigned drawFlags, unsigned colorFg, unsigned colorBg)
{
	_I_VidDrawText(pText, rect, drawFlags, colorFg, colorBg);
}

void VidShiftScreen (int amount)
{
	_I_VidShiftScreen(amount);
}

void VidFillRect(unsigned color, int left, int top, int right, int bottom)
{
	_I_VidFillRect(color, left, top, right, bottom);
}

void VidDrawRect(unsigned color, int left, int top, int right, int bottom)
{
	_I_VidDrawRect(color, left, top, right, bottom);
}

void VidFillRectangle(unsigned color, Rectangle rect)
{
	_I_VidFillRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidFillRectHGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom)
{
	_I_VidFillRectHGradient(colorL, colorR, left, top, right, bottom);
}

void VidFillRectVGradient(unsigned colorU, unsigned colorD, int left, int top, int right, int bottom)
{
	_I_VidFillRectVGradient(colorU, colorD, left, top, right, bottom);
}

void VidDrawRectangle(unsigned color, Rectangle rect)
{
	_I_VidDrawRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

void SetMousePos (unsigned pX, unsigned pY)
{
	//TODO
}



// Window API
Window* CreateWindow (const char* title, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags)
{
	//TODO: Log windows to destroy
	return _I_CreateWindow (title, xPos, yPos, xSize, ySize, proc, flags);
}

bool HandleMessages(Window* pWindow)
{
	return _I_HandleMessages (pWindow);
}

void DefaultWindowProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	_I_DefaultWindowProc(pWindow, messageType, parm1, parm2);
}

void DestroyWindow (Window* pWindow)
{
	_I_DestroyWindow (pWindow);
}

int MessageBox (Window* pWindow, const char* pText, const char* pCaption, uint32_t type)
{
	return _I_MessageBox (pWindow, pText, pCaption, type);
}

int AddControl(Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2)
{
	return _I_AddControl (pWindow, type, rect, text, comboID, p1, p2);
}

void SetScrollBarMin (Window *pWindow, int comboID, int min)
{
	_I_SetScrollBarMin (pWindow, comboID, min);
}

void SetScrollBarMax (Window *pWindow, int comboID, int max)
{
	_I_SetScrollBarMax (pWindow, comboID, max);
}

void SetScrollBarPos (Window *pWindow, int comboID, int pos)
{
	_I_SetScrollBarPos (pWindow, comboID, pos);
}

int GetScrollBarPos (Window *pWindow, int comboID)
{
	return _I_GetScrollBarPos (pWindow, comboID);
}

void AddElementToList (Window* pWindow, int comboID, const char* pText, int optionalIcon)
{
	_I_AddElementToList( pWindow, comboID, pText, optionalIcon );
}

const char* GetElementStringFromList (Window* pWindow, int comboID, int index)
{
	return _I_GetElementStringFromList ( pWindow, comboID, index );
}

void RemoveElementFromList (Window* pWindow, int comboID, int elemIndex)
{
	_I_RemoveElementFromList ( pWindow, comboID, elemIndex );
}

void ResetList (Window* pWindow, int comboID)
{
	_I_ResetList (pWindow, comboID);
}

void SetLabelText (Window *pWindow, int comboID, const char* pText)
{
	_I_SetLabelText(pWindow, comboID, pText);
}

void AddMenuBarItem (Window* pWindow, int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText)
{
	_I_AddMenuBarItem (pWindow, menuBarControlId, comboIdTo, comboIdAs, pText);
}

void SetHugeLabelText (Window *pWindow, int comboID, const char* pText)
{
	_I_SetHugeLabelText (pWindow, comboID, pText);
}

void SetTextInputText(Window* pWindow, int comboID, const char* pText)
{
	_I_SetTextInputText(pWindow, comboID, pText);
}

void SetWindowIcon (Window* pWindow, int icon)
{
	_I_SetWindowIcon (pWindow, icon);
}

void SetIcon (Window* pWindow, int comboID, int icon)
{
	_I_SetIcon (pWindow, comboID, icon);
}

void SetWindowTitle(Window* pWindow, const char* pTitle)
{
	_I_SetWindowTitle (pWindow, pTitle);
}

void RegisterEvent(Window* pWindow, short evType, int parm1, int parm2)
{
	_I_RegisterEvent (pWindow, evType, parm1, parm2);
}

void RegisterEventInsideWndProc(Window* pWindow, short evType, int parm1, int parm2)
{
	_I_RegisterEventInsideWndProc (pWindow, evType, parm1, parm2);
}

uint32_t GetThemingParameter(int type)
{
	return _I_GetThemingParameter(type);
}

void SetThemingParameter(int type, uint32_t parm)
{
	return _I_SetThemingParameter(type, parm);
}
