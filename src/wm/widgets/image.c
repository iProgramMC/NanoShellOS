/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

   Widget library: Image viewer control
******************************************/
#include "../wi.h"

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

void WidgetImage_BlitImageFast(Image* pImage, int x, int y, int width, int height, Rectangle clipRect)
{
	VBEData *pData = g_vbeData;
	int imgWidth = pImage->width, imgHeight = pImage->height;
	if (!width || !height) return;
	
	int yStart = y, yEnd = y + height, yStartBmp = 0, yStartRest = 0, yOffset = 0;
	int xStart = x, xEnd = x + width,  xStartBmp = 0, xStartRest = 0, xOffset = 0;
	
	// if the y-coord is higher than the cliprect's top
	if (yStart < clipRect.top)
	{
		int diff = clipRect.top - yStart;
		yOffset = diff;
		yStart += diff;
		yStartBmp  = diff * imgHeight / height;
		yStartRest = diff % height * imgHeight;
	}
	// if the x-coord is more to the left than the cliprect's left
	if (xStart < clipRect.left)
	{
		int diff = clipRect.left - xStart;
		xOffset = diff;
		xStart += diff;
		xStartBmp  = diff * imgWidth / width;
		xStartRest = diff % width * imgWidth;
	}
	// if the lower y coord is lower than the cliprect's bottom
	if (yEnd > clipRect.bottom)
		yEnd = clipRect.bottom;
	// if the right x coord is higher than the cliprect's right
	if (xEnd > clipRect.right)
		xEnd = clipRect.right;
	
	if (xEnd <= xStart || yEnd <= yStart) return;
	
	bool bIsStretchedVertically = imgHeight != height;
	bool bIsStretchedHorizontally = imgWidth != width;
	
	int offsetSrc = imgWidth * yStartBmp + xStartBmp;
	int offsetDst = pData->m_pitch32 * yStart + xStart;
	int imageY = yStartBmp;
	
	int ysStart = 0, xsStart = 0;
	
	// If we're stretching the image, we calculate starting offsets,
	// because the x, y coordinates we're provided may not necessarily
	// include the full first pixel
	if (bIsStretchedVertically)
	{
		int64_t mp = 0x80000000LL;
		int64_t d1 = mp * yOffset;
		int64_t d2 = mp * height / imgHeight;
		int64_t result = d1 % d2;
		
		ysStart = (int)(result / mp);
	}
	if (bIsStretchedHorizontally)
	{
		int64_t mp = 0x80000000LL;
		int64_t d1 = mp * xOffset;
		int64_t d2 = mp * width / imgWidth;
		int64_t result = d1 % d2;
		
		xsStart = (int)(result / mp);
	}
	
	// TODO: get rid of the divisions
	
	
	// for each scan line of the image:
	for (int yd = yStart, ys = ysStart + 1; yd < yEnd; ys++, yd++)
	{
		const uint32_t* scan_src = &pImage->framebuffer[offsetSrc];
		uint32_t* scan_dst = &pData->m_framebuffer32[offsetDst];
		
		int nPixels = xEnd - xStart;
		
		int imageX = xStartBmp;
		
		if (bIsStretchedHorizontally)
		{
			// stretch the pixel row itself
			for (int xd = xStart, xs = xsStart + 1; xd != xEnd; xd++, xs++)
			{
				*scan_dst = *scan_src;
				scan_dst++;
				
				// see if we need to increase the scan source
				int xWithinBitmap = xStartBmp + (xs * imgWidth) / width;
				if (xWithinBitmap > imgWidth)
					xWithinBitmap = imgWidth;
				if (xWithinBitmap == imageX) continue;
				
				imageX = xWithinBitmap;
				scan_src++;
			}
		}
		else
		{
			// just draw the row as is
			align4_memcpy(scan_dst, scan_src, nPixels * sizeof(uint32_t));
		}
		
		offsetDst += pData->m_pitch32;
		
		// if we aren't stretching, simply add the pitch to both
		if (!bIsStretchedVertically)
		{
			offsetSrc += imgWidth;
			continue;
		}
		
		// determine the Y within the bitmap (ys / dst_height * img_height) == img_y
		int yWithinBitmap = yStartBmp + (ys * imgHeight) / height;
		if (yWithinBitmap > imgHeight)
			yWithinBitmap = imgHeight;
		
		// no change? (note: surely, this won't decrease, only increase)
		if (yWithinBitmap == imageY) continue;
		
		// advance down
		imageY = yWithinBitmap;
		offsetSrc += imgWidth;
	}
	
	DirtyRectLogger(xStart, yStart, xEnd - xStart, yEnd - yStart);
}

void WidgetImage_PartialPaint(Control* this, Rectangle paintRect)
{
	VidSetClipRect(&paintRect);
	
	Image *pImage = (Image*)this->m_imageCtlData.pImage;
	if (pImage)
	{
		int  width  = pImage->width;
		int  height = pImage->height;
		
		if (this->m_imageCtlData.nZoomedWidth != this->m_imageCtlData.pImage->width)
		{
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
		
		Rectangle *start = NULL, *end = NULL;
		
		Rectangle imageRect = { x, y, x + width, y + height };
		
		WmSplitRectWithRectList(paintRect, &imageRect, 1, &start, &end);
		
		for (Rectangle* prect = start; prect != end; prect++)
		{
			VidFillRect(SCROLL_BAR_BACKGD_COLOR, prect->left, prect->top, prect->right - 1, prect->bottom - 1);
		}
		
		WmSplitDone();
		
		if (x <= paintRect.right && y <= paintRect.bottom && x + width >= paintRect.left && y + height >= paintRect.top)
		{
			WidgetImage_BlitImageFast(pImage, x, y, width, height, paintRect);
		}
	}
	else
	{
		VidFillRectangle(SCROLL_BAR_BACKGD_COLOR, paintRect);
		
		VidDrawText ("(No Image)", this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, 0xFF0000, TRANSPARENT);
	}
	
	DrawEdge(this->m_rect, DRE_SUNKEN, 0);
	
	VidSetClipRect(NULL);
}

SAI void Swap(int* a1, int* a2)
{
	int temp = *a1;
	*a1 = *a2;
	*a2 = temp;
}

static void OrderTwo(int* a1, int* a2)
{
	if (*a1 > *a2)
		Swap(a1, a2);
}

bool WidgetImage_OnEvent(Control* this, int eventType, int parm1, UNUSED int parm2, UNUSED Window* pWindow)
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
				
				this->m_imageCtlData.nZoomedWidth = pNewImage->width;
			}
			this->m_imageCtlData.nLastXGot = -1;
			this->m_imageCtlData.nLastYGot = -1;
			
			WidgetImage_PartialPaint(this, this->m_rect);
			
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
			WidgetImage_PartialPaint(this, this->m_rect);
			
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
							
							Rectangle rect = this->m_rect;
							rect.left   += 2;
							rect.top    += 2;
							rect.right  -= 2;
							rect.bottom -= 2;
							
							ScrollRect(&rect, deltaX, deltaY);
							
							Rectangle repaintH, repaintV;
							repaintH = repaintV = rect;
							
							// horizontal repaint
							if (deltaX < 0) repaintH.left  = repaintH.right + deltaX;
							else            repaintH.right = repaintH.left  + deltaX;
							// vertical repaint
							if (deltaY < 0) repaintV.top    = repaintV.bottom + deltaY;
							else            repaintV.bottom = repaintV.top    + deltaY;
							
							// TODO: ensure that the rectangles don't overlap. Repaints are kind of expensive..
							WidgetImage_PartialPaint(this, repaintH);
							WidgetImage_PartialPaint(this, repaintV);
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
						
						VBEData m_gdc;
						
						BuildGraphCtxBasedOnImage(&m_gdc, this->m_imageCtlData.pImage);
						
						VBEData *pBackup = VidSetVBEData(&m_gdc);
						
						int x1 = this->m_imageCtlData.nLastXGot - lx, y1 = this->m_imageCtlData.nLastYGot - ly;
						int x2 = p.x - lx, y2 = p.y - ly;
						
						//VidBlitImage(GetIconImage (ICON_COMPUTER_PANIC, 96),50, 50);
						VidDrawLine(this->m_imageCtlData.nCurrentColor, x1, y1, x2, y2);
						
						VidSetVBEData(pBackup);
						
						// repaint from nLastXGot to p.
						Rectangle rpRect;
						rpRect.left   = this->m_imageCtlData.nLastXGot;
						rpRect.top    = this->m_imageCtlData.nLastYGot;
						rpRect.right  = p.x;
						rpRect.bottom = p.y;
						
						OrderTwo(&rpRect.left, &rpRect.right);
						OrderTwo(&rpRect.top, &rpRect.bottom);
						rpRect.right++;
						rpRect.bottom++;
						
						WidgetImage_PartialPaint(this, rpRect);
					}
					
					this->m_imageCtlData.nLastXGot = p.x;
					this->m_imageCtlData.nLastYGot = p.y;
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
