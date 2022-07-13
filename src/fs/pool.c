/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        Virtual File System module
******************************************/
#include <vfs.h>
#include <memory.h>
#include <misc.h>

FsPoolUnit g_firstUnit;

FileNode* MakeFileNodeFromPool()
{
	// Search for a spot in the pre-existing file nodes.
	FsPoolUnit *pUnit = &g_firstUnit, *pLastUnit = &g_firstUnit;
	while (pUnit)
	{
		//ugly compiler hack
		uint64_t allNodesFull = 
		#if C_FILE_NODES_PER_POOL_UNIT == 64
			~0ULL
		#else
			((1ULL << C_FILE_NODES_PER_POOL_UNIT) - 1)
		#endif
		;
		
		if (pUnit->m_bNodeFree != allNodesFull)
		{
			// Which one?
			for (int i = 0; i < C_FILE_NODES_PER_POOL_UNIT; i++)
			{
				if (!(pUnit->m_bNodeFree & (1ULL << i)))
				{
					// Done. Return this node.
					
					pUnit->m_nNodeLA[i] = GetTickCount();
					pUnit->m_nodes[i].m_pPoolUnit = pUnit;
					
					pUnit->m_bNodeFree |= (1ULL << i);
					
					return &pUnit->m_nodes[i];
				}
			}
		}
		
		pLastUnit = pUnit;
		pUnit     = pUnit->m_pNextUnit;
	}
	
	ASSERT(pLastUnit);
	
	// There are no more free units left. Create a new one.
	FsPoolUnit* pNewUnit = MmAllocateK(sizeof(FsPoolUnit));
	ASSERT(pNewUnit);
	
	pLastUnit->m_pNextUnit = pNewUnit;
	pNewUnit ->m_pNextUnit = NULL;
	
	pNewUnit->m_bNodeFree = 1;
	pNewUnit->m_nNodeLA[0] = GetTickCount();
	
	
	pNewUnit->m_nodes[0].m_pPoolUnit = pNewUnit;
	
	return &pNewUnit->m_nodes[0];
}

void FreeFileNode(FileNode *pFileNode)
{
	FsPoolUnit* pUnit = pFileNode->m_pPoolUnit;
	
	int i = (int)(pFileNode - pUnit->m_nodes);
	
	pUnit->m_bNodeFree &=~ (1ULL << i);
}

void FiPoolDebugDump()
{
	LogMsg("Size of: a single file = %d, a single pool unit = %d.", sizeof(FileNode),sizeof(FsPoolUnit));
	FsPoolUnit *pUnit = &g_firstUnit;
	while (pUnit)
	{
		LogMsg("POOL UNIT: %p%s", pUnit, pUnit == &g_firstUnit ? " (first unit)" : "");
		
		uint8_t* pThing = (uint8_t*)&pUnit->m_bNodeFree;
		LogMsg("Usage: %b%b%b%b%b%b%b%b", pThing[7], pThing[6], pThing[5], pThing[4], pThing[3], pThing[2], pThing[1], pThing[0]);
		
		pUnit = pUnit->m_pNextUnit;
	}
}
