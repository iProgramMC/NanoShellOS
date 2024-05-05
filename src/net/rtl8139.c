#include <main.h>
#include <memory.h>
#include <rtl8139.h>
#include <pci.h>

static PciDevice* g_pRtl8139Device;
static uint32_t   g_Rtl8139BaseIoPort;

#define PORT(x) (g_Rtl8139BaseIoPort + (x))

// The receive buffer as specified by the RTL8139 card is 8K+16 bytes.
// Since we set WRAP=1, we have to allocate RTL_MTU-1 more bytes, since
// the buffer won't be treated as a ring buffer anymore.
static __attribute__((aligned(16))) uint8_t g_Rtl8139ReceiveBuffer[8192 + 16 + RTL_MTU];

void Rtl8139Init()
{
	return; // TODO
	
	g_pRtl8139Device = PciFindDevice(VENDORID_REALTEK, DEVICEID_RTL8139);
	if (!g_pRtl8139Device) {
		LogMsg("No RTL8139 device found.");
	}
	
	// fetch IO port
	g_Rtl8139BaseIoPort = PciGetBarIo(g_pRtl8139Device, 0);
	
	SLogMsg("[RTL8139] Fetched I/O port: %x", g_Rtl8139BaseIoPort);
	
	// enable busmastering DMA on this device
	PciEnableBusMastering(g_pRtl8139Device);
	
	// Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active
	// high. this should essentially *power on* the device.
	WritePort(PORT(RTL_REG_CONFIG1), 0);
	
	// Send reset command
	WritePort(PORT(RTL_REG_CMD), RTL_CMD_RST);
	
	// Wait until reset is complete
	while (ReadPort(PORT(RTL_REG_CMD)) & RTL_CMD_RST)
		asm("pause");
	
	// Send the RTL8139 the address of our receive buffer
	WritePortL(PORT(RTL_REG_RBSTART), (uintptr_t) g_Rtl8139ReceiveBuffer - KERNEL_BASE_ADDRESS);
	
	// Enable the TOK and ROK interrupts
	WritePortW(PORT(RTL_REG_IMR), RTL_IR_TOK | RTL_IR_ROK);
	
	// Configure the receive buffer
	WritePortL(PORT(RTL_REG_RCR), RTL_RCR_WRAP | RTL_RCR_AAP | RTL_RCR_APM | RTL_RCR_AM | RTL_RCR_AB);
	
	// Enable the receive and transmit buffers
	WritePort(PORT(RTL_REG_CMD), RTL_CMD_RE | RTL_CMD_TE);
}
