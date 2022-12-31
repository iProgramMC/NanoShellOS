// stdlib.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _STDLIB__H
#define _STDLIB__H

#include <stdint.h>
#include <stddef.h>
#include <nanoshell/stdlib_types.h>

// Numeric conversion functions
int    atoi       (const char* str);
int    atox       (const char* str);
double atof       (const char *arr);
char*  itoa       (int value, char* buffer, int radix);
char*  ltoa       (long value, char* buffer, int radix);

// Memory management
void* malloc (size_t size);
void* calloc (size_t nmemb, size_t size);
void  free   (void*  ptr);
void* realloc(void*  ptr, size_t sz);

// Arithmetic utilities
int abs(int k);
double fabs(double x);

// Random number generator
int GetRandom(void); // use the kernel RNG
int rand(void);
int rand_r(unsigned int * seed);
void srand(unsigned int seed);

// Communication with the environment
__attribute__((noreturn)) void exit(int status);
__attribute__((noreturn)) void abort(void);
//TODO: atexit
//TODO: getenv
//TODO: system

// Sorting functions
// NOTE: I'd prefer you don't use these right now. They're just bubble sort. I'll implement a smarter sorting algorithm later on.
void qsort(void *pBase, size_t nCount, size_t nElementSize, ComparisonFunc pCompare);
void qsort_r(void *pBase, size_t nCount, size_t nElementSize, ComparisonReentrantFunc pCompareReentrant, void* pArgument);

// Environment
char* getenv(const char* name);

// Safe string conversion
unsigned long long strtoux(const char* str, char ** endptr, int base, unsigned long long max);
long long strtox(const char* str, char ** endptr, int base, long long max);
unsigned long long int strtoull(const char* str, char ** endptr, int base);
unsigned long int strtoul(const char* str, char ** endptr, int base);
long long int strtoll(const char* str, char ** endptr, int base);
long int strtol(const char* str, char ** endptr, int base);
double ldexp(double val, int exp);
double strtod(const char* str, char** endptr);
long double strtold(const char* str, char** endptr);
float strtof(const char* str, char** endptr);

#endif//_STDLIB__H
