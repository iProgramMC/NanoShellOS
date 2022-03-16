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
	uint32_t bus, slot, func;
	uint16_t vendor, device;
} PciDevice;

void PciInit(void);
void PciDump(void);


uint16_t PciConfigReadWord  (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t PciConfigReadDword (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t PciConfigWriteWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

uint16_t PciGetVendorID  (uint8_t bus, uint8_t slot, uint8_t func);
uint16_t PciGetDeviceID  (uint8_t bus, uint8_t slot, uint8_t func);
uint16_t PciGetClassID   (uint8_t bus, uint8_t slot, uint8_t func);
uint16_t PciGetSubClassID(uint8_t bus, uint8_t slot, uint8_t func);
uint32_t PciGetBar       (uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_id);

#endif//_PCI_H