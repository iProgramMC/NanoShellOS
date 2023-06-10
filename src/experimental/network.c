#include <vfs.h>
#include <memory.h>
#include <pci.h>
#define DHCP_MESSAGE 1
#define ICMP_MESSAGE 2
#define DNS_MESSAGE  3
#define ARP_MESSAGE  4

typedef uint8_t MacAddr[6];
typedef uint8_t IPAddr[4];
MacAddr global_mac;

typedef struct {
    uint64_t m_preamble: 56;
    uint8_t  m_start: 8;
    uint64_t m_dest: 48;
    uint64_t m_source: 48;
    uint32_t m_opt: 32;
    uint16_t m_type;
    uint8_t* m_payload;
    uint32_t m_crc: 32; 
} EthPacket;

typedef struct {
    void*  m_device;
    IPAddr m_ip;
    IPAddr m_subnet;
    IPAddr m_router;
} NetDevice;

typedef struct {
  IPAddr   m_ends[2];
  uint16_t m_port;
} TCPSocket;

void NetSendMessage(NetDevice dev, IPAddr addr, int type, void* info /* optional */);
void NetMountDev(NetDevice dev, char* path);

#ifdef _E1000_
typedef struct {
    uint16_t io_base;
    uint32_t mem_base;
    uint8_t is_e;
} E1000Info;

E1000Info* global_e1000;
#define E1000_DEV               0x100E
#define E1000_REG_CTRL 		    0x0000
#define E1000_REG_EEPROM 		0x0014
#define E1000_REG_IMASK 		0x00D0

void E1000WriteL(E1000Info *e, uint16_t addr, uint32_t val)
{
	WritePortL (e->io_base, addr);
	WritePortL (e->io_base + 4, val);
}

void E1000WriteB(E1000Info *e, uint16_t addr, uint32_t val)
{
	WritePortL (e->io_base, addr);
	WritePort (e->io_base + 4, val);
}

uint32_t E1000ReadL(E1000Info *e, uint16_t addr)
{
	WritePortL (e->io_base, addr);
	return ReadPortL (e->io_base + 4);
}

uint32_t E1000ReadProm(E1000Info *e, uint8_t addr)
{
	uint32_t val = 0;
	uint32_t test;
	if(e->is_e == 0)
		test = addr << 8;
	else
		test = addr << 2;

	E1000WriteL(e, E1000_REG_EEPROM, test | 0x1);
	if(e->is_e == 0)
		while(!((val = E1000ReadL(e, E1000_REG_EEPROM)) & (1<<4)))
		;//	printf("is %i val %x\n",e->is_e,val);
	else
		while(!((val = E1000ReadL(e, E1000_REG_EEPROM)) & (1<<1)))
		;//	printf("is %i val %x\n",e->is_e,val);
	val >>= 16;
	return val;
}

void E1000GetMac(E1000Info *e, char *mac)
{
	uint32_t temp;
	temp = E1000ReadProm(e, 0);
	mac[0] = temp &0xff;
	mac[1] = temp >> 8;
	temp = E1000ReadProm(e, 1);
	mac[2] = temp &0xff;
	mac[3] = temp >> 8;
	temp = E1000ReadProm(e, 2);
	mac[4] = temp &0xff;
	mac[5] = temp >> 8;
}

void E1000Setup()
{
    E1000GetMac(global_e1000, global_mac);
}

void E1000EnableInt(E1000Info *e)
{
	E1000Writel(e,REG_IMASK ,0x1F6DC);
	E1000Writel(e,REG_IMASK ,0xff & ~4);
	E1000ReadL(e,0xc0);
}

#endif
