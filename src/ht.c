#include <ht.h>
#include <memory.h>
#include <string.h>

HashTable* HtCreateInternal(HashFunction hf, KeyEqualsFunction ke, OnEraseFunction oe, int capacity)
{
	if (capacity < C_HT_MIN_CAPACITY)
		capacity = C_HT_MIN_CAPACITY;

	HashTable* pHT = MmAllocate(sizeof(HashTable));
	if (!pHT)
		return pHT;

	memset(pHT, 0, sizeof * pHT);

	pHT->m_Hash    = hf;
	pHT->m_Compare = ke;
	pHT->m_OnErase = oe;
	pHT->m_bucketCount = capacity;

	size_t sz = sizeof(HashTableBucket) * capacity;
	pHT->m_pBuckets = MmAllocate(sz);

	if (!pHT->m_pBuckets)
	{
		MmFree(pHT);
		return NULL;
	}

	memset(pHT->m_pBuckets, 0, sz);

	return pHT;
}

HashTable* HtCreate(HashFunction hf, KeyEqualsFunction ke, OnEraseFunction oe)
{
	return HtCreateInternal(hf, ke, oe, C_HT_INITIAL_CAPACITY);
}

// Byte wise swap.
SAI void HtSwapBytes(void* a1, void* a2, size_t sz)
{
	char* c1 = a1, * c2 = a2;
	while (sz--)
	{
		char tmp = *c1;
		*c1 = *c2;
		*c2 = tmp;
		c1++;
		c2++;
	}
}

static eHTForEachOp HtForEachAdd(const void* key, void* data, void* ctxvoid)
{
	HashTable* ctxHt = ctxvoid;
	
	HtSetUnchecked(ctxHt, key, data);
	
	return FOR_EACH_NO_OP;
}

// Resize if possible.
static bool HtResize(HashTable* pHT, int newCapacity)
{
	// TODO: Improve failure tolerance. Some of these HtSetUnchecked's may fail.
	
	HashTable* pHTNew = HtCreateInternal(pHT->m_Hash, pHT->m_Compare, pHT->m_OnErase, newCapacity);
	if (!pHTNew)
		return false;
	
	HtForEach(pHT, HtForEachAdd, pHTNew);

	// just byte-wise swap them. It's fine
	HtSwapBytes(pHT, pHTNew, sizeof(HashTable));

	// delete the new one, with the old one's contents:
	MmFree(pHTNew->m_pBuckets);
	MmFree(pHTNew);
	
	return true;
}

bool HtSetUnchecked(HashTable* pHT, const void* key, void* data)
{
	uint32_t hash = pHT->m_Hash(key);
	uint32_t hashModulo = hash % pHT->m_bucketCount;

	HashTableBucket* pBucket = &pHT->m_pBuckets[hashModulo];

	if (pBucket->m_nSize >= C_MAX_BEFORE_RESIZE)
	{
		// resize in hopes that it'll spread out
		if (!HtResize(pHT, pHT->m_bucketCount * 2))
			return false;

		// retrack
		hashModulo = hash % pHT->m_bucketCount;
		pBucket = &pHT->m_pBuckets[hashModulo];
	}

	// add to the bucket
	HashTableBucketItem
		* pFirst = pBucket->m_pFirst,
		* pNewItem = MmAllocate(sizeof(HashTableBucketItem));

	if (!pNewItem)
		return false;

	// initialize the new item:
	memset(pNewItem, 0, sizeof * pNewItem);
	pNewItem->m_data = data;
	pNewItem->m_key  = key;

	pBucket->m_pFirst = pNewItem;
	pNewItem->m_pNext = pFirst;

	if (pFirst)
		pFirst->m_pPrev = pNewItem;

	pBucket->m_nSize++;

	return true;
}

bool HtSet(HashTable* pHT, const void* key, void* data)
{
	if (HtLookUp(pHT, key))
		return false;

	return HtSetUnchecked(pHT, key, data);
}

void* HtLookUp(const HashTable* pHT, const void* key)
{
	uint32_t hash = pHT->m_Hash(key);
	uint32_t hashModulo = hash % pHT->m_bucketCount;

	HashTableBucket* pBucket = &pHT->m_pBuckets[hashModulo];

	for (HashTableBucketItem* pBI = pBucket->m_pFirst; pBI; pBI = pBI->m_pNext)
	{
		if (pHT->m_Compare(pBI->m_key, key))
			return pBI->m_data;
	}

	return NULL;
}

static void HtEraseInternal(HashTableBucket* pContainerBucket, HashTableBucketItem* pItem, OnEraseFunction oef)
{
	// connect the previous and next together
	if (pItem->m_pPrev)
		pItem->m_pPrev->m_pNext = pItem->m_pNext;
	if (pItem->m_pNext)
		pItem->m_pNext->m_pPrev = pItem->m_pPrev;

	// call our owner's erase function
	if (oef)
		oef(pItem->m_key, pItem->m_data);

	// delete this:
	MmFree(pItem);

	// let the container bucket know that it's got one less item:
	pContainerBucket->m_nSize--;
}

bool HtErase(HashTable* pHT, const void* key)
{
	uint32_t hash = pHT->m_Hash(key);
	uint32_t hashModulo = hash % pHT->m_bucketCount;

	HashTableBucket* pBucket = &pHT->m_pBuckets[hashModulo];

	for (HashTableBucketItem* pBI = pBucket->m_pFirst; pBI; pBI = pBI->m_pNext)
	{
		if (pHT->m_Compare(pBI->m_key, key))
		{
			// erase it from the linked list now
			HtEraseInternal(pBucket, pBI, pHT->m_OnErase);

			return true;
		}
	}

	return false;
}

void HtForEach(HashTable* pHT, ForEachInternalFunction fief, void* ctx)
{
	// for each bucket:
	for (int i = 0; i < pHT->m_bucketCount; i++)
	{
		// for each item within the bucket:
		HashTableBucket* pBucket = &pHT->m_pBuckets[i];

		for (HashTableBucketItem* pBI = pBucket->m_pFirst; pBI;)
		{
			if (fief(pBI->m_key, pBI->m_data, ctx) == FOR_EACH_ERASE)
			{
				HashTableBucketItem* pBIB = pBI;
				pBI = pBI->m_pNext;

				// erase it from the linked list now
				HtEraseInternal(pBucket, pBIB, pHT->m_OnErase);

				continue;
			}

			pBI = pBI->m_pNext;
		}
	}
}

static eHTForEachOp HtForEachDelete(UNUSED const void* key, UNUSED void* data, UNUSED void* ctx)
{
	return FOR_EACH_ERASE;
}

void HtDelete(HashTable* pHT)
{
	// erase every element:
	HtForEach(pHT, HtForEachDelete, NULL);

	// delete the buckets:
	MmFree(pHT->m_pBuckets);

	// delete the hash table itself:
	MmFree(pHT);
}

eHTForEachOp HtForEachMem(UNUSED const void* key, UNUSED void* data, void* ctxvoid)
{
	size_t* ctx = ctxvoid;
	*ctx += sizeof(HashTableBucketItem);
	
	return FOR_EACH_NO_OP;
}

size_t HtGetEstimatedMemUsed(HashTable* pHT)
{
	// start off with the hash table size
	size_t total = sizeof(HashTable);

	// also add the total space occupied by the buckets:
	total += sizeof(HashTableBucket) * pHT->m_bucketCount;

	// for each item in the hash table, add 1 bucket item
	HtForEach(pHT, HtForEachMem, &total);

	return total;
}
