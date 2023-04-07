#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdlib.h>
#define UNUSED __attribute__((unused))

#define DEFAULT_QUEUE_SIZE (4) // 2^4 = 16 elements.

typedef struct Queue
{
    // by default 4 for 16 entries, but can increase
	void ** m_Container;
	
    // the container that stores all the data within the
    // queue. Expands and shrinks as needed.
	int m_Log2ContainerSize;
	
    // the head and tail of the queue.
    // the head represents the position where a new element would be placed
    // if Push() is called, the tail represents the element at the front of the queue.
	int m_Head, m_Tail;
}
Queue;

// exposed functions
Queue* QueueCreate();
void   QueueFree(Queue* pQueue);

bool  QueueEmpty(Queue* pQueue);
void  QueueClear(Queue* pQueue);
void  QueuePush(Queue* pQueue, void* pPtr);
void* QueueFront(Queue* pQueue);
void* QueuePop(Queue* pQueue);

#endif//_QUEUE_H