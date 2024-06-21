#ifndef _IP_
#define _IP_

#include <stdint.h>

typedef uint32_t IPAddress; // IPV4 only for now

#define IP_ADDRESS_UNASSIGNED (0)

#define IP_FROM_OCTETS_N(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))
#define IP_FROM_OCTETS_H(a,b,c,d) (((d)<<24) | ((c)<<16) | ((b)<<8) | (a))

IPAddress IpGetHostAddress(); // note: 0.0.0.0 means no IP address was assigned

void IpSetHostAddress(IPAddress addr); // for use

#endif//_IP_
