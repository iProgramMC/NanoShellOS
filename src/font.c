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
BitmapCharInfo;

typedef struct
{
	uint8_t*   m_bitmap;          //bitmap data is grayscale 0-255
	uint32_t   m_bmWidth, m_bmHeight;
	ScreenFont m_font;
	BitmapCharInfo m_charInfo[127 - 32];
}
BitmapFont;

#define MAX_FONTS 16//max amount of bmfonts to load at one time

// Basic definitions
#if 1
	bool g_uses8by16Font = 0;
	extern VBEData g_mainScreenVBEData;
	extern VBEData* g_vbeData;
	extern uint32_t* g_framebufferCopy;
	
	// Font table
	ScreenFont* const g_pBasicFontData[] = {
		&g_TamsynRegu8x16,
		&g_TamsynBold8x16,
		&g_PaperMFont8x16,
		&g_FamiSans8x8,
		&g_BasicFontData,
		&g_GlcdData,
		&g_TamsynRegu7x14,
		&g_TamsynBold7x14,
		&g_TamsynRegu6x12,
		&g_TamsynBold6x12,
		//g_TestFont16x16,
		//g_TestFont216x16,
	};
	
	SafeLock g_LoadedFontsPoolLock;
	BitmapFont* g_pLoadedFontsPool[MAX_FONTS] = {
		NULL
	};
	
	ScreenFont* g_pCurrentFont = NULL;
	
	uint32_t g_nCurrentFontID = 0;
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
		if (
			(int)x <  g_vbeData->m_clipRect.left ||
			(int)y <  g_vbeData->m_clipRect.top  ||
			(int)x >= g_vbeData->m_clipRect.right ||
			(int)y >= g_vbeData->m_clipRect.bottom
		)
			return;
		
		VidPlotPixelToCopyInlineUnsafeF(x, y, color);
		VidPlotPixelRaw32IF (x, y, color);
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
	BitmapCharInfo chinfo [95];
	
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
		
		//SLogMsg("Line:%s", line);
		
		//Process the line by tokenizing that
		TokenState lineState;
		memset (&lineState, 0, sizeof(lineState));
		char* insn = Tokenize(&lineState, line, " ");
		
		if (!insn)//blank line?
		{
			SLogMsg("Blank line?");
			continue;
		}
			
		//SLogMsg("Insn:%s", insn);
		
		if (strcmp (insn, "char") == 0)
		{
			//Load a BitmapCharInfo
			BitmapCharInfo info;
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
					;//SLogMsg("[Font load] warning: unknown variable '%s' on line %d. Skipping.", var, linen);
			}
			
			chinfo[info_id] = info;
		}
		else
			;//SLogMsg("[Font load] warning: unknown insn '%s', skipping", insn);
	}
	while (1);
	
	LockAcquire(&g_LoadedFontsPoolLock);
	
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
		LockFree(&g_LoadedFontsPoolLock);
		return FONT_BASIC;//Use a basic font
	}
	
	// Setup a basic BMFont structure.
	BitmapFont *pFont = MmAllocate (sizeof (BitmapFont));
	g_pLoadedFontsPool[freeSpot] = pFont;
	
	LockFree(&g_LoadedFontsPoolLock);
	
	memset (pFont, 0, sizeof *pFont);
	
	/*
	pFont->m_bmWidth  = imwidth;
	pFont->m_bmHeight = imheight;
	pFont->m_bitmap   = bitmap;
	pFont->m_width    = chheight/2;
	pFont->m_height   = chheight;
	pFont->m_type     = FONTTYPE_BITMAP;
	pFont->m_bAlreadyBold = true; // can't embolden further
	for (int i = 0; i < 95; i++)
		pFont->m_charInfo [i] = chinfo [i];
	*/
	
	// Set up the ScreenFont structure.
	ScreenFont *pSF  = &pFont->m_font;
	pSF->m_fontType  = FONTTYPE_BITMAP;
	pSF->m_pFontData = (const uint8_t*) pFont; // really, it's just a pointer
	
	// XXX: This should be fine, since GetCharWithInl only uses the local CharacterData's width field.
	pSF->m_charWidth  = 255;
	
	pSF->m_charHeight   = chheight;
	pSF->m_bAlreadyBold = true;  // can't embolden further
	pSF->m_altFontID    = -1;    // we don't have a bold version
	pSF->m_unicodeTableSize = 0; // TODO: We don't have unicode support right now
	pSF->m_pUnicodeTable    = NULL;
	
	for (int i = 0x20; i < 0x7F; i++)
	{
		BitmapCharInfo* pCInfo = &chinfo[i - 0x20];
		CharacterData* pData = &pSF->m_asciiData[i];
		pData->m_width  = pCInfo->xadvance;
		pData->m_offset = pCInfo->yoffset * pFont->m_bmWidth + pCInfo->xoffset;
	}
	
	// Everything not printable is the replacement char with this type of font.
	for (int i = 0; i < 0x20; i++)
		pSF->m_asciiData[i] = pSF->m_replacementChar;
	
	for (int i = 0x7F; i < 0xFF; i++)
		pSF->m_asciiData[i] = pSF->m_replacementChar;
	
	// Fill in the bitmap part of the BitmapFont structure.
	pFont->m_bmWidth  = imwidth;
	pFont->m_bmHeight = imheight;
	pFont->m_bitmap   = bitmap;
	
	for (int i = 0; i < 95; i++)
		pFont->m_charInfo [i] = chinfo [i];
	
	return 0xF000 + freeSpot;
}

void KillFont (int fontID)
{
	if (fontID < 0xF000) return;
	fontID -= 0xF000;
	
	// a fontID < 0 check would be redundant
	if (fontID >= (int) ARRAY_COUNT(g_pLoadedFontsPool)) return;// can't kill fonts not in the pool..
	
	LockAcquire(&g_LoadedFontsPoolLock);
	
	MmFree(g_pLoadedFontsPool [fontID]);
	
	if (g_pCurrentFont == &g_pLoadedFontsPool[fontID]->m_font)
		VidSetFont(FONT_BASIC);//use a temporary font for now.
	
	g_pLoadedFontsPool[fontID] = NULL;
	
	LockFree(&g_LoadedFontsPoolLock);
}

#endif

// Standard font rendering
#if 1 
	__attribute__((always_inline))
	static inline int GetCharWidthInl(char chr)
	{
		uint8_t cx = (uint8_t)chr;
		int charWidth = g_pCurrentFont->m_asciiData[cx].m_width;
		
		if (chr == '\t')
			charWidth *= 4;
		
		return charWidth;
	}
	
	int GetCharWidth(char c)
	{
		return GetCharWidthInl (c);
	}
	
	int GetLineHeight()
	{
		return g_pCurrentFont->m_charHeight;
	}
	
	unsigned VidSetFont(unsigned fontType)
	{
		if (fontType >= FONT_LAST)
		{
			int fontType2 = fontType - 0xF000;
			if (fontType2 < 0 || fontType2 > (int)ARRAY_COUNT(g_pLoadedFontsPool))
			{
				SLogMsg("Can't set the font to that! (%d)", fontType);
				return VidSetFont(FONT_TAMSYN_BOLD);
			}
			else if (g_pLoadedFontsPool[fontType2])
			{
				g_pCurrentFont  = &g_pLoadedFontsPool[fontType2]->m_font;
				g_uses8by16Font = true;
			}
			else
			{
				SLogMsg("Tried to set font to disposed of/non available font?! Can't do that, sorry. Switching to a basic font");
				return VidSetFont(FONT_TAMSYN_BOLD);
			}
			
			g_nCurrentFontID = fontType;
		}
		else
		{
			g_pCurrentFont  = g_pBasicFontData[fontType];
			g_uses8by16Font = (g_pCurrentFont->m_charHeight != 8);
		}
		unsigned old = g_nCurrentFontID;
		g_nCurrentFontID = fontType;
		return old;
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
	
	CharacterData GetCharacterData(char character)
	{
		unsigned char chr = (unsigned char)character;
		
		//if (chr < 0 || chr > 255) // never true, but would be if `chr` becomes an int
		//	return g_pCurrentFont->m_replacementChar;
		
		return g_pCurrentFont->m_asciiData[chr];
	}
	
	void VidPlotChar (char chr, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg /*=0xFFFFFFFF*/)
	{
		uint8_t c = (uint8_t) chr;
		if (!g_pCurrentFont) {
			SLogMsg("Darn it (VidPlotChar)!");
			return;
		}
		
		bool bold = false;
		if ((colorFg & TEXT_RENDER_BOLD) && !g_pCurrentFont->m_bAlreadyBold)
		{
			bold = true;
		}
		
		colorFg &= 0xFFFFFF;
		
		bool trans = colorBg == TRANSPARENT;
		
		CharacterData chrData = GetCharacterData(chr);
		
		int width  = chrData.m_width;
		int height = g_pCurrentFont->m_charHeight;
		
		if (width > g_pCurrentFont->m_charWidth)
			width = g_pCurrentFont->m_charWidth;
		
		const unsigned char * pCharBytes = &g_pCurrentFont->m_pFontData[chrData.m_offset];
		
		if (g_pCurrentFont->m_fontType == FONTTYPE_BITMAP)
		{
			BitmapFont* pFont = (BitmapFont*)g_pCurrentFont->m_pFontData;
			if (c > '~' || c < ' ') c = '?';
			int id = c - ' ';
			for (int y = 0, yi = pFont->m_charInfo[id].yoffset, ys = pFont->m_charInfo[id].y * pFont->m_bmHeight; y < pFont->m_charInfo[id].cheight; y++, yi++, ys += pFont->m_bmHeight)
			{
				for (int x = 0, xi = pFont->m_charInfo[id].xoffset, xs = pFont->m_charInfo[id].x; x < pFont->m_charInfo[id].cwidth; x++, xi++, xs++)
				{
					//TODO OPTIMIZE! :)
					uint8_t  c = pFont->m_bitmap[ys + xs];
					
					if (c == 255)
					{
						VidPlotPixelInlineF (ox+xi, oy+yi, colorFg);
						if (bold) VidPlotPixelInlineF (ox+xi+bold, oy+yi, colorFg);
					}
					else
					{
#define Blend(ox,xi,oy,yi) {\
							if (trans) colorBg = VidReadPixelEx(ox+xi, oy+yi);\
							uint8_t ro = (((colorBg>>16)&255)*(256-c)+((colorFg>>16)&255)*(c))/256;\
							uint8_t go = (((colorBg>> 8)&255)*(256-c)+((colorFg>> 8)&255)*(c))/256;\
							uint8_t bo = (((colorBg>> 0)&255)*(256-c)+((colorFg>> 0)&255)*(c))/256;\
							VidPlotPixelInlineF (ox+xi, oy+yi, ro<<16|go<<8|bo);\
						}
						
						Blend(ox,xi,oy,yi);
						if (bold)
							Blend(ox,xi+1,oy,yi);
					}
				}
			}
			
			DirtyRectLogger(ox + pFont->m_charInfo[id].xoffset, oy + pFont->m_charInfo[id].yoffset, pFont->m_charInfo[id].cwidth, pFont->m_charInfo[id].cheight);
			
			return;
		}
		else if (g_pCurrentFont->m_fontType == FONTTYPE_BIG)
		{
			for (int y = 0; y < height; y++)
			{
				int to = y * 2;
				unsigned short test1 = pCharBytes[to + 1] | pCharBytes[to] << 8;
				
				for (int x = width - 1, bitmask = (1 << (width - 1)); x >= 0; x--, bitmask >>= 1)
				{
					if (test1 & bitmask)
					{
						VidPlotPixelInlineF(ox + x, oy + y, colorFg);
						if (bold) VidPlotPixelInlineF(ox + x + bold, oy + y, colorFg);
					}
					else if (colorBg != TRANSPARENT)
						VidPlotPixelInlineF(ox + x, oy + y, colorBg);
				}
			}
			
			DirtyRectLogger(ox, oy, width, height);
		}
		else if (g_pCurrentFont->m_fontType == FONTTYPE_GLCD)
		{
			int x = 0;
			
			for (x = g_pCurrentFont->m_charWidth - 1; x >= 0; x--)
			{
				for (int y = 0, bitmask = 1; y < height; y++, bitmask <<= 1)
				{
					if (pCharBytes[x] & bitmask)
					{
						VidPlotPixelInlineF(ox + x, oy + y, colorFg);
						if (bold) VidPlotPixelInlineF(ox + x + bold, oy + y, colorFg);
					}
					else if (colorBg != TRANSPARENT)
						VidPlotPixelInlineF(ox + x, oy + y, colorBg);
				}
			}
			
			if (colorBg != TRANSPARENT)
			{
				for (int y = 0; y < height; y++)
					VidPlotPixelInlineF(ox + x, oy + y, colorBg);
			}
			
			DirtyRectLogger(ox, oy, width, height);
		}
		else
		{
			if (c == 0x5)
			{
				RenderIcon(ICON_NANOSHELL16, ox, oy + (g_pCurrentFont->m_charHeight - 16) / 2);
				
				DirtyRectLogger(ox, oy, 16, 16);
				return;
			}
			
			for (int y = 0; y < height; y++)
			{
				for (int x = width - 1, bitmask = (1 << (8 - width)); x >= 0; x--, bitmask <<= 1)
				{
					if (pCharBytes[y] & bitmask)
					{
						VidPlotPixelInlineF(ox + x, oy + y, colorFg);
						if (bold) VidPlotPixelInlineF(ox + x + bold, oy + y, colorFg);
					}
					else if (colorBg != TRANSPARENT)
						VidPlotPixelInlineF(ox + x, oy + y, colorBg);
				}
			}
			
			DirtyRectLogger(ox, oy, width, height);
		}
	}
	
	// Yes, we really have to do this, because VidTextOutInternal is exposed as a system call...
	void VidTextOutInternalEx(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx, int limit)
	{
		int x = ox, y = oy;
		int lineHeight = g_pCurrentFont->m_charHeight;
		
		int width = 0;
		int cwidth = 0, height = lineHeight;
		
		bool bReachedLimit = limit == 0; // it counts as already having reached a limit if the limit is zero
		bool bold = false;
		if ((colorFg & TEXT_RENDER_BOLD) && !g_pCurrentFont->m_bAlreadyBold)
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
				
				if (!bReachedLimit && width + 10 >= limit)
				{
					// continue with just '...'
					pText = "...";
					bReachedLimit = true;
					continue;
				}
			}
			pText++;
		}
		if (cwidth < width)
			cwidth = width;
		
		*widthx  = cwidth;
		*heightx = height;
	}
	
	void VidTextOutInternal(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, bool doNotActuallyDraw, int* widthx, int* heightx)
	{
		VidTextOutInternalEx(pText, ox, oy, colorFg, colorBg, doNotActuallyDraw, widthx, heightx, 0);
	}
	
	void VidTextOut(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg)
	{
		UNUSED int a, b;
		VidTextOutInternal (pText, ox, oy, colorFg, colorBg, false, &a, &b);
	}
	
	void VidTextOutLimit(const char* pText, unsigned ox, unsigned oy, unsigned colorFg, unsigned colorBg, int limit)
	{
		UNUSED int a, b;
		VidTextOutInternalEx (pText, ox, oy, colorFg, colorBg, false, &a, &b, limit);
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
		bool bold = ((flags & TEXT_RENDER_BOLD) && !g_pCurrentFont->m_bAlreadyBold);
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
	
	int MeasureTextUntilSpaceOrMaxedWidth (const char* pText, const char** pTextOut, bool* bReachedMaxSizeBeforeEnd, int xMaxSize)
	{
		int w = 0;
		while (1)
		{
			if (*pText == '\n' || *pText == '\0' || *pText == ' ')
			{
				*pTextOut = pText;
				*bReachedMaxSizeBeforeEnd = false;
				return w;
			}
			int cw = GetCharWidthInl(*pText);
			w += cw;
			if (w >= xMaxSize)
			{
				
				*pTextOut = pText;
				*bReachedMaxSizeBeforeEnd = true;
				return w;
			}
			pText++;
		}
	}
	
	// Makes the text fit in a rectangle of `xSize` width and `ySize` height,
	// and puts it in pTextOut.
	// Make sure sizeof(pTextOut passed) >= sizeof (stringIn)+5 and that xSize is sufficiently large.
	// Returns the y height attained.
	int WrapText(char *pTextOut, const char* text, int xSize)
	{
		bool bReachedMaxSizeBeforeEnd = false;
		char* pto = pTextOut;
		const char* text2;
		int lineHeight = g_pCurrentFont->m_charHeight;
		int x = 0, y = lineHeight;
		while (1)
		{
			int widthWord = MeasureTextUntilSpaceOrMaxedWidth (text, &text2, &bReachedMaxSizeBeforeEnd, xSize);
			//can fit?
			if (x + widthWord > xSize)
			{
				//nope. Line wrap
				x = 0;
				y += lineHeight;
				*pto++ = '\n';
			}
			
			x += widthWord + GetCharWidthInl(' ');
			
			while (text != text2)
				*pto++ = *text++;
			
			if (bReachedMaxSizeBeforeEnd)
			{
				text2--;
				bReachedMaxSizeBeforeEnd = false;
			}
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
		int lineHeight = g_pCurrentFont->m_charHeight;
		const char* text = pText, *text2 = pText;
		int lines = CountLinesInText(pText);
		int startY = rect.top;
		
		bool bold = false;
		if ((colorFg & TEXT_RENDER_BOLD) && !g_pCurrentFont->m_bAlreadyBold)
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
