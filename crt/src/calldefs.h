//  ***************************************************************
//  calldefs.h - Creation date: 21/04/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

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
CALL (VidTextOutInternal, VID_TEXT_OUT_INT, void, const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* width, int* height)
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
CALLI(PutString, CON_PUTSTRING, void, const char* pText)
	SARGS(pText)
CALL_END
CALLI(ReadChar, CON_READCHAR, char, void)
	RARGS()
CALL_END
CALLI(ReadString, CON_READSTR, void, char* pOutBuffer, int maxSize)
	SARGS(pOutBuffer, maxSize)
CALL_END

// Memory allocation
CALLI(AllocateDebug, MM_ALLOCATE_D, void*, size_t size, const char* callerFile, int callerLine)
	RARGS(size, callerFile, callerLine)
CALL_END
CALLI(Free, MM_FREE, void, void* ptr)
	SARGS(ptr)
CALL_END
CALLI(MmDebugDump, MM_DEBUG_DUMP, void, void)
	SARGS()
CALL_END

// File I/O
CALLI(FiOpenDebug, FI_OPEN_D, int /* file descriptor or errcode if negative */, const char* pFileName, int oFlag, const char* pSrcFile, int nSrcLine)
	RARGS(pFileName, oFlag, pSrcFile, nSrcLine)
CALL_END
CALLI(FiClose, FI_CLOSE, int /* err code */, int fd)
	RARGS(fd)
CALL_END
CALLI(FiRead, FI_READ, int /* num bytes read */, int fd, void* pBuf, int nBytes)
	RARGS(fd, pBuf, nBytes)
CALL_END
CALLI(FiWrite, FI_WRITE, int /* num bytes read */, int fd, void* pBuf, int nBytes)
	RARGS(fd, pBuf, nBytes)
CALL_END
CALLI(FiTell, FI_TELL, int /* num bytes into file */, int fd)
	RARGS(fd)
CALL_END
CALLI(FiTellSize, FI_TELLSIZE, int /* num bytes into file */, int fd)
	RARGS(fd)
CALL_END
CALLI(FiSeek, FI_SEEK, int /* err code */, int fd, int offset, int whence)
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

CALL (NsGetVersion, NS_GET_VERSION, int, void)
	RARGS()
CALL_END

CALL (GetThemingParameter, WIN_GET_THEME_PARM, uint32_t, int type)
	RARGS(type)
CALL_END
CALL (SetThemingParameter, WIN_SET_THEME_PARM, void, int type, uint32_t parm)
	SARGS(type, parm)
CALL_END

// Calls V1.3
CALL (CheckboxSetChecked, WIN_CHECKBOX_SET_CHECKED, void, Window* pWindow, int comboID, bool checked)
	SARGS(pWindow, comboID, checked)
CALL_END
CALL (CheckboxGetChecked, WIN_CHECKBOX_GET_CHECKED, bool, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL (TextInputQueryDirtyFlag, WIN_TEXT_INPUT_QUERY_DIRTY_FLAG, bool, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL (TextInputClearDirtyFlag, WIN_TEXT_INPUT_CLEAR_DIRTY_FLAG, void, Window* pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END
CALL (TextInputGetRawText, WIN_TEXT_INPUT_GET_RAW_TEXT, const char*, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALLI(CcRunCCode, CC_RUN_C_CODE, int, const char* pData, int length)
	RARGS(pData, length)
CALL_END
CALLI(FiUnlinkFile, FI_REMOVE_FILE, int, const char* pName)
	RARGS(pName)
CALL_END
CALL (AddControlEx, WIN_ADD_CONTROL_EX, int, Window* pWindow, int type, int amode, Rectangle rect, const char* text, int comboID, int p1, int p2)
	RARGS(pWindow, type, amode, rect, text, comboID, p1, p2)
CALL_END
CALL (RequestRepaintNew, WIN_REQUEST_REPAINT_NEW, void, Window* pWindow)
	SARGS(pWindow)
CALL_END
CALL (ShellAbout, WIN_SHELL_ABOUT, void, const char* text, int icon)
	SARGS(text, icon)
CALL_END
CALL (InputBox, WIN_INPUT_BOX, char*, Window* pWindow, const char* pPrompt, const char* pCaption, const char* pDefaultText)
	RARGS(pWindow, pPrompt, pCaption, pDefaultText)
CALL_END
CALL (ColorInputBox, WIN_COLOR_BOX, uint32_t, Window* pWindow, const char* pPrompt, const char* pCaption)
	RARGS(pWindow, pPrompt, pCaption)
CALL_END
CALL (PopupWindow, WIN_POPUP_WINDOW, void, Window* pWindow, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags)
	SARGS(pWindow, newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags)
CALL_END
CALL (FilePickerBox, WIN_FILE_CHOOSE_BOX, char *, Window * pWindow, const char * prompt, const char * caption, const char * default_text)
	RARGS(pWindow, prompt, caption, default_text)
CALL_END

// Calls V1.4
CALL (VidSetVbeData, VID_SET_VBE_DATA, VBEData*, VBEData* pData)
	RARGS(pData)
CALL_END
CALLI(FiOpenDirD, FI_OPEN_DIR_D, int, const char* pFileName, const char* srcFile, int srcLine)
	RARGS(pFileName, srcFile, srcLine)
CALL_END
CALLI(FiCloseDir, FI_CLOSE_DIR, int, int dd)
	RARGS(dd)
CALL_END
CALLI(FiReadDir, FI_READ_DIR, int, DirEnt* p, int dd)
	RARGS(p, dd)
CALL_END
CALLI(FiSeekDir, FI_SEEK_DIR, int, int dd, int loc)
	RARGS(dd, loc)
CALL_END
CALLI(FiRewindDir, FI_REWIND_DIR, int, int dd)
	RARGS(dd)
CALL_END
CALLI(FiTellDir, FI_TELL_DIR, int, int dd)
	RARGS(dd)
CALL_END
CALLI(FiStatAt, FI_STAT_AT, int, int dd, const char *pfn, StatResult* pres)
	RARGS(dd, pfn, pres)
CALL_END
CALLI(FiStat, FI_STAT, int, const char *pfn, StatResult* pres)
	RARGS(pfn, pres)
CALL_END
CALL (FiGetCwd, FI_GET_CWD, const char*, void)
	RARGS()
CALL_END
CALLI(FiChDir, FI_CHANGE_DIR, int, const char* p)
	RARGS(p)
CALL_END
CALL (GetWidgetEventHandler, WIN_GET_WIDGET_EVENT_HANDLER, WidgetEventHandler, int type)
	RARGS(type)
CALL_END
CALL (SetWidgetEventHandler, WIN_SET_WIDGET_EVENT_HANDLER, void, Window *pWindow, int comboID, WidgetEventHandler handler)
	SARGS(pWindow, comboID, handler)
CALL_END
CALL (SetImageCtlMode, WIN_SET_IMAGE_CTL_MODE, void, Window *pWindow, int comboID, int mode)
	SARGS(pWindow, comboID, mode)
CALL_END
CALL (SetImageCtlColor, WIN_SET_IMAGE_CTL_COLOR, void, Window *pWindow, int comboID, uint32_t color)
	SARGS(pWindow, comboID, color)
CALL_END
CALL (SetImageCtlCurrentImage, WIN_SET_IMAGE_CTL_IMAGE, void, Window *pWindow, int comboID, Image* pImage)
	SARGS(pWindow, comboID, pImage)
CALL_END
CALL (GetImageCtlCurrentImage, WIN_GET_IMAGE_CTL_IMAGE, Image*, Window *pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL (ImageCtlZoomToFill, WIN_IMAGE_CTL_ZOOM_TO_FILL, void, Window *pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END
CALL (SetFocusedControl, WIN_SET_FOCUSED_CONTROL, void, Window *pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END
CALL (PopupWindowEx, WIN_POPUP_WINDOW_EX, void, const char* newWindowTitle, int newWindowX, int newWindowY, int newWindowW, int newWindowH, WindowProc newWindowProc, int newFlags, void* newData)
	SARGS(newWindowTitle, newWindowX, newWindowY, newWindowW, newWindowH, newWindowProc, newFlags, newData)
CALL_END
CALL (ErrNoStr, ERR_GET_STRING, const char*, int errNum)
	RARGS(errNum)
CALL_END
CALL (GetMousePos, VID_GET_MOUSE_POS, Point, void)
	RARGS()
CALL_END
CALL (VidSetClipRectP, VID_SET_CLIP_RECT, void, Rectangle r)
	SARGS(r)
CALL_END
CALL (CbClear, CB_CLEAR, void, void)
	SARGS()
CALL_END
CALL (CbCopyText, CB_COPY_TEXT, bool, const char *pText)
	RARGS(pText)
CALL_END
CALL (CbCopyBlob, CB_COPY_BLOB, bool, void* pData, size_t sz)
	RARGS(pData, sz)
CALL_END
CALL (CbGetCurrentVariant, CB_GET_CURRENT_VARIANT, ClipboardVariant*, void)
	RARGS()
CALL_END
CALL (CbRelease, CB_RELEASE, void, ClipboardVariant* pVar)
	SARGS(pVar)
CALL_END

// Calls V1.5
CALL (RenderIcon, VID_RENDER_ICON, void, int type, int x, int y)
	SARGS(type, x, y)
CALL_END
CALL (RenderIconOutline, VID_RENDER_ICON_OUTLINE, void, int type, int x, int y, uint32_t color)
	SARGS(type, x, y, color)
CALL_END
CALL (RenderIconForceSize, VID_RENDER_ICON_SIZE, void, int type, int x, int y, int size)
	SARGS(type, x, y, size)
CALL_END
CALL (RenderIconForceSizeOutline, VID_RENDER_ICON_SIZE_OUTLINE, void, int type, int x, int y, int size, uint32_t color)
	SARGS(type, x, y, size, color)
CALL_END
CALL (GetRandom, TM_GET_RANDOM, int, void)
	RARGS()
CALL_END

// Calls V1.6
CALLI(ReAllocateDebug, MM_REALLOCATE_D, void*, void* o, size_t s, const char* f, int l)
	RARGS(o, s, f, l)
CALL_END
CALL (ShellExecute, SH_EXECUTE, int, const char* p)
	RARGS(p)
CALL_END
CALL (ShellExecuteResource, SH_EXECUTE_RESOURCE, int, const char* p)
	RARGS(p)
CALL_END

// Calls V1.7
CALL (MemoryMap, MM_MAP_MEMORY_USER, int, void* pMem, size_t nSize, int pFlags, int mFlags, int fd, size_t off, void **pOut)
	RARGS(pMem, nSize, pFlags, mFlags, fd, off, pOut)
CALL_END
CALL (MemoryUnmap, MM_UNMAP_MEMORY_USER, int, void* pMem, size_t nSize)
	RARGS(pMem, nSize)
CALL_END

// Calls V1.8
CALLI(FiRename, FI_RENAME, int, const char * pfnOld, const char * pfnNew)
	RARGS(pfnOld, pfnNew)
CALL_END
CALLI(FiMakeDir, FI_MAKE_DIR, int, const char * path)
	RARGS(path)
CALL_END
CALLI(FiRemoveDir, FI_REMOVE_DIR, int, const char * path)
	RARGS(path)
CALL_END
CALLI(FiCreatePipe, FI_CREATE_PIPE, int, const char * friendlyName, int fds[2], int oflags)
	RARGS(friendlyName, fds, oflags)
CALL_END
CALLI(FiIoControl, FI_IO_CONTROL, int, int fd, unsigned long request, void * argp)
	RARGS(fd, request, argp)
CALL_END
CALL (CallControlCallback, WIN_CALL_CTL_CALLBACK, void, Window * window, int comboid, int event, int parm1, int parm2)
	SARGS(window, comboid, event, parm1, parm2)
CALL_END
CALL (TextInputSetMode, WIN_TEXT_INPUT_SET_MODE, void, Window* pWindow, int comboID, int mode)
	SARGS(pWindow, comboID, mode)
CALL_END

// Calls V1.9
CALL(GetScrollBarMin, WIN_GET_SCROLL_BAR_MIN, int, Window *pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(GetScrollBarMax, WIN_GET_SCROLL_BAR_MAX, int, Window *pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(GetSelectedIndexList, WIN_GET_SEL_INDEX_LIST, int, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(SetSelectedIndexList, WIN_SET_SEL_INDEX_LIST, void, Window* pWindow, int comboID, int index)
	SARGS(pWindow, comboID, index)
CALL_END
CALL(GetSelectedIndexTable, WIN_GET_SEL_INDEX_TABLE, int, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(SetSelectedIndexTable, WIN_SET_SEL_INDEX_TABLE, void, Window* pWindow, int comboID, int selectedIndex)
	SARGS(pWindow, comboID, selectedIndex)
CALL_END
CALL(GetScrollTable, WIN_GET_SCROLL_TABLE, int, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(SetScrollTable, WIN_SET_SCROLL_TABLE, void, Window* pWindow, int comboID, int scroll)
	SARGS(pWindow, comboID, scroll)
CALL_END
CALL(AddTableRow, WIN_ADD_TABLE_ROW, void, Window* pWindow, int comboID, const char* pText[], int optionalIcon)
	SARGS(pWindow, comboID, pText, optionalIcon)
CALL_END
CALL(AddTableColumn, WIN_ADD_TABLE_COLUMN, void, Window* pWindow, int comboID, const char* pText, int width)
	SARGS(pWindow, comboID, pText, width)
CALL_END
CALL(GetRowStringsFromTable, WIN_GET_ROW_STRINGS_FROM_TABLE, bool, Window* pWindow, int comboID, int index, const char * output[])
	RARGS(pWindow, comboID, index, output)
CALL_END
CALL(RemoveRowFromTable, WIN_REMOVE_ROW_FROM_TABLE, void, Window* pWindow, int comboID, int elementIndex)
	SARGS(pWindow, comboID, elementIndex)
CALL_END
CALL(ResetTable, WIN_RESET_TABLE, void, Window* pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END

// Calls V2.0
CALL(VidReadPixel, VID_READ_PIXEL, unsigned, unsigned x, unsigned y)
	RARGS(x,y)
CALL_END
CALL(CfgGetString, CFG_GET_STRING, const char*, const char* parm)
	RARGS(parm)
CALL_END
CALL(VidGetVbeData, VID_GET_VBE_DATA, VBEData*, void)
	RARGS()
CALL_END

// Calls V2.1
CALL(GetWindowTitle, WIN_GET_WINDOW_TITLE, const char*, Window* pWindow)
	RARGS(pWindow)
CALL_END
CALL(GetWindowData, WIN_GET_WINDOW_DATA, void*, Window* pWindow)
	RARGS(pWindow)
CALL_END
CALL(SetWindowData, WIN_SET_WINDOW_DATA, void, Window* pWindow, void* ptr)
	SARGS(pWindow, ptr)
CALL_END
CALL(GetWindowRect, WIN_GET_WINDOW_RECT, void, Window* pWindow, Rectangle* pRectOut)
	SARGS(pWindow, pRectOut)
CALL_END
CALL(CallWindowCallbackAndControls, WIN_CALL_CALLBACK_AND_CTLS, void, int et, int p1, int p2)
	SARGS(et, p1, p2)
CALL_END
CALL(ChangeCursor, WIN_CHANGE_CURSOR, void, Window* pWindow, int cursorID)
	SARGS(pWindow, cursorID)
CALL_END
CALL(SetWindowFlags, WIN_SET_FLAGS, void, Window* pWindow, int flags)
	SARGS(pWindow, flags)
CALL_END
CALL(GetWindowFlags, WIN_GET_FLAGS, int, Window* pWindow)
	RARGS(pWindow)
CALL_END

// Calls V2.2
CALL(SetMousePos, VID_SET_MOUSE_POS, void, int newX, int newY)
	SARGS(newX, newY)
CALL_END

CALL(AddTimer, WIN_ADD_TIMER, int, Window* pWindow, int frequency, int event)
	RARGS(pWindow, frequency, event)
CALL_END

CALL(DisarmTimer, WIN_DISARM_TIMER, void, Window* pWindow, int timerID)
	SARGS(pWindow, timerID)
CALL_END

CALL(ChangeTimer, WIN_CHANGE_TIMER, void, Window* pWindow, int timerID, int newFreq, int newEvent)
	SARGS(pWindow, timerID, newFreq, newEvent)
CALL_END

// Calls V2.3
CALL(UploadCursor, WIN_UPLOAD_CURSOR, int, Image * pImage, int xOff, int yOff)
	RARGS(pImage, xOff, yOff)
CALL_END
CALL(ReleaseCursor, WIN_UPLOAD_CURSOR, void, int cursorID)
	SARGS(cursorID)
CALL_END
CALL(SetListItemText, WIN_SET_LIST_ITEM_TEXT, void, Window* pWindow, int comboID, int index, int icon, const char * pText)
	SARGS(pWindow, comboID, index, icon, pText)
CALL_END
CALL(GetIconImage, WIN_GET_ICON_IMAGE, Image*, int iconID, int size)
	RARGS(iconID, size)
CALL_END
CALL(GetResource, RST_LOOK_UP_RESOURCE, Resource*, int resID)
	RARGS(resID)
CALL_END
CALL(SetControlDisabled, WIN_SET_CONTROL_DISABLED, void, Window* pWindow, int comboID, bool flag)
	SARGS(pWindow, comboID, flag)
CALL_END
CALL(SetControlFocused, WIN_SET_CONTROL_FOCUSED, void, Window* pWindow, int comboID, bool flag)
	SARGS(pWindow, comboID, flag)
CALL_END
CALL(SetControlVisible, WIN_SET_CONTROL_VISIBLE, void, Window* pWindow, int comboID, bool flag)
	SARGS(pWindow, comboID, flag)
CALL_END
CALL(ProgBarSetProgress, WIN_PROG_BAR_SET_PROGRESS, void, Window* pWindow, int comboID, int x)
	SARGS(pWindow, comboID, x)
CALL_END
CALL(ProgBarSetMaxProg, WIN_PROG_BAR_SET_MAX_PROG, void, Window* pWindow, int comboID, int x)
	SARGS(pWindow, comboID, x)
CALL_END

// Calls V2.4
CALL(ComboBoxAddItem, WIN_COMBO_BOX_ADD_ITEM, void, Window* pWindow, int comboID, const char* item, int itemID, int iconID)
	SARGS(pWindow, comboID, item, itemID, iconID)
CALL_END
CALL(ComboBoxGetSelectedItemID, WIN_COMBO_BOX_GET_SELECTED_ITEM, int, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(ComboBoxSetSelectedItemID, WIN_COMBO_BOX_SET_SELECTED_ITEM, void, Window* pWindow, int comboID, int itemID)
	SARGS(pWindow, comboID, itemID)
CALL_END
CALL(ComboBoxClearItems, WIN_COMBO_BOX_CLEAR_ITEMS, void, Window* pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END
CALL(IsControlFocused, WIN_IS_CONTROL_FOCUSED, bool, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(IsControlDisabled, WIN_IS_CONTROL_DISABLED, bool, Window* pWindow, int comboID)
	RARGS(pWindow, comboID)
CALL_END
CALL(TextInputSetFont, WIN_TEXT_INPUT_SET_FONT, void, Window* pWindow, int comboID, unsigned font)
	SARGS(pWindow, comboID, font)
CALL_END
CALL(TextInputRequestCommand, WIN_TEXT_INPUT_REQUEST_COMMAND, void, Window *pWindow, int comboID, int command, void* parm)
	SARGS(pWindow, comboID, command, parm)
CALL_END

// Calls V2.5
CALL(DrawEdge, WIN_DRAW_EDGE, void, Rectangle rect, int style, unsigned bg)
	SARGS(rect, style, bg)
CALL_END
CALL(DrawArrow, WIN_DRAW_ARROW, void, Rectangle rect, eArrowType arrowType, int flags, unsigned color)
	SARGS(rect, arrowType, flags, color)
CALL_END
CALL(TabViewAddTab, WIN_TAB_VIEW_ADD_TAB, void, Window* pWindow, int comboID, int tabID, const char* pTabText, int tabWidth)
	SARGS(pWindow, comboID, tabID, pTabText, tabWidth)
CALL_END
CALL(TabViewRemoveTab, WIN_TAB_VIEW_REMOVE_TAB, void, Window* pWindow, int comboID, int tabID)
	SARGS(pWindow, comboID, tabID)
CALL_END
CALL(TabViewClearTabs, WIN_TAB_VIEW_REMOVE_TAB, void, Window* pWindow, int comboID)
	SARGS(pWindow, comboID)
CALL_END
CALL(SpawnMenu, WIN_SPAWN_MENU, Window*, Window* pParentWindow, WindowMenu* pRoot, int x, int y)
	RARGS(pParentWindow, pRoot, x, y)
CALL_END
CALL(KbGetKeyState, KB_GET_KEY_STATE, KeyState, unsigned char keycode)
	RARGS(keycode)
CALL_END
CALL(LockAcquire, LCK_ACQUIRE, void, SafeLock* ptr)
	SARGS(ptr)
CALL_END
CALL(LockFree, LCK_FREE, void, SafeLock* ptr)
	SARGS(ptr)
CALL_END

// Calls V2.6
CALLI(FiLinkStat, FI_STAT_LINK, int, const char *pfn, StatResult* pres)
	RARGS(pfn, pres)
CALL_END

// Calls V2.7
CALLI(FiFDStat, FI_STAT_FD, int, int fd, StatResult* pres)
	RARGS(fd, pres)
CALL_END
CALLI(FiChangeMode, FI_CHANGE_MODE, int, const char* pfn, int mod)
	RARGS(pfn, mod)
CALL_END
CALLI(FiChangeTime, FI_CHANGE_TIME, int, const char* pfn, int atime, int mtime)
	RARGS(pfn, atime, mtime)
CALL_END
CALLI(FiFDChangeMode, FI_CHANGE_MODE_FD, int, int fd, int mod)
	RARGS(fd, mod)
CALL_END
CALLI(FiFDChangeTime, FI_CHANGE_TIME_FD, int, int fd, int atime, int mtime)
	RARGS(fd, atime, mtime)
CALL_END
CALLI(FiFDChangeDir, FI_CHANGE_DIR_FD, int, int fd)
	RARGS(fd)
CALL_END

// Calls V2.8
CALL(VidSetClipRectEx, VID_SET_CLIP_RECT_EX, void, Rectangle* pOutRect, Rectangle* pRect)
	SARGS(pOutRect, pRect)
CALL_END
CALL(VidTextOutInternalEx, VID_TEXT_OUT_INT_EX, void, const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx, int limit)
	SARGS(pText, ox, oy, colorFg, colorBg, doNotActuallyDraw, widthx, heightx, limit)
CALL_END
CALL(WrapText, VID_WRAP_TEXT, int, char* pTextBufOut, size_t sTextBufOut, const char* pText, int xWidth)
	RARGS(pTextBufOut, sTextBufOut, pText, xWidth)
CALL_END
CALL(GetCharWidth, VID_GET_CHAR_WIDTH, int, int chr)
	RARGS(chr)
CALL_END
CALL(GetLineHeight, VID_GET_LINE_HEIGHT, int, void)
	RARGS()
CALL_END
