// CrappyScript (C) 2023 iProgramInCpp

#include <nanoshell/nanoshell.h>

#define MemReAllocate(ptr,newsize) realloc(ptr,newsize)
#define MemCAllocate(nmemb,sz)     calloc(nmemb,sz)
#define StrDuplicate(psz)          strdup(psz)
#define MemAllocate(sz)            malloc(sz)
#define MemFree(ptr)               free(ptr)
#define MemDebugPrint()
