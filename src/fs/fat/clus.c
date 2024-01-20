/*****************************************
		NanoShell Operating System
		  (C) 2024 iProgramInCpp

     FAT File System --- Cluster List
******************************************/
#include <fat.h>
#include <storabs.h>

static inline ALWAYS_INLINE
uint32_t FatClusterNumberToClusterPageIndex(uint32_t ClusterNumber)
{
	// TODO: Other FAT types
	return ClusterNumber / (PAGE_SIZE / 4);
}

void FatLoadClusterPage(FatFileSystem* pFS, uint32_t pageNumber)
{
	
}

uint32_t FatGetNextCluster(FatFileSystem* pFS, uint32_t ClusterNumber)
{
	uint32_t pageNumber = FatClusterNumberToClusterPageIndex(ClusterNumber);
	uint32_t inClusNumber = ClusterNumber % (PAGE_SIZE / 4); // TODO: Other FAT types
	
	// Fetch the cluster page.
	FatClusterPage* pPage = pFS->m_clusterList.m_pPages[pageNumber];
	
	if (!pPage)
	{
		// Load the cluster page.
		FatLoadClusterPage(pFS, pageNumber);
		
		// Fetch it again.
		pPage = pFS->m_clusterList.m_pPages[pageNumber];
	}
	
	// TODO: Other FAT types.
	return pPage->m_fat32[inClusNumber];
}
