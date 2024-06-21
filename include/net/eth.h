#ifndef _ETH_
#define _ETH_

#include <main.h>
#include <list.h>
#include "mac.h"

#define USE_RTL8139 // Use an RTL8139 for network packet transmission.

typedef struct
{
	MacAddress m_macDst;
	MacAddress m_macSrc;
	uint16_t m_type;
	uint8_t  m_data[];
}
EthernetFrame;

typedef struct
{
	LIST_ENTRY m_entry;
	size_t  m_size;
	// note: last 4 bytes are the CRC from the rtl8139
	uint8_t m_data[];
}
EthernetPacket;

enum // eEtherType
{
	ETHER_TYPE_IPV4 = 0x0800,
	ETHER_TYPE_ARP  = 0x0806,
};

void EthInit();

bool EthSendPacket(MacAddress* dest, void* data, size_t size, uint16_t protocol);

// For use only by the drivers
void EthReceivedPacket(EthernetPacket* pPacket, bool disableInterrupts);

#endif//_ETH_
