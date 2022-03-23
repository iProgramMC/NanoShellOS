/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

           Font renderer module
******************************************/
#include <main.h>
#include <vga.h>
#include <video.h>
#include <memory.h>
#include <string.h>

#include "extra/fonts.h"

//generate a font with, for example, AngelCode's BMFont,
//or Well Oiled Font Maker, or whatever your OS's alternative is
typedef struct
{
	int x, y;            //Coordinates inside texture
	int cwidth, cheight; //Character size inside texture
	int xoffset, yoffset;//Rendering offset
	int xadvance;        //The amount of X to advance
}
CharInfo;

typedef struct
{
	uint8_t   m_width, m_height, m_type;
	CharInfo  m_charInfo[127-32];//include data for the ASCII printables only
	uint8_t*  m_bitmap;          //bitmap data is grayscale 0-255
	uint32_t  m_bmWidth, m_bmHeight;
}
__attribute__((packed))
BitmapFont;

#define MAX_FONTS 16//max amount of bmfonts to load at one time

// Basic definitions
#if 1
	bool g_uses8by16Font = 0;
	extern VBEData g_mainScreenVBEData;
	extern VBEData* g_vbeData;
	extern uint32_t* g_framebufferCopy;
	
	// Font table
	const unsigned char* g_pBasicFontData[] = {
		g_TamsynRegu8x16,
		g_TamsynBold8x16,
		g_TamsynRegu6x12,
		g_TamsynBold6x12,
	};
	
	const unsigned char* g_pCurrentFont = NULL;
#endif

// Pixel plotting
#if 1
	__attribute__((always_inline))
	inline void VidPlotPixelToCopyInlineUnsafeF(unsigned x, unsigned y, unsigned color)
	{
		if (g_vbeData == &g_mainScreenVBEData)
			g_framebufferCopy[x + y * g_vbeData->m_width] = color;
	}
	__attribute__((always_inline))
	inline void VidPlotPixelRaw32IF (unsigned x, unsigned y, unsigned color)
	{
		g_vbeData->m_dirty = 1;
		g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32] = color;
	}
	__attribute__((always_inline))
	inline void VidPlotPixelInlineF(unsigned x, unsigned y, unsigned color)
	{
		if (!((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()))
		{
			VidPlotPixelToCopyInlineUnsafeF(x, y, color);
			VidPlotPixelRaw32IF (x, y, color);
		}
	}
	
	//a VidReadPixel alternative that actually works on anything other than the main screen :)
	//Will be useful for blending pixels
	__attribute__((always_inline))
	inline uint32_t VidReadPixelEx(unsigned x, unsigned y)
	{
		if (!((int)x < 0 || (int)y < 0 || (int)x >= GetScreenSizeX() || (int)y >= GetScreenSizeY()))
		{
			//NB: I check for this specifically to avoid reading from GPU memory. It's slow. Very slow
			if (g_vbeData == &g_mainScreenVBEData)
			{
				return g_framebufferCopy[x + y * g_vbeData->m_width];
			}
			else
			{
				return g_vbeData->m_framebuffer32[x + y * g_vbeData->m_pitch32];
			}
		}
		return -1;
	}
#endif

// Standard font rendering
#if 1 
	__attribute__((always_inline))
	static inline int GetCharWidthInl(char c)
	{
		if (g_pCurrentFont[2] == FONTTYPE_SMALL)
		{
			if (c == 0x5)
				return 16;
			return g_pCurrentFont[3 + 256 * g_pCurrentFont[1] + c];
		}
		else if (g_pCurrentFont[2] == FONTTYPE_BIG)
		{
			if (c > '~' || c < ' ') c = '?';
			return g_pCurrentFont[3 + g_pCurrentFont[1]*2 * (128-32) + c-32]+1;
		}
		else if (g_pCurrentFont[2] == FONTTYPE_MONOSPACE)
		{
			if (c == '\t')
				return 4 * g_pCurrentFont[0];
		}
		
		return g_pCurrentFont[0] + (g_pCurrentFont[2] == FONTTYPE_GLCD);
	}
	int GetCharWidth(char c)
	{
		return GetCharWidthInl (c);
	}
	int GetLineHeight()
	{
		return g_pCurrentFont[1];
	}
	
	void VidSetFont(unsigned fontType)
	{
		if (fontType >= FONT_LAST)
		{
			return;
		}
		g_pCurrentFont  = g_pBasicFontData[fontType];
		g_uses8by16Font = (g_pCurrentFont[1] != 8);
	}
	
	void VidPlotChar (char c, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/)
	{
		if (!g_pCurrentFont) {
			SLogMsg("Darn it (VidPlotChar)!");
			return;
		}
		
		bool bold = false;
		if (colorFg & TEXT_RENDER_BOLD)
		{
			bold = true;
		}
		colorFg &= 0xFFFFFF;
		
		int width = g_pCurrentFont[0], height = g_pCurrentFont[1];
		const unsigned char* test = g_pCurrentFont + 3;
		if (g_pCurrentFont[2] == FONTTYPE_BIG)
		{
			if (c > '~' || c < ' ') c = '?';
			const unsigned char* testa = (const unsigned char*)(g_pCurrentFont + 3);
			for (int y = 0; y < height; y++)
			{
				int to = ((c-' ') * height + y)*2;
				unsigned short test1 = testa[to+1]|testa[to]<<8;
				
				for (int x = 0, bitmask = 1; x < width; x++, bitmask <<= 1)
				{
					if (test1 & bitmask)
					{
						VidPlotPixel(ox + x, oy + y, colorFg);
						if (bold) VidPlotPixel(ox + x + bold, oy + y, colorFg);
					}
					else if (colorBg != TRANSPARENT)
						VidPlotPixel(ox + x, oy + y, colorBg);
				}
			}
		}
		else if (g_pCurrentFont[2] == FONTTYPE_GLCD)
		{
			int x = 0;
			for (x = 0; x < width; x++)
			{
				for (int y = 0, bitmask = 1; y < height; y++, bitmask <<= 1)
				{
					if (test[c * width + x] & bitmask)
					{
						VidPlotPixel(ox + x, oy + y, colorFg);
						if (bold) VidPlotPixel(ox + x + bold, oy + y, colorFg);
					}
					else if (colorBg != TRANSPARENT)
						VidPlotPixel(ox + x, oy + y, colorBg);
				}
			}
			if (colorBg != TRANSPARENT)
				for (int y = 0; y < height; y++)
				{
					VidPlotPixel(ox + x, oy + y, colorBg);
				}
		}
		else
		{
			for (int y = 0; y < height; y++)
			{
				for (int x = 0, bitmask = /*(1 << (width - 1))*/ 1 << 7; x < width; x++, bitmask >>= 1)
				{
					if (test[c * height + y] & bitmask)
					{
						VidPlotPixel(ox + x, oy + y, colorFg);
						if (bold) VidPlotPixel(ox + x + bold, oy + y, colorFg);
					}
					else if (colorBg != TRANSPARENT)
						VidPlotPixel(ox + x, oy + y, colorBg);
				}
			}
		}
	}
	void VidTextOutInternal(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx)
	{
		int x = ox, y = oy;
		int lineHeight = g_pCurrentFont[1];
		
		int width = 0;
		int cwidth = 0, height = lineHeight;
		
		bool bold = false;
		if (colorFg & TEXT_RENDER_BOLD)
		{
			bold = true;
		}
		
		while (*pText)
		{
			//print this character:
			char c = *pText;
			if (c == '\n')
			{
				y += lineHeight;
				height += lineHeight;
				x = ox;
				if (cwidth < width)
					cwidth = width;
				width = 0;
			}
			else
			{
				int cw = GetCharWidthInl(c) + bold;
				
				if (!doNotActuallyDraw)
					VidPlotChar(c, x, y, colorFg, colorBg);
				
				x += cw;
				width += cw;
			}
			pText++;
		}
		if (cwidth < width)
			cwidth = width;
		
		*widthx  = cwidth;
		*heightx = height;
	}
	void VidTextOut(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg)
	{
		UNUSED int a, b;
		VidTextOutInternal (pText, ox, oy, colorFg, colorBg, false, &a, &b);
	}
	
	int CountLinesInText (const char* pText)
	{
		int lc = 1;
		while (*pText)
		{
			if (*pText == '\n') lc++;
			pText++;
		}
		return lc;
	}
	
	static int MeasureTextUntilNewLineI (const char* pText, const char** pTextOut, uint32_t flags)
	{
		bool bold = (flags & TEXT_RENDER_BOLD) != 0;
		int w = 0;
		while (1)
		{
			if (*pText == '\n' || *pText == '\0')
			{
				*pTextOut = pText;
				return w;
			}
			int cw = GetCharWidthInl(*pText) + bold;
			w += cw;
			pText++;
		}
	}
	int MeasureTextUntilNewLine (const char* pText, const char** pTextOut)
	{
		return MeasureTextUntilNewLineI(pText, pTextOut, 0x00000000);
	}
	
	int MeasureTextUntilSpace (const char* pText, const char** pTextOut)
	{
		int w = 0;
		while (1)
		{
			if (*pText == '\n' || *pText == '\0' || *pText == ' ')
			{
				*pTextOut = pText;
				return w;
			}
			int cw = GetCharWidthInl(*pText);
			w += cw;
			pText++;
		}
	}
	
	// Makes the text fit in a rectangle of `xSize` width and `ySize` height,
	// and puts it in pTextOut.
	// Make sure sizeof(pTextOut passed) >= sizeof (stringIn)+5 and that xSize is sufficiently large.
	// Returns the y height attained.
	int WrapText(char *pTextOut, const char* text, int xSize)
	{
		char* pto = pTextOut;
		const char* text2;
		int lineHeight = g_pCurrentFont[1];
		int x = 0, y = lineHeight;
		while (1)
		{
			int widthWord = MeasureTextUntilSpace (text, &text2);
			//can fit?
			if (x + widthWord > xSize)
			{
				//nope. Line wrap
				x = 0;
				y += lineHeight;
				*pto++ = '\n';
			}
			
			while (text != text2)
				*pto++ = *text++;
			
			if (*text2 == '\n')
			{
				x = 0;
				y += lineHeight;
				*pto++ = '\n';
			}
			if (*text2 == ' ')
			{
				*pto++ = ' ';
			}
			if (*text2 == '\0')
			{
				*pto = 0;
				return y;
			}
			text = text2 + 1;
		}
		*pto = 0;
		return y;
	}
	
	void VidDrawText(const char* pText, Rectangle rect, unsigned drawFlags, unsigned colorFg, unsigned colorBg)
	{
		int lineHeight = g_pCurrentFont[1];
		const char* text = pText, *text2 = pText;
		int lines = CountLinesInText(pText);
		int startY = rect.top;
		
		bool bold = false;
		if (colorFg & TEXT_RENDER_BOLD)
		{
			bold = true;
		}
		
		if (drawFlags & TEXTSTYLE_VCENTERED)
			startY += ((rect.bottom - rect.top - lines * lineHeight) / 2);
		
		if (drawFlags & TEXTSTYLE_WORDWRAPPED)
		{
			if (drawFlags & (TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED | TEXTSTYLE_RJUSTIFY))
			{
				//draw some red text to attract the programmer's attention
				VidTextOut ("Can't do everything at once! >:( -- it's still a todo.  Just centering for now.", rect.left, rect.top, 0xFF0000, TRANSPARENT);
			}
			else
			{
				int x = rect.left, y = rect.top;
				while (1)
				{
					
					int widthWord = MeasureTextUntilSpace (text, &text2);
					//can fit?
					if (x + widthWord > rect.right)
					{
						//nope. Line wrap
						y += lineHeight;
						x = rect.left;
					}
					
					while (text != text2)
					{
						VidPlotChar(*text, x, y, colorFg, colorBg);
						int cw = GetCharWidthInl(*text) + bold;
						x += cw;
						text++;
					}
					if (*text2 == '\n')
					{
						x = rect.left;
						y += lineHeight;
					}
					if (*text2 == ' ')
					{
						int cw = GetCharWidthInl(' ') + bold;
						x += cw;
					}
					if (*text2 == '\0') return;
					text = text2 + 1;
				}
			}
		}
		
		for (int i = 0; i < lines; i++)
		{
			int t = MeasureTextUntilNewLineI (text, &text2, colorFg);
			
			int startX = rect.left;
			
			if (drawFlags & (TEXTSTYLE_HCENTERED | TEXTSTYLE_RJUSTIFY))
			{
				if (drawFlags & TEXTSTYLE_HCENTERED)
					startX += (rect.right - rect.left - t) / 2;
				else
					startX += (rect.right - rect.left - t);
			}
			
			while (text != text2)
			{
				VidPlotChar(*text, startX, startY, colorFg, colorBg);
				int cw = GetCharWidthInl(*text) + bold;
				startX += cw;
				text++;
			}
			startY += lineHeight;
			
			text = text2 + 1;
		}
	}
#endif
