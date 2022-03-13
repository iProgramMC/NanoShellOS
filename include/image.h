/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

         IMAGE module header file
******************************************/
#ifndef _IMAGE_H
#define _IMAGE_H

#define BI_RGB 0 // Only supported mode
// Can be used only with X bit bitmaps - not supported
#define BI_RLE8 1
#define BI_RLE4 2
// Huffman 1D tree - not supported
#define BI_BITFIELDS 3
// BITMAPV4INFOHEADER+ - not supported
#define BI_JPEG 4
#define BI_PNG 5
// only for Windows CE 5.0 with .NET 4.0 or later - not supported
#define BI_ALPHABITFIELDS 6
// only Windows Metafile CMYK - not supported
#define BI_CMYK 11
#define BI_CMYKRLE8 12
#define BI_CMYKRLE4 13


typedef struct BmpHeaderStruct
{
	// bitmap header
	uint16_t bitmapHeader;
	uint32_t bitmapSize;
	uint32_t reserved;
	uint32_t pixArrayOffset;
	// dib header (bmp info header)
	uint32_t dibHeaderType;
	// For this loader, we're only going to support BITMAPINFOHEADER, with no
	// gaps and no other features
	int32_t bmpWidthPixels;
	int32_t bmpHeightPixels;
	uint16_t colorPlaneCount; // must be 1
	uint16_t imageBpp;
	uint32_t compressionMethod;
	uint32_t imageSize; // can be 0 for BI_RGB bitmaps
	int32_t bmpHResPPM; // ignored
	int32_t bmpVResPPM; // ignored
	uint32_t numColors;
	uint32_t numImportantColors;
}__attribute__((packed))
BmpHeaderStruct;

//BmpLoadErrors
enum
{
	BMPERR_SUCCESS,
	BMPERR_PATH_EMPTY,
	BMPERR_FILE_NOT_FOUND,
	BMPERR_FILE_TOO_SMALL,
	BMPERR_INVALID_HEADER,
	BMPERR_NO_IMAGE_POINTER,
	BMPERR_BAD_BPP,
	BMPERR_BAD_COLOR_PLANES,
	BMPERR_BAD_ALLOC,
};

Image *LoadBitmap (void* pBmpData, int *pErrorOut);
Image *BitmapAllocate(int width, int height, uint32_t default_color);
void BuildGraphCtxBasedOnImage(VBEData *pData, Image *pImage);

#endif//_IMAGE_H