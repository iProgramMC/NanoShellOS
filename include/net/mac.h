#ifndef _MAC_A
#define _MAC_A

#include <stdint.h>

typedef struct
{
	uint8_t o[6];
}
MacAddress;

#define MAC_AS_PARMS(m) (m).o[0], (m).o[1], (m).o[2], (m).o[3], (m).o[4], (m).o[5]
#define MAC_PRINTF_FMT  "%b:%b:%b:%b:%b:%b"

#endif//_MAC_A
