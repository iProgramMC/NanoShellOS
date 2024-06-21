#include <main.h>
#include <memory.h>
#include <rtl8139.h>
#include <pci.h>
#include <idt.h>
#include <string.h>

#define RTL_PADDING     (16)   // padding for the rx buffer, the documentation says this is needed
#define RX_BUFFER_SIZE  (8192)
#define TX_BUFFER_SIZE  (8192)
#define TX_BUFFER_COUNT (4)    // RTL8139 has slots for four transmit buffers
#define RX_READ_POINTER_MASK (~3)

static MacAddress r_macAddr;
static PciDevice* r_pDevice;
static uint32_t   r_baseIoPort;
static int        r_irqNumber;
static int        r_packetNum = 0;

#define PORT(x) (r_baseIoPort + (x))

// The receive buffer as specified by the RTL8139 card is 8K+16 bytes.
// Since we set WRAP=1, we have to allocate RTL_MTU-1 more bytes, since
// the buffer won't be treated as a ring buffer anymore.
static __attribute__((aligned(16))) uint8_t r_receiveBuffer[RX_BUFFER_SIZE + RTL_PADDING + RTL_MTU];
static __attribute__((aligned(16))) uint8_t r_transmitBuffer[TX_BUFFER_SIZE * TX_BUFFER_COUNT];
static bool r_txBufferUsed[TX_BUFFER_COUNT];
static int  r_txBufferNextIndex;
static int  r_rxBufferPtr;

static bool Rtl8139CheckPacket(Rtl8139PacketHeader* pHeader)
{
	if (pHeader->m_flags & (RTL_PKT_FAE | RTL_PKT_CRC | RTL_PKT_LONG | RTL_PKT_RUNT))
		return false;
	
	if (~pHeader->m_flags & RTL_PKT_ROK)
		return false;
	
	if (pHeader->m_length < 20 || pHeader->m_length > RX_BUFFER_SIZE)
		return false;
	
	return true;
}

static void Rtl8139ReceivePackets()
{
	while (true)
	{
		uint8_t stat = ReadPort(PORT(RTL_REG_CMD));
		// means buffer is empty
		if (stat & RTL_CMD_BUFE)
			break;
		
		uint8_t* pBuffer = &r_receiveBuffer[r_rxBufferPtr];
		
		Rtl8139PacketHeader* pHeader = (Rtl8139PacketHeader*) pBuffer;
		
		if (!Rtl8139CheckPacket(pHeader)) {
			SLogMsg("ERROR: Received bad packet. Flags: %x, Size: %x", pHeader->m_flags, pHeader->m_length);
			// TODO: Properly reset
			continue;
		}
		
		// Now copy the data and send it over to the ethernet task
		EthernetPacket* pPacket = MmAllocateID(sizeof(EthernetPacket) + pHeader->m_length);
		
		pPacket->m_size = pHeader->m_length;
		memcpy(pPacket->m_data, pHeader->m_data, pHeader->m_length);
		
		// +4 for the CRC, +3 for dword alignment.
		r_rxBufferPtr = ((r_rxBufferPtr + pHeader->m_length + 4 + 3) & RX_READ_POINTER_MASK) % RX_BUFFER_SIZE;
		
		// subtract 16 to avoid overflow
		WritePortL(PORT(RTL_REG_CAPR), r_rxBufferPtr - 0x10);
		WritePort(PORT(RTL_REG_ISR), RTL_IR_ROK);
		
		EthReceivedPacket(pPacket, false);
	}
}

static void Rtl8139InterruptHandler()
{
	uint16_t Status = ReadPort(PORT(RTL_REG_ISR));
	SLogMsg(">> [RTL8139] Status: %x", Status);
	
	if (Status & RTL_IR_TOK)
	{
		SLogMsg("[RTL8139] TOK interrupt");
		WritePort(PORT(RTL_REG_ISR), RTL_IR_TOK);
	}
	
	if (Status & RTL_IR_ROK)
	{
		SLogMsg("[RTL8139] ROK interrupt");
		Rtl8139ReceivePackets();
	}
}

static void Rtl8139FetchMacAddress()
{
	uint32_t part1 = ReadPortL(PORT(RTL_REG_MAC));
	uint32_t part2 = ReadPortL(PORT(RTL_REG_MAC + 4));
	
	memcpy(r_macAddr.o + 0, &part1, sizeof(uint32_t));
	memcpy(r_macAddr.o + 4, &part2, sizeof(uint16_t));
}

// Programming interface

void Rtl8139GetMacAddress(MacAddress* pAddr)
{
	*pAddr = r_macAddr;
}

bool Rtl8139SendPacket(void* pData, size_t size)
{
	if (size > TX_BUFFER_SIZE) {
		SLogMsg("Error man, you can't send data this big in one packet");
		return false;
	}
	
	cli;
	int pnum = ++r_packetNum;
	sti;
	
	SLogMsg("[RTL8139] Sending packet with id %d, size %d", pnum, size);
	
	cli;
	
	int idx = r_txBufferNextIndex;
	// wait until that slot is free
	while (r_txBufferUsed[idx])
	{
		sti;
		
		KeTaskDone();
		asm("hlt"); // wait for it...
		
		cli;
	}
	
	r_txBufferNextIndex = (r_txBufferNextIndex + 1) % 4;
	
	uint8_t* bufData = &r_transmitBuffer[TX_BUFFER_SIZE * idx];
	uint32_t dataPhy = (uint32_t) bufData - KERNEL_BASE_ADDRESS;
	
	memcpy(bufData, pData, size);
	
	// send start address
	WritePortL(PORT(RTL_REG_TSAD0 + 4 * idx), dataPhy);
	
	// send configuration.  Just the size for now.
	WritePortL(PORT(RTL_REG_TSD0 + 4 * idx), size);
	
	sti;
	
	// Wait for the packet to be processed by the card. Boron would do this asynchronously
	
	while (true)
	{
		uint32_t status = ReadPortL(PORT(RTL_REG_TSD0 + 4 * idx));
		
		//SLogMsg("Status = %x", status);
		if (status & RTL_TSD_TOK) {
			SLogMsg("[RTL8139] Packet %d: %d bytes sent successfully", pnum, size);
			cli;
			r_txBufferUsed[idx] = false;
			sti;
			return true;
		}
		if (status & RTL_TSD_OWN) {
			SLogMsg("[RTL8139] Packet %d: DMA access was successful", pnum);
		}
		if (status & RTL_TSD_TUN) {
			SLogMsg("[RTL8139] Packet %d: FIFO exhausted during packet transmission", pnum);
			break;
		}
		if (status & RTL_TSD_TABT) {
			SLogMsg("[RTL8139] Packet %d: Transmit aborted", pnum);
			break;
		}
		if (status & RTL_TSD_OWC) {
			SLogMsg("[RTL8139] Packet %d: Out of window collision", pnum);
			break;
		}
		if (status & RTL_TSD_CRS) {
			SLogMsg("[RTL8139] Packet %d: Carrier sense lost", pnum);
			break;
		}
		
		KeTaskDone();
	}
	
	return false;
}

bool Rtl8139Init()
{
	r_pDevice = PciFindDevice(VENDORID_REALTEK, DEVICEID_RTL8139);
	if (!r_pDevice) {
		LogMsg("No RTL8139 device found.");
		return false;
	}
	
	// Fetch IO port
	r_baseIoPort = PciGetBarIo(r_pDevice, 0);
	
	// Fetch IRQ number
	r_irqNumber = PciGetIrqData(r_pDevice) & 0xFF;
	
	SLogMsg("[RTL8139] Fetched I/O port: %x, IRQ# %d", r_baseIoPort, r_irqNumber);
	
	// Connect IRQ handler
	KeRegisterIrqHandler(r_irqNumber, Rtl8139InterruptHandler, false);
	
	// enable busmastering DMA on this device
	PciEnableBusMastering(r_pDevice);
	
	// Send 0x00 to the CONFIr_1 register (0x52) to set the LWAKE + LWPTN to active
	// high. this should essentially *power on* the device.
	WritePort(PORT(RTL_REG_CONFIG1), 0);
	
	// Send reset command
	WritePort(PORT(RTL_REG_CMD), RTL_CMD_RST);
	
	// Wait until reset is complete
	while (ReadPort(PORT(RTL_REG_CMD)) & RTL_CMD_RST)
		asm("pause");
	
	// Send the RTL8139 the address of our receive buffer
	WritePortL(PORT(RTL_REG_RBSTART), (uintptr_t) r_receiveBuffer - KERNEL_BASE_ADDRESS);
	
	// Enable the TOK and ROK interrupts
	WritePortW(PORT(RTL_REG_IMR), RTL_IR_TOK | RTL_IR_ROK);
	
	// Configure the receive buffer
	WritePortL(PORT(RTL_REG_RCR), RTL_RCR_WRAP | RTL_RCR_AAP | RTL_RCR_APM | RTL_RCR_AM | RTL_RCR_AB);
	
	// Enable the receive and transmit buffers
	WritePort(PORT(RTL_REG_CMD), RTL_CMD_RE | RTL_CMD_TE);
	
	r_rxBufferPtr = 0;
	
	// Fetch MAC address
	Rtl8139FetchMacAddress();
	
	uint8_t* p = r_macAddr.o;
	LogMsg("RTL8139 initialized with MAC address %b:%b:%b:%b:%b:%b", p[0], p[1], p[2], p[3], p[4], p[5]);
	
	// Send a test packet
	
	
	
	
	return true;
}
