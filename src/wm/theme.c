/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

           Window Themes module
******************************************/
#include <wtheme.h>
#include <config.h>
#include <print.h>
#include <image.h>
#include <vfs.h>
#include <icon.h>

typedef struct
{
	const char* m_name;
	//int m_colors[P_THEME_PARM_COUNT];
	ThemeData m_data;
}
Theme;

const Theme g_themes[] = {
{
	"Default", {
		.Black                          = 0,
		.BackgroundColor                = 0x007f7f,
		.ButtonMiddleColor              = 0xcccccc,
		.WindowBackgroundColor          = 0xbbbbbb,
		.WindowBorderColor              = 0xaaaaaa,
		.WindowTitleActiveColor         = 63,
		.WindowTitleInactiveColor       = 0x7f7f7f,
		.WindowTitleActiveColorBright   = 0x0000ff,
		.WindowTitleInactiveColorBright = 0xeeeeee,
		.WindowTitleTextColorShadow     = 63,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x316ac5,
		.SelectedMenuItemColorBright    = 0xc1d2ee,
		.MenuBarHeight                  = DEFAULT_MENU_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x808080,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xececec,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00007f,
		.SelectedItemColor              = 0x00007f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6f6f6f,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xcccccc,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x404040,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"Dark", {
		.Black                          = 0,
		.BackgroundColor                = 0x00003f,
		.ButtonMiddleColor              = 0x404040,
		.WindowBackgroundColor          = 0x202020,
		.WindowBorderColor              = 0,
		.WindowTitleActiveColor         = 0,
		.WindowTitleInactiveColor       = 0x0f0f0f,
		.WindowTitleActiveColorBright   = 0x000044,
		.WindowTitleInactiveColorBright = 0x3f3f3f,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0xffffff,
		.WindowTextColorLight           = 0,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x0000ff,
		.SelectedMenuItemColorBright    = 0x00007f,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0x606060,
		.ButtonShadowColor              = 0x303030,
		.ButtonEdgeColor                = 0x000000,
		.ButtonHoverButton              = 0x505050,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00008f,
		.SelectedItemColor              = 0x00008f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0xffffff,
		.ListBackgroundColor            = 0,
		.TooltipBackgroundColor         = 0x33312b,
		.TooltipTextColor               = 0xffffff,
		.ScrollBarBackgroundColor       = 0x101010,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0xffffff,
		.TableViewAltRowColor           = 0x101010,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x181818,
		.CaptionButtonIconColor         = 0xffffff,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"Noir", {
		.Black                          = 0,
		.BackgroundColor                = 0x101010,
		.ButtonMiddleColor              = 0x181818,
		.WindowBackgroundColor          = 0x181818,
		.WindowBorderColor              = 0x000000,
		.WindowTitleActiveColor         = 0x000000,
		.WindowTitleInactiveColor       = 0x181818,
		.WindowTitleActiveColorBright   = 0x000000,
		.WindowTitleInactiveColorBright = 0x181818,
		.WindowTitleTextColorShadow     = 0x000000,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0xffffff,
		.WindowTextColorLight           = 0x000000,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x707070,
		.SelectedMenuItemColorBright    = 0x505050,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0x404040,
		.ButtonShadowColor              = 0x101010,
		.ButtonEdgeColor                = 0x000000,
		.ButtonHoverButton              = 0x202020,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x000060,
		.SelectedItemColor              = 0x000060,
		.SelectedTextColor              = 0xFFFFFF,
		.DeselectedTextColor            = 0xFFFFFF,
		.ListBackgroundColor            = 0x000000,
		.TooltipBackgroundColor         = 0x000000,
		.TooltipTextColor               = 0xFFFFFF,
		.ScrollBarBackgroundColor       = 0x080808,
		.SelectedMenuItemTextColor      = 0xFFFFFF,
		.DeselectedMenuItemTextColor    = 0xFFFFFF,
		.TableViewAltRowColor           = 0x080808,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x080808,
		.CaptionButtonIconColor         = 0xFFFFFF,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"Redmond", {
		.Black                          = 0,
		.BackgroundColor                = 0x3a6ea5,
		.ButtonMiddleColor              = 0xc0c0c0,
		.WindowBackgroundColor          = 0xc0c0c0,
		.WindowBorderColor              = 0xb0b0b0,
		.WindowTitleActiveColor         = 0x000080,
		.WindowTitleInactiveColor       = 0x808080,
		.WindowTitleActiveColorBright   = 0x1084d0,
		.WindowTitleInactiveColorBright = 0xb5b5b5,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x316ac5,
		.SelectedMenuItemColorBright    = 0xc1d2ee,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x808080,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xececec,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00007f,
		.SelectedItemColor              = 0x00007f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6f6f6f,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xcccccc,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x404040,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"VGA", {
		.Black                          = COLR_BLACK,
		.BackgroundColor                = COLOR_TEAL,
		.ButtonMiddleColor              = COLR_LGRAY,
		.WindowBackgroundColor          = COLR_LGRAY,
		.WindowBorderColor              = COLR_LGRAY,
		.WindowTitleActiveColor         = COLR_DBLUE,
		.WindowTitleInactiveColor       = COLR_DGRAY,
		.WindowTitleActiveColorBright   = COLR_DBLUE,
		.WindowTitleInactiveColorBright = COLR_DGRAY,
		.WindowTitleTextColorShadow     = COLR_DBLUE,
		.WindowTitleTextColor           = COLR_WHITE,
		.WindowTextColor                = COLR_BLACK,
		.WindowTextColorLight           = COLR_WHITE,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = COLR_DGRAY,
		.SelectedMenuItemColorBright    = COLR_LGRAY,
		.MenuBarHeight                  = DEFAULT_MENU_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = COLR_WHITE,
		.ButtonShadowColor              = COLR_DGRAY,
		.ButtonEdgeColor                = COLR_BLACK,
		.ButtonHoverButton              = COLR_LGRAY,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = COLR_DBLUE,
		.SelectedItemColor              = COLR_DBLUE,
		.SelectedTextColor              = COLR_WHITE,
		.DeselectedTextColor            = COLR_BLACK,
		.ListBackgroundColor            = COLR_WHITE,
		.TooltipBackgroundColor         = COLR_WHITE,
		.TooltipTextColor               = COLR_BLACK,
		.ScrollBarBackgroundColor       = COLR_DGRAY,
		.SelectedMenuItemTextColor      = COLR_WHITE,
		.DeselectedMenuItemTextColor    = COLR_BLACK,
		.TableViewAltRowColor           = COLR_LGRAY,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = COLR_BLACK,
		.CaptionButtonIconColor         = COLR_BLACK,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"Sandy", {
		.Black                          = 0,
		.BackgroundColor                = 0x3a6ea5,
		.ButtonMiddleColor              = 0xece9d8,
		.WindowBackgroundColor          = 0xece9d8,
		.WindowBorderColor              = 0xb0b0b0,
		.WindowTitleActiveColor         = 0x0054e3,
		.WindowTitleInactiveColor       = 0x7a96df,
		.WindowTitleActiveColorBright   = 0x3d95ff,
		.WindowTitleInactiveColorBright = 0x9db9eb,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x316ac5,
		.SelectedMenuItemColorBright    = 0xc1d2ee,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x808080,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xececec,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00007f,
		.SelectedItemColor              = 0x00007f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6f6f6f,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xcccccc,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x404040,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
/*{
	"White", {
		.Black                          = 0,
		.BackgroundColor                = 0xffffff,
		.ButtonMiddleColor              = 0xffffff,
		.WindowBackgroundColor          = 0xffffff,
		.WindowBorderColor              = 0,
		.WindowTitleActiveColor         = 0,
		.WindowTitleInactiveColor       = 0xffffff,
		.WindowTitleActiveColorBright   = 0,
		.WindowTitleInactiveColorBright = 0xffffff,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0x808080,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x316ac5,
		.SelectedMenuItemColorBright    = 0xc1d2ee,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x808080,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xececec,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00007f,
		.SelectedItemColor              = 0x00007f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6f6f6f,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xcccccc,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},*/
{
	"Rose", {
		.Black                          = 0,
		.BackgroundColor                = 0x808080,
		.ButtonMiddleColor              = 0xcfafb7,
		.WindowBackgroundColor          = 0xcfafb7,
		.WindowBorderColor              = 0xaf959c,
		.WindowTitleActiveColor         = 0x9f6070,
		.WindowTitleInactiveColor       = 0x9e8189,
		.WindowTitleActiveColorBright   = 0xd8ccd0,
		.WindowTitleInactiveColorBright = 0xd5d0d2,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x7f6c71,
		.SelectedMenuItemColorBright    = 0xedc9d2,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x7f6c71,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xeac7d0,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x7c5b64,
		.SelectedItemColor              = 0x7c5b64,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6d5e62,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xf2e8ea,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x3f3638,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"Desert", {
		.Black                          = 0,
		.BackgroundColor                = 0xa28d68,
		.ButtonMiddleColor              = 0xd5ccbb,
		.WindowBackgroundColor          = 0xd5ccbb,
		.WindowBorderColor              = 0xafa89a,
		.WindowTitleActiveColor         = 0x008080,
		.WindowTitleInactiveColor       = 0xa28d68,
		.WindowTitleActiveColorBright   = 0x84bdaa,
		.WindowTitleInactiveColorBright = 0xe8d080,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x008080,
		.SelectedMenuItemColorBright    = 0x84bdaa,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x7f7a70,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xeae0ce,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x008080,
		.SelectedItemColor              = 0x008080,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6d6960,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xe2d9c7,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x3f3d38,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
/*
{
	"Rainy Day", {
		.Black                          = 0,
		.BackgroundColor                = 0,
		.ButtonMiddleColor              = 0x8399b1,
		.WindowBackgroundColor          = 0x8399b1,
		.WindowBorderColor              = 0xb0b0b0,
		.WindowTitleActiveColor         = 0x4f657d,
		.WindowTitleInactiveColor       = 0x808080,
		.WindowTitleActiveColorBright   = 0x80b4d0,
		.WindowTitleInactiveColorBright = 0xb0bcd0,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x316ac5,
		.SelectedMenuItemColorBright    = 0xc1d2ee,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x808080,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xececec,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00007f,
		.SelectedItemColor              = 0x00007f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6f6f6f,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xcccccc,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x404040,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},
{
	"Calm", {
		.Black                          = 0,
		.BackgroundColor                = 0x510401,
		.ButtonMiddleColor              = 0xe6d8ae,
		.WindowBackgroundColor          = 0xe6d8ae,
		.WindowBorderColor              = 0,
		.WindowTitleActiveColor         = 0x800000,
		.WindowTitleInactiveColor       = 0xc6a646,
		.WindowTitleActiveColorBright   = 0xc09c38,
		.WindowTitleInactiveColorBright = 0xe0c888,
		.WindowTitleTextColorShadow     = 0,
		.WindowTitleTextColor           = 0xffffff,
		.WindowTextColor                = 0,
		.WindowTextColorLight           = 0xffffff,
		.SystemFont                     = DEFAULT_SYSTEM_FONT,
		.TitleBarHeight                 = DEFAULT_TITLE_BAR_HEIGHT,
		.TitleBarFont                   = DEFAULT_TITLE_BAR_FONT,
		.SelectedMenuItemColor          = 0x316ac5,
		.SelectedMenuItemColorBright    = 0xc1d2ee,
		.MenuBarHeight                  = DEFAULT_TITLE_BAR_HEIGHT,
		.BorderSize                     = DEFAULT_BORDER_SIZE,
		.ButtonHilightColor             = 0xffffff,
		.ButtonShadowColor              = 0x808080,
		.ButtonEdgeColor                = 0,
		.ButtonHoverButton              = 0xececec,
		.ScrollBarSize                  = DEFAULT_SCROLL_BAR_SIZE,
		.MenuBarSelectionColor          = 0x00007f,
		.SelectedItemColor              = 0x00007f,
		.SelectedTextColor              = 0xffffff,
		.DeselectedTextColor            = 0,
		.ListBackgroundColor            = 0xffffff,
		.TooltipBackgroundColor         = 0xfff8d8,
		.TooltipTextColor               = 0,
		.ScrollBarBackgroundColor       = 0x6f6f6f,
		.SelectedMenuItemTextColor      = 0xffffff,
		.DeselectedMenuItemTextColor    = 0,
		.TableViewAltRowColor           = 0xcccccc,
		.MenuItemHeight                 = DEFAULT_MENU_ITEM_HEIGHT,
		.ButtonDarkerShadowColor        = 0x404040,
		.CaptionButtonIconColor         = 0,
		.BorderSizeNoResize             = DEFAULT_BORDER_SIZE_NORESIZE,
	}
},*/
};

uint32_t g_ThemingParms[P_THEME_PARM_COUNT];

const uint32_t* GetThemingParameters()
{
	return g_ThemingParms;
}

const uint32_t* GetThemeParms(int themeNumber)
{
	if (themeNumber < 0 || themeNumber >= (int)ARRAY_COUNT(g_themes))
		themeNumber = 0;
	
	return g_themes[themeNumber].m_data.m_raw_data;
}

bool WouldThemeChange(int themeNumber)
{
	if (themeNumber < 0 || themeNumber >= (int)ARRAY_COUNT(g_themes)) return true;
	
	for (int i = P_BLACK; i < P_THEME_PARM_COUNT; i++)
	{
		if (GetThemingParameter(i) != g_themes[themeNumber].m_data.m_raw_data[i])
			return true;
	}
	
	return false;
}

void ApplyTheme(int themeNumber)
{
	if (themeNumber < 0 || themeNumber >= (int)ARRAY_COUNT(g_themes)) return;
	SLogMsg("Applying theme %s", g_themes[themeNumber].m_name);
	
	for (int i = P_BLACK; i < P_THEME_PARM_COUNT; i++)
	{
		SetThemingParameter(i, g_themes[themeNumber].m_data.m_raw_data[i]);
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
	
	FiClose(fd);
	
	*pSizeOut = read_size;
	return pMem;
}

uint8_t* ThmLoadImageBWFile(const char *pPath, int *pSizeOut, int *pWidthOut, int *pHeightOut)
{
	int sz = 0;
	uint8_t* entireFile = ThmLoadEntireFile(pPath, &sz);
	if (!entireFile) return NULL;
	
	int errorCode = -1;
	Image* imgFile = LoadImageFile(entireFile, sz, &errorCode);
	
	// if an image didn't load, just pass its raw output. This is insane and you shouldn't do that.
	if (!imgFile)
	{
		SLogMsg("LoadImageFile didn't like it. Error Code: %x", errorCode);
		return NULL;
	}
	
	sz = imgFile->width * imgFile->height;
	uint8_t * pBWImage = MmAllocate(sz);
	
	for (int i = 0; i < sz; i++)
	{
		// TODO: Do something other than just simply grabbing the green channel
		uint32_t color = imgFile->framebuffer[i];
		uint8_t* colorBytes = (uint8_t*)&color;
		
		pBWImage[i] = colorBytes[1];
	}
	
	*pSizeOut   = sz;
	*pWidthOut  = imgFile->width;
	*pHeightOut = imgFile->height;
	
	MmFree(imgFile);
	MmFree(entireFile);
	
	return pBWImage;
}

void ThmLoadFont(ConfigEntry *pEntry)
{
	//load some properties
	ConfigEntry
	* pBmpPath, * pFntPath, * pSysFont, * pTibFont, * pChrHeit;
	
	char buffer1[128], buffer2[128], buffer3[128], buffer4[128], buffer5[128];
	//TODO: Ensure safety
	sprintf(buffer1, "%s::Bitmap",       pEntry->value);
	sprintf(buffer2, "%s::FontData",     pEntry->value);
	sprintf(buffer3, "%s::SystemFont",   pEntry->value);
	sprintf(buffer4, "%s::TitleBarFont", pEntry->value);
	sprintf(buffer5, "%s::ChrHeight",    pEntry->value);
	
	pBmpPath = CfgGetEntry (buffer1),
	pFntPath = CfgGetEntry (buffer2),
	pSysFont = CfgGetEntry (buffer3),
	pTibFont = CfgGetEntry (buffer4);
	pChrHeit = CfgGetEntry (buffer5);
	
	if (!pBmpPath || !pFntPath) return;
	
	bool bSysFont = false, bTibFont = false;
	if (pSysFont)
		bSysFont = strcmp (pSysFont->value, "yes") == 0;
	if (pTibFont)
		bTibFont = strcmp (pTibFont->value, "yes") == 0;
	
	// Load Data
	int nBitmapWidth = 128, nBitmapHeight = 16;
	int nChrHeit = 16;
	if (pChrHeit)
		nChrHeit = atoi (pChrHeit->value);
	
	int sizeof_bmp = 0, sizeof_fnt = 0;
	uint8_t
	*pFnt = ThmLoadEntireFile (pFntPath->value, &sizeof_fnt),
	*pBmp = ThmLoadImageBWFile(pBmpPath->value, &sizeof_bmp, &nBitmapWidth, &nBitmapHeight);
	
	if (!pBmp)
	{
		SLogMsg("Loading bitmap failed!");
		return;
	}
	if (!pFnt)
	{
		SLogMsg("Loading font failed!");
		return;
	}
	
	int font_id = CreateBMFont ((char*)pFnt, pBmp, nBitmapWidth, nBitmapHeight, nChrHeit);
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
