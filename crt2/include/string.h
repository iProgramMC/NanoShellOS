// string.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _STRING__H
#define _STRING__H

#include <sys/types.h>

int    memcmp     (const void* s1, const void* s2, size_t n);
void*  memcpy     (void* dest, const void* src, size_t n);
void*  memmove    (void* dest, const void* src, size_t n);
void*  memset     (void* s, int c, size_t n);
size_t strlen     (const char* s);
char*  strcpy     (char *dest, const char* src);
char*  strncpy    (char *dest, const char *src, size_t n);
size_t strlcpy    (char *dest, const char *src, size_t n);
int    strcmp     (const char* as, const char* bs);
int    strncmp    (const char* s1, const char* s2, size_t n);
char*  strcat     (char* dest, const char* after);
void   strtolower (char* as);
void   strtoupper (char* as);
void   memtolower (char* as, int w);
void   memtoupper (char* as, int w);
size_t strgetlento(const char* str, char chr);
char*  strdup     (const char *pText);
char*  strstr     (const char *haystack, const char *needle);
size_t strnlen    (const char* str, size_t szmax);
char*  strchr     (const char* s, int c);
char*  strrchr    (const char* s, int c);
char*  strchrnul  (const char* s, int c); // GNU extension
size_t strspn     (const char* s, const char* accept);
size_t strcspn    (const char* s, const char* reject);
void*  memchr     (const void* s, int c, size_t n);
void*  memrchr    (const void* s, int c, size_t n);
void*  rawmemchr  (const void* s, int c);

const char * strerror (int errnum);

#endif//_STRING__H
