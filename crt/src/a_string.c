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
#include <limits.h>

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

int isdigit(int c)
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
		if (a[i] != b[i])
			return a[i] - b[i];
	}
	return 0;
}

void* memmove(void* dstptr, const void* srcptr, size_t size)
{
	uint8_t* dst = dstptr;
	const uint8_t* src = srcptr;
	
	// test for overlap
	if (src + size > dst || dst + size > src)
	{
		// overlap found. depending on the direction, copy a different way
		if (dst < src)
		{
			while (size--)
				*dst++ = *src++;
		}
		//move backwards
		else
		{
			dst += size;
			src += size;
			
			while (size--)
				*(--dst) = *(--src);
		}
	}
	else
	{
		while (size--)
			*dst++ = *src++;
	}
	
	return dstptr;
}

void* memcpy(void* dstptr, const void* srcptr, size_t size)
{
	// simply copy:
	uint8_t* dst = dstptr;
	const uint8_t* src = srcptr;
	
	while (size--)
		*dst++ = *src++;
	
	return dstptr;
}

void* memset(void* bufptr, int val, size_t size)
{
	uint8_t* buf = bufptr;
	while (size--)
		*buf++ = val;
	
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

long atol(const char* str)
{
	long f = 0;
	long s = 1;
	long i = 0;
	if (str[0] == '-')
	{
		i++;
		s = -1;
	}
	for (; str[i] != '\0'; i++)
		f = f * 10 + str[i] - '0';
	
	return s * f;
}

long long atoll(const char* str)
{
	long long f = 0;
	long long s = 1;
	long long i = 0;
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
	while (*str++)
		len++;
	
	return len;
}

char* strcpy(char* ds, const char* ss)
{
	char* dso = ds;
	
	while (*ss)
	{
		*ds++ = *ss++;
	}
	
	*ds++ = *ss++;
	
	return dso;
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

int strcmp(const char* s1, const char* s2)
{
	while (true)
	{
		// If we've reached the end in both cases, they're equal
		if (!*s1 && !*s2)
			return 0;
		
		// If either one of these pointers points to a null character,
		// it's fine.
		if (*s1 != *s2)
			return *s1 - *s2;
		
		s1++, s2++;
	}
}

char* strcat(char* dest, const char* after)
{
	char* destold = dest;
	
	// go to the end
	while (*dest++);
	
	// subtract one since (dest++) == 0 increments dest 1 extra time
	dest--;
	
	// strcpy the src string over
	strcpy(dest, after);
	
	return destold;
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
	size_t i;
	for (i = 0; i < n && src[i] != '\0'; i++)
		dst[i] = src[i];
	for (; i < n; i++)
		dst[i] = '\0';
	return dst;
}

size_t strlcpy(char *dst, const char *src, size_t n)
{
	n--;
	size_t i;
	size_t srclen = 0;
	for (i = 0; i < n && src[i] != '\0'; i++)
	{
		dst[i] = src[i];
		srclen++;
	}
	dst[i] = '\0';
	
	// traverse the rest of src to get its length:
	while (src[i++])
		srclen++;
	
	return srclen;
}

char* ltoa(long ival, char* buffer, int radix)
{
	assert(radix > 1 && radix < 37);
	const char* lut = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	char temp[50];
	int i = 0;
	
	unsigned long value = (unsigned long)ival;
	
	if (ival < 0)
	{
		value = -ival;
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

char* itoa(int value, char* buffer, int radix)
{
	return ltoa(value, buffer, radix);
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
	
	if (c == 0)
	{
		return sc;
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

char* strstr(const char *haystack, const char *needle)
{
	const size_t len_nd = strlen(needle);
	const size_t len_hs = strlen(haystack);
	
	if (len_nd > len_hs)
		return NULL; // needle is too big to be in the haystack
	
	if (len_nd == len_hs)
	{
		if (strcmp(haystack, needle) == 0)
			return (char*)haystack;
		return NULL;
	}
	
	const size_t len = len_hs - len_nd;
	
	for (size_t i = 0; i < len; i++)
	{
		if (strncmp(haystack + i, needle, len_nd) == 0)
			return (char*)haystack + i;
	}
	
	return NULL;
}

size_t strspn(const char* s, const char* accept)
{
	const uint8_t* str = (const uint8_t*)s;
	const uint8_t* acc = (const uint8_t*)accept;
	
	bool lut[256]; // could be a bitset, but nah, not right now
	memset (&lut, 0, sizeof lut);
	while (*acc)
	{
		// cast to unsigned because negative indices aren't a thing
		lut[*acc++] = 1;
	}
	
	size_t matching = 0;
	
	while (*str)
	{
		if (!lut[*str])
			return matching;
		matching++;
		str++;
	}
	
	return matching;
}

size_t strcspn(const char* s, const char* reject)
{
	const uint8_t* str = (const uint8_t*)s;
	const uint8_t* rej = (const uint8_t*)reject;
	
	bool lut[256]; // could be a bitset, but nah, not right now
	memset (&lut, 0, sizeof lut);
	while (*rej)
	{
		// cast to unsigned because negative indices aren't a thing
		lut[*rej++] = 1;
	}
	
	size_t matching = 0;
	
	while (*str)
	{
		if (lut[*str])
			return matching;
		matching++;
		str++;
	}
	
	return matching;
}

void* memchr(const void* s, int c, size_t n)
{
	const uint8_t* ptr = s;
	uint8_t match = c;
	
	while (n--)
	{
		if (*ptr == match)
			return (void*)ptr;
		
		ptr++;
	}
	
	return NULL;
}

void* memrchr(const void* s, int c, size_t n)
{
	const uint8_t* ptr = s;
	uint8_t match = c;
	
	ptr += n;
	
	while (n--)
	{
		ptr--;
		
		if (*ptr == match)
			return (void*)ptr;
	}
	
	return NULL;
}

void* rawmemchr(const void* s, int c)
{
	const uint8_t* ptr = s;
	uint8_t match = c;
	
	while (true)
	{
		if (*ptr == match)
			return (void*)ptr;
		
		ptr++;
	}
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

#include <errno.h>
#include <string.h>
#include <ctype.h>

// String to Unsigned X.
unsigned long long strtoux(const char* str, char ** endptr, int base, unsigned long long max)
{
	errno = 0;
	unsigned long long val = 0;
	
	// skip white space
	while (*str && isspace(*str)) str++;
	if (*str == '\0')
	{
		//well, we haven't really found any numbers
		if (endptr)
			*endptr = (char*)str;
		errno = EINVAL;
		return 0;
	}
	
	// *str isn't zero, so surely *(str+1) can be accessed. Check it
	if (*str == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		if (base == 0 || base == 16)
		{
			base = 16;
			str += 2;
		}
		else
		{
			// well, base wasn't either 16 or zero and we've still got the 0x prefix.
			// Set endptr to point to 'x' and return
			if (endptr)
				*endptr = (char*)&str[1];
			return 0;
		}
	}
	// if we're starting with zero, we're handling octal
	else if (*str == '0')
	{
		if (base == 0 || base == 8)
			base = 8;
		str++;
	}
	
	// if base is STILL zero, then we're dealing with decimal.
	if (base == 0) base = 10;
	
	// keep reading digits
	while (*str)
	{
		int digit = 0;
		if (*str >= '0' && *str <= '9')
			digit = *str - '0';
		else if (*str >= 'A' && *str <= 'Z')
			digit = *str - 'A' + 10;
		else if (*str >= 'a' && *str <= 'z')
			digit = *str - 'a' + 10;
		else
			// we've reached the end.
			break;

		// check in what situation we are about to overflow.
		
		// max without the last digit.
		unsigned long long maxwld = max / base;
		// if the value exceeds the max without the last digit already, this means that any digit we add will make it fail.
		if (val > maxwld)
		{
			val = max;
			errno = ERANGE;
			if (endptr) *endptr = (char*)str;
			return val;
		}
		// if it's equal
		if (val == maxwld)
		{
			int digthresh = max % base;
			
			// if we're trying to tack on a digit bigger than the max's last digit, then we're going to overflow, so don't
			if (digit > digthresh)
			{
				val = max;
				errno = ERANGE;
				if (endptr) *endptr = (char*)str;
				return val;
			}
		}
		
		// if it's none of these, whatever digit we add will be valid.
		val = val * base + digit;
		
		str++;
	}
	
	if (endptr)
		*endptr = (char*)str;
	
	return val;
}

// String to Signed X.
long long strtox(const char* str, char ** endptr, int base, long long max)
{
	errno = 0;
	
	long long val = 0; long long sign = 1;
    long long min = -max - 1;
	
	// skip white space
	while (*str && isspace(*str)) str++;
	if (*str == '\0')
	{
		//well, we haven't really found any numbers
		if (endptr)
			*endptr = (char*)str;
		errno = EINVAL;
		return 0;
	}
	
    // if we have a sign, treat it
    if (*str == '+' || *str == '-')
    {
        sign = (*str == '-') ? (-1) : (1);
        str++;
    }

	// *str isn't zero, so surely *(str+1) can be accessed. Check it
	if (*str == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		if (base == 0 || base == 16)
		{
			base = 16;
			str += 2;
		}
		else
		{
			// well, base wasn't either 16 or zero and we've still got the 0x prefix.
			// Set endptr to point to 'x' and return
			if (endptr)
				*endptr = (char*)&str[1];
			return 0;
		}
	}
	// if we're starting with zero, we're handling octal
	else if (*str == '0')
	{
		if (base == 0 || base == 8)
			base = 8;
		str++;
	}
	
	// if base is STILL zero, then we're dealing with decimal.
	if (base == 0) base = 10;
	
	// keep reading digits
	while (*str)
	{
		int digit = 0;
		if (*str >= '0' && *str <= '9')
			digit = *str - '0';
		else if (*str >= 'A' && *str <= 'Z')
			digit = *str - 'A' + 10;
		else if (*str >= 'a' && *str <= 'z')
			digit = *str - 'a' + 10;
		else
			// we've reached the end.
			break;

		// check in what situation we are about to overflow.
		
		// max without the last digit.
		long long maxwld = max / base;
		// if the value exceeds the max without the last digit already, this means that any digit we add will make it fail.
		if (val > maxwld)
		{
			val = sign < 0 ? min : max;
			errno = ERANGE;
			if (endptr) *endptr = (char*)str;
			return val;
		}
		// if it's equal
		if (val == maxwld)
		{
			int digthresh = max % base;
            // if we're in the negatives, bump it up
            if (sign < 0) digthresh++;
			
			// if we're trying to tack on a digit bigger than the max's last digit, then we're going to overflow, so don't
			if (digit > digthresh)
			{
				val = sign < 0 ? min : max;
				errno = ERANGE;
				if (endptr) *endptr = (char*)str;
				return val;
			}
		}
		
		// if it's none of these, whatever digit we add will be valid.
		val = val * base + digit;
		
		str++;
	}
	
	if (endptr)
		*endptr = (char*)str;
	
	return (long long) val * sign;
}

unsigned long long int strtoull(const char* str, char ** endptr, int base)
{
	return strtoux(str, endptr, base, ULLONG_MAX);
}

unsigned long int strtoul(const char* str, char ** endptr, int base)
{
	return (unsigned long int)strtoux(str, endptr, base, ULONG_MAX);
}

long long int strtoll(const char* str, char ** endptr, int base)
{
	return strtox(str, endptr, base, LLONG_MAX);
}

long int strtol(const char* str, char ** endptr, int base)
{
	return (long int)strtox(str, endptr, base, LONG_MAX);
}

double ldexp(UNUSED double val, UNUSED int exp)
{
	// TODO
	ASSERT(!"ldexp used!");
	return 0.0;
}

double strtod(UNUSED const char* str, UNUSED char** endptr)
{
	// TODO
	ASSERT(!"strtod used!");
	return 0.0;
}

long double strtold(UNUSED const char* str, UNUSED char** endptr)
{
	// TODO
	ASSERT(!"strtold used!");
	return 0.0;
}

float strtof(UNUSED const char* str, UNUSED char** endptr)
{
	// TODO
	ASSERT(!"strtof used!");
	return 0.0f;
}
