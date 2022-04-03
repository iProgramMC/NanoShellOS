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

#define DEVICEID_BXGFX          0x1111
#define DEVICEID_BXGFXVBOX      0xBEEF
#define DEVICEID_VBXGUESTDEVICE 0xCAFE

typedef struct PciDevice {
	union {
		struct {
			uint16_t vendor, device;
		};
		uint32_t vendev;
	};
	uint32_t bus, slot, func;
} PciDevice;

void PciInit(void);
void PciDump(void);


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
uint32_t PciGetBar       (PciDevice *pDevice, uint8_t bar_id);

PciDevice* PciFindDevice (uint16_t vendorid, uint16_t deviceid);

#endif//_PCI_H