#include "queue.h"

static void QueueDestroyContainer(Queue* pQueue);
static void QueueInitWithPO2Size(Queue* pQueue, int po2size);

static int QueueAdvanceIndex(Queue* pQueue, int position)
{
	return (position + 1) & ((1 << pQueue->m_Log2ContainerSize) - 1);
}

Queue* QueueCreate()
{
	Queue* pq = calloc(1, sizeof(Queue));
	
	QueueInitWithPO2Size(pq, DEFAULT_QUEUE_SIZE);
	
	return pq;
}

void QueueFree(Queue* pq)
{
	QueueDestroyContainer(pq);
}

// Internal Operations //
static void QueueDestroyContainer(Queue* pQueue)
{
	if (pQueue->m_Container)
	{
		free(pQueue->m_Container);
		
		pQueue->m_Container = NULL;
		
		// we don't have elements
		pQueue->m_Log2ContainerSize = -1;
	}
}

static void QueueInitWithPO2Size(Queue* pQueue, int po2size)
{
	QueueDestroyContainer(pQueue);
	
	pQueue->m_Container = calloc(1 << po2size, sizeof(void*));
	pQueue->m_Log2ContainerSize = po2size;
	pQueue->m_Head = pQueue->m_Tail = 0;
}

static void QueueExpand(Queue* pQueue)
{
	int elementCount = 1 << pQueue->m_Log2ContainerSize;
	
	// note: There are two situations:
	// 1. The head and tail follow a proper order. So the tail is lower than the head.
	if (pQueue->m_Head >= pQueue->m_Tail)
	{
		// This case is trivial. All we need to do is expand the container.
		
		void** newStuff = calloc(elementCount * 2, sizeof(void*));
		
		for (int i = pQueue->m_Tail; i < pQueue->m_Head; i++)
			newStuff[i] = pQueue->m_Container[i];
		
		// delete the queue container
		free(pQueue->m_Container);
		
		pQueue->m_Container = newStuff;
		pQueue->m_Log2ContainerSize++;
		return;
	}
	
	// 2. The tail is higher than the head. This means that aside from expanding the
	// container, we also need to move everything from the tail up until the end.
	
	void** newStuff = calloc(elementCount * 2, sizeof(void*));
	
	// copy the elements from 0 to the head
	for (int i = 0; i < pQueue->m_Head; i++)
		newStuff[i] = pQueue->m_Container[i];
	
	// copy the elements from the head until the old element count. Shift the index
	// into newStuff by the element count.
	for (int i = pQueue->m_Tail, j = pQueue->m_Tail + elementCount; i < elementCount; i++, j++)
		newStuff[j] = pQueue->m_Container[i];
	
	// delete the queue container
	free(pQueue->m_Container);
	
	// set the new queue container
	pQueue->m_Container = newStuff;
	
	// increase the size of the container
	pQueue->m_Log2ContainerSize++;
	
	// increase the tail by elementCount to reflect the changes
	pQueue->m_Tail += elementCount;
}

static void QueueShrink(UNUSED Queue* pQueue)
{
	// TODO: nop for now.
}

// Exposed Operations //

void QueueClear(Queue* pQueue)
{
	QueueInitWithPO2Size(pQueue, DEFAULT_QUEUE_SIZE);
}

bool QueueEmpty(Queue* pQueue)
{
	return pQueue->m_Head == pQueue->m_Tail;
}

void QueuePush(Queue* pQueue, void* x)
{
	// if the snake would try to bite its own tail.
	// Ideally, this condition is only true once.
	while (QueueAdvanceIndex(pQueue, pQueue->m_Head) == pQueue->m_Tail)
	{
		// needs to be expanded
		QueueExpand(pQueue);
	}
	
	pQueue->m_Container[pQueue->m_Head] = x;
	
	pQueue->m_Head = QueueAdvanceIndex(pQueue, pQueue->m_Head);
}

void* QueueFront(Queue* pQueue)
{
	if (QueueEmpty(pQueue)) return NULL;
	
	return pQueue->m_Container[pQueue->m_Tail];
}

void* QueuePop(Queue* pQueue)
{
	// if we have no elements in the queue, what are you doing?!
	if (QueueEmpty(pQueue)) return NULL;

	void* element = QueueFront(pQueue);
	pQueue->m_Tail = QueueAdvanceIndex(pQueue, pQueue->m_Tail);

	// shrink if possible
	QueueShrink(pQueue);

	return element;
}

