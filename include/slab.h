#ifndef _SLAB_H
#define _SLAB_H

void * SlabAllocate(int size);
void   SlabFree(void* ptr);

#endif//_SLAB_H
