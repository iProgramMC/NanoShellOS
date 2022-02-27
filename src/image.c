/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

            Image loader module
******************************************/
#include <main.h>
#include <video.h>
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
