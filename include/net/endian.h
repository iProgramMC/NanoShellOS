#ifndef _NET_ENDIAN_
#define _NET_ENDIAN_

#include <stdint.h>

uint32_t ByteSwap32(uint32_t swp);
uint16_t ByteSwap16(uint16_t swp);

uint32_t HostToNet32(uint32_t host); // htonl
uint16_t HostToNet16(uint16_t host); // htons

uint32_t NetToHost32(uint32_t net);  // ntohl
uint16_t NetToHost16(uint16_t net);  // ntohs

#endif//_NET_ENDIAN_
