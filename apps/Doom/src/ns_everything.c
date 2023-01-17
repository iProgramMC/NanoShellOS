/* File I/O (C) 2022 iProgramInCpp*/
#include <stdio.h>
#include <nsstandard.h>

#define TOLOWER(a) tolower(a)//(((a)>='A'&&(a)<='Z')?((a)+0x20):(a))

//taken from https://code.woboq.org/userspace/glibc/string/strcasecmp.c.html

/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less than,
   equal to or greater than S2.  */
int strcasecmp (const char *s1, const char *s2)
{
	const unsigned char *p1 = (const unsigned char *) s1;
	const unsigned char *p2 = (const unsigned char *) s2;
	int result;
	
	if (p1 == p2)
		return 0;
	
	while ((result = tolower (*p1) - tolower (*p2++)) == 0)
		if (*p1++ == '\0')
			break;
	return result;
}
/* Compare no more than N characters of S1 and S2,
   ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less
   than, equal to or greater than S2.  */
int strncasecmp (const char *s1, const char *s2, size_t n)
{
	const unsigned char *p1 = (const unsigned char *) s1;
	const unsigned char *p2 = (const unsigned char *) s2;
	int result;
	if (p1 == p2 || n == 0)
		return 0;
	while ((result = tolower (*p1) - tolower (*p2++)) == 0)
		if (*p1++ == '\0' || --n == 0)
			break;
	return result;
}

int system(const char* cmd)
{
	LogMsg("NS: Shell command attempt ('%s')", cmd);
	return -1;
}
