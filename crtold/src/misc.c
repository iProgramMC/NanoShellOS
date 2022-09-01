//
// crt.c
//
// Copyright (C) 2022 iProgramInCpp.
//
// The standard NanoShell library internal implementation
// -- Misc. API functions
//

#include "crtlib.h"
#include "crtinternal.h"

#ifdef USE_VIDEO

void SetMousePos (UNUSED unsigned pX, UNUSED unsigned pY)
{
	//TODO
}

void VidDrawRectangle(unsigned color, Rectangle rect)
{
	VidDrawRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

void VidFillRectangle(unsigned color, Rectangle rect)
{
	VidFillRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

#endif


#ifdef USE_FILE
int remove (const char* filename)
{
	return _I_FiRemoveFile(filename);
}
#endif

#ifdef USE_CC
int CcRunCCode(const char* data, int length)
{
	return _I_CcRunCCode (data, length);
}
#endif

#ifdef USE_MISC

void sprintf(char*a, const char*c, ...);
//futureproofing here:
char g_VersionString[10] = "VX.XX";
int NsGetVersion ();
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

#endif



