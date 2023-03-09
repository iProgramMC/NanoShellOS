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
}
__attribute__((packed))
BmpHeaderStruct;

typedef struct TgaHeaderStruct
{
	uint8_t  magic1;   //offset 0
	uint8_t  colorMap; //offset 1
	uint8_t  encoding; //offset 2
	uint16_t cMapOrig; //offset 3,4 
	uint16_t cMapLen;  //offset 5,6
	uint8_t  cMapEnt;  //offset 7
	uint16_t x;        //offset 8,9
	uint16_t y;        //offset 10,11
	uint16_t width;    //offset 12,13
	uint16_t height;   //offset 14,15
	uint8_t  imageBpp; //offset 16
	uint8_t  pixelType;//offset 17
}
__attribute__((packed))
TgaHeaderStruct;

//TgaEncoding
enum
{
	TGA_NO_IMAGE,
	TGA_UNCOMPRESSED_COLORMAPPED,
	TGA_UNCOMPRESSED,
	TGA_UNCOMPRESSED_GRAYSCALE,
	TGA_RLE_COLORMAPPED = 9,
	TGA_RLE,
};

//BmpLoadErrors
enum
{
	BMPERR_SUCCESS,            // 0
	BMPERR_PATH_EMPTY,         // 1
	BMPERR_FILE_NOT_FOUND,     // 2
	BMPERR_FILE_TOO_SMALL,     // 3
	BMPERR_INVALID_HEADER,     // 4
	BMPERR_NO_IMAGE_POINTER,   // 5
	BMPERR_BAD_BPP,            // 6
	BMPERR_BAD_COLOR_PLANES,   // 7
	BMPERR_BAD_ALLOC,          // 8
	BMPERR_UNKNOWN_FORMAT,     // 9
};

// to load specific formats
Image *LoadBitmap (void* pBmpData, int *pErrorOut);
Image *LoadTarga (void* pTgaData, int *pErrorOut);

// to load any image file
Image *LoadImageFile(void *pImageData, int *pErrorOut);

Image *BitmapDuplicate(Image* pSourceImage);
Image *BitmapAllocate(int width, int height, uint32_t default_color);
void BuildGraphCtxBasedOnImage(VBEData *pData, Image *pImage);

Image *GetImageFromResource(int resID);

#endif//_IMAGE_H