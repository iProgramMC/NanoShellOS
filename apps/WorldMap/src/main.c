/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
           WorldMap application

             Main source file
******************************************/
#include <nsstandard.h>

#define CHART_WINDOW_WIDTH  (356 + 2 * 10)
#define CHART_WINDOW_HEIGHT (184 + 2 * 10 + 40)

#define RES_WORLD_MAP (1000)

Image gWorldMap = {
	356, 184, NULL
};

Image g_world_map_icon;

//TODO: Country 0x47 is missing - relocate 0x99 to there

int gSelectedCountry = 0;
int gSelectedCountryMax = 0x9A;

void SetSelectedCountry(int country)
{
	gSelectedCountry = country;
	uint32_t* pDst = (uint32_t*)gWorldMap       .framebuffer;//since it's const uint32_t by default
	uint32_t* pSrc = (uint32_t*)g_world_map_icon.framebuffer;//since it's const uint32_t by default
	
	int total_pixels = gWorldMap.width * gWorldMap.height;
	for (int i = 0; i < total_pixels; i++)
	{
		uint32_t pixelSrc = pSrc[i] & 0xFFFFFF;
		
		if (pixelSrc == 0x00FFFFFF) // White - ocean.
		{
			pDst[i] = 0x0000007F;
			continue;
		}
		if (pixelSrc == ((uint32_t) country << 4)) // The selected country
		{
			pDst[i] = 0x0000B020;
			continue;
		}
		// not selected - just draw it normally
		pDst[i] = 0x00007F00;
	}
}

void CALLBACK WndProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			// Create a world map image
			int total_pixels = gWorldMap.width * gWorldMap.height;
			gWorldMap.framebuffer = malloc (sizeof (uint32_t) * total_pixels);
			if (!gWorldMap.framebuffer)
			{
				MessageBox(pWindow, "The world map screen buffer could not be allocated.", "World map test", MB_OK | ICON_ERROR << 16);
				DefaultWindowProc(pWindow, EVENT_DESTROY, 0, 0);
			}
			
			// Add a scroll bar control
			Rectangle r;
			RECT(r, 10, 10, CHART_WINDOW_WIDTH - 26, 1);
			AddControl (pWindow, CONTROL_TEXT, r, "Country name here", 0x101, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			RECT(r, 10, 10 + 20, CHART_WINDOW_WIDTH - 26, 1);
			AddControl (pWindow, CONTROL_HSCROLLBAR, r, NULL, 0x100, (0 << 16 | gSelectedCountryMax), (1 << 16) | 0x10);
			
			SetSelectedCountry(0x10);
			
			break;
		}
		case EVENT_DESTROY:
		{
			// Be nice. Dispose of the world map
			if (gWorldMap.framebuffer)
			{
				free((void*)gWorldMap.framebuffer);
				gWorldMap.framebuffer = NULL;
			}
			break;
		}
		case EVENT_PAINT:
		{
			// blit the image in question
			VidBlitImage(&gWorldMap,  10, 10 + 40);
			break;
		}
		case EVENT_SCROLLDONE:
		{
			// update the selected country
			int pos = GetScrollBarPos (pWindow, 0x100);
			if (gSelectedCountry != pos)
			{
				SetSelectedCountry(pos);
				
				char test[100];
				sprintf (test, "Country ID: %x  (%d)   ",  pos, pos);
				SetLabelText(pWindow, 0x101, test);
				
				RegisterEventInsideWndProc (pWindow, EVENT_PAINT, 0, 0);
			}
			
			break;
		}
		case EVENT_KEYRAW:
		{
			break;
		}
		case EVENT_COMMAND:
			
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int main()
{
	g_world_map_icon = *GetImage(GetResource(RES_WORLD_MAP));
	
	Window* pWindow = CreateWindow ("World map test", CW_AUTOPOSITION, CW_AUTOPOSITION, CHART_WINDOW_WIDTH, CHART_WINDOW_HEIGHT, WndProc, WF_NOMINIMZ);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}
