/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

         PCI Device Handlermodule
******************************************/
#include <main.h>
#include <string.h>
#include <print.h>
#include <pci.h>
#include <storabs.h>

//NOTE: The PCI driver will only use access mechanism #1 (ports 0xcf8 and 0xcfc)

uint16_t PciUConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address = (uint32_t)((bus << 16u) | (slot << 11u) | (func << 8u) | (offset & 0xFCu) | 0x80000000u);
	
	// Write address
	WritePortL(0xCF8, address);
	
	// Read data
	uint32_t input = ReadPortL(0xCFC);
	return (uint16_t)((input >> ((offset & 2) * 8)) & 0xffff);
}
uint32_t PciUConfigReadDword (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address = (uint32_t)((bus << 16u) | (slot << 11u) | (func << 8u) | (offset & 0xFCu) | 0x80000000u);
	
	// Write address
	WritePortL(0xCF8, address);
	
	// Read data
	return ReadPortL(0xCFC);
}
void PciUConfigWriteDword (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
	uint32_t address = (uint32_t)((bus << 16u) | (slot << 11u) | (func << 8u) | (offset & 0xFCu) | 0x80000000u);
	
	// Write address
	WritePortL(0xCF8, address);
	WritePortL(0xCFC, value);
}

uint16_t PciUGetVendorID(uint8_t bus, uint8_t slot, uint8_t func)
{
	return PciUConfigReadWord (bus, slot, func, 0);
}

uint16_t PciUGetDeviceID(uint8_t bus, uint8_t slot, uint8_t func)
{
	return PciUConfigReadWord (bus, slot, func, 2);
}
uint16_t PciUGetClassID(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint32_t value = PciUConfigReadWord (bus, slot, func, 10);
	return (value & 0xFF00) >> 8;
}
uint16_t PciUGetSubClassID(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint32_t value = PciUConfigReadWord (bus, slot, func, 10);
	return (value & 0x00FF);
}
uint32_t PciUGetBar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_id)
{
	uint32_t bar_address = PciUConfigReadDword (bus, slot, func, 16 + 4 * bar_id);
	uint8_t type = (bar_address >> 1) & 3;
	
	switch (type)
	{
		case 0x0000:
			//32-bit MMIO
			return bar_address & 0xFFFFFFF0u;
		case 0x0002:
			//64-bit MMIO. TODO
			//Also get BAR1
			return bar_address & 0xFFFFFFF0u;//|(BAR1<<32) <-- BAR1 as raw value.
		default:
			//Reserved:
			return 0x00000000;
	}
	
	//return (value & ~0xF);
}
extern bool     g_IsBGADevicePresent;
extern uint32_t g_BGADeviceBAR0;

PciDevice g_RegisteredDevices[256]; int g_nRegisteredDevices = 0;

PciDevice* PciFindDevice (uint16_t vendorid, uint16_t deviceid)
{
	PciDevice placeholder;
	placeholder.vendor = vendorid;
	placeholder.device = deviceid;
	
	uint32_t vendev = placeholder.vendev;//what to look out for
	for (int i = 0; i < g_nRegisteredDevices; i++)
	{
		if (g_RegisteredDevices[i].vendev == vendev)
			return &g_RegisteredDevices[i];
	}
	
	return NULL;
}

void PciProbe ()
{
	for (uint32_t bus = 0; bus < 256; bus++)
		for (uint32_t slot = 0; slot < 32; slot++)
			for (uint32_t func = 0; func < 8; func++)
			{
				uint16_t VendorID = PciUGetVendorID(bus,slot,func);
				if (VendorID == 0xFFFF) continue;//Assume that no PCI device with a vendorID of 0xFFFF exists.
				
				uint16_t DeviceID = PciUGetDeviceID  (bus,slot,func);
				uint16_t ClassID  = PciUGetClassID   (bus,slot,func);
				uint16_t SClassID = PciUGetSubClassID(bus,slot,func);
				ILogMsg("Vendor: %x  Device: %x.  ClassID: %x, SubclassID: %x", VendorID, DeviceID,  ClassID,SClassID);

				if ((VendorID == VENDORID_QEMU       && DeviceID == DEVICEID_BXGFX    ) || 
					(VendorID == VENDORID_VIRTUALBOX && DeviceID == DEVICEID_BXGFXVBOX)
				)
				{
					g_IsBGADevicePresent = true;
					g_BGADeviceBAR0      = PciUGetBar(bus, slot, func, 0);
				}
				
				if (g_nRegisteredDevices >= (int)ARRAY_COUNT(g_RegisteredDevices)) continue;
				
				PciDevice* pDevice = &g_RegisteredDevices[g_nRegisteredDevices++];
				pDevice->bus  = bus;
				pDevice->slot = slot;
				pDevice->func = func;
				pDevice->vendor = VendorID;
				pDevice->device = DeviceID;
			}
}

void PciFindAhciDevices()
{
	KeVerifyInterruptsEnabled;
	for (int i = 0; i < g_nRegisteredDevices; i++)
	{
		PciDevice* pDevice = &g_RegisteredDevices[i];
		uint16_t ClassID  = PciUGetClassID   (pDevice->bus, pDevice->slot, pDevice->func);
		uint16_t SClassID = PciUGetSubClassID(pDevice->bus, pDevice->slot, pDevice->func);
		
		if (ClassID == CLASSID_MASSSTORAGE && SClassID == SCLASSID_SATA)
		{
			AhciOnDeviceFound(pDevice);
		}
	}
}

static char text[100];

uint16_t PciConfigReadWord  (PciDevice *pDevice, uint8_t offset)
{
	return PciUConfigReadWord(pDevice->bus, pDevice->slot, pDevice->func, offset);
}
uint32_t PciConfigReadDword (PciDevice *pDevice, uint8_t offset)
{
	return PciUConfigReadDword(pDevice->bus, pDevice->slot, pDevice->func, offset);
}
void PciConfigWriteDword (PciDevice *pDevice, uint8_t offset, uint32_t value)
{
	PciUConfigWriteDword(pDevice->bus, pDevice->slot, pDevice->func, offset, value);
}

uint16_t PciGetVendorID  (PciDevice *pDevice)
{
	return PciUGetVendorID(pDevice->bus, pDevice->slot, pDevice->func);
}
uint16_t PciGetDeviceID  (PciDevice *pDevice)
{
	return PciUGetDeviceID(pDevice->bus, pDevice->slot, pDevice->func);
}
uint16_t PciGetClassID   (PciDevice *pDevice)
{
	return PciUGetClassID(pDevice->bus, pDevice->slot, pDevice->func);
}
uint16_t PciGetSubClassID(PciDevice *pDevice)
{
	return PciUGetSubClassID(pDevice->bus, pDevice->slot, pDevice->func);
}
uint32_t PciGetBar       (PciDevice *pDevice, uint8_t bar_id)
{
	return PciUGetBar(pDevice->bus, pDevice->slot, pDevice->func, bar_id);
}

const char* PciGetVendorIDText(uint16_t venid)
{
	switch (venid)
	{
		//hardcoded PCI IDs - includes the big boys
		case 0x8086: return "Intel Corporation";
		case 0x1234: return "QEMU internal";
		case 0x80EE: return "VirtualBox internal";
		case 0x1043: return "Asustek Computer, Inc.";
		case 0x1414: return "Microsoft Corporation";
		case 0x2646: return "Kingston Technology Company";
		case 0x10DE: return "NVIDIA Corporation";
		case 0x1022: return "Advanced Micro Devices, Inc.";
		case 0x106B: return "Apple, Inc.";
		//TODO
		
		//default
		default:
			sprintf(text, "VEN%x", venid);
			return text;
	}
}

void PciDump ()
{
	for (int i = 0; i < g_nRegisteredDevices; i++)
	{
		PciDevice* pDevice = &g_RegisteredDevices[i];
		LogMsg("- Ven %x Dev %x Bus %b Slot %b Dev %b (%s)", 
			pDevice->vendor,
			pDevice->device,
			pDevice->bus,
			pDevice->slot,
			pDevice->func,
			PciGetVendorIDText(pDevice->vendor)
		);
	}
}

void PciInit()
{
	PciProbe();
}



