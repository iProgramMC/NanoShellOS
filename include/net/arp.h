#ifndef _ARP_
#define _ARP_

// TODO: Not going to worry about ipv6 right now, but boron will have support for it.
// This is the ipv4 version

typedef struct
{
	uint16_t m_htype;   // hardware link type - for ethernet this would be 1
	uint16_t m_ptype;   // protocol address type - for ipv4 that would be 0x0800
	uint8_t  m_hlen;    // hardware address length - 6
	uint8_t  m_plen;    // protocol address length - 4
	uint8_t  m_sha[6];  // sender hardware address
	uint8_t  m_spa[4];  // sender protocol address
	uint8_t  m_tha[6];  // target hardware address
	uint8_t  m_tpa[4];  // target protocol address
}
ArpPacket;

void ArpInit();

#endif//_ARP_
