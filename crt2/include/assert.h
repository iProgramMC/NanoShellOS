// assert.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _ASSERT__H
#define _ASSERT__H

void OnAssertionFail(const char *cond_msg, const char *file, int line);
#define assert(cond) do { if (!(cond)) OnAssertionFail(#cond, __FILE__, __LINE__); } while (0)
#define ASSERT assert

#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif

#define STATIC_ASSERT _Static_assert

#endif//_ASSERT__H
