// limits.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __LIMITS__H
#define __LIMITS__H

#define CHAR_BIT (8)
#define SCHAR_MIN (-128)
#define SCHAR_MAX (127)
#define UCHAR_MAX (255)
#define CHAR_MIN (-128)
#define CHAR_MAX (127)
#define SHRT_MIN (-32768)
#define SHRT_MAX (32767)
#define USHRT_MAX (65535)
#define INT_MIN (-2147483648)
#define INT_MAX (2147483647)
#define UINT_MAX (4294967295U)
#define LONG_MIN INT_MIN
#define LONG_MAX INT_MAX
#define ULONG_MAX UINT_MAX
#define LLONG_MIN (-9223372036854775808LL)
#define LLONG_MAX (9223372036854775807LL)
#define ULLONG_MAX (18446744073709551615ULL)

#endif//__LIMITS__H