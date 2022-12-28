/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

           Window Themes module
******************************************/
#include <wtheme.h>
#include <config.h>
#include <print.h>
#include <vfs.h>

typedef struct
{
	const char* m_name;
	int m_colors[P_THEME_PARM_COUNT];
}
Theme;

const Theme g_themes[] = {
	{
		"Default",
		{
			0x000000,0x007f7f,0xcccccc,0xbbbbbb,0x000000,0x00003f,0x7f7f7f,0x0000ff,0xeeeeee,0x00003f,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	{
		"Dark",
		{
			0x000000,0x00003f,0x303030,0x202020,0x000000,0x000000,0x0f0f0f,0x000044,0x3f3f3f,0x000000,0xffffff,0xffffff,0x000000,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	{
		"Redmond",
		{
			0x000000,0x3a6ea5,0xc0c0c0,0xc0c0c0,0x000000,0x000080,0x808080,0x1084d0,0xb5b5b5,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	{
		"Sandy",
		{
			0x000000,0x3a6ea5,0xECE9D8,0xECE9D8,0x000000,0x0054E3,0x7A96DF,0x3D95FF,0x9DB9EB,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	{
		"Black",
		{
			0x000000,0x000000,0x000000,0x000000,0x000000,0x800080,0x008000,0x800080,0x008000,0x000000,0xffffff,0xffffff,0x000000,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	/*{
		"White",
		{
			0x000000,0xffffff,0xffffff,0xffffff,0x000000,0x000000,0xffffff,0x000000,0xffffff,0x000000,0x808080,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},*/
	{
		"Rose",
		{
			0x000000,0x808080,0xcfafb7,0xcfafb7,0x000000,0x9f6070,0xa0a0a4,0xd8ccd0,0xd0d4d0,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	{
		"Desert",
		{
			0x000000,0xa28d68,0xd5ccbb,0xd5ccbb,0x000000,0x008080,0xa28d68,0x84bdaa,0xe8d080,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	{
		"Rainy Day",
		{
			0x000000,0x000000,0x8399b1,0x8399b1,0x000000,0x4f657d,0x808080,0x80b4d0,0xb0bcd0,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},
	/*
	{
		"Calm",
		{
			0x000000,0x510401,0xe6d8ae,0xe6d8ae,0x000000,0x800000,0xc6a646,0xc09c38,0xe0c888,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT,0x00316AC5,0x00C1D2EE
		}
	},*/
};

void ApplyTheme(int themeNumber)
{
	if (themeNumber < 0 || themeNumber >= (int)ARRAY_COUNT(g_themes)) return;
	SLogMsg("Applying theme %s", g_themes[themeNumber].m_name);
	for (int i = P_BLACK; i <= P_THEME_PARM_COUNT; i++)
	{
		SetThemingParameter(i, g_themes[themeNumber].m_colors[i]);
	}
}

const char* GetThemeName(int themeNumber)
{
	if (themeNumber < 0 || themeNumber >= (int)ARRAY_COUNT(g_themes)) return "Unknown Theme";
	
	return g_themes[themeNumber].m_name;
}

void SetDefaultTheme()
{
	ApplyTheme(TH_DEFAULT);
}

void SetDarkTheme()
{
	ApplyTheme(TH_DARK);
}

uint32_t g_ThemingParms[P_THEME_PARM_COUNT];

uint32_t GetThemingParameter(int type)
{
	if (type < 0 || type >= P_THEME_PARM_COUNT) return 0;
	return g_ThemingParms[type];
}
void SetThemingParameter(int type, uint32_t parm)
{
	if (type < 0 || type >= P_THEME_PARM_COUNT) return;
	g_ThemingParms[type] = parm;
}

uint8_t* ThmLoadEntireFile (const char *pPath, int *pSizeOut)
{
	int fd = FiOpen (pPath, O_RDONLY);
	if (fd < 0) return NULL;
	
	int size = FiTellSize (fd);
	
	uint8_t* pMem = MmAllocate (size + 1);
	int read_size = FiRead (fd, pMem, size);
	pMem[read_size] = 0;//for text parsers
	
	*pSizeOut = read_size;
	return pMem;
}

void ThmLoadFont(ConfigEntry *pEntry)
{
	//load some properties
	ConfigEntry
	* pBmpPath, * pFntPath, * pSysFont, * pTibFont, * pBmpSize, * pChrHeit;
	
	char buffer1[128], buffer2[128], buffer3[128], buffer4[128], buffer5[128], buffer6[128];
	//TODO: Ensure safety
	sprintf(buffer1, "%s::Bitmap",       pEntry->value);
	sprintf(buffer2, "%s::FontData",     pEntry->value);
	sprintf(buffer3, "%s::SystemFont",   pEntry->value);
	sprintf(buffer4, "%s::TitleBarFont", pEntry->value);
	sprintf(buffer5, "%s::BmpSize",      pEntry->value);
	sprintf(buffer6, "%s::ChrHeight",    pEntry->value);
	
	pBmpPath = CfgGetEntry (buffer1),
	pFntPath = CfgGetEntry (buffer2),
	pSysFont = CfgGetEntry (buffer3),
	pTibFont = CfgGetEntry (buffer4);
	pBmpSize = CfgGetEntry (buffer5);
	pChrHeit = CfgGetEntry (buffer6);
	
	if (!pBmpPath || !pFntPath) return;
	
	bool bSysFont = false, bTibFont = false;
	if (pSysFont)
		bSysFont = strcmp (pSysFont->value, "yes") == 0;
	if (pTibFont)
		bTibFont = strcmp (pTibFont->value, "yes") == 0;
	
	// Load Data
	int nBmpSize = 128;
	if (pBmpSize)
		nBmpSize = atoi (pBmpSize->value);
	int nChrHeit = 16;
	if (pChrHeit)
		nChrHeit = atoi (pChrHeit->value);
	
	int sizeof_bmp = 0, sizeof_fnt = 0;
	uint8_t
	*pBmp = ThmLoadEntireFile (pBmpPath->value, &sizeof_bmp),
	*pFnt = ThmLoadEntireFile (pFntPath->value, &sizeof_fnt);
	
	int font_id = CreateFont ((char*)pFnt, pBmp, nBmpSize, nBmpSize, nChrHeit);
	if (bSysFont)
		SetThemingParameter (P_SYSTEM_FONT, font_id);
	if (bTibFont)
		SetThemingParameter (P_TITLE_BAR_FONT, font_id);
}

void ThmLoadExtraFonts()
{
	ConfigEntry *pFontsToLoadEntry = CfgGetEntry ("Theming::FontsToLoad");
	
	if (!pFontsToLoadEntry) return;
	
	//we only support one font for now
	ThmLoadFont(pFontsToLoadEntry);
}

//make sure pOut is initialized first - if the config entry is missing this won't work
void ThmLoadFromConfig(uint32_t *pOut, const char *pString)
{
	CfgGetIntValue((int*)pOut, pString, *pOut);
}

void LoadDefaultThemingParms()
{
	SetThemingParameter(P_BLACK, 0x000000);
	
	// Dark mode:
	SetDefaultTheme();
	
	// Load config stuff
	ThmLoadFromConfig(&g_ThemingParms[P_TITLE_BAR_HEIGHT], "Theming::TitleBarHeight");
	ThmLoadExtraFonts();
}

void LoadThemingParmsFromFile(const char* pString)
{
	//TODO
	SLogMsg("TODO: LoadThemingParmsFromFile (\"%s\")", pString);
}

void SaveThemingParmsToFile(const char* pString)
{
	//TODO
	SLogMsg("TODO: SaveThemingParmsToFile (\"%s\")", pString);
}
