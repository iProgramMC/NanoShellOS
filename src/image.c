/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

            Image loader module
******************************************/
#include <main.h>
#include <video.h>
#include <string.h>
#include <image.h>
#include <memory.h>

//Allocates an image on the heap and returns a pointer to it.
//It reads data from a Windows BMP-format data block.
Image *LoadBitmap (void* pBmpData, int *error)
{
	BmpHeaderStruct* pStruct = (BmpHeaderStruct*)pBmpData;
	
	if (pStruct->bitmapHeader != 0x4D42)//BM
	{
		SLogMsg("Invalid bm header");
		*error = BMPERR_INVALID_HEADER;
		return NULL;
	}
	if (pStruct->dibHeaderType != 40)
	{
		SLogMsg("Invalid dibheadertype");
		*error = BMPERR_INVALID_HEADER;
		return NULL;
	}
	
	int bpp = pStruct->imageBpp;
	
	if (bpp != 24 && bpp != 32)
	{
		SLogMsg("Invalid image bpp");
		*error = BMPERR_BAD_BPP;
		return NULL;
	}
	if (pStruct->colorPlaneCount != 1)
	{
		SLogMsg("Invalid color plane count");
		*error = BMPERR_BAD_COLOR_PLANES;
		return NULL;
	}
	
	int  imageWid = pStruct->bmpWidthPixels, imageHei = pStruct->bmpHeightPixels;
	bool flipped = imageHei < 0;
	if (imageHei < 0)
		imageHei = -imageHei;
	
	Image* pImage = (Image*)MmAllocate(sizeof(Image) + imageWid*imageHei*sizeof(uint32_t));
	if (!pImage)
	{
		SLogMsg("Could not allocate pImage of size %d",sizeof(Image)+imageWid*imageHei*sizeof(uint32_t));
		*error = BMPERR_BAD_ALLOC;
		return NULL;
	}
	pImage->width  = imageWid;
	pImage->height = imageHei;
	uint32_t* fb = (uint32_t*)(&pImage[1]);
	pImage->framebuffer = fb;//Get the pointer to data after the image struct.
	
	uint8_t* pixelArray = (uint8_t*)pBmpData + pStruct->pixArrayOffset;
	
	if (!flipped)
	{
		for (int j = pImage->height - 1; j >= 0; j--)
		{
			for (int i=0; i<pImage->width; i++)
			{
				if (bpp == 32)
				{
					fb[j * pImage->width + i] =  (*(uint32_t*)pixelArray);
					pixelArray += 4;
				} else {
					fb[j * pImage->width + i] = ((*(uint32_t*)pixelArray) & 0xFFFFFF);
					pixelArray += 3;
				}
			}
				
			//LogMsg("Loaded row %d of %d", j,pImage->height);
		}
	}
	else
	{
		for (int j = 0; j < pImage->height; j++)
		{
			for (int i=0; i<pImage->width; i++)
			{
				if (bpp == 32)
				{
					fb[j * pImage->width + i] =  (*(uint32_t*)pixelArray);
					pixelArray += 4;
				} else {
					fb[j * pImage->width + i] = ((*(uint32_t*)pixelArray) & 0xFFFFFF);
					pixelArray += 3;
				}
			}
				
			//LogMsg("Loaded row %d of %d", j,pImage->height);
		}
	}
	
	SLogMsg("Loaded image successfully");
	*error = BMPERR_SUCCESS;
	return pImage;
}

Image *BitmapAllocate(int width, int height, uint32_t default_color)
{
	uint32_t fb_size = width * height * sizeof (uint32_t);
	
	uint32_t to_size = fb_size + sizeof (Image);
	
	Image *pImage = MmAllocate(to_size);
	if (!pImage)
		return NULL;
	
	pImage->framebuffer = (const uint32_t*)((uint8_t*)pImage + sizeof *pImage);
	pImage->width       = width;
	pImage->height      = height;
	
	// Initialize to white
	memset_ints ((uint32_t*)pImage->framebuffer, default_color, width * height);
	
	return pImage;
}

void BuildGraphCtxBasedOnImage(VBEData *pData, Image *pImage)
{
	pData->m_version      = VBEDATA_VERSION_1;
	pData->m_width        = pImage->width;
	pData->m_height       = pImage->height;
	pData->m_pitch        = pImage->width * 4;
	pData->m_pitch16      = pImage->width * 2;
	pData->m_pitch32      = pImage->width;
	//pData->m_clipRect     = { 0, 0, pImage->width, pImage->height };
	pData->m_framebuffer8 = (uint8_t*)pImage->framebuffer;
	pData->m_bitdepth     = 2;
	pData->m_dirty        = false;
}
