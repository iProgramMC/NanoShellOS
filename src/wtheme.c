/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

           Window Themes module
******************************************/

#include <wtheme.h>

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
			0x000000,0x007f7f,0xcccccc,0xbbbbbb,0x000000,0x00003f,0x7f7f7f,0x0000ff,0xeeeeee,0x00003f,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	{
		"Dark",
		{
			0x000000,0x00003f,0x303030,0x202020,0x000000,0x000000,0x0f0f0f,0x000044,0x3f3f3f,0x000000,0xffffff,0xffffff,0x000000,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	{
		"Redmond",
		{
			0x000000,0x3a6ea5,0xc0c0c0,0xc0c0c0,0x000000,0x000080,0x808080,0x1084d0,0xb5b5b5,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	{
		"Calm",
		{
			0x000000,0x510401,0xe6d8ae,0xe6d8ae,0x000000,0x800000,0xc6a646,0xc09c38,0xe0c888,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	{
		"Black",
		{
			0x000000,0x000000,0x000000,0x000000,0x000000,0x800080,0x008000,0x800080,0x008000,0x000000,0xffffff,0xffffff,0x000000,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	/*{
		"White",
		{
			0x000000,0xffffff,0xffffff,0xffffff,0x000000,0x000000,0xffffff,0x000000,0xffffff,0x000000,0x808080,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},*/
	{
		"Rose",
		{
			0x000000,0x808080,0xcfafb7,0xcfafb7,0x000000,0x9f6070,0xa0a0a4,0xd8ccd0,0xd0d4d0,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	{
		"Desert",
		{
			0x000000,0xa28d68,0xd5ccbb,0xd5ccbb,0x000000,0x008080,0xa28d68,0x84bdaa,0xe8d080,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
	{
		"Rainy Day",
		{
			0x000000,0x000000,0x8399b1,0x8399b1,0x000000,0x4f657d,0x808080,0x80b4d0,0xb0bcd0,0x000000,0xffffff,0x000000,0xffffff,DEFAULT_SYSTEM_FONT,DEFAULT_TITLE_BAR_HEIGHT,DEFAULT_TITLE_BAR_FONT
		}
	},
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
