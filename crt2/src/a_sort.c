//  ***************************************************************
//  a_sort.c - Creation date: 29/12/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include "crtlib.h"

// TODO: Implement a more efficient sorting algorithm. We will just use bubble sorting
// for now, since the spec doesn't say what algorithm we should be using.

static __attribute__((always_inline))
void swap_char(char * c, char * d)
{
	char tmp = *c;
	*c = *d;
	*d = tmp;
}

void qsort(void *pBase, size_t nCount, size_t nElementSize, ComparisonFunc pCompare)
{
	for (size_t i = 0; i < nCount; i++)
	{
		void *p1 = (void *)((uintptr_t)pBase + i * nElementSize);
		
		for (size_t j = i + 1; j < nCount; j++)
		{
			void *p2 = (void *)((uintptr_t)pBase + j * nElementSize);
			if (pCompare(p1, p2) <= 0)
				continue;
			
			// swap p1 and p2
			char *p1_bytes = (char *)p1;
			char *p2_bytes = (char *)p2;
			
			for (size_t k = 0; k < nElementSize; k++)
				swap_char(&p1_bytes[k], &p2_bytes[k]);
		}
	}
}

void qsort_r(void *pBase, size_t nCount, size_t nElementSize, ComparisonReentrantFunc pCompareReentrant, void* pArgument)
{
	for (size_t i = 0; i < nCount; i++)
	{
		void *p1 = (void *)((uintptr_t)pBase + i * nElementSize);
		
		for (size_t j = i + 1; j < nCount; j++)
		{
			void *p2 = (void *)((uintptr_t)pBase + j * nElementSize);
			if (pCompareReentrant(p1, p2, pArgument) <= 0)
				continue;
			
			// swap p1 and p2
			char *p1_bytes = (char *)p1;
			char *p2_bytes = (char *)p2;
			
			for (size_t k = 0; k < nElementSize; k++)
				swap_char(&p1_bytes[k], &p2_bytes[k]);
		}
	}
}
