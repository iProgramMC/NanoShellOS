// setjmp.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _SETJMP___H
#define _SETJMP___H

__attribute__((returns_twice)) int  setjmp (jmp_buf buffer);
__attribute__((noreturn))      void longjmp(jmp_buf buffer, int value);

// The NanoShell names for the functions (they're the same)
__attribute__((returns_twice)) int  SetJump (JumpBuffer env);
__attribute__((noreturn))      void LongJump(JumpBuffer env, int value);

#endif//_SETJMP___H