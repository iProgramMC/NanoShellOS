#ifndef _HT_H
#define _HT_H

// Note for usage: The 'key' and 'data' pointers are never dereferenced internally by HashTable,
// so invalid pointers are fair game.

// Only time they can be accessed is in the Hash or KeyEquals functions. Even then, you can just
// use them verbatim, and not dereference them.

#include <main.h>

#define C_HT_INITIAL_CAPACITY (16)
#define C_HT_MIN_CAPACITY     (4)
#define C_MAX_BEFORE_RESIZE   (2)

typedef enum
{
	FOR_EACH_NO_OP,
	FOR_EACH_ERASE,
}
eHTForEachOp;

typedef uint32_t     (*HashFunction)           (const void* key);
typedef bool         (*KeyEqualsFunction)      (const void* key1, const void* key2);
typedef eHTForEachOp (*ForEachInternalFunction)(const void* key, void* data, void* ctx);
typedef void         (*OnEraseFunction)        (const void* key, void* data);

typedef struct HashTableBucketItem
{
	struct HashTableBucketItem
		* m_pNext, * m_pPrev;

	const void* m_key;

	void* m_data;
}
HashTableBucketItem;

typedef struct HashTableBucket
{
	HashTableBucketItem* m_pFirst;
	int m_nSize;
}
HashTableBucket;

typedef struct HashTable
{
	int               m_bucketCount;
	HashTableBucket  *m_pBuckets;
	HashFunction      m_Hash;
	KeyEqualsFunction m_Compare;
	OnEraseFunction   m_OnErase;
}
HashTable;

// Create a hash table object with the following items.
HashTable* HtCreate(HashFunction, KeyEqualsFunction, OnEraseFunction);

// Delete a hash table object.
void HtDelete(HashTable*);

// Set an entry within the hash table. The Unchecked version doesn't check
// for duplicates. In the case of a duplicate existing, HashTableLookUp can
// return any of the elements, and HashTableErase will only erase one of them.
bool HtSetUnchecked(HashTable*, const void* key, void* data);
bool HtSet(HashTable*, const void* key, void* data);

// Looks up an item with a certain key. If it wasn't found, this returns NULL
void* HtLookUp(const HashTable*, const void* key);

// Tries to erase an item from a hash table.
// The return value is whether or not we were able to erase the item.
bool HtErase(HashTable*, const void* key);

// Run the 'ForEachInternal' function for each item in the hash table.
// It returns an eForEachOp. If:
// result == FOR_EACH_NO_OP, does not modify the item.
// result == FOR_EACH_ERASE, the item gets erased automatically from the hash table.
// This can be used for an optimized mass purge, for instance.
void HtForEach(HashTable*, ForEachInternalFunction, void* ctx);

// Get the memory usage of a hash table.
size_t HtGetEstimatedMemUsed(HashTable*);

#endif//_HT_H