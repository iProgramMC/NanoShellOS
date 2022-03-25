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
#include <icon.h>

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
		g_PaperMFont8x16,
		g_FamiSans8x8,
		g_BasicFontData,
		g_GlcdData,
		g_TamsynRegu7x14,
		g_TamsynBold7x14,
		g_TamsynRegu6x12,
		g_TamsynBold6x12,
		//g_TestFont16x16,
		//g_TestFont216x16,
	};
	
	BitmapFont* g_pLoadedFontsPool[MAX_FONTS] = {
		NULL
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

// BMFont loading:
#if 1
//WORK: pFntFileData MUST be loaded from a file or otherwise writable as we use
//destructive tokenizing functions.

//Returns a handle to a basic font.
int CreateFont(char* pFntFileData, uint8_t *bitmap, uint32_t imwidth, uint32_t imheight, uint32_t chheight)
{
	CharInfo chinfo [95];
	
	char* pFileData = pFntFileData;
	
	TokenState pNLState;
	memset (&pNLState, 0, sizeof(pNLState));
	char* line;
	int   linen = 0;
	do
	{
		linen++;
		line = Tokenize (&pNLState, pFileData, "\n");
		pFileData = NULL;
		
		if (!line) break;
		
		SLogMsg("Line:%s", line);
		
		//Process the line by tokenizing that
		TokenState lineState;
		memset (&lineState, 0, sizeof(lineState));
		char* insn = Tokenize(&lineState, line, " ");
		
		if (!insn)//blank line?
		{
			SLogMsg("Blank line?");
			continue;
		}
			
		SLogMsg("Insn:%s", insn);
		
		if (strcmp (insn, "char") == 0)
		{
			//Load a CharInfo
			CharInfo info;
			int info_id = 0;
			
			//Load its properties.
			while (1)
			{
				insn = Tokenize(&lineState, NULL, " ");
				if (!insn) break;
				
				char* var = insn, *val = strchr(insn, '=');
				if (!val)//no equals? ignore this token and move on
					continue;
				*val++ = 0;
				
				int vali = atoi (val);
				
				if (strcmp (var, "id") == 0)
					//Set its ID.
					info_id = vali - 32;
				else if (strcmp (var, "x") == 0)
					info.x = vali;
				else if (strcmp (var, "y") == 0)
					info.y = vali;
				else if (strcmp (var, "width") == 0)
					info.cwidth = vali;
				else if (strcmp (var, "height") == 0)
					info.cheight = vali;
				else if (strcmp (var, "xoffset") == 0)
					info.xoffset = vali;
				else if (strcmp (var, "yoffset") == 0)
					info.yoffset = vali;
				else if (strcmp (var, "xadvance") == 0)
					info.xadvance = vali;
				else
					SLogMsg("[Font load] warning: unknown variable '%s' on line %d. Skipping.", var, linen);
			}
			
			chinfo[info_id] = info;
		}
		else
			SLogMsg("[Font load] warning: unknown insn '%s', skipping", insn);
	}
	while (1);
	
	// Search for a free spot in the font pool
	int freeSpot = -1;
	for (size_t i = 0; i < ARRAY_COUNT(g_pLoadedFontsPool); i++)
	{
		if (g_pLoadedFontsPool[i] == NULL)
		{
			freeSpot = i;
			break;
		}
	}
	if (freeSpot < 0)
	{
		//doing this makes CreateFont have a failsafe
		SLogMsg("Could not initialize a font, resorting to a default one.");
		return FONT_BASIC;//Use a basic font
	}
	
	// Setup a basic BMFont structure.
	BitmapFont *pFont = MmAllocate (sizeof (BitmapFont));
	g_pLoadedFontsPool[freeSpot] = pFont;
	
	pFont->m_bmWidth  = imwidth;
	pFont->m_bmHeight = imheight;
	pFont->m_bitmap   = bitmap;
	pFont->m_width    = chheight/2;
	pFont->m_height   = chheight;
	pFont->m_type     = FONTTYPE_BITMAP;
	for (int i = 0; i < 95; i++)
		pFont->m_charInfo [i] = chinfo [i];
	
	return 0xF000 + freeSpot;
}

void KillFont (int fontID)
{
	if (fontID < 0xF000) return;
	fontID -= 0xF000;
	//a fontID<0 check would be redundant
	if ((uint32_t)fontID >= ARRAY_COUNT(g_pLoadedFontsPool)) return;//Can't kill OOB fonts.
	
	MmFree(g_pLoadedFontsPool [fontID]);
	
	if (g_pCurrentFont == (const unsigned char*)g_pLoadedFontsPool [fontID])
		VidSetFont(FONT_BASIC);//use a temporary font for now.
	
	g_pLoadedFontsPool[fontID] = NULL;
}

#endif

// Standard font rendering
#if 1 
	__attribute__((always_inline))
	static inline int GetCharWidthInl(char c)
	{
		if (g_pCurrentFont[2] == FONTTYPE_BITMAP)
		{
			if (c > '~' || c < ' ') c = '?';
			
			BitmapFont* pFont = (BitmapFont*)g_pCurrentFont;
			
			int id = c - ' ';
			return pFont->m_charInfo[id].xadvance;
		}
		else if (g_pCurrentFont[2] == FONTTYPE_SMALL)
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
			int fontType2 = fontType - 0xF000;
			if (fontType2 < 0 || fontType2 > (int)ARRAY_COUNT(g_pLoadedFontsPool))
			{
				SLogMsg("Can't set the font to that! (%d)", fontType);
			}
			else if (g_pLoadedFontsPool [fontType2])
			{
				g_pCurrentFont  = (const unsigned char*) g_pLoadedFontsPool [fontType2];
				g_uses8by16Font = true;
			}
			else
			{
				SLogMsg("Tried to set font to disposed of/non available font?! Can't do that, sorry. Switching to a basic font");
				VidSetFont(FONT_TAMSYN_BOLD);
			}
			return;
		}
		g_pCurrentFont  = g_pBasicFontData[fontType];
		g_uses8by16Font = (g_pCurrentFont[1] != 8);
	}
	
	const char* VidGetFontName(unsigned fontType)
	{
		if (fontType >= FONT_LAST)
		{
			int fontType2 = fontType - 0xF000;
			if (fontType2 < 0 || fontType2 > (int)ARRAY_COUNT(g_pLoadedFontsPool))
			{
				return "Unrecognized Font";
			}
			else if (g_pLoadedFontsPool [fontType2])
			{
				return "User loaded font";
			}
			else
			{
				return "Unloaded Font";
			}
		}
		switch (fontType)
		{
			case FONT_TAMSYN_REGULAR:       return "Tamsyn Regular 8x16";
			case FONT_TAMSYN_BOLD:          return "Tamsyn Bold 8x16";
			case FONT_TAMSYN_MED_REGULAR:   return "Tamsyn Regular 7x14";
			case FONT_TAMSYN_MED_BOLD:      return "Tamsyn Bold 7x14";
			case FONT_TAMSYN_SMALL_REGULAR: return "Tamsyn Regular 6x12";
			case FONT_TAMSYN_SMALL_BOLD:    return "Tamsyn Bold 6x12";
			case FONT_BASIC:                return "NanoShell System font";
			case FONT_GLCD:                 return "GLCD font 6x8";
			case FONT_FAMISANS:             return "Fami Sans font 8x8";
			case FONT_PAPERM:               return "Paper M font 8x16";
		}
		return "Unknown Font";
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
		
		bool trans = colorBg == TRANSPARENT;
		
		int width = g_pCurrentFont[0], height = g_pCurrentFont[1];
		const unsigned char* test = g_pCurrentFont + 3;
		if (g_pCurrentFont[2] == FONTTYPE_BITMAP)
		{
			if (c > '~' || c < ' ') c = '?';
			
			BitmapFont* pFont = (BitmapFont*)g_pCurrentFont;
			
			int id = c - ' ';
			for (int y = 0, yi = pFont->m_charInfo[id].yoffset, ys = pFont->m_charInfo[id].y * pFont->m_bmHeight; y < pFont->m_charInfo[id].cheight; y++, yi++, ys += pFont->m_bmHeight)
			{
				for (int x = 0, xi = pFont->m_charInfo[id].xoffset, xs = pFont->m_charInfo[id].x; x < pFont->m_charInfo[id].cwidth; x++, xi++, xs++)
				{
					//TODO OPTIMIZE! :)
					uint8_t  c = pFont->m_bitmap[ys + xs];
					
					if (c == 255)
					{
						VidPlotPixel (ox+xi, oy+yi, colorFg);
					}
					else
					{
						if (trans) colorBg = VidReadPixelEx(ox+xi, oy+yi);
						
						uint8_t ro = (((colorBg>>16)&255)*(256-c)+((colorFg>>16)&255)*(c))/256;
						uint8_t go = (((colorBg>> 8)&255)*(256-c)+((colorFg>> 8)&255)*(c))/256;
						uint8_t bo = (((colorBg>> 0)&255)*(256-c)+((colorFg>> 0)&255)*(c))/256;
						//VidPlotPixel (ox + xi, oy + yi, ro<<16|go<<8|bo<<0);
						VidPlotPixel (ox+xi, oy+yi, ro<<16|go<<8|bo);
					}
				}
			}
			
			return;
		}
		else if (g_pCurrentFont[2] == FONTTYPE_BIG)
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
			if (c == 0x5)
			{
				RenderIcon(ICON_NANOSHELL16, ox, oy + (g_pCurrentFont[1] - 16) / 2);
				return;
			}
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
