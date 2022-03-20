/*****************************************
		NanoShell Operating System
	   (C) 2020-2021 iProgramInCpp

  String manipulation module header file
******************************************/
#ifndef _STRING_H
#define _STRING_H
#include <main.h>

// Memory functions
int memcmp(const void*ap,const void*bp,size_t size);
void* memcpy(void* dstptr, const void* srcptr, size_t size);// TODO: I don't think  we need restrict keyword here
void* memmove(void* restrict dstptr, const void* restrict srcptr, size_t size);
void* memset(void* bufptr, BYTE val, size_t size);
void memtolower(char* as, int w);
void memtoupper(char* as, int w);
void* fast_memset(void* bufptr, BYTE val, size_t size);

// String functions
size_t strgetlento(const char* str, char chr);
int atoi(const char* str) ;
size_t strlen(const char* str) ;
void* strcpy(const char* ds, const char* ss);
void strtolower(char* as);
void strtoupper(char* as);
int strcmp(const char* as, const char* bs);
void strcat(char* dest, const char* after);
char* strchr (char* stringToSearch, const char characterToSearchFor);
char* strdup (const char* pText);//! Make sure to free this.
void fast_memcpy(void* restrict dest, const void* restrict src, int size);//aligns to 32 bytes!!
void fmemcpy32 (void* restrict dest, const void* restrict src, int size);

void memset_ints(void* restrict dest, uint32_t src, int num_ints);
void memcpy_ints(void* restrict dest, const void* restrict src, int num_ints);
void memset_shorts(void* restrict dest, uint32_t src, int num_shorts);

void align4_memcpy(void* restrict dest, const void* restrict src, int size);
void align8_memcpy(void* restrict dest, const void* restrict src, int size);
void align16_memcpy(void* restrict dest, const void* restrict src, int size);

void fmemcpy128(void* restrict dest, const void* restrict src, int size);
void fmemcpy256(void* restrict dest, const void* restrict src, int size);

//Other string functions:
bool EndsWith(const char* pText, const char* pCheck);

//requires 4 byte aligned size.
void ZeroMemory (void* bufptr1, size_t size);

//BetterStrTok: https://github.com/iProgramMC/BetterStrTok
typedef struct {
    bool m_bInitted;
    char*m_pContinuation;
    char*m_pReturnValue;
} TokenState;

char* Tokenize (TokenState* pState, char* pString, char* separator);


#endif//_STRING_H