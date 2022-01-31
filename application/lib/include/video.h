#ifndef _VIDEO_H
#define _VIDEO_H

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
	FONT_LAST,
};

#define TRANSPARENT 0xFFFFFFFF

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
 * Gets the screen width.
 */
int GetScreenSizeX();

/**
 * Gets the screen height.
 */
int GetScreenSizeY();

/**
 * Gets the width (distance between right and left) of the rectangle.
 */
int GetWidth(Rectangle* rect);

/**
 * Gets the height (distance between bottom and top) of the rectangle.
 */
int GetHeight(Rectangle* rect);

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
 * Blits an image onto the screen.  VidBlitImageResize automatically optimizes this to a VidBlitImage
 * if the width and height specified match the image width and height.
 */
void VidBlitImage(Image* pImage, int x, int y);
void VidBlitImageResize(Image* pImage, int x, int y, int w, int h);

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
 * Forces the mouse position to go somewhere.
 */
void SetMousePos (unsigned pX, unsigned pY);

/**
 * Handler routine to update mouse data.
 */
void OnUpdateMouse (uint8_t flags, uint8_t dx, uint8_t dy, uint8_t dz);


#endif//_VIDEO_H