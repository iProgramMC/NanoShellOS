//  ***************************************************************
//  a_math.c - Creation date: 06/12/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

int abs (int a)
{
	if (a < 0)
		return -a;
	
	return a;
}

double fabs (double x)
{
	if (x < 0)
		return -x;
	
	return x;
}

static unsigned int s_randomSeed;

void _I_RandInit()
{
	s_randomSeed = GetRandom();
}

// TODO: Replace this with a better RNG.
static unsigned int temper(unsigned int x)
{
	x ^= x >> 11;
	x ^= x << 7 & 0x9d2c5680;
	x ^= x << 15 & 0xefc60000;
	x ^= x >> 18;
	return x;
}

int rand_r(unsigned int * seed)
{
	return (temper(*seed = *seed * 1103515245 + 12345) / 2) & 0x7FFFFFFF;
}

int rand()
{
	return rand_r(&s_randomSeed);
}

void srand(unsigned int seed)
{
	s_randomSeed = seed;
}
