
// Video Call Functions:
CALL (GetScreenSizeX, VID_GET_SCREEN_WIDTH, int)
	RARGS()
CALL_END
CALL (GetScreenSizeY, VID_GET_SCREEN_HEIGHT, int)
	RARGS()
CALL_END
CALL (VidPlotPixel, VID_PLOT_PIXEL, void, unsigned x, unsigned y, unsigned color)
	SARGS(x,y,color)
CALL_END
CALL (VidFillScreen, VID_FILL_SCREEN, void, unsigned color)
	SARGS(color)
CALL_END
CALL (VidDrawVLine, VID_DRAW_V_LINE, void, unsigned color, int top, int bottom, int x)
	SARGS(color,top,bottom,x)
CALL_END
CALL (VidDrawHLine, VID_DRAW_H_LINE, void, unsigned color, int left, int right, int y)
	SARGS(color,left,right,y)
CALL_END
CALL (VidDrawLine, VID_DRAW_LINE, void, unsigned p, int x1, int y1, int x2, int y2)
	SARGS(p,x1,y1,x2,y2)
CALL_END
CALL (VidSetFont, VID_SET_FONT, void, unsigned fontType)
	SARGS(fontType)
CALL_END
CALL (VidPlotChar, VID_PLOT_CHAR, void, char c,unsigned ox,unsigned oy,unsigned colorFg,unsigned colorBg)
	SARGS(c,ox,oy,colorFg,colorBg)
CALL_END
CALL (VidBlitImage, VID_BLIT_IMAGE, void, Image*pImage, int x, int y)
	SARGS(pImage,x,y)
CALL_END
CALL (VidTextOut, VID_TEXT_OUT, void, const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg)
	SARGS(pText,ox,oy,colorFg,colorBg)
CALL_END
CALL (VidTextOutInternal, VID_TEXT_OUT, void, const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* width, int* height)
	SARGS(pText,ox,oy,colorFg,colorBg,doNotActuallyDraw,width,height)
CALL_END
CALL (VidDrawText, VID_DRAW_TEXT, void, const char* pText, Rectangle rect, unsigned drawFlags, unsigned colorFg, unsigned colorBg)
	SARGS(pText, rect, drawFlags, colorFg, colorBg)
CALL_END
CALL (VidShiftScreen, VID_SHIFT_SCREEN, void, int amount)
	SARGS(amount)
CALL_END
CALL (VidFillRect, VID_FILL_RECT, void, unsigned color, int left, int top, int right, int bottom)
	SARGS(color,left,top,right,bottom)
CALL_END
CALL (VidDrawRect, VID_DRAW_RECT, void, unsigned color, int left, int top, int right, int bottom)
	SARGS(color,left,top,right,bottom)
CALL_END
CALL (VidFillRectHGradient, VID_FILL_RECT_H_GRADIENT, void, unsigned colorL, unsigned colorR, int left, int top, int right, int bottom)
	SARGS(colorL,colorR,left,top,right,bottom)
CALL_END
CALL (VidFillRectVGradient, VID_FILL_RECT_V_GRADIENT, void, unsigned colorU, unsigned colorD, int left, int top, int right, int bottom)
	SARGS(colorU,colorD,left,top,right,bottom)
CALL_END

// Window Call Functions:
CALL (CreateWindow, WIN_CREATE, Window*, const char*pTitle, int xPos, int yPos, int xSize, int ySize, WindowProc proc, int flags)
	RARGS(pTitle, xPos, yPos, xSize, ySize, proc, flags)
CALL_END

CALL (DestroyWindow, WIN_DESTROY, void, Window* pWindow)
	SARGS(pWindow)
CALL_END

CALL (DefaultWindowProc, WIN_DEFAULT_PROC, void, Window* pWindow, int messageType, int parm1, int parm2)
	SARGS(pWindow, messageType, parm1, parm2)
CALL_END

CALL (MessageBox, WIN_MESSAGE_BOX, int, Window* pWindow, const char* pText, const char* pCaption, uint32_t type)
	RARGS(pWindow, pText, pCaption, type)
CALL_END

CALL (AddControl, WIN_ADD_CONTROL, int, Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2)
	RARGS(pWindow, type, rect, text, comboID, p1, p2)
CALL_END

CALL (HandleMessages, WIN_HANDLE_MESSAGES, bool, Window* pWindow)
	RARGS(pWindow)
CALL_END
