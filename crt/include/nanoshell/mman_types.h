// nanoshell/mman_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __MMAN_TYPES_H
#define __MMAN_TYPES_H

#include <sys/types.h>

// mmap() flags
#define PROT_NONE  (0 << 0)
#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2) //not applicable here

#define MAP_FAILED ((void*) -1) //not NULL

#define MAP_FILE      (0 << 0) //retroactive, TODO
#define MAP_SHARED    (1 << 0) //means changes in the mmapped region will be written back to the file on unmap/close
#define MAP_PRIVATE   (1 << 1) //means changes won't be committed back to the source file
#define MAP_FIXED     (1 << 4) //fixed memory mapping means that we really want it at 'addr'.
#define MAP_ANONYMOUS (1 << 5) //anonymous mapping, means that there's no file backing this mapping :)
#define MAP_ANON      (1 << 5) //synonymous with "MAP_ANONYMOUS"
#define MAP_NORESERVE (0 << 0) //don't reserve swap space, irrelevent here

#define MAP_DONTREPLACE (1 << 30) //don't clobber preexisting fixed mappings there. Used with MAP_FIXED to create...
#define MAP_FIXED_NOREPLACE (MAP_DONTREPLACE | MAP_FIXED)


#endif//__MMAN_TYPES_H