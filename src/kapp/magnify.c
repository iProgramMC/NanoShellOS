/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       MAGNIFYble Application module
******************************************/

#include <wbuiltin.h>

#define MAGSCALE 3

#define MAGWID   240
#define MAGHEI   120

#define DEF_MAGNIFY_WID (MAGWID*MAGSCALE+6)
#define DEF_MAGNIFY_HEI (MAGHEI*MAGSCALE+6 + TITLE_BAR_HEIGHT)

void CALLBACK PrgMagnifyProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			RECT (r, 3, 3 + TITLE_BAR_HEIGHT, MAGWID*MAGSCALE, MAGHEI*MAGSCALE);
			
			// a la C#'s  new Bitmap(320,200);
			Image *pImage = BitmapAllocate(MAGWID, MAGHEI, 0x00FFFFFF);
			
			((uint32_t*)pImage->framebuffer)[5] = 0x112233;
			
			AddControlEx (pWindow, CONTROL_IMAGE, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, 1000, (int) pImage, IMAGECTL_ZOOM);
			
			ImageCtlZoomToFill(pWindow, 1000);
			
			break;
		}
		case EVENT_UPDATE:
		{
			Image *pImage = GetImageCtlCurrentImage(pWindow, 1000);
			
			register uint32_t* pfb = ((uint32_t*)pImage->framebuffer);//Shh.. don't tell 
			
			bool bUpdated = false;
			Point p = GetMousePos();
			
			int oldpx = GET_X_PARM((int)pWindow->m_data);
			int oldpy = GET_Y_PARM((int)pWindow->m_data);
			
			/*if (oldpx != p.x || oldpy != p.y)
				bUpdated = true;*/
			
			pWindow->m_data = (void*)(MAKE_MOUSE_PARM(p.x, p.y));
			
			//if (!bUpdated) break;
			
			// Paint
			int xp = p.x - MAGWID / 2;
			int yp = p.y - MAGHEI / 2;
			
			VidSetVBEData(NULL);
			
			for (int y = 0; y < MAGHEI; y++)
			{
				xp = p.x - MAGWID / 2;
				for (int x = 0; x < MAGWID; x++)
				{
					register uint32_t px = VidReadPixelU(xp, yp);
					
					if (!bUpdated)
					{
						if (*pfb != px)
						{
							*pfb = px;
							bUpdated = true;
						}
						pfb++;
					}
					else
					{
						*(pfb++) = px;
					}
					
					xp++;
				}
				yp++;
			}
			
			if (!bUpdated) break;
			
			VidSetVBEData(&pWindow->m_vbeData);
			
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
			break;
		}
		case EVENT_PAINT:
			break;
		case EVENT_COMMAND:
			
			break;
		case EVENT_RELEASECURSOR:
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void PrgMagnifyTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("Magnifier", CW_AUTOPOSITION, CW_AUTOPOSITION, DEF_MAGNIFY_WID, DEF_MAGNIFY_HEI, PrgMagnifyProc, WF_ALWRESIZ);
	pWindow->m_iconID = ICON_FOLDER;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	int nextUpdateIn = 0;
	while (HandleMessages (pWindow))
	{
		if (nextUpdateIn <= GetTickCount())
		{
			//RegisterEventInsideWndProc(pWindow, EVENT_UPDATE, 0, 0);
			//CallWindowCallbackAndControls(pWindow, EVENT_UPDATE, 0, 0);
			WindowRegisterEvent(pWindow,  EVENT_UPDATE, 0, 0);
			nextUpdateIn = GetTickCount() +10;
		}
	}
#endif
}
