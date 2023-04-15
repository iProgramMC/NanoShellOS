/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Window themes module header file
******************************************/
#ifndef _WTHEME_H
#define _WTHEME_H

#include <main.h>
#include <window.h>

enum
{
	TH_DEFAULT,
	TH_DARK,
	TH_REDMOND,
	TH_CALM,
	TH_BLACK,
	//TH_WHITE,//--fix edges pls
	TH_ROSE,
	TH_DESERT,
	TH_RAINYDAY,
	
};

typedef union
{
	uint32_t m_raw_data[P_THEME_PARM_COUNT];
	
	struct
	{
		uint32_t
		Black,
		BackgroundColor,
		ButtonMiddleColor,
		WindowBackgroundColor,
		WindowBorderColor,
		WindowTitleActiveColor,
		WindowTitleInactiveColor,
		WindowTitleActiveColorBright,
		WindowTitleInactiveColorBright,
		WindowTitleTextColorShadow,
		WindowTitleTextColor,
		WindowTextColor,
		WindowTextColorLight,
		SystemFont,
		TitleBarHeight,
		TitleBarFont,
		SelectedMenuItemColor,
		SelectedMenuItemColorBright,
		MenuBarHeight,
		BorderSize,
		ButtonHilightColor,
		ButtonShadowColor,
		ButtonEdgeColor,
		ButtonHoverButton,
		ScrollBarSize,
		MenuBarSelectionColor,
		SelectedItemColor,
		SelectedTextColor,
		DeselectedTextColor,
		ListBackgroundColor,
		TooltipBackgroundColor,
		TooltipTextColor,
		ScrollBarBackgroundColor,
		SelectedMenuItemTextColor,
		DeselectedMenuItemTextColor,
		TableViewAltRowColor,
		MenuItemHeight,
		ButtonDarkerShadowColor,
		CaptionButtonIconColor,
		BorderSizeNoResize;
	};
}
ThemeData;

void ApplyTheme(int themeID);
const char* GetThemeName(int themeID);

#endif//_WTHEME_H