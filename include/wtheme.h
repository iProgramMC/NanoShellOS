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


void ApplyTheme(int themeID);
const char* GetThemeName(int themeID);

#endif//_WTHEME_H