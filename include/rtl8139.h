#ifndef _RTL8139_
#define _RTL8139_

#include <main.h>
#include <net/eth.h>

// Maximum transmission unit (packet) size.  The RTL8139 supports a maximum of 1500 bytes.
#define RTL_MTU (1500)

#define RTL_REG_MAC     (0)    // Start of MAC address registers (2x4)
#define RTL_REG_MAR     (8)    // Start of MAR registers (2x4)
#define RTL_REG_TSD0    (0x10) // Start of TSD registers (4x4)
#define RTL_REG_TSAD0   (0x20) // Start of TSAD registers (4x4)
#define RTL_REG_RBSTART (0x30)
#define RTL_REG_CMD     (0x37)
#define RTL_REG_CAPR    (0x38) // Current address of packet read
#define RTL_REG_CBR     (0x3A) // Current rx buffer address
#define RTL_REG_IMR     (0x3C) // Interrupt mask register
#define RTL_REG_ISR     (0x3E) // Interrupt status register
#define RTL_REG_RCR     (0x44) // Receive config register
#define RTL_REG_CONFIG0 (0x51)
#define RTL_REG_CONFIG1 (0x52)

// RTL_REG_CMD:
#define RTL_CMD_RST  (1 << 4) // reset
#define RTL_CMD_RE   (1 << 3) // receiver enable
#define RTL_CMD_TE   (1 << 2) // transmit enable
#define RTL_CMD_BUFE (1 << 0) // rx buffer empty

// RTL_REG_CONFIG1:
#define RTL_CONFIG1_PMEN  (1 << 0) // Power management enable
#define RTL_CONFIG1_LWACT (1 << 4) // LWAKE active mode

// RTL_REG_IMR and RTL_REG_ISR:
#define RTL_IR_ROK   (1 << 0)
#define RTL_IR_RER   (1 << 1)
#define RTL_IR_TOK   (1 << 2)
#define RTL_IR_TER   (1 << 3)
#define RTL_IR_RXOVW (1 << 4)

// RTL_REG_RCR
#define RTL_RCR_AAP  (1 << 0) // Accept all packets with a physical destination address
#define RTL_RCR_APM  (1 << 1) // Accept physical match
#define RTL_RCR_AM   (1 << 2) // Accept multicast packets
#define RTL_RCR_AB   (1 << 3) // Accept broadcast packets
#define RTL_RCR_WRAP (1 << 7)

// RTL_REG_TSD#
#define RTL_TSD_OWN  (1 << 12) // The DMA operation was successful
#define RTL_TSD_TUN  (1 << 13) // The FIFO was exhausted during packet transfer
#define RTL_TSD_TOK  (1 << 14) // Transmission of a packet was successful
#define RTL_TSD_OWC  (1 << 29) // Out of window collision
#define RTL_TSD_TABT (1 << 30) // Transmission of a packet was aborted
#define RTL_TSD_CRS  (1 << 31) // Carrier sense lost

// Received Packet Header
typedef struct
{
	uint16_t m_flags;
	uint16_t m_length;
	uint8_t  m_data[];
}
Rtl8139PacketHeader;

#define RTL_PKT_ROK  (1 << 0) // receive OK
#define RTL_PKT_FAE  (1 << 1) // frame alignment error
#define RTL_PKT_CRC  (1 << 2) // CRC error
#define RTL_PKT_LONG (1 << 3) // packet size exceeds 4096 bytes
#define RTL_PKT_RUNT (1 << 4) // packet size is less than 64 bytes
#define RTL_PKT_ISE  (1 << 5) // invalid symbol error
#define RTL_PKT_BAR  (1 << 13) // broadcast address received
#define RTL_PKT_PAM  (1 << 14) // physical address matched
#define RTL_PKT_MAR  (1 << 15) // multicast address received

void Rtl8139GetMacAddress(MacAddress* pAddr);
bool Rtl8139SendPacket(void* pData, size_t size);
bool Rtl8139Init();

#endif//_RTL8139_
