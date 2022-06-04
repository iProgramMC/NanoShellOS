/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

   Widget library: Image viewer control
******************************************/
#include <widget.h>
#include <video.h>
#include <image.h>
#include <icon.h>
#include <clip.h>
#include <print.h>
#include <misc.h>
#include <keyboard.h>
#include <wmenu.h>
#include <string.h>

extern VBEData* g_vbeData, g_mainScreenVBEData;

void SetImageCtlMode (Window *pWindow, int comboID, int mode)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_parm2 = mode;
			return;
		}
	}
}
void SetImageCtlColor (Window *pWindow, int comboID, uint32_t color)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_imageCtlData.nCurrentColor = color;
			return;
		}
	}
}
bool WidgetImage_OnEvent(Control* this, int eventType, int parm1, UNUSED int parm2, Window* pWindow);
void SetImageCtlCurrentImage (Window *pWindow, int comboID, Image* pImage)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control *pThis = &pWindow->m_pControlArray[i];
		if (pThis->m_comboID == comboID)
		{
			// Force a refresh of the image.
			pThis->m_parm1 = (int)pImage;
			if (pThis->m_imageCtlData.pImage)
				MmFree(pThis->m_imageCtlData.pImage);
			pThis->m_imageCtlData.pImage = NULL;
			
			WidgetImage_OnEvent (pThis, EVENT_IMAGE_REFRESH, 0, 0, pWindow);
			
			return;
		}
	}
}
Image* GetImageCtlCurrentImage (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return pWindow->m_pControlArray[i].m_imageCtlData.pImage;
		}
	}
	return NULL;
}
void ImageCtlZoomToFill (Window *pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_imageCtlData.nZoomedWidth = GetWidth (&pWindow->m_pControlArray[i].m_rect);
		}
	}
}
bool WidgetImage_OnEvent(Control* this, int eventType, int parm1, UNUSED int parm2, Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		case EVENT_IMAGE_REFRESH:
		{
			if (!this->m_imageCtlData.pImage)
			{
				Image* pImageToCopy = (Image*)this->m_parm1;
				
				size_t n_pixels = pImageToCopy->width * pImageToCopy->height;
				size_t new_size = sizeof (Image) + n_pixels * sizeof (uint32_t);
				
				Image* pNewImage    = (Image*)MmAllocateK(new_size);
				pNewImage->framebuffer = (uint32_t*)((uint8_t*)pNewImage + sizeof (Image));
				
				//yes I am aware that I break the const-ness rule, but it's ok because we're supposed to do initialization
				memcpy_ints ((uint32_t*)pNewImage->framebuffer, pImageToCopy->framebuffer, n_pixels);
				pNewImage->width  = pImageToCopy->width;
				pNewImage->height = pImageToCopy->height;
				
				this->m_imageCtlData.pImage = pNewImage;
				
				this->m_imageCtlData.nZoomedWidth  = pNewImage->width;
			}
			this->m_imageCtlData.nLastXGot = -1;
			this->m_imageCtlData.nLastYGot = -1;
			break;
		}
		case EVENT_DESTROY:
		{
			if (this->m_imageCtlData.pImage)
			{
				MmFree(this->m_imageCtlData.pImage);
				this->m_imageCtlData.pImage = NULL;
			}
			break;
		}
		case EVENT_PAINT:
		{
			// Draw a rectangle to surround the things we put inside
			//VidDrawHLine(WINDOW_BACKGD_COLOR - 0x0F0F0F, this->m_rect.left + 8, this->m_rect.right - 8, (this->m_rect.top + this->m_rect.bottom) / 2);
			
			VidSetClipRect(&this->m_rect);
			
			Image *pImage = (Image*)this->m_imageCtlData.pImage;
			if (pImage)
			{
				bool ogSize = false;
				int  width  = pImage->width;
				int  height = pImage->height;
				
				if (this->m_imageCtlData.nZoomedWidth != this->m_imageCtlData.pImage->width)
				{
					ogSize = false;
					width  = this->m_imageCtlData.nZoomedWidth;
					
					uint32_t  tmp = height;
					tmp *= this->m_imageCtlData.nZoomedWidth;
					tmp /= this->m_imageCtlData.pImage->width;
					
					height = (int)tmp;
				}
				
				int x = (GetWidth (&this->m_rect) - width)  / 2;
				int y = (GetHeight(&this->m_rect) - height) / 2;
				
				if (this->m_parm2 & IMAGECTL_PAN)
				{
					x += this->m_imageCtlData.nCurPosX;
					y += this->m_imageCtlData.nCurPosY;
				}
				
				x += this->m_rect.left;
				y += this->m_rect.top;
				
				if (x <= this->m_rect.right && y <= this->m_rect.bottom && x + width >= this->m_rect.left && y + height >= this->m_rect.top)
				{
					if (GetWidth(&this->m_rect) > this->m_imageCtlData.nZoomedWidth)
					{
						VidFillRectangle(0x3f0000, this->m_rect);
					}
					if (ogSize)
						VidBlitImage (pImage, x, y);
					else
						VidBlitImageResize(pImage, x, y, width, height);
				}
				else
				{
					if (!parm2)
					{
						VidFillRectangle(0x3f0000, this->m_rect);
					}
					VidDrawText ("(Out of View)", this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0xFF0000, TRANSPARENT);
				}
			}
			else
			{
				if (!parm2)
				{
					VidFillRectangle(0x3f0000, this->m_rect);
				}
				VidDrawText ("(No Image)", this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0xFF0000, TRANSPARENT);
			}
			
			VidSetClipRect(NULL);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (this->m_parm2  & (IMAGECTL_PAN | IMAGECTL_PEN))
			{
				if (RectangleContains(&this->m_rect, &p))
				{
					int deltaX = 0, deltaY = 0;
					if (this->m_imageCtlData.nLastXGot != -1)
					{
						deltaX = p.x - this->m_imageCtlData.nLastXGot;
						deltaY = p.y - this->m_imageCtlData.nLastYGot;
					}
					else
					{
						this->m_imageCtlData.nLastXGot = p.x;
						this->m_imageCtlData.nLastYGot = p.y;
					}
					
					bool dragged = false;
					if (this->m_parm2 & IMAGECTL_PAN)
					{
						if (!(this->m_parm2 & IMAGECTL_PEN) || KbGetKeyState(KEY_CONTROL) == KEY_PRESSED)
						{
							this->m_imageCtlData.nCurPosX += deltaX;
							this->m_imageCtlData.nCurPosY += deltaY;
							
							dragged = true;
						}
					}
					if ((this->m_parm2 & IMAGECTL_PEN) && !dragged)
					{
						int lx = (GetWidth (&this->m_rect) - this->m_imageCtlData.pImage->width)  / 2;
						int ly = (GetHeight(&this->m_rect) - this->m_imageCtlData.pImage->height) / 2;
						
						if (this->m_parm2 & IMAGECTL_PAN)
						{
							lx += this->m_imageCtlData.nCurPosX;
							ly += this->m_imageCtlData.nCurPosY;
						}
						
						lx += this->m_rect.left;
						ly += this->m_rect.top;
						
						VBEData *pBackup = g_vbeData, m_gdc;
						
						BuildGraphCtxBasedOnImage(&m_gdc, this->m_imageCtlData.pImage);
						
						VidSetVBEData(&m_gdc);
						
						//VidBlitImage(GetIconImage (ICON_COMPUTER_PANIC, 96),50, 50);
						VidDrawLine(this->m_imageCtlData.nCurrentColor, this->m_imageCtlData.nLastXGot - lx, this->m_imageCtlData.nLastYGot - ly, p.x - lx, p.y - ly);
						
						VidSetVBEData(pBackup);
					}
					
					this->m_imageCtlData.nLastXGot = p.x;
					this->m_imageCtlData.nLastYGot = p.y;
					
					WidgetImage_OnEvent(this, EVENT_PAINT, 0, !dragged, pWindow);
				}
			}
			else
			{
				this->m_imageCtlData.nCurPosX = 0;
				this->m_imageCtlData.nCurPosY = 0;
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			this->m_imageCtlData.nLastXGot = this->m_imageCtlData.nLastYGot = -1;
			break;
		}
	}
	return false;
}
