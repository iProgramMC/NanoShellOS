/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

            Icon listing module
******************************************/
#include <icon.h>
#include <time.h>
#include <image.h>
#include <window.h>
#include <process.h>
#include <vfs.h>

#define C_TRANSPAR TRANSPARENT
typedef uint32_t IconColor;

#include <icons/minimize.h>
#include <icons/maximize.h>
#include <icons/restore.h>
#include <icons/close.h>

Image * g_iconTable[] = {
	NULL,
	#define  ICON(x) NULL,
	#define  ICOB(x) &g_ ## x ## _icon,
	#include "icons.h"
	#undef   ICON
	#undef   ICOB
};

const char* g_iconFileNames[] = {
	NULL,
	#define  ICON(x) "/Res/Icons/" #x ".png",
	#define  ICOB(x) NULL,
	#include "icons.h"
	#undef   ICON
	#undef   ICOB
};

STATIC_ASSERT(ARRAY_COUNT(g_iconTable)     == ICON_COUNT, "Change this array if adding icons.");
STATIC_ASSERT(ARRAY_COUNT(g_iconFileNames) == ICON_COUNT, "Change this array if adding icons.");

bool g_bLoadedIcons = false;

Image* LoadImageFromFile(const char* filename)
{
	int fd = FiOpen(filename, O_RDONLY);
	if (fd < 0)
	{
		SLogMsg("Error, can't load image file %s (%s)", filename, GetErrNoString(fd));
		return NULL;
	}
	
	int size = FiTellSize(fd);
	uint8_t* pData = MmAllocate (size);
	if (!pData)
	{
		SLogMsg("Error, can't allocate data for file %s", filename);
		FiClose(fd);
		return NULL;
	}
	
	FiSeek(fd, 0, SEEK_SET);
	FiRead(fd, pData, size);
	
	FiClose(fd);
	
	int errCode = 0;
	Image* pImg = LoadImageFile(pData, size, &errCode);
	MmFree(pData);
	
	if (!pImg)
		SLogMsg("Error, can't load image file %s (failed to decode with error #%d)", filename, errCode);
	
	return pImg;
}

void LoadIcons()
{
	KeVerifyInterruptsEnabled;
	
	cli;
	
	if (g_bLoadedIcons)
	{
		sti;
		return;
	}
	
	g_bLoadedIcons = true;
	
	sti;
	
	for (int i = 0; i < (int)ARRAY_COUNT(g_iconFileNames); i++)
	{
		if (!g_iconFileNames[i])
			continue;
		
		g_iconTable[i] = LoadImageFromFile(g_iconFileNames[i]);
	}
}

Image *GetIconImageFromResource(int resID)
{
	Resource* pRes = ExLookUpResource(resID);
	
	if (!pRes) return NULL;
	if (pRes->m_type != RES_ICON) return NULL;
	
	return (Image*)pRes->m_data;
}

Image* GetIconImage(IconType type, int sz)
{
	if (type == ICON_NULL) return NULL;
	
	if (type >= ICON_COUNT || type < ICON_NULL)
	{
		return GetIconImageFromResource(type);
	}
	
	if (sz == 16)
	{
		// Convert certain icons to their 16x counterparts:
		switch (type)
		{
			#define CASE(typo) case ICON_ ## typo: type = ICON_ ## typo ## 16; break;
			CASE(CABINET)
			CASE(COMPUTER)
			CASE(ERROR)
			case ICON_FOLDER:      type = ICON_FOLDER16_CLOSED; break;
			case ICON_CLOCK_EMPTY: type = ICON_CLOCK16;         break;
			CASE(FOLDER_PARENT)
			CASE(EXECUTE_FILE)
			CASE(FILE)
			CASE(TEXT_FILE)
			CASE(NANOSHELL)
			CASE(NANOSHELL_LETTERS)
			CASE(COMMAND)
			CASE(SCRAP_FILE)
			CASE(FILE_CSCRIPT)
			CASE(HOME)
			CASE(CLOCK)
			CASE(APPLICATION)
			CASE(CALCULATOR)
			CASE(FONTS)
			CASE(RESMON)
			CASE(NOTES)
			CASE(RUN)
			CASE(CHAIN)
			CASE(SYSMON)
			CASE(FILE_MKDOWN)
			CASE(DEVTOOL)
			CASE(DLGEDIT)
			CASE(MAGNIFY)
			CASE(BOMB_SPIKEY)
			CASE(COMPUTER_SHUTDOWN)
			CASE(ACTION_SAVE)
			CASE(ACTION_OPEN)
			CASE(PASTE)
			CASE(DELETE)
			CASE(COPY)
			CASE(BACK)
			CASE(FORWARD)
			CASE(UNDO)
			CASE(REDO)
			CASE(FILE_SEARCH)
			CASE(FILE_PROPERTIES)
			CASE(PROPERTIES)
			CASE(WHATS_THIS)
			CASE(VIEW_ICON)
			CASE(VIEW_LIST)
			CASE(VIEW_TABLE)
			CASE(SORT_ALPHA)
			CASE(FORM)
			CASE(JOURNAL)
			CASE(PACKAGER)
			CASE(FOLDER_SETTINGS)
			CASE(PIPE)
			CASE(CHESS)
			CASE(CRAYONS)
			CASE(CHAIN_BROKEN)
			CASE(SHORTCUT_OVERLAY)
			#undef CASE
		}
	}
	
	return g_iconTable[type];
}

void RenderIcon(IconType type, int x, int y)
{
	bool bShortcut = type & ICON_SHORTCUT_FLAG;
	type &= ~ICON_SHORTCUT_FLAG;
	
	Image* p = GetIconImage(type, -1);
	if (!p) return;
	VidBlitImage(p, x, y);
	
	if (bShortcut)
		RenderIcon(ICON_SHORTCUT_OVERLAY, x, y);
}

void RenderIconOutline(IconType type, int x, int y, uint32_t color)
{
	Image* p = GetIconImage(type, -1);
	if (!p) return;
	VidBlitImageOutline(p, x, y, color);
}

void RenderThumbClock(int x, int y, int size);

bool IsMonochromeIcon(IconType type)
{
	switch (type)
	{
		case ICON_MINIMIZE:
		case ICON_MAXIMIZE:
		case ICON_CLOSE:
		case ICON_RESTORE:
		return true;
	}
	return false;
}

void RenderIconForceSize(IconType type, int x, int y, int size)
{
	bool bShortcut = type & ICON_SHORTCUT_FLAG;
	type &= ~ICON_SHORTCUT_FLAG;
	
	Image *p = GetIconImage(type, size);
	if (!p) return;
	
	if (IsMonochromeIcon(type))
		VidBlitImageResizeOutline(p, x, y, size, size, CAPTION_BUTTON_ICON_COLOR);
	else
		VidBlitImageResize(p, x, y, size, size);
	
	if (type == ICON_CLOCK_EMPTY)
	{
		RenderThumbClock(x, y, size);
	}
	
	if (bShortcut)
		RenderIconForceSize(ICON_SHORTCUT_OVERLAY, x, y, size);
}


void RenderIconForceSizeOutline(IconType type, int x, int y, int size, uint32_t color)
{
	Image *p = GetIconImage(type, size);
	if (!p) return;
	
	VidBlitImageResizeOutline(p, x, y, size, size, color);
}

// mini clock for one specific icon
const int g_mini_clock_cosa[]={    0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,   0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995,-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105 };
const int g_mini_clock_sina[]={-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105,   0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,    0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995 };

SAI void RenderThumbClockHand(int deg, int len, int cenX, int cenY, unsigned color)
{
	int begPointX = cenX,                                         begPointY = cenY;
	int endPointX = cenX + (g_mini_clock_cosa[deg] * len / 1000), endPointY = cenY + (g_mini_clock_sina[deg] * len / 1000);
	VidDrawLine (color, begPointX, begPointY, endPointX, endPointY);
}

void RenderThumbClock(int x, int y, int size)//=32
{
	if (size == 16) return;
	//render simple clock:
	TimeStruct* time = TmReadTime();
	
	int centerX = x + size/2, centerY = y + size/2;
	int diameter = size;
	int handMaxLength = (2 * diameter / 5);
	
	RenderThumbClockHand(time->hours % 12 * 5 + time->minutes / 12, 4 * handMaxLength / 9, centerX, centerY, 0xFF0000);
	RenderThumbClockHand(time->minutes,                             6 * handMaxLength / 9, centerX, centerY, 0x000000);
	RenderThumbClockHand(time->seconds,                             8 * handMaxLength / 9, centerX, centerY, 0x000000);
}

