/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Scribble Application module
******************************************/

#include <wbuiltin.h>
#include <image.h>

#define DEF_SCRIB_WID 500
#define DEF_SCRIB_HEI 400
extern Image *g_background;
void CALLBACK PrgPaintProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			RECT (r, 10, 10 + TITLE_BAR_HEIGHT, DEF_SCRIB_WID - 20, DEF_SCRIB_HEI - TITLE_BAR_HEIGHT - 20- 40);
			
			// a la C#'s  new Bitmap(320,200);
			Image *pImage = BitmapAllocate(320, 200, 0x00FFFFFF);
			
			/*VBEData m_data;
			// a la C#'s Graphics.FromBitmap
			BuildGraphCtxBasedOnImage(&m_data, pImage);
			
			VidSetVBEData(&m_data);
			
			// Draw an example image.
			VidBlitImage(GetIconImage (ICON_COMPUTER_PANIC, 96),2,2);
			
			VidSetVBEData(&pWindow->m_vbeData);*/
			
			
			AddControlEx (pWindow, CONTROL_IMAGE, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, 1000, (int) pImage, IMAGECTL_PAN | IMAGECTL_PEN);
			
			MmFree (pImage);//no leaks!!
			
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			int width_thing = DEF_SCRIB_WID - 20;
			int button_spac = width_thing / 6;
			int button_widt = button_spac - 10;
			
			const char *text[] = {
				"Pen", "Fill", "Pan", "Color", "TestImage1", "TestImage2"
			};
			
			for (int i = 0; i < 6; i++)
			{
				Rectangle r;
				RECT(r, 15 + button_spac * i, DEF_SCRIB_HEI - 40, button_widt, 20);
				
				//TODO: How would I make these buttons responsive to horizontal sizing?
				AddControlEx(pWindow, CONTROL_BUTTON, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, text[i], 2000 + i, 0, 0);
			}
			
			ChangeCursor (pWindow, CURSOR_PENCIL);
			
			break;
		}
		case EVENT_PAINT:
			//VidFillRect (0xFF00FF, 10, 40, 100, 120);
			//VidTextOut ("Hey, it's the window :)", 50, 50, TRANSPARENT, 0xe0e0e0);
			break;
		case EVENT_COMMAND:
			switch (parm1)
			{
				case 2000://Pen
					SetImageCtlMode(pWindow, 1000, IMAGECTL_PAN | IMAGECTL_PEN);
					break;
				case 2001://Fill
					SetImageCtlMode(pWindow, 1000, IMAGECTL_PAN | IMAGECTL_FILL);
					break;
				case 2002://Pan
					SetImageCtlMode(pWindow, 1000, IMAGECTL_PAN);
					break;
				case 2003://Color
				{
					uint32_t val = ColorInputBox(pWindow, "Select a color to use as the pen/fill color.", "Scribble");
					if (val != TRANSPARENT)
						SetImageCtlColor(pWindow, 1000, val);
					break;
				}
				case 2004://TestImage1
				{
					Image *pImage = GetIconImage (ICON_COMPUTER_PANIC, 96);
					SetImageCtlCurrentImage(pWindow, 1000, pImage);
					break;
				}
				case 2005://TestImage2
				{
					Image *pImage = g_background;
					if (!pImage) return;
					SetImageCtlCurrentImage(pWindow, 1000, pImage);
					break;
				}
			}
			break;
		case EVENT_RELEASECURSOR:
			// g_paint1X = g_paint1Y = -1;
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void PrgPaintTask (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("Scribble!", CW_AUTOPOSITION, CW_AUTOPOSITION, DEF_SCRIB_WID, DEF_SCRIB_HEI, PrgPaintProc, WF_ALWRESIZ);
	pWindow->m_iconID = ICON_DRAW;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
