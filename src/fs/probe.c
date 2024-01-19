/*****************************************
		NanoShell Operating System
		  (C) 2024 iProgramInCpp

        File System Probing module
******************************************/

// Abstract:
// This module performs a probe on all available drives for file systems.

#include <fat.h> // need this for the MasterBootRecord
#include <storabs.h>


// returns whether or not this is actually the correct file system to use for the drive
bool FsMountExt2Partition(DriveID driveID, int partitionStart, int partitionSizeSec);


void FsFoundPartition(int DriveId, int PartIndex, MasterBootRecord* BootRecord)
{
	Partition* Part = &BootRecord->m_Partitions[PartIndex];
	
	// For now, just mount ext2 partitions. We'll add detection code later.
	if (FsMountExt2Partition(DriveId, Part->m_StartLBA, Part->m_PartSizeSectors))
		return;
	
	SLogMsg("drive %d part %d doesn't have a valid file system", DriveId, PartIndex);
}

void FsProbeDrive(int i)
{
	if (!StIsDriveAvailable(i)) return;
	
	union
	{
		uint8_t bytes[512];
		MasterBootRecord s;
	} mbr;
	
	memset(mbr.bytes, 0, sizeof mbr.bytes);
	
	// read one sector from block 0
	DriveStatus ds = StDeviceRead( 0, mbr.bytes, i, 1 );
	if (ds != DEVERR_SUCCESS)
	{
		LogMsg("Device failure %x on drive %d while trying to find an Ext2 file system! Not proceeding!", ds);
		return;
	}
	
	// probe each partition
	for (int pi = 0; pi < 4; pi++)
	{
		Partition* pPart = &mbr.s.m_Partitions[pi];
		
		if (pPart->m_PartTypeDesc != 0)
		{
			// This is a valid partition.  Mount it.
			FsFoundPartition(i, pi, &mbr.s);
		}
	}
	
	// well, we could try without an MBR here too, but I'm too lazy to get the size of the drive itself right now.
}

void FsProbeDrives()
{
	// probe each drive
	for (int i = 0; i < 0x100; i++)
	{
		FsProbeDrive(i);
	}
}
