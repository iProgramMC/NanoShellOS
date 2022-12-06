/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Scribble Application module
******************************************/

#include <wbuiltin.h>
#include <main.h>
#include <vfs.h>
#include <image.h>

#define DEF_SCRIB_WID 500
#define DEF_SCRIB_HEI 400
extern Image *g_background;

void PaintLoadImage(Window* pWindow, const char* pFN)
{
	// load a file
	int fd = FiOpen(pFN, O_RDONLY);
	if (fd < 0)
	{
		char buffer[1024];
		snprintf(buffer, 1023, "Can't open '%s' (%d), try another", buffer, fd);
		MessageBox(pWindow, buffer, "Scribble", MB_OK | ICON_ERROR << 16);
		
		return;
	}
	uint8_t* pData = MmAllocate(FiTellSize(fd));
	if (!pData)
	{
		MessageBox(pWindow, "Insufficient memory to complete this operation.", "Scribble", MB_OK | ICON_ERROR << 16);
		FiClose(fd);
		return;
	}
	FiRead(fd, pData, FiTellSize(fd));
	FiClose(fd);
	
	// try to load it as an image
	int erc = 0;
	Image *pImage = LoadImageFile(pData, &erc);
	if (!pImage)
	{
		MmFree(pData);
		SLogMsg("Got error code %d while reading the image", erc);
		MessageBox(pWindow, "This is not a valid image file that NanoShell can read.", "Scribble", MB_OK | ICON_ERROR << 16);
		return;
	}
	
	SetImageCtlCurrentImage (pWindow, 1000, pImage);
	
	MmFree(pImage); // gotta free it. The control has already copied it
	MmFree(pData);
}

void CALLBACK PrgPaintProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			RECT (r, 10, 10 + TITLE_BAR_HEIGHT, DEF_SCRIB_WID - 20, DEF_SCRIB_HEI - TITLE_BAR_HEIGHT - 20- 40);
			
			// TODO: we shouldn't need to do this if we have the data
			Image *pImage = BitmapAllocate(320, 200, 0x00FFFFFF);
			
			AddControlEx (pWindow, CONTROL_IMAGE, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, 1000, (int) pImage, IMAGECTL_PAN | IMAGECTL_PEN);
			
			MmFree (pImage);//no leaks!!
			
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			if (pWindow->m_data)
			{
				const char* data = (const char*)pWindow->m_data;
				
				PaintLoadImage(pWindow, data);
				
				MmFree(pWindow->m_data);
			}
			
			int width_thing = DEF_SCRIB_WID - 20;
			int button_spac = width_thing / 6;
			int button_widt = button_spac - 10;
			
			const char *text[] = {
				"Pen", "Fill", "Pan", "Color", "TestImage1", "Load..."
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
					char *pFN = FilePickerBox (pWindow, "Type in a file path to open.", "Scribble", NULL);
					if (!pFN) break;
					
					PaintLoadImage(pWindow, pFN);
					
					MmFree(pFN);
					
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

void PrgPaintTask (int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("Scribble!", CW_AUTOPOSITION, CW_AUTOPOSITION, DEF_SCRIB_WID, DEF_SCRIB_HEI, PrgPaintProc, WF_ALWRESIZ);
	pWindow->m_iconID = ICON_DRAW;
	pWindow->m_data   = (void*)argument;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	while (HandleMessages (pWindow));
}
