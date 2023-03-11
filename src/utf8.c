//  ***************************************************************
//  utf8.c - Creation date: 11/03/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <main.h>
#include <string.h>

SAI int Utf8DecodeCharacterInline(const char* pByteSeq, int* pSizeOut)
{
	const uint8_t * chars = (const uint8_t*)pByteSeq;
	
	uint8_t char0 = chars[0];
	
	if (char0 < 0x80)
	{
		// 1 byte ASCII character. Fine
		if (pSizeOut) *pSizeOut = 1;
		return char0;
	}
	
	char char1 = chars[1];
	if (char1 == 0) return REPLACEMENT_CHAR; // this character is broken.
	if (char0 < 0xE0)
	{
		// 2 byte character.
		int codepoint = ((char0 & 0x1F) << 6) | (char1 & 0x3F);
		if (pSizeOut) *pSizeOut = 2;
		return codepoint;
	}
	
	char char2 = chars[2];
	if (char2 == 0) return REPLACEMENT_CHAR; // this character is broken.
	if (char0 < 0xF0)
	{
		// 3 byte character.
		int codepoint = ((char0 & 0xF) << 12) | ((char1 & 0x3F) << 6) | (char2 & 0x3F);
		if (pSizeOut) *pSizeOut = 3;
		return codepoint;
	}
	
	char char3 = chars[3];
	// 4 byte character.
	if (char3 == 0) return REPLACEMENT_CHAR;
	
	int codepoint = ((char0 & 0x07) << 18) | ((char1 & 0x3F) << 12) | ((char2 & 0x3F) << 6) | (char3 & 0x3F);
	if (pSizeOut) *pSizeOut = 4;
	return codepoint;
}

int Utf8DecodeCharacter(const char* pByteSeq, int* pSizeOut)
{
	return Utf8DecodeCharacterInline(pByteSeq, pSizeOut);
}

int Utf8GetCharacterAndIncrement(const char** pByteSeq)
{
	int sizeOut = 0;
	int code = Utf8DecodeCharacterInline(*pByteSeq, &sizeOut);
	(*pByteSeq) += sizeOut;
	return code;
}

void Utf8EncodeCharacter(char * pByteSeq, int * pSizeOut, int codePoint)
{
	if (codePoint < 0)
		return Utf8EncodeCharacter(pByteSeq, pSizeOut, REPLACEMENT_CHAR); // okay?
	
	if (codePoint < 0x80)
	{
		*pByteSeq = (char)codePoint;
		*pSizeOut = 1;
		return;
	}
	
	if (codePoint < 0x800)
	{
		*(pByteSeq++) = 0b11000000 | (codePoint >> 6);
		*(pByteSeq++) = 0b10000000 | (codePoint & 0x3F);
		*pSizeOut = 2;
		return;
	}
	
	if (codePoint < 0x10000)
	{
		*(pByteSeq++) = 0b11000000 | (codePoint >> 12);
		*(pByteSeq++) = 0b10000000 | ((codePoint >> 6) & 0x3F);
		*(pByteSeq++) = 0b10000000 | (codePoint & 0x3F);
		*pSizeOut = 3;
		return;
	}
	
	if (codePoint < 0x110000)
	{
		*(pByteSeq++) = 0b11000000 | (codePoint >> 18);
		*(pByteSeq++) = 0b10000000 | ((codePoint >> 12) & 0x3F);
		*(pByteSeq++) = 0b10000000 | ((codePoint >> 6) & 0x3F);
		*(pByteSeq++) = 0b10000000 | (codePoint & 0x3F);
		*pSizeOut = 4;
		return;
	}
	
	return Utf8EncodeCharacter(pByteSeq, pSizeOut, REPLACEMENT_CHAR);
}

void Utf8ConcatenateCharacter(char* pByteSeq, int codePoint)
{
	char buffer[MAX_UTF8_SIZE + 1];
	int  size = 0;
	
	Utf8EncodeCharacter(buffer, &size, codePoint);
	buffer[size] = 0;
	
	strcat(pByteSeq, buffer);
}
