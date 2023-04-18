/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

      Control panel - Display Applet
******************************************/

#include <wbuiltin.h>

extern VBEData g_mainScreenVBEData;

void CplDisplay(Window* pWindow)
{
	char buff[2048];
	sprintf (buff, 
		"Display: %s\n"
		"Driver Name: %s\n\n"
		"Screen Size: %d x %d\n\n"
		"Framebuffer map address: 0x%X",
		
		"Generic VESA VBE-capable device",
		"NanoShell Basic VBE Display Driver",
		GetScreenWidth(), GetScreenHeight(),
		g_mainScreenVBEData.m_framebuffer32
	);
	MessageBox(pWindow, buff, "Display adapter info", MB_OK | ICON_ADAPTER << 16);
}