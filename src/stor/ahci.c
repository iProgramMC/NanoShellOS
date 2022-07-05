/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

     Storage Abstraction: AHCI Driver
******************************************/

#include <memory.h>
#include <storabs.h>

//! TODO: Use interrupts instead of polling for this driver.

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define PXCMD_CR  (1 << 15)
#define PXCMD_FR  (1 << 14)
#define PXCMD_FRE (1 << 4)
#define PXCMD_ST  (1 << 0)

//TODO: Allow handling port 30 and 31 too. Do this only if some weird hardware maps the AHCI devices there.
//To be honest, I'm not sure it's worth the trouble of using MmMapPhysicalMemory instead of the fast version
//(which handles physical pages)

static AhciController g_ahciControllers[32];
static int            g_ahciControllerNum = 0;

static AhciDevice g_ahciDevices[64];
static int        g_ahciDeviceNum = 0;

static AhciController* AhciRegisterController()
{
	if (g_ahciControllerNum >= (int)ARRAY_COUNT(g_ahciControllers))
		return NULL;
	
	return &g_ahciControllers[g_ahciControllerNum++];
}

static AhciDevice* AhciRegisterDevice()
{
	if (g_ahciDeviceNum >= (int)ARRAY_COUNT(g_ahciDevices))
		return NULL;
	
	return &g_ahciDevices[g_ahciDeviceNum++];
}

static int AhciCheckType(HbaPort* pPort)
{
	uint32_t ssts = pPort->m_sataStatus;
	
	uint32_t ipm = (ssts >> 8) & 0xf, det = ssts & 0xf;
	
	if (det != HBA_PORT_DET_PRESENT) return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)  return AHCI_DEV_NULL;
	
	switch (pPort->m_signature)
	{
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}

// According to AHCI spec, at least one device must be found with this controller.
static void AhciProbeController(AhciController *pController)
{
	if (pController->m_pMem->m_nPortImplemented & (3U << 31))
	{
		SLogMsg("There are devices at ports 30 or 31. We won't be able to reach those.");
	}
	for (int i = 0; i < 30; i++)
	{
		if (pController->m_pMem->m_nPortImplemented & (1 << i))
		{
			// Found device!
			int dt = AhciCheckType (&pController->m_pMem->m_ports[i]);
			
			switch (dt)
			{
				case AHCI_DEV_SATA:
					SLogMsg("Found SATA device at port %d on controller id %d", i, pController->m_nID);
					break;
				case AHCI_DEV_SATAPI:
					SLogMsg("Found SATAPI device at port %d on a controller id %d", i, pController->m_nID);
					break;
				case AHCI_DEV_SEMB:
					SLogMsg("Found SEMB device at port %d on a controller id %d", i, pController->m_nID);
					break;
				case AHCI_DEV_PM:
					SLogMsg("Found SEMB device at port %d on a controller id %d", i, pController->m_nID);
					break;
			}
			
			if (dt != AHCI_DEV_NULL)
			{
				AhciDevice *pDev = AhciRegisterDevice();
				pDev->m_pDev    = pController->m_pDev;
				pDev->m_pMem    = pController->m_pMem;
				pDev->m_pPort   = &pController->m_pMem->m_ports[i];
				pDev->m_nContID = pController->m_nID;
				pDev->m_nDevID  = i;
				
				pController->m_pDevices[i]   = pDev;
			}
		}
	}
}

static void AhciEnsureIdlePort (AhciDevice *pDev)
{
	if (!(pDev->m_pPort->m_cmdState & (PXCMD_CR | PXCMD_FR | PXCMD_FRE | PXCMD_ST)))
	{
		// Place these inside an idle state.
		pDev->m_pPort->m_cmdState &= ~PXCMD_ST;
		pDev->m_pPort->m_cmdState &= ~PXCMD_FRE;
		
		WaitMS(500);
		
		while (pDev->m_pPort->m_cmdState & (PXCMD_CR | PXCMD_FR))
		{
			__asm__ volatile ("pause");
		}
		
		//TODO: if CR or FR do not clear correctly.
	}
}

static void AhciEnsureIdlePorts (AhciController *pController)
{
	for (int i = 0; i < 30; i++)
	{
		if (pController->m_pDevices[i])
		{
			AhciEnsureIdlePort(pController->m_pDevices[i]);
		}
	}
}

static void AhciSetupCommandSlotsPort (AhciDevice *pDev)
{
	// No need for S64A. We are on a 32-bit system.
	pDev->m_pPort->clbu = pDev->m_pPort->fbu = 0;
	
	uint32_t clbPhys, fbPhys;
	void *pClb, *pFb;
	
	// Good thing apparently the maximum size of this is a kilobyte.
	// For now, waste 3 kib with this. However in the future this may be resolved.
	pClb = MmAllocateSinglePagePhy(&clbPhys);
	pFb  = MmAllocateSinglePagePhy(& fbPhys);
	
	pDev->m_pPort->clb = clbPhys;
	pDev->m_pPort-> fb =  fbPhys;
	
	// It is good practice for system software to 'zero-out' the memory allocated and referenced by PxCLB and PxFB.
	memset (pClb, 0, 4096);
	memset (pFb,  0, 4096);
	
	// After setting PxFB and PxFBU to the physical address of the FIS receive
	// area, system software shall set PxCMD.FRE to 1.
	pDev->m_pPort->m_cmdState |= PXCMD_FRE;
	
	// For each implemented port, clear the PxSERR register, by writing 1s to each implemented bit location
	pDev->m_pPort->m_sErr = 0;
}

static void AhciSetupCommandSlots (AhciController *pController)
{
	for (int i = 0; i < 30; i++)
	{
		if (pController->m_pDevices[i])
		{
			AhciSetupCommandSlotsPort(pController->m_pDevices[i]);
		}
	}
}

//TODO: Properly communicate such error codes to the user.
void AhciOnDeviceFound (PciDevice *pPCI)
{
	//AHCI Base Address is BAR5.
	
	uintptr_t nAHCIBaseAddr = PciGetBar (pPCI, 5);
	
	SLogMsg("The AHCI Controller's base address is %x.", nAHCIBaseAddr);
	
	// This BAR has additional flags.
	uint16_t barFlags = (uint16_t)(nAHCIBaseAddr & 0xFFF);
	nAHCIBaseAddr &= ~0xFFF;
	// bit 3: Prefetchable (PF)
	// bits 2 and 1: Indicate that this range can be mapped anywhere in 32-bit address space.
	// bit 0: RTE: Indicates a request for register memory space
	// ^^^ this was taken from the spec
	
	HbaMem *pHBA = MmMapPhysMemFastRW (nAHCIBaseAddr, true);
	if (!pHBA)
	{
		SLogMsg("Seems like you mapped too many MMIO devices. Wanna tone it down a bit, man?");
		return;
	}
	
	AhciController* pDev = AhciRegisterController();
	if (!pDev)
	{
		SLogMsg("Too many AHCI controllers have been registered. Tone it down a bit, man!");
		return;
	}
	
	pDev->m_pDev = pPCI;
	pDev->m_pMem = pHBA;
	pDev->m_nID  = g_ahciControllerNum;
	for (size_t i = 0; i < ARRAY_COUNT(pDev->m_pDevices); i++)
		pDev->m_pDevices[i] = NULL;
	
	// Indicate that system is aware of AHCI by setting GHC.AE to 1.
	pHBA->m_globalHBACtl |= (1U << 31);
	
	// Determine which ports are implemented by the HBA, by reading the PI register.
	AhciProbeController(pDev);
	
	// Ensure that the controller is not in the running state by reading and examining each
	// implemented port's PxCMD register. If PxCMD.ST, PxCMD.CR, PxCMD.FRE and PxCMD.FR are
	// all cleared, the port is in an idle state.
	AhciEnsureIdlePorts(pDev);
	
	// Determine how many command slots the HBA suppports by reading CAP.NCS
	int nCommandSlots = (pHBA->m_capabilities >> 8) & 0xF;
	
	// For each implemented port, system software shall allocate memory for and program:
	AhciSetupCommandSlots(nCommandSlots);
	
	// At this point the HBA is in a minimally initialized state. System software may provide additional 
	// programming of the GHC register and port PxCMD and PxSCTL registers based on the policies of the 
	// operating system and platform – these details are beyond the scope of this specification.
}
