#include <memory.h>
#include <string.h>
#include "png.h"
#include "tinfl.h"

// Thanks to <https://gist.github.com/qookei/3e0b068ee8c6926da3016d12f95a4578> (qookei)

enum
{
	FILTER_NONE,
	FILTER_SUB,
	FILTER_UP,
	FILTER_AVERAGE,
	FILTER_PAETH,
};

typedef struct
{
	uint32_t m_size;
	char     m_type[4];
	void*    m_data;
}
PNGChunk;

typedef struct
{
	void*  m_ptr;
	size_t m_size;
	size_t m_index;
}
PNGState;

SAI void* OffsetOf(void* thing, size_t offset)
{
	return (void*) ( (uintptr_t) thing + offset );
}

SAI uint32_t BigEndToHost32(uint32_t val)
{
	// byte swap from big endian
	uint8_t* ptr = (uint8_t*) &val;
	uint32_t out = 0;
	out |= ptr[3];
	out |= ptr[2] << 8;
	out |= ptr[1] << 16;
	out |= ptr[0] << 24;
	return out;
}

bool PNGCheck(PNGState* pState)
{
	if (pState->m_size < 8)
		return true;
	
	pState->m_index += 8;
	
	return memcmp(OffsetOf(pState->m_ptr, pState->m_index - 8), "\x89PNG\r\n\x1A\n", 8) != 0;
}

SAI uint8_t PNGSubFilter(uint8_t* line, size_t pixelSize, size_t x, size_t i)
{
	if (x == 0)
		return line[pixelSize * x + i];
	
	return line[pixelSize * x + i] + line[pixelSize * (x - 1) + i];
}

SAI uint8_t PNGUpFilter(uint8_t* line, uint8_t* prevLine, size_t pixelSize, size_t x, size_t i)
{
	if (!prevLine)
		return line[pixelSize * x + i];
	
	return line[pixelSize * x + i] + prevLine[pixelSize * x + i];
}

SAI uint8_t PNGAverageFilter(uint8_t* line, uint8_t* prevLine, size_t pixelSize, size_t x, size_t i)
{
	uint8_t left = 0, top = 0;
	if (x)
		left = line[pixelSize * (x - 1) + i];
	if (prevLine)
		top  = prevLine[pixelSize * x + i];
	
	return ((left + top) / 2) + line[pixelSize * x + i];
}

SAI int PNGAbs(int x)
{
	if (x < 0)
		return -x;
	
	return x;
}

SAI uint8_t PNGPaethFilter(uint8_t *line, uint8_t *prevLine, size_t pixelSize, size_t x, size_t i) {
	// using int as calculations "must be performed exactly, without overflow"
	int a = 0, b = 0, c = 0, d, p;
	int pa, pb, pc;
	
	d = line[pixelSize * x + i];
	
	if (x)
		a = line[pixelSize * (x - 1) + i];
	
	if (prevLine)
		b = prevLine[pixelSize * x + i];
	
	if (prevLine && x)
		c = prevLine[pixelSize * (x - 1) + i];
	
	p = a + b - c;
	
	pa = PNGAbs(p - a);
	pb = PNGAbs(p - b);
	pc = PNGAbs(p - c);
	
	if (pa <= pb && pa <= pc)
		return d + a;
	if (pb <= pc)
		return d + b;
	
	return d + c;
}

bool PNGFetchN(PNGState* pState, void* out, size_t count)
{
	if (pState->m_index + count > pState->m_size)
		return false;
	
	memcpy(out, OffsetOf(pState->m_ptr, pState->m_index), count);
	pState->m_index += count;
	return true;
}

bool PNGFetch8(PNGState* pState, uint8_t* out)
{
	return PNGFetchN(pState, out, 1);
}

bool PNGFetch16(PNGState* pState, uint16_t* out)
{
	return PNGFetchN(pState, out, 2);
}

bool PNGFetch32(PNGState* pState, uint32_t* out)
{
	return PNGFetchN(pState, out, 4);
}

bool PNGFetchNextChunk(PNGState* pState, PNGChunk* pOut)
{
	if (!PNGFetch32(pState, &pOut->m_size))
		return false;
	
	pOut->m_size = BigEndToHost32(pOut->m_size);
	
	if (!PNGFetchN(pState, pOut->m_type, 4))
		return false;
	
	if (pState->m_index + pOut->m_size + 4 > pState->m_size)
		return false;
	
	pOut->m_data = OffsetOf(pState->m_ptr, pState->m_index);
	pState->m_index += pOut->m_size + 4;
	
	return true;
}

#define C_OUTPUT_SIZE_FACTOR 200

bool PNGDecompressIDAT(PNGState* pState, void* pOutData, size_t bufSize)
{
	// duplicate the state to iterate over the PNG chunks without affecting the in state
	PNGState stateCopy = *pState;
	void* pInput = NULL;
	size_t inputSize = 0;
	
	PNGChunk chk;
	
	// Gather scattered IDAT chunks into one buffer (I hope this is correct)
	while (PNGFetchNextChunk(&stateCopy, &chk))
	{
		if (memcmp(chk.m_type, "IDAT", 4))
			continue;
		
		pInput = MmReAllocate(pInput, inputSize + chk.m_size);
		memcpy(OffsetOf(pInput, inputSize), chk.m_data, chk.m_size);
		inputSize += chk.m_size;
	}
	
	if (!pInput)
	{
		// No IDATs?!
		SLogMsg("Error, no idats exist");
		return false;
	}
	
	// Decompress it
	//size_t outSize = bufSize;
	
	size_t bytesWritten = tinfl_decompress_mem_to_mem(pOutData, bufSize, pInput, inputSize, TINFL_FLAG_PARSE_ZLIB_HEADER);
	if (bytesWritten == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED)
	{
		SLogMsg("Error, decompression failed");
		MmFree(pInput);
		return false;
	}
	
	MmFree(pInput);
	return true;
}

SAI uint32_t MakeColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
	if (alpha == 0)
		return TRANSPARENT;
	
	return red << 16 | green << 8 | blue;
}

bool PNGParsePLTE(PNGState* state, uint32_t* palette, int* palSizeOut)
{
	PNGState stateCopy = *state;
	PNGChunk chk;
	bool found = false;
	while (PNGFetchNextChunk(&stateCopy, &chk))
	{
		if (!memcmp(chk.m_type, "PLTE", 4))
		{
			found = true;
			break;
		}
	}
	
	if (!found)
		return false;
	
	if (chk.m_size % 3 != 0) // "A chunk length not divisible by 3 is an error." - libpng spec
	{
		SLogMsg("Error, PLTE chunk's size is not divisible by three.");
		return false;
	}
	
	int palSize;
	*palSizeOut = palSize = (int)chk.m_size / 3;
	
	uint8_t* data = chk.m_data;
	
	// copy the palette entries:
	for (int i = 0; i < palSize; i++)
	{
		*(palette++) = data[0] << 16 | data[1] << 8 | data[2];
		data += 3;
	}
	
	return true;
}

bool PNGParseTRNS(PNGState* state, uint32_t* palette, int palSize)
{
	PNGState stateCopy = *state;
	PNGChunk chk;
	bool found = false;
	while (PNGFetchNextChunk(&stateCopy, &chk))
	{
		if (!memcmp(chk.m_type, "tRNS", 4))
		{
			found = true;
			break;
		}
	}
	
	// if we don't have a tRNS chunk it's completely fine
	if (!found)
		return true;
	
	if ((int)chk.m_size != palSize) // for color type 3, 
	{
		SLogMsg("Error, tRNS size %d does not match palette size of %d.", (int)chk.m_size, palSize);
		return false;
	}
	
	// update the palette entries if their alpha is zero
	uint8_t* data = chk.m_data;
	for (int i = 0; i < palSize; i++)
	{
		if (*data == 0)
			*palette = TRANSPARENT;
		
		data++;
		palette++;
	}
	
	return true;
}

//void SDumpBytesAsHex (void *nAddr, size_t nBytes, bool as_bytes);

Image* LoadPNG(void* imgData, size_t imgSize, int* error)
{
	PNGState state = { imgData, imgSize, 0 };
	
	if (PNGCheck(&state))
	{
		SLogMsg("Error, header check failed");
	InvalidHeader:
		*error = BMPERR_INVALID_HEADER;
		return NULL;
	}
	
	PNGChunk chk;
	if (!PNGFetchNextChunk(&state, &chk))
	{
		SLogMsg("Error, there are no chunks in this png");
		goto InvalidHeader;
	}
	
	if (memcmp(chk.m_type, "IHDR", 4) != 0)
	{
		SLogMsg("Error, the first chunk isn't an IHDR");
		goto InvalidHeader;
	}
	
	if (chk.m_size != 13)
	{
		SLogMsg("Error, the IHDR isn't exactly 13 bytes");
		goto InvalidHeader;
	}
	
	uint32_t width  = BigEndToHost32(*(uint32_t*) OffsetOf(chk.m_data, 0));
	uint32_t height = BigEndToHost32(*(uint32_t*) OffsetOf(chk.m_data, 4));
	uint8_t  bitDepth    = *(uint8_t*) OffsetOf(chk.m_data, 8);
	uint8_t  colorType   = *(uint8_t*) OffsetOf(chk.m_data, 9);
	uint8_t  filter      = *(uint8_t*) OffsetOf(chk.m_data, 11);
	uint8_t  interlace   = *(uint8_t*) OffsetOf(chk.m_data, 12);
	//uint8_t  compression = *(uint8_t*) OffsetOf(chk.m_data, 10);
	
	bool isTrueColor = colorType & 0x2;
	bool hasPalette  = colorType & 0x1;
	bool hasAlpha    = colorType & 0x4;
	//bool isGrayScale = !isTrueColor;
	
	//SLogMsg("width: %u, height: %u, bpp: %d, color type: %d, compression: %d, filter: %d, interlace: %d\n",
	//		 width, height, bitDepth, colorType, compression, filter, interlace);
	
	if (bitDepth > 8 || (bitDepth & (bitDepth - 1)))
	{
		SLogMsg("Error, bit depth %d is not a power of two or is bigger than 8.", bitDepth);
		*error = BMPERR_BAD_BPP;
		return NULL;
	}
	
	int palSize = 0;
	uint32_t palette[256]; // palette in standard nanoshell colors
	
	if (hasPalette)
	{
		if (!PNGParsePLTE(&state, palette, &palSize) || !PNGParseTRNS(&state, palette, palSize))
		{
			SLogMsg("Error, invalid palette");
			*error = BMPERR_BAD_BPP;
			return NULL;
		}
	}
	
	size_t numChannels = 1;
	if (isTrueColor)
		numChannels += 2;
	if (hasAlpha)
		numChannels++;
	if (hasPalette)
		numChannels = 1;
	
	size_t pixelSize = numChannels * (bitDepth / 8);
	
	if (pixelSize > 4 || pixelSize < 1 || bitDepth != 8)
	{
		SLogMsg("Error, don't support HDR, or bit depth higher than eight per channel    pixelSize=%d   bitDepth=%d   numChannels=%d",pixelSize,bitDepth,numChannels);
		*error = BMPERR_BAD_BPP;
		return NULL;
	}
	
	if (filter != 0 || interlace != 0) // TODO
	{
		SLogMsg("Error, don't support filtered or interlaced pngs right now");
		goto InvalidHeader;
	}
	
	size_t rawSize = (width * height * pixelSize) + height;
	size_t outSize = (width * height * sizeof(uint32_t));
	
	Image* image   = MmAllocate(sizeof(Image) + outSize);
	if (!image)
	{
		SLogMsg("Error, can't allocate image");
		*error = BMPERR_BAD_ALLOC;
		return NULL;
	}
	
	void* rawData = MmAllocate(rawSize);
	if (!rawData)
	{
		SLogMsg("Error, can't allocate temporary raw data");
		MmFree(image);
		*error = BMPERR_BAD_ALLOC;
		return NULL;
	}
	
	image->width  = width;
	image->height = height;
	image->framebuffer = (uint32_t*) OffsetOf(image, sizeof(*image)); // wow!
	
	if (!PNGDecompressIDAT(&state, rawData, rawSize))
	{
		SLogMsg("Error, can't decompress IDAT");
	BadColorPlanes:
		MmFree(image);
		*error = BMPERR_BAD_COLOR_PLANES;
		return NULL;
	}
	
	// dump the raw IDAT data
	//SDumpBytesAsHex(rawData,rawSize,true);
	
	uint8_t* buf = rawData;
	
	for (uint32_t y = 0; y < height; y++)
	{
		uint8_t filterMethod = buf[y * (width * pixelSize + 1)];
		
		uint8_t* prevLine = NULL;
		uint8_t* line = buf + y * (width * pixelSize + 1) + 1;
		
		if (y)
			prevLine = buf + (y - 1) * (width * pixelSize + 1) + 1;
		
		for (uint32_t x = 0; x < width; x++)
		{
			for (size_t i = 0; i < pixelSize; i++)
			{
				uint8_t actualValue;
				
				switch (filterMethod)
				{
					case FILTER_NONE:
						actualValue = line[x * pixelSize + i];
						break;
						
					case FILTER_SUB:
						actualValue = PNGSubFilter(line, pixelSize, x, i);
						break;
						
					case FILTER_UP:
						actualValue = PNGUpFilter(line, prevLine, pixelSize, x, i);
						break;
						
					case FILTER_AVERAGE:
						actualValue = PNGAverageFilter(line, prevLine, pixelSize, x, i);
						break;
						
					case FILTER_PAETH:
						actualValue = PNGPaethFilter(line, prevLine, pixelSize, x, i);
						break;
						
					default:
						SLogMsg("Error, don't support filter %d", filterMethod);
						goto BadColorPlanes;
				}
				
				line[pixelSize * x + i] = actualValue;
			}
		}
	}
	
	// write proper ARGB pixels instead
	uint32_t* outArray = (uint32_t*) image->framebuffer;
	for (uint32_t y = 0, outIndex = 0; y < height; y++)
	{
		uint8_t*  line    = buf + y * (width * pixelSize + 1) + 1;
		uint32_t* outLine = &outArray[outIndex];
		
		for (uint32_t x = 0; x < width; x++, outIndex++, outLine++)
		{
			uint32_t offset = pixelSize * x;
			if (hasPalette)
			{
				*outLine = palette[line[offset]];
				continue;
			}
			
			uint8_t alpha = 255, red = 0, green = 0, blue = 0;
			
			if (pixelSize > 2)
			{
				red = line[offset + 0], green = line[offset + 1], blue = line[offset + 2];
				if (hasAlpha)
					alpha = line[offset + 3];
			}
			else
			{
				red = green = blue = line[offset];
				if (hasAlpha)
					alpha = line[offset + 1];
			}
			
			*outLine = MakeColor(red, green, blue, alpha);
		}
	}
	
	MmFree(rawData);
	
	return image;
}
