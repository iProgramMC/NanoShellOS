/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

     Storage Abstraction: AHCI Driver
******************************************/

#include <memory.h>
#include <storabs.h>
#include <misc.h>

//! TODO: Use interrupts instead of polling for this driver.

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define PXCMD_CR  (1 << 15) // Commands Running
#define PXCMD_FR  (1 << 14) // FIS Receive Running
#define PXCMD_FRE (1 << 4)  // FIS Receive Enable
#define PXCMD_POD (1 << 2)  // Power On Device
#define PXCMD_SUD (1 << 1)  // Spin Up Device
#define PXCMD_ST  (1 << 0)  // Start

#define PXSCTL_DET_INIT    (1 << 1)
#define PXSSTS_DET_PRESENT (3)

#define CAPSEXT_BOH (1 << 0)

#define BOHC_BOS (1 << 0) // BIOS owned semphore
#define BOHC_OOS (1 << 1) // OS owned semaphore
#define BOHC_BB  (1 << 4) // BIOS Busy (polling bit while BIOS cleans things up for us)

#define ATAS_DRQ (1 << 3)
#define ATAS_BSY (1 << 7)

#define PXI_TFE (1 << 30)

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
					SLogMsg("Found PM device at port %d on a controller id %d", i, pController->m_nID);
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
				pDev->m_pParent = pController;
				
				pController->m_pDevices[i]   = pDev;
			}
		}
	}
}

void AhciOnDeviceFound (PciDevice *pPCI)
{
	//AHCI Base Address is BAR5.
	
	uintptr_t nAHCIBaseAddr = PciGetBar (pPCI, 5);
	
	SLogMsg("The AHCI Controller's base address is %x.", nAHCIBaseAddr);
	
	// This BAR has additional flags.
	UNUSED uint16_t barFlags = (uint16_t)(nAHCIBaseAddr & 0xFFF);
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
}

static void AhciPortCommandStop (AhciDevice *pDev)
{
	SLogMsg("STOPPING command engine...");
	if (pDev->m_pPort->m_cmdState & (PXCMD_CR | PXCMD_FR | PXCMD_FRE | PXCMD_ST))
	{
		SLogMsg("Setting flags.");
		pDev->m_pPort->m_cmdState &= ~PXCMD_ST;
		pDev->m_pPort->m_cmdState &= ~PXCMD_FRE;
		
		SLogMsg("Waiting 500 ms.");
		WaitMS(500); //TODO: is a wait actually needed here? the spec says we need it
		
		SLogMsg("Waiting for PXCMD_CR and FR to go out. Intial flags: %x.", pDev->m_pPort->m_cmdState);
		while (pDev->m_pPort->m_cmdState & (PXCMD_CR | PXCMD_FR))
		{
			asm ("pause":::"memory");
		}
		SLogMsg("Done!");
	}
	SLogMsg("Stop Done");
}

static void AhciStopCommandEngine (AhciController *pController)
{
	for (int i = 0; i < 30; i++)
	{
		if (pController->m_pDevices[i])
		{
			AhciPortCommandStop(pController->m_pDevices[i]);
		}
	}
}

static void AhciPortWaitFull (AhciDevice *pDev)
{
	while (true)
	{
		if (!(pDev->m_pPort->m_tfd & (ATAS_DRQ | ATAS_BSY)))
			return;
		
		asm("pause":::"memory");
	}
}
static bool AhciPortWait (AhciDevice *pDev)
{
	int timeout = 10000000;//some ridiculously high amount
	while (timeout)
	{
		if (!(pDev->m_pPort->m_tfd & (ATAS_DRQ | ATAS_BSY)))
			return true;
		
		timeout--;
		
		asm("pause":::"memory");
	}
	
	return false;
}

static void AhciPortReset(AhciDevice *pDev)
{
	AhciPortCommandStop(pDev);

	uint32_t state = pDev->m_pPort->m_intStatus;
	asm("":::"memory");
	pDev->m_pPort->m_intStatus = state;
	
	if (!AhciPortWait (pDev))
	{
		SLogMsg("Intrusive reset");
		// Perform more 'intrusive' HBA<->Port comm reset (sec. 10.4.2 of spec).
		pDev->m_pPort->m_sCtl = PXSCTL_DET_INIT;
		
		// Spec says 1 ms. Going to wait 2 instead, just to be safe.
		WaitMS(2);
		
		pDev->m_pPort->m_sCtl = 0;
	}
	
	while ((pDev->m_pPort->m_sataStatus & 0xF) != PXSSTS_DET_PRESENT)
	{
		asm ("pause":::"memory");
	}
	
	pDev->m_pPort->m_sErr = ~0; // Write all 1s to SATA error register.
}

static void AhciPerformBiosHandoff(volatile HbaMem *pHBA)
{
	SLogMsg("Performing BIOS/OS handoff...");
	if (pHBA->m_capabilitiesExt & CAPSEXT_BOH)
	{
		SLogMsg("Initting.");
		pHBA->m_BOHC |= BOHC_OOS;
		
		SLogMsg("Waiting for BIOS to finish cleaning up.");
		while (pHBA->m_BOHC & BOHC_BOS)
		{
			asm ("pause":::"memory");
		}
		
		SLogMsg("More Waiting...");
		
		WaitMS(25);
		
		// if BIOS Busy is still set, give it a while (2 seconds)
		if (pHBA->m_BOHC & BOHC_BB)
			WaitMS(2000);
	}
	SLogMsg("Done.");
}

void AhciControllerInit(AhciController* pDev)
{
	volatile HbaMem *pHBA = pDev->m_pMem;
	
	// Indicate that system is aware of AHCI by setting GHC.AE to 1.
	pHBA->m_globalHBACtl |= (1U << 31);
	
	pDev->m_nMaxCommands = (pHBA->m_capabilities & 0x1F00) >> 8;
	
	// Transfer ownership from BIOS if supported.
	AhciPerformBiosHandoff(pDev->m_pMem);
	
	// Determine which ports are implemented by the HBA, by reading the PI register.
	AhciProbeController(pDev);
	
	// Ensure that the controller is not in the running state by reading and examining each
	// implemented port's PxCMD register. If PxCMD.ST, PxCMD.CR, PxCMD.FRE and PxCMD.FR are
	// all cleared, the port is in an idle state.
	AhciStopCommandEngine(pDev);
}

void AhciPortCommandStart(AhciDevice *pDev)
{
	while (pDev->m_pPort->m_cmdState & PXCMD_CR)
	{
		asm("pause":::"memory");
	}
	
	pDev->m_pPort->m_cmdState |= PXCMD_FRE;
	pDev->m_pPort->m_cmdState |= PXCMD_ST;
}

static int AhciGetCommandSlot(AhciDevice *pDevice)
{
	uint32_t slots = pDevice->m_pPort->m_sAct | pDevice->m_pPort->m_cmdIssue;
	for (int i = 0; i < pDevice->m_pParent->m_nMaxCommands; i++)
	{
		if (!(slots & (1 << i))) return i;
	}
	
	SLogMsg("NO empty command slots on port!");
	return -1;
}

static HbaCmdHeader* AhciGetActiveHeader(AhciDevice *pDevice, int cmdSlot)
{
	return &(((HbaCmdHeader*)pDevice->m_pCommandListBase)[cmdSlot]);
}

static void AhciPortCommandWait (AhciDevice *pDev, int cmdSlot)
{
	// Wait on command completion after command issue, and double check any error.
	
	while (true)
	{
		// If command has been processed:
		if (!(pDev->m_pPort->m_cmdIssue & (1 << cmdSlot)))
			break;
		
		// If there's been an error:
		if (pDev->m_pPort->m_intStatus & PXI_TFE) // Task File Error
		{
			SLogMsg("AHCI Port command %d failed!", cmdSlot);
			return;
		}
		
		asm ("pause":::"memory");
		
		KeTaskDone();
	}
	
	if (pDev->m_pPort->m_intStatus & PXI_TFE)
	{
		SLogMsg("AHCI Port command %d failed!", cmdSlot);
		return;
	}
}

static void AhciDumpDevRecord(AhciDevice *pDev)
{
	char model_number[41];
	model_number[40] = 0;
	
	memcpy (model_number, &pDev->m_pDevIDRecord[27], 20 * 2);
	
	for (int i = 0; i < 40; i += 2)
	{
		char aux;
		aux = model_number[i+1];
		model_number[i+1] = model_number[i];
		model_number[i]   = aux;
	}
	
	for (int i = 39; i > 0; i--)
	{
		if (model_number[i] != ' ') break;
		
		model_number[i] = 0;
	}
	
	LogMsg("Found AHCI drive (address %x): '%s'", pDev, model_number);
}

static void AhciPortIdentify (AhciDevice *pDev)
{
	// Perform ATA_IDENTIFY command on an ATA/ATAPI drive, and store capacity and ID record.
	uint32_t mem = 0;
	
	int cmdSlot = AhciGetCommandSlot (pDev);
	if (cmdSlot < 0)
	{
		SLogMsg("Cannot run identify command (yet). No command slots are left. Might wanna wait first?");
		return;
	}
	
	HbaCmdHeader *pHeader = AhciGetActiveHeader(pDev, cmdSlot);
	
	// Reload the interrupt status;
	uint32_t hold = pDev->m_pPort->m_intStatus;
	asm("":::"memory");
	pDev->m_pPort->m_intStatus = hold;
	
	uint8_t *devIdRecord = MmAllocateSinglePagePhy (&mem);
	ASSERT(devIdRecord);
	
	HbaCmdTable *pTable = pDev->m_pCommandTableBase[cmdSlot];
	memset (pTable, 0, sizeof *pTable);
	
	pTable->prdt_entry[0].m_dataBase  = mem;
	pTable->prdt_entry[0].m_dataBaseU = 0;
	pTable->prdt_entry[0].m_dataBaseCount = 512 - 1;
	pHeader->m_prdtLength = 1; // 1 PRD.
	
	// Setup command FIS.
	FisRegH2D* pCmdFis = (FisRegH2D*) &pTable->cfis;
	pCmdFis->fis_type = FIS_TYPE_REG_H2D;
	pCmdFis->c        = 1;
	
	if (pDev->m_pPort->m_signature == SATA_SIG_ATAPI)
		pCmdFis->command = ATA_IDENTIFY_PACKET;
	else
		pCmdFis->command = ATA_IDENTIFY;
	
	pCmdFis->device = 0;
	
	// Wait on previous command to complete.
	AhciPortWaitFull (pDev);
	
	// Issue the command.
	SLogMsg("Issuing command ...");
	pDev->m_pPort->m_cmdIssue |= (1 << cmdSlot);
	
	// Wait for it.
	SLogMsg("Waiting for command ...");
	AhciPortCommandWait (pDev, cmdSlot);
	
	// And there we go! We have an ID record now.
	// Dump it all.
	
	SLogMsg("Dumping identify data obtained: ");
	for (int i = 0; i < 512; i += 16)
	{
		for (int j = 0; j <  16; j++)
		{
			SLogMsgNoCr("%b ", devIdRecord[i+j]);
		}
		SLogMsgNoCr("    ");
		for (int j = 0; j <  16; j++)
		{
			char c = devIdRecord[i+j];
			if (c < 32) c = '.';
			if (c == 127) c = '.';
			SLogMsgNoCr("%c", c);
		}
		SLogMsg("");
	}
	
	memcpy (pDev->m_pDevIDRecord, devIdRecord, sizeof (pDev->m_pDevIDRecord));
	MmFree (devIdRecord);
	
	AhciDumpDevRecord(pDev);
}

void AhciPortInit(AhciDevice *pDev)
{
	AhciPortReset (pDev);
	AhciPortCommandStart (pDev);
	
	// Spin up, power on device. If the capability isn't supported the bits
	// will be read-only and this won't do anything.
	SLogMsg("SpinUp, PowerOn device");
	pDev->m_pPort->m_cmdState |= PXCMD_POD | PXCMD_SUD;
	
	WaitMS(100); // Why?
	
	SLogMsg("Commands Stop");
	AhciPortCommandStop (pDev);
	
	// 1kbyte align as per spec. Pages always allocate in 4096-byte alignment :)
	
	// Allocate the command list base.
	
	uint32_t mem;
	
	pDev->m_pCommandListBase = MmAllocateSinglePagePhy (&mem);
	ASSERT(pDev->m_pCommandListBase && "Huh?");
	memset(pDev->m_pCommandListBase, 0, 4096);
	
	pDev->m_pPort->m_cmdListBase  = mem;
	pDev->m_pPort->m_cmdListBaseU = 0;
	
	// Allocate the place received FISes will be copied to.
	pDev->m_pFisBase = MmAllocateSinglePagePhy (&mem);
	ASSERT(pDev->m_pFisBase && "Huh?");
	memset(pDev->m_pFisBase, 0, 4096);
	
	pDev->m_pPort->m_fisBase  = mem;
	pDev->m_pPort->m_fisBaseU = 0;
	
	HbaCmdHeader* pTable = (HbaCmdHeader*)pDev->m_pCommandListBase;
	
	for (int i = 0; i < pDev->m_pParent->m_nMaxCommands; i++)
	{
		HbaCmdHeader *pHeader = &pTable[i];
		
		pHeader->m_desc.cfl =  sizeof (FisRegH2D) / sizeof (uint32_t);
		
		// Allocate a 
		pDev->m_pCommandTableBase[i] = MmAllocateSinglePagePhy(&mem);
		memset (pDev->m_pCommandTableBase[i], 0, 4096);
		
		pHeader->m_cmdTableBase  = mem;
		pHeader->m_cmdTableBaseU = 0;
	}
	
	AhciPortCommandStart(pDev);
	
	AhciPortIdentify(pDev);
}

void StAhciInit()
{
	if (!g_ahciControllerNum)
	{
		LogMsg("No AHCI controllers found.");
		return;
	}
	LogMsg("Initializing AHCI controllers...");
	for (int i = 0; i < g_ahciControllerNum; i++)
	{
		AhciControllerInit (&g_ahciControllers[i]);
	}
	for (int i = 0; i < g_ahciDeviceNum; i++)
	{
		AhciPortInit (&g_ahciDevices[i]);
	}
}
