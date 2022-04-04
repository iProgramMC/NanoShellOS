/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

    FAT File System Disk Check Utility
******************************************/
#include <fat.h>
#ifdef EXPERIMENTAL
bool g_CheckdiskErrorValue = false;

void CheckDiskFatScanFileOrDir(FileNode *pNode, uint32_t *pFat)
{
	// First, check the file
	uint32_t cluster = pNode->m_implData2;
	
	while (cluster != 0 && cluster < FAT_END_OF_CHAIN)
	{
		uint32_t next_cluster = pFat[cluster];
		pFat[cluster] = 0;
		cluster = next_cluster;
	}
	
	if (cluster == 0)
	{
		LogMsg("File node %s is corrupted: encountered zero in file chain", pNode->m_name);
		
		if (pNode->children)
			LogMsg("This is a directory.  Not checking children");
		
		g_CheckdiskErrorValue = true;
		
		return;
	}
	
	if (!pNode->children)
	{
		// If this is a directory, most likely it needs to be loaded.
		if (pNode->m_type & FILE_TYPE_DIRECTORY)
		{
			bool b = FsOpenDir (pNode);
			if (!b)
			{
				LogMsg("File directory %s is corrupted.", pNode->m_name);
				g_CheckdiskErrorValue = true;
				return;
			}
		}
	}
	
	if (pNode->children)
	{
		FileNode *child = pNode->children;
		while (child)
		{
			CheckDiskFatScanFileOrDir (child, pFat);
			
			child = child->next;
		}
	}
}

void CheckDiskFatCheckLostChains (uint32_t* pFat, size_t nFat)
{
	for (size_t i = 2; i < nFat; i++)
	{
		if (pFat[i])
		{
			LogMsg("Found lost chain starting from cluster %d.", i);
			
			// Zero out so it won't find more lost chains
			uint32_t cluster = i;
			
			while (cluster != 0 && cluster < FAT_END_OF_CHAIN)
			{
				uint32_t next_cluster = pFat[cluster];
				pFat[cluster] = 0;
				cluster = next_cluster;
			}
			
			if (cluster == 0)
			{
				LogMsg("Additionally, this lost chain is corrupted.");
			}
			
			g_CheckdiskErrorValue = true;
		}
	}
}

void CheckDiskFat(uint32_t** pFatOut, int fat_number)
{
	g_CheckdiskErrorValue = false;
	
	if (fat_number < 0 || fat_number >= 32)
	{
		LogMsg("Could not open Fat%d.", fat_number);
		return;
	}
	if (!s_Fat32Structures[fat_number].m_bMounted)
	{
		LogMsg("Could not open Fat%d.", fat_number);
		return;
	}
	LogMsg("The type of the file system is FAT.\n");
	
	FatFileSystem *pFat32 = &s_Fat32Structures[fat_number];
	
	// Get the root file node of the FAT:
	char path[100];
	sprintf (path, "/Fat%d", fat_number);
	FileNode *pNode = FsResolvePath (path);
	
	if (!pNode) return;
	
	// First of all, check the FAT.
	
	uint32_t nBytesPerFAT     = 512 * pFat32->m_bpb.m_nSectorsPerFat32;
	
	uint32_t* pFatCopy = MmAllocateK( nBytesPerFAT );
	*pFatOut = pFatCopy;
	
	memcpy_ints (pFatCopy, pFat32->m_pFat, nBytesPerFAT / 4);
	
	pFat32->m_bReportErrors = true;
	
	// This code works on the _copy_.
	
	// Look through all the files recursively.
	
	LogMsg("Verifying files...");
	CheckDiskFatScanFileOrDir(pNode, pFatCopy);
	
	LogMsg("Checking for lost chains...");
	CheckDiskFatCheckLostChains(pFatCopy, nBytesPerFAT / 4);
	
	LogMsg("");
	
	pFat32->m_bReportErrors = false;
	
	int nErrors = pFat32->m_nReportedErrors;
	pFat32->m_nReportedErrors = 0;
	
	if (g_CheckdiskErrorValue)
		LogMsg("The checkdisk utility has found errors.");
	else
		LogMsg("NanoShell has scanned the file system and found no errors.  No further action is required.");
	
	if (nErrors)
	{
		LogMsg("NanoShell has found %d other errors in the file system.");
	}
	
	if (nErrors || g_CheckdiskErrorValue)
	{
		LogMsg("No action has been taken on behalf on NanoShell.  Please use other software which provides a check-disk utility which fixes the file system.");
	}
	g_CheckdiskErrorValue = false;
}


void CheckDiskFatMain(int fat_number)
{
	uint32_t* pFat = NULL;
	CheckDiskFat(&pFat, fat_number);
	
	// If it failed, or succeeded, free the copy of fat we have worked on, we don't need it
	if (pFat)
		MmFree (pFat);
}

#endif
