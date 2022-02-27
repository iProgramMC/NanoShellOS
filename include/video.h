/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

         Video module header file
******************************************/
#ifndef _VIDEO_H
#define _VIDEO_H

#include <multiboot.h>
#include <mouse.h>

//Bochs GFX data
#define VBE_DISPI_BANK_ADDRESS          0xA0000
#define VBE_DISPI_BANK_SIZE_KB          64

#define VBE_DISPI_MAX_XRES              1024
#define VBE_DISPI_MAX_YRES              768

#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF

#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_ID0                   0xB0C0
#define VBE_DISPI_ID1                   0xB0C1
#define VBE_DISPI_ID2                   0xB0C2
#define VBE_DISPI_ID3                   0xB0C3
#define VBE_DISPI_ID4                   0xB0C4
#define VBE_DISPI_ID5                   0xB0C5

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xFD000000//<-- Only used if multiboot didn't set the video mode for us.

/**
 * An enum with the font types available.
 */
enum {
	FONT_TAMSYN_REGULAR,
	FONT_TAMSYN_BOLD,
	FONT_PAPERM,
	FONT_FAMISANS,
	FONT_BASIC,
	FONT_GLCD,
	FONT_TAMSYN_MED_REGULAR,
	FONT_TAMSYN_MED_BOLD,
	FONT_TAMSYN_SMALL_REGULAR,
	FONT_TAMSYN_SMALL_BOLD,
	//FONT_BIGTEST,
	//FONT_BIGTEST2,
	FONT_LAST,
};

enum
{
	FONTTYPE_MONOSPACE,
	FONTTYPE_SMALL,//varying width
	FONTTYPE_GLCD,
	FONTTYPE_BIG,
	FONTTYPE_BITMAP,
};

#define TRANSPARENT 0xFFFFFFFF

#define CLICK_INFO_MAX 256
typedef struct
{
	uint16_t width, height;
	int16_t leftOffs, topOffs;
	const uint32_t* bitmap;
	bool m_transparency;//optimization
	
	bool m_resizeMode;
	uint16_t boundsWidth, boundsHeight;
}
Cursor;

enum
{
	CLICK_LEFT,
	CLICK_RIGHT,
	CLICK_LEFTD,
	CLICK_LEFTR,
};

typedef struct
{
	int clickType, clickedAtX, clickedAtY;
}
ClickInfo;

typedef struct
{
	int left, top, right, bottom;
}
Rectangle;

typedef struct
{
	int x, y;
}
Point;

typedef struct
{
	short width, height;
	const uint32_t *framebuffer;
}
Image;

typedef struct
{
	bool     m_available;			    //if the vbe display is available
	unsigned m_width, m_height, m_pitch;//bytes per row
	int      m_bitdepth;                //bits per pixel, only values we support: 0=8, 1=16, 2=32
	bool     m_dirty;					//useful if the framebuffer won't directly be pushed to the screen
	union {
		uint32_t* m_framebuffer32; //for ease of addressing
		uint16_t* m_framebuffer16;
		uint8_t * m_framebuffer8;
	};
	int m_pitch32, m_pitch16;      //uint32_t's and uint16_t's per row.
}
VBEData;

#define FLAGS_TOO(flags, color) (flags | (color & 0XFFFFFF))

#define TEXT_RENDER_TRANSPARENT 0xFFFFFFFF
#define TEXT_RENDER_BOLD        0x01000000

/**
 * Sets the current VBE data, or NULL for the mainscreen.
 */
void VidSetVBEData(VBEData* pData);

/**
 * Gets the width of the current VBE context.
 */
int GetScreenSizeX();

/**
 * Gets the height of the current VBE context.
 */
int GetScreenSizeY();

/**
 * Gets the screen width.
 */
int GetScreenWidth();

/**
 * Gets the screen height.
 */
int GetScreenHeight();

/**
 * Gets the width (distance between right and left) of the rectangle.
 */
int GetWidth(Rectangle* rect);

/**
 * Gets the height (distance between bottom and top) of the rectangle.
 */
int GetHeight(Rectangle* rect);

/**
 * Initializes the graphics API based on the multiboot info.
 */
void VidInitialize (multiboot_info_t* pInfo);

/**
 * Attempts to change the screen resolution.
 */
bool VidChangeScreenResolution(int xSize, int ySize);
bool BgaChangeScreenResolution(int xSize, int ySize);//<-- raw version, do not use

/**
 * Plots a single pixel on the screen.
 */
void VidPlotPixel(unsigned x, unsigned y, unsigned color);

/**
 * Fills the screen with a certain color.
 */
void VidFillScreen(unsigned color);

/**
 * Draws a vertical line of 1px thickness.
 */
void VidDrawVLine(unsigned color, int top, int bottom, int x);

/**
 * Draws a horizontal line of 1px thickness.
 */
void VidDrawHLine(unsigned color, int left, int right, int y);

/**
 * Draws a line of 1px thickness.
 */
void VidDrawLine(unsigned p, int x1, int y1, int x2, int y2);

/**
 * Sets the current screen font.
 */
void VidSetFont(unsigned fontType);

/**
 * Draws a character in "colorFg" with an optional colorBg (if it's 0xFFFFFFFF we don't draw any).
 */
void VidPlotChar (char c, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/);

/**
 * Blits an image onto the screen.
 */
void VidBlitImage(Image* pImage, int x, int y);

/**
 * Blits an image onto the screen, re-sizing it to widthXheight pixels.
 */
void VidBlitImageResize(Image* p, int gx, int gy, int width, int height);

/**
 * Prints a string in "colorFg" with an optional colorBg (if it's 0xFFFFFFFF we don't draw any).
 */
void VidTextOut(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/);

/**
 * Prints a string in "colorFg" with an optional colorBg (if it's 0xFFFFFFFF we don't draw any).
 * Requires widthx and widthy to be valid pointers to integers.  Use this to measure text.
 */
void VidTextOutInternal(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx);

/**
 * Makes the text fit in a rectangle of `xSize` width and `ySize` height,
 * and puts it in pTextOut.
 * Make sure sizeof(pTextOut passed) >= sizeof (stringIn)+5 and that xSize is sufficiently large.
 * Returns the y height attained.
 */
int WrapText(char *pTextOut, const char* pTextToWrap, int xSize);

/**
 * Draws text inside a rectangle with the specified flags.
 */
#define TEXTSTYLE_HCENTERED 1
#define TEXTSTYLE_VCENTERED 2
#define TEXTSTYLE_WORDWRAPPED 4
//TODO: Add word wrap
void VidDrawText(const char* pText, Rectangle rect, unsigned drawFlags, unsigned colorFg, unsigned colorBg);

/**
 * Shifts the screen up, by a certain amount of pixels.  Anything larger than the screen height will
 * effectively clear the screen.
 */
void VidShiftScreen (int amount);

/**
 * Fills a rectangle on the screen.  The ranges of pixels are all inclusive, so
 * pixels[right][bottom] is also getting drawn.
 */
void VidFillRect(unsigned color, int left, int top, int right, int bottom);

/**
 * Draws a rectangle's contour on the screen.  The ranges of pixels are all inclusive, so
 * pixels[right][bottom] is also getting drawn.
 */
void VidDrawRect(unsigned color, int left, int top, int right, int bottom);

/**
 * Fills a rectangle on the screen.  The ranges of pixels are all inclusive, so
 * pixels[rect.right][rect.bottom] is also getting drawn.
 */
void VidFillRectangle(unsigned color, Rectangle rect);

/**
 * Fills a rectangle on the screen.  The ranges of pixels are all inclusive, so
 * pixels[rect.right][rect.bottom] is also getting drawn.
 * This fills in a left-to-right gradient from colorL to colorR.
 */
void VidFillRectHGradient(unsigned colorL, unsigned colorR, int left, int top, int right, int bottom);

/**
 * Fills a rectangle on the screen.  The ranges of pixels are all inclusive, so
 * pixels[rect.right][rect.bottom] is also getting drawn.
 * This fills in a top-to-bottom gradient from colorU to colorD.
 */
void VidFillRectVGradient(unsigned colorU, unsigned colorD, int left, int top, int right, int bottom);

/**
 * Draws a rectangle's contour on the screen.  The ranges of pixels are all inclusive, so
 * pixels[rect.right][rect.bottom] is also getting drawn.
 */
void VidDrawRectangle(unsigned color, Rectangle rect);

/**
 * Checks if the video subsystem is available and has been initialized correctly.
 */
bool VidIsAvailable();

/**
 * Sets the cursor to be visible.
 */
void SetMouseVisible(bool cursor);

/**
 * Resets the current cursor to the default.
 */
void SetDefaultCursor();

/**
 * Sets the cursor to a custom one.
 */
void SetCursor(Cursor* pCursor);

/**
 * Gets the current cursor.
 */
Cursor* GetCurrentCursor();

/**
 * Forces the mouse position to go somewhere.
 */
void SetMousePos (unsigned pX, unsigned pY);

/**
 * Gets the width of a character 'c' in the current font.
 */
int GetCharWidth(char c);

/**
 * Gets the height of a in the current font.
 */
int GetLineHeight();

/**
 * Counts the number of lines in a string of text.
 */
int CountLinesInText (const char* pText);

/**
 * Handler routine to update mouse data.
 */
void OnUpdateMouse (uint8_t flags, uint8_t dx, uint8_t dy, uint8_t dz);

/**
 * Gets the name of a font.
 */
const char* VidGetFontName(unsigned fontType);


#endif//_VIDEO_H