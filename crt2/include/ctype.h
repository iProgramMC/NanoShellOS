// ctype.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _CTYPE__H
#define _CTYPE__H

// Character classification
int isalnum(int c);
int isalpha(int c);
int isascii(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);

// Character space mapping functions
int toupper(int c);
int tolower(int c);

#endif//_CTYPE__H
