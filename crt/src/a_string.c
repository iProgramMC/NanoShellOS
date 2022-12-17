//  ***************************************************************
//  a_string.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

// Character operations

int isalnum(int c)
{
	return (c  >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

int isalpha(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isascii(int c)
{
	return (c >= 0 && c < 0x80);
}

int isblank(int c)
{
	return (c == ' ' || c == '\n' || c == '\0');
}

int iscntrl(int c)
{
	return (c >= 0 && c <= 0x1F);
}

bool isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

int isgraph(int c)
{
	return (c >= '!' && c <= 0x7E);
}

int islower(int c)
{
	return (c >= 'a' && c <= 'z');
}

int isprint(int c)
{
	return (c >= ' ' && c <= 0x7E);
}

int isspace(int c)
{
	return (c == ' ' || c == '\n');
}

int isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}

int isxdigit(int c)
{
	return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || (c >= '0' && c <= '9');
}

int tolower (int c)
{
	if (isupper(c)) return c + ('a' - 'A'); else return c;
}

int toupper (int c)
{
	if (islower(c)) return c - ('a' - 'A'); else return c;
}

// Memory operations.

int memcmp(const void* ap, const void* bp, size_t size)
{
	const char* a = (const char*) ap;
	const char* b = (const char*) bp;
	for (size_t i = 0; i < size; i++)
	{
		if (a[i] != b[i]) return a[i] - b[i];
	}
	return 0;
}

void* memmove(void* restrict dstptr, const void* restrict srcptr, size_t size)
{
	uint8_t* dst = (uint8_t*) dstptr;
	const uint8_t* src = (const uint8_t*) srcptr;
	
	//move forwards
	if (dst < src)
	{
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	}
	//move backwards
	else
	{
		for (size_t i = size; i > 0; i--)
			dst[i-1] = src[i-1];
	}
	
	return dstptr;
}

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size)
{
	return memmove(dstptr, srcptr, size);
}

void* memset(void* bufptr, uint8_t val, size_t size)
{
	uint8_t* buf = (uint8_t*) bufptr;
	for (size_t i = 0; i < size; i++)
	{
		buf[i] = val;
	}
	return bufptr;
}

// String operations.

size_t strgetlento(const char* str, char chr) 
{
	size_t len = 0;
	while (str[len] != chr)
	{
		len++;
	}
	return len;
}

int atoi(const char* str)
{
	int f = 0;
	int s = 1;
	int i = 0;
	if (str[0] == '-')
	{
		i++;
		s = -1;
	}
	for (; str[i] != '\0'; i++)
		f = f * 10 + str[i] - '0';
	
	return s * f;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
	{
		len++;
	}
	return len;
}

void* strcpy(const char* ds, const char* ss)
{
	return memcpy((void*)ds, (void*)ss, strlen(ss) + 1);
}

void strtolower(char* as)
{
	while(*as != 0)
	{
		if(*as >= 'A' && *as <= 'Z')
			*as += ('a' - 'A');
		as++;
	}
}

void strtoupper(char* as)
{
	while(*as != 0)
	{
		if(*as >= 'a' && *as <= 'z')
			*as -= ('a' - 'A');
		as++;
	}
}

void memtolower(char* as, int w)
{
	int a = 0;
	while(a <= w)
	{
		if(*as >= 'A' && *as <= 'Z')
			*as += ('a' - 'A');
		as++;
		a++;
	}
}

void memtoupper(char* as, int w)
{
	int a = 0;
	while(a <= w)
	{
		if(*as >= 'a' && *as <= 'z')
			*as -= ('a' - 'A');
		as++;
		a++;
	}
}

int strcmp(const char* as, const char* bs)
{
	size_t al = strlen(as);
	size_t bl = strlen(bs);
	if (al < bl)
	{
		return -1;
	}
	else if (al > bl)
	{
		return 1;
	}
	else if (al == bl)
	{
		return memcmp((void*)as, (void*)bs, al + 1); // Also compare null term
	}
	return 0;
}

void strcat(char* dest, const char* after)
{
	char* end = strlen(dest) + dest;
	memcpy(end, after, strlen(after) + 1);
}

char* strchr (const char* str, int c);

char* Tokenize (TokenState* pState, char* pString, char* separator)
{
    int len, i;
	
    if(!pState->m_bInitted)
	{
        pState->m_bInitted = 1;
        pState->m_pContinuation = NULL;
        pState->m_pReturnValue = NULL;
    }
    else if (!pState->m_pContinuation)
	{
		return NULL;
	}
	
    if(!pString)
	{
        pString = pState->m_pContinuation;
    }
	
    if (!pString)
	{
		return NULL;
	}
	
    len = strlen (pString);

    for (i = 0; i < len; i++)
	{
        if (strchr(separator, pString[i]))
		{
            pState->m_pContinuation = &pString[i+1];
            pString[i] = 0;
            pState->m_pReturnValue = pString;
            return pString;
        }
    }
	
    // not found, hit null term:
    pState->m_pContinuation = NULL;
    pState->m_pReturnValue = pString;
    return pString;
}

void *malloc (size_t size);

char *strdup (const char *pText)
{
	size_t len = strlen (pText) + 1;
	char *p = malloc (len);
	if (p)
		memcpy (p, pText, len);
	return p;
}

char * strncpy(char *dst, const char *src, size_t n)
{
	if (n != 0)
	{
		register char *d = dst;
		register const char *s = src;

		do
		{
			if ((*d++ = *s++) == 0)
			{
				// null-pad the remaining n-1 bytes
				while (--n != 0)
					*d++ = 0;
				break;
			}
		}
		while (--n != 0);
	}
	return (dst);
}

char* itoa(int value, char* buffer, int radix)
{
	assert(radix > 1 && radix < 37);
	const char* lut = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	char temp[50];
	int i = 0;
	
	if (value < 0)
	{
		value = -value;
		*buffer++ = '-';
	}
	
	do
	{
		temp[i++] = lut[value % radix];
		value /= radix;
	}
	while (value);
	
	// store it as reversed
	for (int j = 0; j < i; j++)
		buffer[j] = temp[i - j - 1];
	
	return buffer;
}

char* ltoa(long value, char* buffer, int radix)
{
	return itoa((int)value, buffer, radix);
}

static char* strchr_i(const char* s, int c, bool bReturnNulPos)
{
	// promotion to 'char' intended
	char* sc = (char*) s;
	
	while (*sc)
	{
		if (*sc == c) return sc;
		
		sc++;
	}
	
	// assert(*sc == 0);
	
	return bReturnNulPos ? sc : NULL;
}

char* strchr(const char* s, int c)
{
	return strchr_i(s, c, false);
}

char* strchrnul(const char* s, int c)
{
	return strchr_i(s, c, true);
}

char* strrchr(const char* s, int c)
{
	// promotion to 'char' intended
	char* sc = (char*) s;
	while (*sc) sc++;
	
	while (sc != s)
	{
		sc--;
		if (*sc == c) return sc;
	}
	
	return NULL;
}

int atox(const char* str) 
{
	int f = 0;
	int s = 1;
	int i = 0;
	if (str[0] == '-')
	{
		i++;
		s = -1;
	}
	for (; str[i] != '\0'; i++)
	{
		f = f * 16;
		if (str[i] >= 'a' && str[i] <= 'f')
			f += str[i] - 'a' + 0xa;
		else if (str[i] >= 'A' && str[i] <= 'F')
			f += str[i] - 'A' + 0xA;
		else
			f += str[i] - '0';
	}
	
	return s * f;
}

size_t strnlen(const char* ptext, size_t n)
{
	size_t k = 0;
	while (*ptext)
	{
		k++;
		if (k == n) return n;
		ptext++;
	}
	return k;
}

int strncmp(const char *s1, const char *s2, register size_t n)
{
	unsigned char u1, u2;
	while (n-- > 0)
	{
		u1 = (unsigned char) *s1++;
		u2 = (unsigned char) *s2++;
		if (u1 != u2)
			return u1 - u2;
		if (u1 == '\0')
			return 0;
	}
	return 0;
}

char * strstr (const char *s1, const char *s2)
{
	const char *p = s1;
	const size_t len = strlen (s2);
	for (; (p = strchr (p, *s2)) != 0; p++)
	{
		if (strncmp (p, s2, len) == 0)
			return (char *)p;
	}
	return (0);
}

double atof(const char *arr)
{
	double value = 0;
	bool decimal = 0;
	double scale = 1;
	int negative = 0; 
	
	// parse the negative sign
	if (*arr == '-')
	{
		arr++;
		negative = 1;
	}
	
	while (*arr)
	{
		// if we're parsing the decimal part
		if (decimal)
		{
			scale /= 10;
			value += (*arr - '0') * scale;
		}
		else
		{
			if (*arr == '.') 
				decimal = 1;
			else
				value = value * 10.0 + (*arr - '0');
		}
		arr++;
	}
	
	if (negative) value = -value;
	
	return  value;
}
