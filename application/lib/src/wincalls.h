
// System CALL V1
#if WCALL_VERSION >= 10
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
#endif

// System CALL V1.1
#if WCALL_VERSION >= 11

// Window stuff
CALL (RequestRepaint, WIN_REQUEST_REPAINT, void, Window* pWindow)
	SARGS(pWindow)
CALL_END
CALL (SetLabelText, WIN_SET_LABEL_TEXT, void, Window* pWindow, int comboID, const char* pText)
	SARGS(pWindow, comboID, pText)
CALL_END
CALL (AddMenuBarItem, WIN_ADD_MENUBAR_ITEM, void, Window* pWindow, int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText)
	SARGS(pWindow, menuBarControlId, comboIdTo, comboIdAs, pText)
CALL_END
CALL (SetScrollBarMin, WIN_SET_SCROLL_BAR_MIN, void, Window *pWindow, int comboID, int min)
	SARGS(pWindow, comboID, min)
CALL_END
CALL (SetScrollBarMax, WIN_SET_SCROLL_BAR_MAX, void, Window *pWindow, int comboID, int max)
	SARGS(pWindow, comboID, max)
CALL_END
CALL (SetScrollBarPos, WIN_SET_SCROLL_BAR_POS, void, Window *pWindow, int comboID, int pos)
	SARGS(pWindow, comboID, pos)
CALL_END
CALL (GetScrollBarPos, WIN_GET_SCROLL_BAR_POS, int, Window *pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL (AddElementToList, WIN_ADD_ELEM_TO_LIST, void, Window* pWindow, int comboID, const char* pText, int optionalIcon)
	SARGS(pWindow, comboID, pText, optionalIcon)
CALL_END
CALL (RemoveElementFromList, WIN_REM_ELEM_FROM_LIST, void, Window* pWindow, int comboID, int elemIndex)
	SARGS(pWindow, comboID, elemIndex)
CALL_END
CALL (GetElementStringFromList, WIN_GET_ELEM_STR_FROM_LIST, const char*, Window* pWindow, int comboID, int elemIndex)
	RARGS(pWindow, comboID, elemIndex)
CALL_END
CALL (ResetList, WIN_CLEAR_LIST, void, Window* pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END

// Console I/O
CALL (PutString, CON_PUTSTRING, void, const char* pText)
	SARGS(pText)
CALL_END
CALL (ReadChar, CON_READCHAR, char, void)
	RARGS()
CALL_END
CALL (ReadString, CON_READSTR, void, char* pOutBuffer, int maxSize)
	SARGS(pOutBuffer, maxSize)
CALL_END

// Memory allocation
CALL (AllocateDebug, MM_ALLOCATE_D, void*, size_t size, const char* callerFile, int callerLine)
	RARGS(size, callerFile, callerLine)
CALL_END
CALL (Free, MM_FREE, void, void* ptr)
	SARGS(ptr)
CALL_END
CALL (MmDebugDump, MM_DEBUG_DUMP, void, void)
	SARGS()
CALL_END

// File I/O
CALL (FiOpenDebug, FI_OPEN_D, int /* file descriptor or errcode if negative */, const char* pFileName, int oFlag, const char* pSrcFile, int nSrcLine)
	RARGS(pFileName, oFlag, pSrcFile, nSrcLine)
CALL_END
CALL (FiClose, FI_CLOSE, int /* err code */, int fd)
	RARGS(fd)
CALL_END
CALL (FiRead, FI_READ, size_t /* num bytes read */, int fd, void* pBuf, int nBytes)
	RARGS(fd, pBuf, nBytes)
CALL_END
CALL (FiWrite, FI_WRITE, size_t /* num bytes read */, int fd, void* pBuf, int nBytes)
	RARGS(fd, pBuf, nBytes)
CALL_END
CALL (FiTell, FI_TELL, int /* num bytes into file */, int fd)
	RARGS(fd)
CALL_END
CALL (FiTellSize, FI_TELLSIZE, int /* num bytes into file */, int fd)
	RARGS(fd)
CALL_END
CALL (FiSeek, FI_SEEK, int /* err code */, int fd, int offset, int whence)
	RARGS(fd, offset, whence)
CALL_END

CALL (SetHugeLabelText, WIN_SET_HUGE_LABEL_TEXT, void, Window* pWindow, int comboID, const char* pText)
	SARGS(pWindow, comboID, pText)
CALL_END
CALL (SetTextInputText, WIN_SET_INPUT_TEXT_TEXT, void, Window* pWindow, int comboID, const char* pText)
	SARGS(pWindow, comboID, pText)
CALL_END
CALL (SetWindowIcon, WIN_SET_WINDOW_ICON, void, Window* pWindow, int iconID)
	SARGS(pWindow, iconID)
CALL_END
CALL (SetWindowTitle, WIN_SET_WINDOW_TITLE, void, Window* pWindow, const char* pText)
	SARGS(pWindow, pText)
CALL_END
CALL (GetTickCount, TM_GET_TICK_COUNT, int, void)
	RARGS()
CALL_END
CALL (GetTime, TM_GET_TIME, TimeStruct*, void)
	RARGS()
CALL_END
CALL (GetCpuType, CPU_GET_TYPE, const char*, void)
	RARGS()
CALL_END
CALL (GetCpuName, CPU_GET_NAME, const char*, void)
	RARGS()
CALL_END
CALL (GetConsole, CON_GET_CURRENT_CONSOLE, void*, void)
	RARGS()
CALL_END
CALL (RegisterEvent, WIN_REGISTER_EVENT, void, Window* pWindow, short evType, int parm1, int parm2)
	SARGS(pWindow, evType, parm1, parm2)
CALL_END
CALL (RegisterEventInsideWndProc, WIN_REGISTER_EVENT2, void, Window* pWindow, short evType, int parm1, int parm2)
	SARGS(pWindow, evType, parm1, parm2)
CALL_END

CALL (VidBlitImageResize, VID_BLIT_IMAGE_RESIZE, void, Image*pImage, int x, int y, int width, int height)
	SARGS(pImage,x,y, width, height)
CALL_END

CALL (TmSleep, TM_SLEEP, void, int ms)
	SARGS(ms)
CALL_END

CALL (SetIcon, WIN_SET_ICON, void, Window* pWindow, int comboID, int icon)
	SARGS(pWindow,comboID,icon)
CALL_END

CALL (NsGetVersion, TM_SLEEP, int, void)
	RARGS()
CALL_END

#endif
