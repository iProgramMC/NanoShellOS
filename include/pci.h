/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

          pci module header file
******************************************/
#ifndef _PCI_H
#define _PCI_H

#define VENDORID_INTEL      0x8086
#define VENDORID_VIRTUALBOX 0x80EE
#define VENDORID_QEMU       0x1234

#define VENDORID_BXGFX      0x1111
#define VENDORID_BXGFXVBOX  0xBEEF
#define VENDOR

void PciInit(void);

#endif//_PCI_H