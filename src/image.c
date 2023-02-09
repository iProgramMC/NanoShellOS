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

//Note: As a specification, the image loader family of functions
//returns the framebuffer as part of the image block, so that a
//single MmFree(pImage) is enough.

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
	/*if (pStruct->dibHeaderType != 40)
	{
		SLogMsg("Invalid dibheadertype");
		*error = BMPERR_INVALID_HEADER;
		return NULL;
	}*/
	
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
	
	SLogMsg("Pixel array offset: %x.  BPP: %x. Flipped:%s", pStruct->pixArrayOffset, bpp, flipped?"true":"false");
	
	uint32_t stride = (pImage->width * (bpp >> 3) + 3) & ~3; // Align to DWORD
	
	if (!flipped)
	{
		for (int j = pImage->height - 1, e = 0; j >= 0; j--, e++)
		{
			pixelArray = (uint8_t*)pBmpData + pStruct->pixArrayOffset + stride * e;
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
Image *LoadTarga (void* pTgaData, int *error)
{
	TgaHeaderStruct* pStruct = (TgaHeaderStruct*)pTgaData;
	
	if (pStruct->width < 1 || pStruct->height < 1)
	{
		return NULL;
	}
	
	Image* pImage = (Image*)MmAllocate(sizeof(Image) + pStruct->width * pStruct->height *sizeof(uint32_t));
	if (!pImage)
	{
		SLogMsg("Could not allocate pImage of size %d", sizeof(Image) + pStruct->width * pStruct->height * sizeof(uint32_t));
		*error = BMPERR_BAD_ALLOC;
		return NULL;
	}
	pImage->width  = pStruct->width;
	pImage->height = pStruct->height;
	
	uint32_t* fb = (uint32_t*)(&pImage[1]);
	pImage->framebuffer = fb;//Get the pointer to data after the image struct.
	
	uint8_t* dataPtr = (uint8_t*)pTgaData + 18;
	
	switch (pStruct->encoding)
	{
		//TODO: more
		case TGA_UNCOMPRESSED: // 2
		{
			if ((pStruct->imageBpp != 24 && pStruct->imageBpp != 32))
			{
				*error = BMPERR_BAD_BPP;
				MmFree(pImage);
				return NULL;
			}
			for (int i = 0, y = 0; y < pStruct->height; y++)
			{
				int lineOffset = (pStruct->y ? y : (pStruct->height - y - 1)) * pStruct->width * (pStruct->imageBpp >> 3);
				for (int x = 0; x < pStruct->width; x++)
				{
					fb[i++] = ((pStruct->imageBpp == 32 ? (255 - dataPtr[lineOffset + 3]) : 0x00) << 24) + 
							  (dataPtr[lineOffset + 2] << 16) + 
							  (dataPtr[lineOffset + 1] << 8) +
							  (dataPtr[lineOffset]);
					lineOffset += pStruct->imageBpp >> 3;
				}
			}
			
			break;
		}
		case TGA_UNCOMPRESSED_GRAYSCALE: // 3
		{
			if (pStruct->imageBpp != 8)
			{
				*error = BMPERR_BAD_BPP;
				MmFree(pImage);
				return NULL;
			}
			for (int i = 0, y = 0; y < pStruct->height; y++)
			{
				int lineOffset = (pStruct->y ? y : (pStruct->height - y - 1)) * pStruct->width;
				for (int x = 0; x < pStruct->width; x++)
				{
					const uint8_t b = dataPtr[lineOffset];
					fb[i++] = (b << 16) | (b << 8) | b;
					lineOffset++;
				}
			}
			
			break;
		}
		case TGA_RLE: // 10
		{
			int byteOffset = (pStruct->colorMap ? (pStruct->cMapEnt >> 3) * pStruct->cMapLen : 0);
			
            if (pStruct->cMapLen != 0 || pStruct->colorMap != 0)
			{
				*error = BMPERR_BAD_COLOR_PLANES;
				MmFree(pImage);
				return NULL;
			}
			if (pStruct->imageBpp != 24 && pStruct->imageBpp != 32)
			{
				*error = BMPERR_BAD_BPP;
				MmFree(pImage);
				return NULL;
			}
            int y = 0, i = 0;
            for (int x = 0; x < pStruct->width * pStruct->height; )
			{
                int k = dataPtr[byteOffset++];
                if (k > 127)
				{
                    k -= 127; x += k;
                    while (k--)
					{
                        if (i % pStruct->width == 0)
						{
							i = ((pStruct->y ? y : pStruct->height - y - 1) * pStruct->width);
							y++;
						}
						fb[i++] = ((pStruct->imageBpp == 32 ? dataPtr[byteOffset + 3] : 0xFF) << 24) + 
								  (dataPtr[byteOffset + 2] << 16) + 
								  (dataPtr[byteOffset + 1] << 8) +
								  (dataPtr[byteOffset]);
                    }
                    byteOffset += pStruct->imageBpp >> 3;
                }
				else
				{
                    k++; x += k;
                    while (k--)
					{
                        if (i % pStruct->width == 0)
						{
							i = ((pStruct->y ? y : pStruct->height - y - 1) * pStruct->width);
							y++;
						}
						fb[i++] = ((pStruct->imageBpp == 32 ? dataPtr[byteOffset + 3] : 0xFF) << 24) + 
								  (dataPtr[byteOffset + 2] << 16) + 
								  (dataPtr[byteOffset + 1] << 8) +
								  (dataPtr[byteOffset]);
                        byteOffset += pStruct->imageBpp >> 3;
                    }
                }
            }
			break;
		}
		default:
		{
			SLogMsg("Unknown targa encoding scheme %d", pStruct->encoding);
			*error = BMPERR_BAD_COLOR_PLANES;
			MmFree(pImage);
			return NULL;
		}
	}
	
	*error = 0;
	
	return pImage;
}

Image* LoadImageFile(void *pImageData, int *pErrorOut)
{
	//this tries to guess the specific format to load
	
	///    HEADERED FILE TYPES    ///
	/// (examples: bmp, png, gif) ///
	
	// Is this a Windows bitmap file?
	if (*((uint16_t*)pImageData) == 0x4D42) // 'BM'
	{
		// Yes, load it as if it were a bmp file without trying others
		return LoadBitmap (pImageData, pErrorOut);
	}
	
	/// HEADERLESS FILE TYPES ///
	
	// Try a Targa file
	if (*((uint8_t*)pImageData) == 0) // The first byte must be a zero
	{
		Image* pImg = LoadTarga(pImageData, pErrorOut);
		if (pImg) return pImg;
		
		//*pErrorOut = BMPERR_UNKNOWN_FORMAT;
		return NULL;
	}
	
	*pErrorOut = BMPERR_UNKNOWN_FORMAT;
	return NULL;
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
