// nanoshell/graphics.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _NANOSHELL_GRAPHICS__H
#define _NANOSHELL_GRAPHICS__H

#include <nanoshell/graphics_types.h>

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

#endif//_NANOSHELL_GRAPHICS__H