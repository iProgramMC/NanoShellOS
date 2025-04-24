/*****************************************
		NanoShell Operating System
	   (C) 2020-2021 iProgramInCpp

        String manipulation module
******************************************/
#include <string.h>
#include <memory.h>

bool EndsWith(const char* pText, const char* pCheck)
{
	int slt = strlen (pText), slc = strlen (pCheck);
	if (slt < slc) return false; //obviously, it can't.
	
	const char* pTextEnd = pText + slt;
	pTextEnd -= slc;
	return (strcmp (pTextEnd, pCheck) == 0);
}

bool StartsWith(const char* pText, const char* pCheck)
{
	int slt = strlen (pText), slc = strlen (pCheck);
	if (slt < slc) return false; //obviously, it can't.
	
	char text[slc+1];
	memcpy (text, pText, slc);
	text[slc] = 0;
	
	return (strcmp (text, pCheck) == 0);
}

int memcmp(const void* ap, const void* bp, size_t size)
{
	const uint8_t* a = (const uint8_t*) ap;
	const uint8_t* b = (const uint8_t*) bp;
	for(size_t i = 0; i < size; i++)
	{
		if(a[i] < b[i])
			return -1;
		else if(b[i] < a[i])
			return 1;
	}
	return 0;
}
void* memcpy(void* dstptr, const void* srcptr, size_t size)
{
	uint8_t* dst = (uint8_t*) dstptr;
	const uint8_t* src = (const uint8_t*) srcptr;
	for(size_t i = 0; i < size; i++)
	{
		dst[i] = src[i];
	}
	return dstptr;
}
// Moves in increments of four
void fmemcpy32 (void* dest, const void* src, int size) {
	uint32_t* dst = (uint32_t*)dest;
	uint32_t* ser = (uint32_t*)src;
	
	while (size > 0) {
		//! Should be somewhat fast. Needs to copy 3 145 728 bytes for a 1024x768x32 screen
		*(dst++) = *(ser++); 
		size -= 4;
	}
}
void fmemcpy128 (void* dest, const void* src, int size) {
	uint32_t* dst = (uint32_t*)dest;
	uint32_t* ser = (uint32_t*)src;
	
	while (size > 0) {
		//! Should be somewhat fast. Needs to copy 3 145 728 bytes for a 1024x768x32 screen
		*(dst++) = *(ser++); 
		*(dst++) = *(ser++); 
		*(dst++) = *(ser++); 
		*(dst++) = *(ser++); 
		size -= 16;
	}
}

void memmove_ints(void* pMemOut, const void* pMemIn, int nSize)
{
	uint32_t* pMemOut32 = (uint32_t*)pMemOut;
	const uint32_t* pMemIn32 = (const uint32_t*)pMemIn;
	
	if (pMemOut == pMemIn) return;
	
	if (pMemOut <= pMemIn)
	{
		while (nSize--)
		{
			*pMemOut32++ = *pMemIn32++;
		}
	}
	else
	{
		pMemOut32 += nSize;
		pMemIn32  += nSize;
		while (nSize--)
		{
			*(--pMemOut32) = *(--pMemIn32);
		}
	}
}

void* memmove(void* dstptr, const void* srcptr, size_t size)
{
	uint8_t* dst = (uint8_t*) dstptr;
	const uint8_t* src = (const uint8_t*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i > 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}
void* memset(void* bufptr, int val, size_t size)
{
	uint8_t* buf = (uint8_t*) bufptr;
	for(size_t i = 0; i < size; i++)
	{
		buf[i] = val;
	}
	return bufptr;
}
void* fast_memset(void* bufptr, int val, size_t size);
//NOTE: size must be 4 byte aligned!!
void ZeroMemory (void* bufptr1, size_t size)
{
	size &= ~3;
	uint32_t* bufptr = (uint32_t*)bufptr1;
	
	while (size) {
		*bufptr = 0;
		size -= 4;
		bufptr++;
	}
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
int atoihex(const char* str) 
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
		char c = str[i];
		
		if (c >= '0' && c <= '9')
			f += c - '0';
		else if (c >= 'A' && c <= 'F')
			f += c - 'A' + 0xA;
		else if (c >= 'a' && c <= 'f')
			f += c - 'a' + 0xA;
	}
	
	return s * f;
}
size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}
size_t strnlen(const char* str, size_t nchars) 
{
	size_t len = 0;
	while (str[len])
		len++, nchars--;
	return len;
}
char* strcpy(char* ds, const char* ss)
{
	return memcpy(ds, ss, strlen(ss) + 1);
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

char* strcat(char* dest, const char* after)
{
	char* end = strlen(dest) + dest;
	memcpy(end, after, strlen(after) + 1);
	return dest;
}

char* strpcat(char* dest, const char* after)
{
	char* end = strlen(dest) + dest;
	size_t sz = strlen(after) + 1;
	memcpy(end, after, sz);
	return end + sz - 1;
}

char* strchr (const char* stringToSearch1, const char characterToSearchFor)
{
	// discard the 'const' qualifier, because the function was defined this way
	char* stringToSearch = (char*)stringToSearch1;
	
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

char* strrchr (const char* stringToSearch1, const char characterToSearchFor)
{
	// discard the 'const' qualifier, because the function was defined this way
	char* stringToSearch = (char*)stringToSearch1;
	
	int sl = strlen (stringToSearch);
	
	char*  p1 = stringToSearch + sl - 1;
	while (p1 >= stringToSearch)
	{
		if (*p1 == characterToSearchFor)
			return p1;
		p1--;
	}
	return NULL;
}

char* MmStringDuplicate (const char* pText)
{
	int len1 = strlen(pText);
	char* ret = MmAllocate(len1 + 1);
	strcpy (ret, pText);
	return ret;
}

char* Tokenize (TokenState* pState, char* pString, char* separator) {
    int len, i;
    if(!pState->m_bInitted) {
        pState->m_bInitted = 1;
        pState->m_pContinuation = NULL;
        pState->m_pReturnValue = NULL;
    }
    else if (!pState->m_pContinuation) return NULL;
    if(!pString) {
        pString = pState->m_pContinuation;
    }
    if (!pString) return NULL;
    len = strlen (pString);

    for (i = 0; i < len; i++) {
        if (strchr(separator, pString[i])) {
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

// Works like strncpy, except that the destination buffer will always contain a NULL terminator afterwards.
char* SafeStringCopy(char *DestinationBuffer, size_t szDestinationBuffer, const char* Source)
{
	size_t len = strnlen(Source, szDestinationBuffer - 1);
	memcpy(DestinationBuffer, Source, len);
	DestinationBuffer[len] = 0;
	return DestinationBuffer;
}

// note: This is an inefficient algorithm, and will lead to a stack overflow if the strings are too long.
// Make sure to cap their lengths. However, for now, this will do.
bool WildcardMatches(const char* pattern, const char* candidate)
{
	// if we have gone through all the characters in the pattern...
	if (*pattern == '\0')
	{
		// check if we have also matched all of the candidate's characters.
		return (*candidate == 0);
	}
	
	// if we have a wildcard...
	if (*pattern == '*')
	{
		pattern++;
		
		for (; *candidate; candidate++)
		{
			if (WildcardMatches(pattern, candidate))
				return true;
		}
		
		// also check the case where there's no string to match.
		return WildcardMatches(pattern, candidate);
	}
	
	// If this isn't a single character wildcard or a regular wildcard, this means that we should
	// check if the current character in the pattern matches the current character in the candidate.
	// If they don't match, we can easily discard the entire match as false.
	if (*pattern != '?')
	{
		if (*pattern != *candidate) return false;
	}
	// If it is, we shouldn't have a null character.
	else
	{
		if (*candidate == '\0')
			return false;
	}
	
	return WildcardMatches(pattern + 1, candidate + 1);
}

size_t strlcat(char* dst, const char* src, size_t sz)
{
	size_t npi = 0;
	size_t slen_src = strlen(src);
	size_t slen_dst = sz;
	
	// find nullptr
	for (; npi < sz; npi++)
	{
		if (dst[npi] == 0)
			break;
	}
	
	// no nullptr in the first sz bytes. Error!
	if (npi == sz)
		return slen_src + slen_dst;
	
	slen_dst = npi;

	size_t return_value = slen_src + slen_dst;
	
	// the string is already full:
	if (npi == sz - 1)
		return 0;
	
	for (size_t idx = 0; npi < sz - 1; npi++, idx++)
	{
		if (src[idx] == 0)
			break;
		
		dst[npi] = src[idx];
	}
	
	for (; npi < sz; npi++)
		dst[npi] = 0;
	
	return return_value;
}
