/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

  VirtualBox Guest Additions Driver Code
******************************************/
#include <main.h>
#include <memory.h>
#include <pci.h>
#include <video.h>

#ifdef EXPERIMENTAL

#define VB_VMMDEV_VERSION         (0x00010003)
#define VB_REQUEST_HEADER_VERSION (0x10001)
#define VB_REQUEST_GUEST_INFO     (50)
#define VB_REQUEST_GET_MOUSE      (1)
#define VB_REQUEST_SET_MOUSE      (2)

#define VB_MOUSE_ON               ((1 << 0) | (1 << 4))

#define BIT(n) (1u << n)

#define VMMDEV_EVENT_MOUSE_CAPABILITIES_CHANGED   BIT(0)
#define VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST       BIT(2)
#define VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST BIT(5)
#define VMMDEV_EVENT_MOUSE_POSITION_CHANGED       BIT(9)

typedef struct
{
	uint32_t nSize;
	uint32_t nVersion;
	uint32_t nRequestCode;
	 int32_t nReturnCode;
	uint32_t nReserved1, nReserved2;
}
VBHeader;

typedef struct
{
	VBHeader header;
	uint32_t nVersion;
	uint32_t nOsType;
}
VBGuestInfo;

typedef struct
{
	VBHeader header;
	uint32_t nFeatures;
	uint32_t nX;
	uint32_t nY;
}
VBMousePacket;

typedef struct
{
	VBHeader header;
	uint32_t nEvents;
}
VBAcknowledge;

static uint32_t gVbPort;
static uint32_t gVbVmmDevP, *gVbVmmDev;

static VBAcknowledge* gVbAck;
static uint32_t       gVbAckPhys;

static VBMousePacket* gVbMouse;
static uint32_t       gVbMousePhys;

void IrqVirtualBoxA();

void IrqVirtualBox()
{
	if (!gVbVmmDev[2])
		// Nothing to process, maybe this interrupt was from something sharing the line
		return;
	
	gVbAck->nEvents = gVbVmmDev[2];
	
	WritePortL(gVbPort, gVbAckPhys);//Acknowledge events
	
	if (gVbAck->nEvents & VMMDEV_EVENT_MOUSE_POSITION_CHANGED)
	{
		WritePortL(gVbPort, gVbMousePhys);//Send mouse request packet
		
		// the mouse coordinates are scaled to the range (0x0000, 0xffff) independently
		// in each dimension, so scale them back
		uint32_t
		x = (gVbMouse->nX * GetScreenWidth ()) / 0xFFFF,
		y = (gVbMouse->nY * GetScreenHeight()) / 0xFFFF;
		
		SetMousePos (x, y);
	}
	
	// Flush PIC - TODO
	//WritePort(0x20, 0x20);
	//WritePort(0xA0, 0x20);
}

void VbTellHost()
{
	VBGuestInfo* pPacket;
	// Allocate some space for the guest_info packet
	uint32_t nPacketPhys;
	pPacket = MmAllocateSinglePagePhy (&nPacketPhys);
	
	// Populate The Packet
	pPacket->header.nSize        = sizeof (*pPacket);
	pPacket->header.nVersion     = VB_REQUEST_HEADER_VERSION;
	pPacket->header.nRequestCode = VB_REQUEST_GUEST_INFO;
	pPacket->header.nReturnCode  = 0;
	pPacket->header.nReserved1   = 0;
	pPacket->header.nReserved2   = 0;
	pPacket->nVersion = VB_VMMDEV_VERSION;
	pPacket->nOsType  = 0; // Unknown 32-bit
	
	// Finally, send it to VM
	WritePortL (gVbPort, nPacketPhys);
}

void VbSetupAcknowledge()
{
	VBAcknowledge* pPacket;
	// Allocate some space for the guest_info packet
	uint32_t nPacketPhys;
	pPacket = MmAllocateSinglePagePhy (&nPacketPhys);
	
	// Populate The Packet
	pPacket->header.nSize        = sizeof (*pPacket);
	pPacket->header.nVersion     = VB_REQUEST_HEADER_VERSION;
	pPacket->header.nRequestCode = VB_REQUEST_GUEST_INFO;
	pPacket->header.nReturnCode  = 0;
	pPacket->header.nReserved1   = 0;
	pPacket->header.nReserved2   = 0;
	pPacket->nEvents  = 0; // Unknown 32-bit
	
	gVbAck     = pPacket;
	gVbAckPhys = nPacketPhys;
}

extern bool g_virtualMouseEnabled, g_ps2DisableMovement;
extern bool g_ps2MouseAvail, g_mouseInitted, g_mouseAvailable;
void VbEnableMouseDriver(uint32_t status)
{
	// Send host
	// Allocate some space for the packet
	uint32_t nPacketPhys;
	gVbMouse = MmAllocateSinglePagePhy (&nPacketPhys);
	
	// Populate The Packet
	gVbMouse->header.nSize        = sizeof (*gVbMouse);
	gVbMouse->header.nVersion     = VB_REQUEST_HEADER_VERSION;
	gVbMouse->header.nRequestCode = VB_REQUEST_SET_MOUSE;
	gVbMouse->header.nReturnCode  = 0;
	gVbMouse->header.nReserved1   = 0;
	gVbMouse->header.nReserved2   = 0;
	gVbMouse->nFeatures = status;
	gVbMouse->nX = gVbMouse->nY = 0;
	
	// Finally, send it to VM
	WritePortL (gVbPort, nPacketPhys);
	
	gVbMousePhys = nPacketPhys;
	
	// Once sent, we can change the packet to a get packet right away to use in the interrupt handler
	gVbMouse->header.nRequestCode = VB_REQUEST_GET_MOUSE;
	
	// Once initted, tell the rest of the OS
	g_virtualMouseEnabled = false;
	g_ps2DisableMovement  = false;
	g_ps2MouseAvail       = false;
	g_mouseInitted        = true;
	g_mouseAvailable      = true;
}

int VbGuestGetInterruptNumber()
{
	PciDevice *pDevice = PciFindDevice (VENDORID_VIRTUALBOX, DEVICEID_VBXGUESTDEVICE);
	
	if (!pDevice)
	{
		SLogMsg("No VirtualBox device -- you are NOT running this in VirtualBox.");
		return -1;
	}
	
	uint32_t vboxIrq = PciConfigReadDword(pDevice, 60) & 0xFF;
	LogMsg("Vbox IRQ: %d", vboxIrq);
	return vboxIrq;
}

void VbGuestInit()
{
	// Locate the guest device.  PCI bus probing should have found it- if there is no
	// VirtualBox device, this returns null
	PciDevice *pDevice = PciFindDevice (VENDORID_VIRTUALBOX, DEVICEID_VBXGUESTDEVICE);
	
	if (!pDevice)
	{
		SLogMsg("No VirtualBox device -- you are NOT running this in VirtualBox.");
		return;
	}
	
	// BAR0 is the IO port
	gVbPort    = PciGetBar (pDevice, 0) & 0xFFFFFFFC;
	// BAR1 is the memory-mapped VmmDevMem area
	gVbVmmDevP = PciGetBar (pDevice, 1) & 0xFFFFFFF0;
	
	// Setup acknowledgement packet
	VbSetupAcknowledge();
	
	// Map the vmmdevmem into memory
	
	LogMsg("gVbVmmDevP: %x",gVbVmmDevP);
	
	//!! VmmDevMem is 4 megabytes according to https://www.virtualbox.org/svn/vbox/trunk/include/VBox/VMMDev.h
	gVbVmmDev = (uint32_t*)MmMapPhysicalMemoryRW (MMIO_VBOX_HINT, gVbVmmDevP, gVbVmmDevP + 0x400000, true);
	
	LogMsg("gVbVmmDevE: %x",gVbVmmDev);
	
	// Tell host about us
	VbTellHost();
	
	// Enable mouse
	VbEnableMouseDriver (VB_MOUSE_ON);
	
	// Enable VmmDevMem interrupts - all of them for now
	gVbVmmDev[3] = 0xFFFFFFFF;
}

#endif

