//  ***************************************************************
//  fs/tmp/pack.c - Creation date: 18/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <tmpfs.h>
#include <memory.h>
#include <debug.h>
#include <misc.h>
#include <time.h>
#include <string.h>
#include <tar.h>

static uint32_t OctToBin(char *data, uint32_t size)
{
	uint32_t value = 0;
	while (size > 0)
	{
		size--;
		value *= 8;
		value += *data++ - '0';
	}
	return value;
}

void FsTempCreatePermanentFile(FileNode* pNode, const char* fileName, uint8_t* buffer, uint32_t fileSize);

// bPermanent - States that this memory never goes away and we don't need to duplicate it.
void FsInitializeInitRd(void* pRamDisk, bool bPermanent)
{
	uint32_t epochTime = GetEpochTime();
	// Add files to the root FS.
	for (Tar *ramDiskTar = (Tar *) pRamDisk; !memcmp(ramDiskTar->ustar, "ustar", 5);)
	{
		uint32_t fileSize = OctToBin(ramDiskTar->size, 11);
		uint32_t pathLength = strlen(ramDiskTar->name);
		uint32_t mtime = OctToBin(ramDiskTar->mtime, 11);
		bool hasDotSlash = false;

		if (ramDiskTar->name[0] == '.')
		{
			pathLength -= 1;
			hasDotSlash = true;
		}

		if (pathLength > 0)
		{
			char *pname = ramDiskTar->name + hasDotSlash;
			char buffer[PATH_MAX];
			
			if (strlen(pname) >= sizeof buffer) goto _skip;
			
			strcpy(buffer, pname);
			
			// For each slash character in the buffer:
			int lastRemovedSlashIndex = -1;
			for (int i = 0; buffer[i] != 0; i++)
			{
				if (buffer[i] == '/')
				{
					// try removing it...
					buffer[i] = 0;
					
					lastRemovedSlashIndex = i;
					
					// add the directory, if there is one
					if (buffer[0] != 0)
					{
						int res = FiMakeDir(buffer);
						if (res < 0 && res != -EEXIST)
						{
							LogMsg("Could not create directory '%s': %s", buffer, GetErrNoString(res));
						}
					}
					
					// restore it
					buffer[i] = '/';
				}
			}
			
			if (buffer[0] != 0)
			{
				//note: this should be safe anyway even if lastRemovedSlashIndex == -1, as it
				//just gets neutralized to zero.
				if (buffer[lastRemovedSlashIndex + 1] != 0)
				{
					/*RootFileNode* node = FsRootCreateFileAt(buffer, &ramDiskTar->buffer, fileSize);
					if (!node) continue;
					
					node->node.m_modifyTime = mtime;
					node->node.m_createTime = epochTime;*/
					
					if (bPermanent)
					{
						char buf1[PATH_MAX];
						
						strcpy(buf1, buffer);
						
						// get rid of the last slash
						char* ptr = strrchr(buf1, '/');
						if (!ptr) ptr = buf1;
						*ptr = 0;
						ptr++;
						
						FileNode *pNode = FsResolvePath(buf1, false);
						
						FsTempCreatePermanentFile(pNode, ptr, (uint8_t*)&ramDiskTar->buffer, fileSize);
						
						FsReleaseReference(pNode);
					}
					else
					{
						// do it the regular way
						int fd = FiOpen(buffer, O_CREAT | O_WRONLY);
						if (fd < 0)
						{
							LogMsg("Could not create file '%s': %s", buffer, GetErrNoString(fd));
						}
						else
						{
							uint32_t wr = FiWrite(fd, &ramDiskTar->buffer, fileSize);
							if (wr != fileSize)
								LogMsg("File %s couldn't be fully written. (%d/%d bytes)", buffer, wr, fileSize);
							
							FiClose(fd);
						}
					}
				}
			}
		}
	_skip:

		// Advance to the next entry
		ramDiskTar = (Tar *) ((uintptr_t) ramDiskTar + ((fileSize + 511) / 512 + 1) * 512);
	}
	SLogMsg("Init ramdisk is fully setup!");
}

void FsInitRdInit()
{
	// Initialize the ramdisk
	multiboot_info_t* pInfo = KiGetMultibootInfo();
	if (pInfo->mods_count != 1)
		KeBugCheck(BC_EX_INITRD_MISSING, NULL);

	//Usually the mods table is below 1m. However, if it's below 8M, we
	//can still access it through the identity mapping at 0xC0000000
	if (pInfo->mods_addr >= 0x800000)
	{
		LogMsg("Module table starts at %x.  OS state not supported", pInfo->mods_addr);
		KeStopSystem();
	}
	//The initrd module is here.
	multiboot_module_t *initRdModule = (void*) (pInfo->mods_addr + 0xc0000000);

	//Precalculate an address we can use
	void* pInitrdAddress = (void*)(0xc0000000 + initRdModule->mod_start);
	
	// We should no longer have the problem of it hitting our frame bitset.
	
	SLogMsg("Init Ramdisk module Start address: %x, End address: %x", initRdModule->mod_start, initRdModule->mod_end);
	//If the end address went beyond 1 MB:
	if (initRdModule->mod_end >= 0x100000)
	{
		//Actually go to the effort of mapping the initrd to be used.
		pInitrdAddress = MmMapPhysicalMemory (initRdModule->mod_start, initRdModule->mod_end);
	}
	
	SLogMsg("Physical address that we should load from: %x", pInitrdAddress);
	
	// Load the initrd last so its entries show up last when we type 'ls'.
	FsInitializeInitRd(pInitrdAddress, true);
}
