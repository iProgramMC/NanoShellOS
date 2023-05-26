/*****************************************
		NanoShell Operating System
		  (C) 2023 iProgramInCpp

               Misc module
******************************************/
#include <multiboot.h>
#include <main.h>
#include <misc.h>

SAI void SwapBytes(void* a1, void* a2, size_t sz)
{
	char *s1 = a1, *s2 = a2;
	while (sz)
	{
		char tmp = *s1;
		*s1 = *s2;
		*s2 = tmp;
		sz--;
		s1++, s2++;
	}
}

SAI void* OffsetBy(void* array, size_t elemSize, size_t index)
{
	return (void*)((uintptr_t)array + index * elemSize);
}

// I broke out my textbook for this one. The algorithm in the text book uses 1-indexing, rather than 0 indexing.
void HeapCombine(void* array, size_t elemSize, ComparisonFunc comp, void *ctx, size_t index, size_t elemMax)
{
	size_t par = index, chi = index * 2;
	
	while (chi <= elemMax)
	{
		if (chi < elemMax)
		{
			// compare to see which child gets to swap
			if (comp(ctx, OffsetBy(array, elemSize, chi), OffsetBy(array, elemSize, chi + 1)) < 0)
				chi++;
		}
		
		void *ppar = OffsetBy(array, elemSize, par);
		void *pchi = OffsetBy(array, elemSize, chi);
		
		if (comp(ctx, ppar, pchi) < 0)
		{
			SwapBytes(ppar, pchi, elemSize);
			par = chi;
			chi = chi * 2;
		}
		// immediately stop as there's not much to do
		else break;
	}
}

void HeapSort(void* array, size_t elemSize, size_t elemCount, ComparisonFunc comp, void *ctx)
{
	// Note that the original version uses 1-indexing, and our arrays start from 0,
	// so I'll subtract one elemSize from the array pointer to make it start from 1.
	// This has no effect on the operation of the program since we never access the -1'th element.
	array = (void*)((uintptr_t)array - elemSize);
	
	for (size_t i = elemCount / 2; i > 0; i--)
		HeapCombine(array, elemSize, comp, ctx, i, elemCount);
	
	for (size_t i = elemCount; i > 1; i--)
	{
		SwapBytes(OffsetBy(array, elemSize, i), OffsetBy(array, elemSize, 1), elemSize);
		HeapCombine(array, elemSize, comp, ctx, 1, i - 1);
	}
}
