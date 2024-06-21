#include <stdint.h>

uint32_t ByteSwap32(uint32_t swp)
{
	uint8_t bytes[sizeof(uint32_t)];
	
	bytes[0] = (uint8_t)((swp >> 24) & 0xFF);
	bytes[1] = (uint8_t)((swp >> 16) & 0xFF);
	bytes[2] = (uint8_t)((swp >>  8) & 0xFF);
	bytes[3] = (uint8_t)( swp        & 0xFF);
	
	uint32_t result = 0;
	result |= bytes[0];
	result |= bytes[1] << 8;
	result |= bytes[2] << 16;
	result |= bytes[3] << 24;
	
	return result;
}

uint16_t ByteSwap16(uint16_t swp)
{
	uint8_t bytes[sizeof(uint16_t)];
	
	bytes[0] = (uint8_t)((swp >> 8) & 0xFF);
	bytes[1] = (uint8_t)( swp       & 0xFF);
	
	uint32_t result = 0;
	result |= bytes[0];
	result |= bytes[1] << 8;
	
	return result;
}

uint32_t HostToNet32(uint32_t host)
{
	return ByteSwap32(host);
}

uint32_t NetToHost32(uint32_t net)
{
	return ByteSwap32(net);
}

uint16_t HostToNet16(uint16_t host)
{
	return ByteSwap16(host);
}

uint16_t NetToHost16(uint16_t net)
{
	return ByteSwap16(net);
}
