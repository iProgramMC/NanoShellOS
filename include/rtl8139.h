#ifndef _RTL8139_
#define _RTL8139_

// Maximum transmission unit (packet) size.  The RTL8139 supports a maximum of 1500 bytes.
#define RTL_MTU (1500)

#define RTL_REG_MAC     (0)    // Start of MAC address registers (6)
#define RTL_REG_MAR     (8)    // Start of MAR registers (8)
#define RTL_REG_RBSTART (0x30)
#define RTL_REG_CMD     (0x37)
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

#endif//_RTL8139_
