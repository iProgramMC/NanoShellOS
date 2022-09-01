//  ***************************************************************
//  a_mem.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

// Max memory allocations at once -- increase if necessary
#define MMMAX 1024

// Memory management
// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
// Replace This with a mmap-based implementation!!!!
// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO

static void*  g_AllocatedMemoryList[MMMAX];
static size_t g_AllocatedMemorySize[MMMAX];

void *malloc (size_t size)
{
	int fs = -1;
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i] == NULL)
		{
			fs = i;
			break;
		}
	}
	if (fs == -1) return NULL; //Too many!
	
	g_AllocatedMemoryList[fs] = _I_AllocateDebug (size, "usertask", 1);
	
	if (!g_AllocatedMemoryList[fs]) return NULL;//Can't allocate?
	
	g_AllocatedMemorySize[fs] = size;
	
	return g_AllocatedMemoryList[fs];
}

void free (void* ptr)
{
	_I_Free(ptr);
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i] == ptr)
		{
			g_AllocatedMemoryList[i]  = NULL;
			g_AllocatedMemorySize[i]  = 0;
		}
	}
}

void* realloc (void* ptr, size_t sz)
{
	if (!ptr) // why would you?!
		return malloc(sz);
	
	int found = -1;
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i] == ptr)
		{
			found = i;
			break;
		}
	}
	void* new = _I_ReAllocateDebug(ptr, sz, "usertask", 1);
	if (new)
	{
		if (found >= 0)
		{
			g_AllocatedMemoryList[found] = new;
			g_AllocatedMemorySize[found] = sz;
		}
	}
	
	return new;
}

//well, do you need this after you use mmap? I mean, you'll get rid of everything when quitting anyway :)
void _I_FreeEverything()
{
	
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i])
		{
			_I_Free(g_AllocatedMemoryList[i]);
			g_AllocatedMemoryList[i] = NULL;
		}
	}
}
