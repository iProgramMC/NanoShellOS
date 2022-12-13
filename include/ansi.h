//  ***************************************************************
//  ansi.h - Creation date: 13/12/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// This header is solely responsible for providing easy access to ANSI escape codes.

#ifndef _ANSI_H
#define _ANSI_H

// Misc. commands
#define ANSI_CLEAR_LINE_TO_END      "\e[0K"
#define ANSI_CLEAR_LINE_TO_BEGIN    "\e[1K"
#define ANSI_CLEAR_SCREEN           "\e[2J"
#define ANSI_GO_TO(constantX, constantY) "\e[" #constantX ";" #constantY "H"
#define ANSI_GO_UP(by)              "\e[" #by "A"
#define ANSI_GO_DOWN(by)            "\e[" #by "B"
#define ANSI_GO_LEFT(by)            "\e[" #by "C"
#define ANSI_GO_RIGHT(by)           "\e[" #by "D"
#define ANSI_GO_IN_LINE(at)         "\e[" #at "G"
#define ANSI_INVERT                 "\e[7m"
#define ANSI_UNINVERT               "\e[27m"

// Color codes
enum
{
	ANSI_BLACK  = 30,
	ANSI_RED    = 31,
	ANSI_GREEN  = 32,
	ANSI_BROWN  = 33,
	ANSI_NAVY   = 34,
	ANSI_PURPLE = 35,
	ANSI_TURQ   = 36,
	ANSI_LGRAY  = 37,
	ANSI_DGRAY  = 90,
	ANSI_BRED   = 91,
	ANSI_LIME   = 92,
	ANSI_YELLOW = 93,
	ANSI_BLUE   = 94,
	ANSI_MAGENTA= 95,
	ANSI_CYAN   = 96,
	ANSI_WHITE  = 97,
};

#endif//_ANSI_H