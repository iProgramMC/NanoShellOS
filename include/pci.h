/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

          pci module header file
******************************************/
#ifndef _PCI_H
#define _PCI_H

#define VENDORID_INTEL          0x8086
#define VENDORID_VIRTUALBOX     0x80EE
#define VENDORID_QEMU           0x1234
#define VENDORID_REALTEK        0x10EC

#define DEVICEID_BXGFX          0x1111
#define DEVICEID_BXGFXVBOX      0xBEEF
#define DEVICEID_VBXGUESTDEVICE 0xCAFE
#define DEVICEID_RTL8139        0x8139

typedef struct PciDevice {
	union {
		struct {
			uint16_t vendor, device;
		};
		uint32_t vendev;
	};
	uint32_t bus, slot, func;
	uint16_t mclass, sclass;
} PciDevice;

void PciInit(void);
void PciDump(void);

// structure:
//  CLASSID_XXXXX      = 0x??,
//      SCLASSID_XXXXX = 0x??,
//      SCLASSID_YYYYY = 0x??,
//      ...,
//  ...,

enum
{
	//https://pci-ids.ucw.cz/read/PD/
	CLASSID_UNCLASSIFIED = 0x00,
	
	CLASSID_MASSSTORAGE  = 0x01,
		SCLASSID_SCSI    = 0x00,
		SCLASSID_IDE     = 0x01,
		SCLASSID_FLP     = 0x02,
		SCLASSID_IPI     = 0x03,
		SCLASSID_RAID    = 0x04,
		SCLASSID_ATA     = 0x05,
		SCLASSID_SATA    = 0x06,
		SCLASSID_SASCSI  = 0x07,
		SCLASSID_NVM     = 0x08,//NVMHCI, NVMe
		
	CLASSID_NETWORKING   = 0x02,
	
	CLASSID_DISPLAY      = 0x03,
		SCLASSID_VGA     = 0x00,
		SCLASSID_XGA     = 0x01,
		SCLASSID_3D      = 0x02,
	
	CLASSID_MULTIMEDIA   = 0x04,
	
	CLASSID_MEMORY       = 0x05,
	
	CLASSID_BRIDGE       = 0x06,
		SCLASSID_HOST    = 0x00,
		SCLASSID_ISA     = 0x01,
		SCLASSID_EISA    = 0x02,
		SCLASSID_MCRCHNL = 0x03,
		SCLASSID_PCI     = 0x04,
		SCLASSID_PCMCIA  = 0x05,
		SCLASSID_NUBUS   = 0x06,
		SCLASSID_CARDBUS = 0x07,
		SCLASSID_RACEWAY = 0x08,
		SCLASSID_STPCIPCI= 0x09,
		SCLASSID_INFBPCI = 0x0A,
	
	CLASSID_COMMS        = 0x07,
	
	CLASSID_GENERIC      = 0x08,
	
	CLASSID_INPUT        = 0x09,
	
	CLASSID_DOCKING      = 0x0A,
	
	CLASSID_PROCESSOR    = 0x0B,
	
	CLASSID_SERIAL_BUS   = 0x0C,
		SCLASSID_FWIRE   = 0x00,
		SCLASSID_ACCESS  = 0x01,
		SCLASSID_SSA     = 0x02,
		SCLASSID_USB     = 0x03, //XHCI, UHCI etc.
		SCLASSID_FIBRE   = 0x04,
		SCLASSID_SMBUS   = 0x05,
		SCLASSID_INFINIB = 0x06,
		SCLASSID_IPMI    = 0x07,
		SCLASSID_SERCOS  = 0x08,
		SCLASSID_CANBUS  = 0x09,
	
	CLASSID_WIRELESS     = 0x0D,
	
	CLASSID_INTELLIGENT  = 0x0E,
	
	CLASSID_SATELLITE    = 0x0F,
	
	CLASSID_ENCRYPTION   = 0x10,
	
	CLASSID_SIGNAL_PROC  = 0x11,
	
	CLASSID_PROC_ACCEL   = 0x12,
	
	CLASSID_NON_ESSENTIAL= 0x13,
	
	CLASSID_COPROCESSOR  = 0x40,
	
	CLASSID_UNASSIGNED   = 0xFF,
	
	
	
	SCLASSID_UNK = 0x80,
};


uint16_t PciUConfigReadWord   (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t PciUConfigReadDword  (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t PciUConfigWriteWord  (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void     PciUConfigWriteDword (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t v);

uint16_t PciUGetVendorID      (uint8_t bus, uint8_t slot, uint8_t func);
uint16_t PciUGetDeviceID      (uint8_t bus, uint8_t slot, uint8_t func);
uint16_t PciUGetClassID       (uint8_t bus, uint8_t slot, uint8_t func);
uint16_t PciUGetSubClassID    (uint8_t bus, uint8_t slot, uint8_t func);
uint32_t PciUGetBar           (uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_id);

uint16_t PciConfigReadWord    (PciDevice *pDevice, uint8_t offset);
uint32_t PciConfigReadDword   (PciDevice *pDevice, uint8_t offset);
void     PciConfigWriteDword  (PciDevice *pDevice, uint8_t offset, uint32_t v);

uint16_t PciGetVendorID  (PciDevice *pDevice);
uint16_t PciGetDeviceID  (PciDevice *pDevice);
uint16_t PciGetClassID   (PciDevice *pDevice);
uint16_t PciGetSubClassID(PciDevice *pDevice);
uint32_t PciGetBarAddress(PciDevice *pDevice, uint8_t bar_id);
uint32_t PciGetBarIo     (PciDevice *pDevice, uint8_t bar_id);
uint32_t PciGetBar       (PciDevice *pDevice, uint8_t bar_id);
uint16_t PciGetIrqData   (PciDevice *pDevice);

PciDevice* PciFindDevice (uint16_t vendorid, uint16_t deviceid);

void PciEnableBusMastering(PciDevice *pDevice);

// Offsets in the PCI device structure
#define PCI_OFF_VENDOR_ID (0)
#define PCI_OFF_DEVICE_ID (2)
#define PCI_OFF_COMMAND   (4)
#define PCI_OFF_STATUS    (6)
#define PCI_OFF_REVPROG   (8)  // Revision Id, Prog If
#define PCI_OFF_CLASSID   (10) // Class Id, Subclass Id

// For general devices (type 0x0):
#define PCI_OFF_BAR_START (16)
#define PCI_OFF_INTDATA   (0x3C)

// Command Word
#define PCI_CMD_IOSPACE    (1 << 0)
#define PCI_CMD_MEMSPACE   (1 << 1)
#define PCI_CMD_BUSMASTER  (1 << 2)
#define PCI_CMD_SPECCYCLE  (1 << 3)
#define PCI_CMD_MEMWRINV   (1 << 4)
#define PCI_CMD_VGAPALSNP  (1 << 5)
#define PCI_CMD_PARERRRES  (1 << 6)
#define PCI_CMD_SERRNENAB  (1 << 8)
#define PCI_CMD_FASTB2BEN  (1 << 9)
#define PCI_CMD_INTDISABLE (1 << 10)

#endif//_PCI_H