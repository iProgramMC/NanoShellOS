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

int memcmp(const void* ap, const void* bp, size_t size)
{
	const uint8_t* a = (const uint8_t*) ap;
	const uint8_t* b = (const uint8_t*) bp;
	for (size_t i = 0; i < size; i++)
	{
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
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
	if(al < bl)
	{
		return -1;
	}
	else if(al > bl)
	{
		return 1;
	}
	else if(al == bl)
	{
		return memcmp((void*)as, (void*)bs, al + 1);//Also compare null term
	}
	return 0;
}

void strcat(char* dest, const char* after)
{
	char* end = strlen(dest) + dest;
	memcpy(end, after, strlen(after) + 1);
}

char* strchr (char* stringToSearch, const char characterToSearchFor)
{
	while (*stringToSearch)
	{
		if (*stringToSearch == characterToSearchFor)
		{
			return stringToSearch;
		}
		stringToSearch++;
	}
	return NULL;
}

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
