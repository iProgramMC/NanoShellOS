//  ***************************************************************
//  utf8.h - Creation date: 11/03/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _UTF8_H
#define _UTF8_H

// The Unicode point of the replacement character.
#define REPLACEMENT_CHAR (0xFFFD)

// The maximum size of a UTF-8 character.
#define MAX_UTF8_SIZE (4)

// Decodes a character, and spits out a working Unicode point and the size the character occupied.
int Utf8DecodeCharacter(const char* pByteSeq, int* pSizeOut);

// Decode a character from UTF-8 data and increment the pointer.
int Utf8GetCharacterAndIncrement(const char** pByteSeq);

// Encode a Unicode point into a UTF-8 byte sequence.
void Utf8EncodeCharacter(char * pByteSeq, int * pSizeOut, int codePoint);

// Concatenates a UTF-8 code sequence to a string.
void Utf8ConcatenateCharacter(char* pByteSeq, int codePoint);


#endif//_UTF8_H