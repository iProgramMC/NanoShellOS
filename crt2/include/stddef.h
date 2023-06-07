#ifndef _STDDEF_H
#define _STDDEF_H 1

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WCHAR_TYPE__ wchar_t;

#ifdef NULL
#undef NULL
#endif

#define NULL ((void *)0)

#define offsetof(s, m) __builtin_offsetof(s, m)

#endif
