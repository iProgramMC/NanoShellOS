// string.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _STRING__H
#define _STRING__H

int    memcmp     (const void* ap, const void* bp, size_t size);
void*  memcpy     (void* restrict dstptr, const void* restrict srcptr, size_t size);
void*  memmove    (void* restrict dstptr, const void* restrict srcptr, size_t size);
void*  memset     (void* bufptr, BYTE val, size_t size);
size_t strlen     (const char* str);
void*  strcpy     (const char* ds, const char* ss);
int    strcmp     (const char* as, const char* bs);
int    strncmp    (const char* s1, const char* s2, size_t n);
void   strcat     (char* dest, const char* after);
void   strtolower (char* as);
void   strtoupper (char* as);
void   memtolower (char* as, int w);
void   memtoupper (char* as, int w);
size_t strgetlento(const char* str, char chr);
char*  strdup     (const char *pText);
char*  strstr     (char *string, char *substring);
char*  strncpy    (char *dst, const char *src, size_t n);
size_t strnlen    (const char* str, size_t szmax);
char*  strchr     (const char* s, int c);
char*  strrchr    (const char* s, int c);
char*  strchrnul  (const char* s, int c); // GNU extension

const char * strerror (int errnum);

#endif//_STRING__H
