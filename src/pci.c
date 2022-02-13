/*****************************************
		NanoShell Operating System
	   (C) 2021-2022 iProgramInCpp

            Image loader module
******************************************/
#include <main.h>
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

				if ((VendorID == VENDORID_QEMU && DeviceID == VENDORID_BXGFX) || 
					(VendorID == VENDORID_VIRTUALBOX && DeviceID == VENDORID_BXGFXVBOX))
				{
					g_IsBGADevicePresent = true;
					g_BGADeviceBAR0      = PciGetBar(bus, slot, func, 0);
				}
			}
}


void PciInit()
{
	PciProbe();
}



