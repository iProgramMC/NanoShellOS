#include <net/eth.h>
#include <task.h>

#ifdef USE_RTL8139
#include <rtl8139.h>

void EthBackendGetMacAddress(MacAddress* pAddr)
{
	return Rtl8139GetMacAddress(pAddr);
}

bool EthBackendSendPacket(void* pData, size_t size)
{
	return Rtl8139SendPacket(pData, size);
}

bool EthBackendInit()
{
	return Rtl8139Init();
}

#endif

Task* g_pEthTask;
LIST_ENTRY g_ethPacketsRxQueue;

void EthProcessPacket(EthernetPacket* pPacket)
{
	EthernetFrame* pFrame = (EthernetFrame*) pPacket->m_data;
	
	SLogMsg("EthProcessPacket: frame received from MAC " MAC_PRINTF_FMT " (dest " MAC_PRINTF_FMT ")",
		MAC_AS_PARMS(pFrame->m_macSrc),
		MAC_AS_PARMS(pFrame->m_macDst));
}

void EthTask()
{
	while (true)
	{
		while (true)
		{
			// check if queue is empty
			cli;
			if (IsListEmpty(&g_ethPacketsRxQueue))
				// break out, start waiting
				break;
			
			// ok, pop one off and process it
			PLIST_ENTRY pEntry = g_ethPacketsRxQueue.Flink;
			RemoveHeadList(&g_ethPacketsRxQueue);
			sti;
			
			EthernetPacket* pPacket = CONTAINING_RECORD(pEntry, EthernetPacket, m_entry);
			
			EthProcessPacket(pPacket);
			MmFree(pPacket);
		}
		
		// note, this will turn interrupts back on
		WaitObject(&g_ethPacketsRxQueue);
	}
}

void EthReceivedPacket(EthernetPacket* pPacket, bool disableInterrupts)
{
	if (disableInterrupts) cli;
	InsertTailList(&g_ethPacketsRxQueue, &pPacket->m_entry);
	if (disableInterrupts) sti;
	
	KeUnsuspendTasksWaitingForObject(&g_ethPacketsRxQueue);
}

bool EthSendPacket(MacAddress* dest, void* data, size_t size, uint16_t protocol)
{
	EthernetFrame *pFrame = MmAllocate(sizeof(EthernetFrame) + size);
	
	// copy data
	memcpy(pFrame->m_data, data, size);
	
	// initialize frame
	memcpy(&pFrame->m_macDst, dest, sizeof(MacAddress));
	EthBackendGetMacAddress(&pFrame->m_macSrc);
	
	pFrame->m_type = HostToNet16(protocol);
	
	// fire!
	return EthBackendSendPacket(pFrame, sizeof(EthernetFrame) + size);
}

void EthInit()
{
	InitializeListHead(&g_ethPacketsRxQueue);
	
	// Initialize backend
	if (!EthBackendInit()) {
		SLogMsg("Couldn't initialize ethernet backend, therefore not continuing");
		return;
	}
	
	int errCode = 0;
	g_pEthTask = KeStartTask(EthTask, 0, &errCode);
	if (!g_pEthTask) {
		SLogMsg("Couldn't initialize ethernet task, not continuing");
		return;
	}
	
	KeTaskAssignTag(g_pEthTask, "Network Service");
	KeUnsuspendTask(g_pEthTask);
	KeDetachTask(g_pEthTask);
}

