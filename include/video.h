/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

         Video module header file
******************************************/
#ifndef _VIDEO_H
#define _VIDEO_H

#include <multiboot.h>
#include <mouse.h>

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
	FONT_BIGTEST,
	FONT_LAST,
};

enum
{
	FONTTYPE_MONOSPACE,
	FONTTYPE_SMALL,//varying width
	FONTTYPE_GLCD,
	FONTTYPE_BIG,
};

#define TRANSPARENT 0xFFFFFFFF

#define CLICK_INFO_MAX 256
typedef struct
{
	uint16_t width, height;
	int16_t leftOffs, topOffs;
	uint32_t* bitmap;
	bool m_transparency;//optimization
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
 * Handler routine to update mouse data.
 */
void OnUpdateMouse (uint8_t flags, uint8_t dx, uint8_t dy, uint8_t dz);


#endif//_VIDEO_H