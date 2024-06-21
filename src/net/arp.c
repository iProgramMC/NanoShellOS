#include <main.h>
#include <memory.h>
#include <ht.h>
#include <net/arp.h>
#include <net/mac.h>

HashTable* g_arpHashTable;

const MacAddress g_zeroAddress;

static uint32_t ArpHash(const void* key)
{
	// the key is an ipv4 address
	return (uint32_t) key;
}

static bool ArpKeyEquals(const void* key1, const void* key2)
{
	// keys are again, ipv4 addresses
	return key1 == key2;
}

static void ArpOnErase(const void* key, void* data)
{
	// the data is a pointer to a MAC address, so free it
	MmFree(data);
}

void ArpInit()
{
	g_arpHashTable = HtCreate(ArpHash, ArpKeyEquals, ArpOnErase);
}

bool ArpSendPacket(MacAddress* pAddr, uint32_t ipv4)
{
	ArpPacket pck;
	memset(&pck, 0, sizeof pck);
	
}

bool ArpLookup(uint32_t ipv4)
{
	if (!ArpSendPacket(&g_zeroAddress, ipv4))
		return false;
	
	return true;
}
