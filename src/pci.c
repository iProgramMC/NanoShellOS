/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

         PCI Device Handlermodule
******************************************/
#include <main.h>
#include <string.h>
#include <print.h>
#include <pci.h>

//NOTE: The PCI driver will only use access mechanism #1 (ports 0xcf8 and 0xcfc)

uint16_t PciConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address = (uint32_t)((bus << 16u) | (slot << 11u) | (func << 8u) | (offset & 0xFCu) | 0x80000000u);
	
	// Write address
	WritePortL(0xCF8, address);
	
	// Read data
	uint32_t input = ReadPortL(0xCFC);
	return (uint16_t)((input >> ((offset & 2) * 8)) & 0xffff);
}
uint32_t PciConfigReadDword (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address = (uint32_t)((bus << 16u) | (slot << 11u) | (func << 8u) | (offset & 0xFCu) | 0x80000000u);
	
	// Write address
	WritePortL(0xCF8, address);
	
	// Read data
	return ReadPortL(0xCFC);
}
uint16_t PciConfigWriteWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address = (uint32_t)((bus << 16u) | (slot << 11u) | (func << 8u) | (offset & 0xFCu) | 0x80000000u);
	
	// Write address
	WritePortL(0xCF8, address);
	
	// Read data
	uint32_t input = ReadPortL(0xCFC);
	return (uint16_t)((input >> ((offset & 2) * 8)) & 0xffff);
}

uint16_t PciGetVendorID(uint8_t bus, uint8_t slot, uint8_t func)
{
	return PciConfigReadWord (bus, slot, func, 0);
}

uint16_t PciGetDeviceID(uint8_t bus, uint8_t slot, uint8_t func)
{
	return PciConfigReadWord (bus, slot, func, 2);
}
uint16_t PciGetClassID(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint32_t value = PciConfigReadWord (bus, slot, func, 10);
	return (value & 0xFF00) >> 8;
}
uint16_t PciGetSubClassID(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint32_t value = PciConfigReadWord (bus, slot, func, 10);
	return (value & 0x00FF);
}
uint32_t PciGetBar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_id)
{
	uint32_t bar_address = PciConfigReadDword (bus, slot, func, 16 + 4 * bar_id);
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

void PciProbe ()
{
	for (uint32_t bus = 0; bus < 256; bus++)
		for (uint32_t slot = 0; slot < 32; slot++)
			for (uint32_t func = 0; func < 8; func++)
			{
				uint16_t VendorID = PciGetVendorID(bus,slot,func);
				if (VendorID == 0xFFFF) continue;//Assume that no PCI device with a vendorID of 0xFFFF exists.
				
				uint16_t DeviceID = PciGetDeviceID  (bus,slot,func);
				LogMsg("Vendor: %x  Device: %x", VendorID, DeviceID);

				if ((VendorID == VENDORID_QEMU       && DeviceID == DEVICEID_BXGFX    ) || 
					(VendorID == VENDORID_VIRTUALBOX && DeviceID == DEVICEID_BXGFXVBOX)
				)
				{
					g_IsBGADevicePresent = true;
					g_BGADeviceBAR0      = PciGetBar(bus, slot, func, 0);
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

static char text[100];

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



