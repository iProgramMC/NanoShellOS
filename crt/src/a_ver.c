//  ***************************************************************
//  a_ver.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

char g_VersionString[10] = "VX.XX";

const char* GetVersionString()
{
	if (g_VersionString[1] == 'X')
	{
		int ver = NsGetVersion();
		//major version and minor version:
		//NanoShell V1.00 (when that comes out) will have a version number of 100
		//Current version as of Feb 10,2022 (NanoShell V0.30) has a version code of 30.
		//Some software may naively just put a 0 in the major version number, but
		//we should expect an eventual V1.00 or more.
		sprintf(g_VersionString, "V%d.%02d", ver/100, ver%100);
	}
	
	return g_VersionString;
}
