#include <net/ip.h>

IPAddress g_myIpAddress;

IPAddress IpGetHostAddress()
{
	return g_myIpAddress;
}

void IpSetHostAddress(IPAddress addr)
{
	g_myIpAddress = addr;
}