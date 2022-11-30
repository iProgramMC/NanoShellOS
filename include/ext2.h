//  ***************************************************************
//  ext2.h - Creation date: 18/11/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#ifndef _EXT2_H
#define _EXT2_H

#include <vfs.h>
#include <string.h>
#include <misc.h>
#include <print.h>
#include <memory.h>
#include <storabs.h>

#define EXT2_SIGNATURE (0xEF53)

typedef union Ext2SuperBlock
{
	uint8_t m_bytes[1024];
	
	struct
	{
		// Base superblock fields
		uint32_t m_nInodes;
		uint32_t m_nBlocks;
		uint32_t m_nBlocksSuperUser;
		uint32_t m_nUnallocatedBlocks;
		uint32_t m_nUnallocatedInodes;
		uint32_t m_firstDataBlock;
		uint32_t m_log2BlockSize;
		uint32_t m_log2FragmentSize;
		uint32_t m_blocksPerGroup;
		uint32_t m_fragsPerGroup;
		uint32_t m_inodesPerGroup;
		uint32_t m_lastMountTime;
		uint32_t m_lastWrittenTime;
		uint16_t m_nMountsSinceLastCheck;
		uint16_t m_nMountsAllowedBeforeCheck;
		uint16_t m_nE2Signature;           // EXT2_SIGNATURE
		uint16_t m_fsState;
		uint16_t m_errorDetectedAction;
		uint16_t m_minorVersion;
		uint32_t m_lastCheckTime;
		uint32_t m_forceCheckAfter;
		uint32_t m_systemID;
		uint32_t m_majorVersion;
		uint16_t m_uidThatCanUseReservedBlocks;
		uint16_t m_gidThatCanUseReservedBlocks;
		// Extended superblock fields
		uint32_t m_firstNonReservedInode;
		uint16_t m_inodeStructureSize;
		uint16_t m_blockGroupThisIsPartOf;
		uint32_t m_optionalFeatures;
		uint32_t m_requiredFeatures;
		uint32_t m_readOnlyFeatures;
		char     m_fileSystemID[16];
		char     m_volumeName[16];
		char     m_pathVolumeLastMountedTo[64];
		uint32_t m_compressionAlgos;
		uint8_t  m_nBlocksPreallocateFiles;
		uint8_t  m_nBlocksPreallocateDirs;
		uint16_t m_unused;
		char     m_journalID[16];
		uint32_t m_journalInode;
		uint32_t m_journalDevice;
		uint32_t m_headOfOrphanInodeList;
	}
	__attribute__((packed));
}
Ext2SuperBlock;

typedef union Ext2BlockGroupDescriptor
{
	uint8_t m_bytes[32];
	struct
	{
		uint32_t m_blockAddrBlockUsageBmp;
		uint32_t m_blockAddrInodeUsageBmp;
		uint32_t m_startBlockAddrInodeTable;
		uint16_t m_nUnallocatedBlocks;
		uint16_t m_nUnallocatedInodes;
		uint16_t m_nDirs;
	}
	__attribute__((packed));
}
Ext2BlockGroupDescriptor;

typedef struct Ext2Inode
{
	uint16_t m_permissions;
	uint16_t m_uid;
	uint32_t m_size; // lower 32 bits
	uint32_t m_lastAccessTime;
	uint32_t m_creationTime;
	uint32_t m_lastModTime;
	uint32_t m_deletionTime;
	uint16_t m_gid;
	uint16_t m_nLinks;
	uint32_t m_diskSectors;
	uint32_t m_flags;
	uint32_t m_osSpecific1;
	uint32_t m_directBlockPointer[12];
	uint32_t m_singlyIndirBlockPtr;
	uint32_t m_doublyIndirBlockPtr;
	uint32_t m_triplyIndirBlockPtr;
	uint32_t m_nGeneration;
	uint32_t m_extendedAttributeBlock;
	uint32_t m_upperSize; // upper 32 bits of m_size.
	uint32_t m_fragmentBlockAddr;
	uint32_t m_osSpecific2[3];
	//no point in putting *this* into a union, since it's 128 bytes already
}
__attribute__((packed))
Ext2Inode;

typedef struct Ext2DirEnt
{
	uint32_t m_inode;
	uint16_t m_entrySize;
	uint8_t  m_nameLength;
	uint8_t  m_typeIndicator;
	char     m_name[];
}
__attribute__((packed))
Ext2DirEnt;

// File System State
enum
{
	E2_FS_CLEAN = 1,
	E2_FS_HAS_ERRORS,
};

// Error Detected Action
enum
{
	E2_ERR_IGNORE = 1,  // Ignore the error
	E2_ERR_MOUNT_AS_RO, // Re-mount as read only
	E2_ERR_PANIC,       // Panic
};

// Creator ID
enum
{
	E2_CREATOR_LINUX,
	E2_CREATOR_HURD,
	E2_CREATOR_MASIX,
	E2_CREATOR_FREEBSD,
	E2_CREATOR_BSDLITE,
};

// Optional features
enum
{
	E2_OPT_PREALLOCATE         = (1 << 0),
	E2_OPT_AFS_SERVER_INODES   = (1 << 1),
	E2_OPT_JOURNAL             = (1 << 2),
	E2_OPT_INODE_EXT_ATTRS     = (1 << 3),
	E2_OPT_FS_RESIZE_SELF      = (1 << 4),
	E2_OPT_DIRS_USE_HASH_INDEX = (1 << 5),
	
	E2_OPT_UNSUPPORTED_FLAGS = -1,
};

// Required features
enum
{
	E2_REQ_COMPRESSION_USED = (1 << 0),
	E2_REQ_DIR_TYPE_FIELD   = (1 << 1),
	E2_REQ_REPLAY_JOURNAL   = (1 << 2),
	E2_REQ_USE_JOURNAL_DEV  = (1 << 3),
	
	E2_REQ_UNSUPPORTED_FLAGS = (-1) & ~(E2_REQ_DIR_TYPE_FIELD),
};

// Read-only Feature Flags
enum
{
	E2_ROF_SPARSE_SBLOCKS_AND_GDTS = (1 << 0),
	E2_ROF_FS_USES_64BIT_SIZES     = (1 << 1),
	E2_ROF_DIR_IN_BINARY_TREE      = (1 << 2),
	
	//well, we can claim we support 64-bit sizes by making files bigger than 64 MB unmodifiable. Has its reasons.
	E2_ROF_UNSUPPORTED_FLAGS = (-1) & ~(E2_ROF_FS_USES_64BIT_SIZES | E2_ROF_SPARSE_SBLOCKS_AND_GDTS),
};

// Ext2 Version 1.0 and earlier defaults
enum
{
	E2_DEF_FIRST_NON_RESERVED_INODE = 11,
	E2_DEF_INODE_STRUCTURE_SIZE     = 128,
};

// Inode Type
enum
{
	E2_INO_FIFO        = 0x1000,
	E2_INO_CHAR_DEV    = 0x2000,
	E2_INO_DIRECTORY   = 0x4000,
	E2_INO_BLOCK_DEV   = 0x6000,
	E2_INO_FILE        = 0x8000,
	E2_INO_SYM_LINK    = 0xA000,
	E2_INO_UNIX_SOCKET = 0xC000,
	E2_INO_TYPE_MASK   = 0xF000,
};

// Inode Permissions
enum
{
	E2_PERM_OTHER_EXEC  = 00001,
	E2_PERM_OTHER_WRITE = 00002,
	E2_PERM_OTHER_READ  = 00004,
	E2_PERM_GROUP_EXEC  = 00010,
	E2_PERM_GROUP_WRITE = 00020,
	E2_PERM_GROUP_READ  = 00040,
	E2_PERM_USER_EXEC   = 00100,
	E2_PERM_USER_WRITE  = 00200,
	E2_PERM_USER_READ   = 00400,
	E2_PERM_STICKY_BIT  = 01000,
	E2_PERM_SET_GID     = 02000,
	E2_PERM_SET_UID     = 04000,
	
	E2_PERM_ANYONE_WRITE = E2_PERM_OTHER_WRITE | E2_PERM_GROUP_WRITE | E2_PERM_USER_WRITE,
	E2_PERM_ANYONE_READ  = E2_PERM_OTHER_READ  | E2_PERM_GROUP_READ  | E2_PERM_USER_READ,
	E2_PERM_ANYONE_EXEC  = E2_PERM_OTHER_EXEC  | E2_PERM_GROUP_EXEC  | E2_PERM_USER_EXEC,
};

// Reserved Inodes
enum
{
	E2_RINO_BAD = 1, // This inode is made entirely of bad blocks, so they can't be used by other files.
	E2_RINO_ROOT,    // This inode represents the root directory.
	E2_RINO_ACL_IDX, // ACL index inode
	E2_RINO_ACL_DATA,// ACL data inode
	E2_RINO_BOOTLDR, // Boot loader inode
	E2_RINO_UNDELETE,// Undelete directory inode
};

// Inode Flags
enum
{
	E2_INO_FLAG_SYNCH_UPDATE          = 0x00000008, // new data is written instantly to disk
	E2_INO_FLAG_IMMUTABLE             = 0x00000010, // content may not be changed
	E2_INO_FLAG_APPEND_ONLY           = 0x00000020,
	E2_INO_FLAG_NO_INCLUDE_IN_DUMP    = 0x00000040, // don't include in 'dump' command, whatever that is
	E2_INO_FLAG_NO_UPDATE_LAST_ACCESS = 0x00000080,
	E2_INO_FLAG_HASH_INDEXED_DIR      = 0x00010000,
	E2_INO_FLAG_AFS_DIRECTORY         = 0x00020000,
	E2_INO_FLAG_JOURNAL_FILE_DATA     = 0x00040000,
};

// Directory Entry Type Indicators
enum
{
	E2_DETI_UNKNOWN,
	E2_DETI_REG_FILE,
	E2_DETI_DIRECTORY,
	E2_DETI_CHAR_DEV,
	E2_DETI_BLOCK_DEV,
	E2_DETI_FIFO,
	E2_DETI_SOCKET,
	E2_DETI_SYMLINK,
};

// Ext2 Major revision level
enum
{
	E2_GOOD_OLD_REV,
	E2_DYNAMIC_REV,
};



// OS specifics

// Binary search tree node.
typedef struct Ext2InodeCacheUnit
{
	uint32_t m_inodeNumber;
	struct Ext2InodeCacheUnit *pLeft, *pRight, *pParent;
	bool m_bPermanent; // if false, this can get deleted if its reference count (will add soon) is zero
	
	FileNode  m_node;
	Ext2Inode m_inode;
	
	void*    m_pBlockBuffer;
	uint32_t m_nLastBlockRead;
}
Ext2InodeCacheUnit;

typedef struct Ext2FileSystem
{
	bool     m_bMounted;
	DriveID  m_driveID;
	uint32_t m_lbaStart;
	uint32_t m_sectorCount;
	Ext2SuperBlock m_superBlock; // super block cache
	bool     m_bIsReadOnly;
	
	// cache
	uint32_t m_firstNonReservedInode;
	uint32_t m_inodesPerGroup;
	uint32_t m_blocksPerGroup;
	uint32_t m_fragmentSize;
	uint32_t m_blockSize;
	uint32_t m_inodeSize;
	uint32_t m_log2BlockSize;
	uint32_t m_sectorsPerBlock;
	uint32_t m_blockGroupCount;
	uint8_t* m_pBlockBitmapPtr; // Stores all the block bitmaps.
	uint8_t* m_pInodeBitmapPtr; // Stores all the inode bitmaps.
	uint32_t m_blocksPerBlockBitmap;
	uint32_t m_blocksPerInodeBitmap;
	
	Ext2BlockGroupDescriptor *m_pBlockGroups;
	Ext2InodeCacheUnit       *m_pInodeCacheRoot;
	
	uint8_t *m_pBlockBuffer;
}
Ext2FileSystem;

// Not actually related to Ext2, but we need it.
void FsRootCreateFileAtRoot(const char *pFileName, void *pContents, size_t sz);

// Adds an inode to the inode cache.
Ext2InodeCacheUnit *Ext2AddInodeToCache(Ext2FileSystem *pFS, uint32_t inodeNo, Ext2Inode *pInode, const char *pName);

// Dumps the inode cache tree of a file system.
void Ext2DumpInodeCacheTree(Ext2FileSystem *pFS);

// Look up an inode in the inode cache.
Ext2InodeCacheUnit *Ext2LookUpInodeCacheUnit(Ext2FileSystem *pFS, uint32_t inodeNo);

// Delete the inode cache tree.
void Ext2DeleteInodeCacheTree(Ext2FileSystem *pFS);

// Read an inode and add it to the inode cache. (or if it's in the inode cache, retrieve it from there or refresh it.)
Ext2InodeCacheUnit *Ext2ReadInode(Ext2FileSystem *pFS, uint32_t inodeNo, const char *pName, bool bForceReRead);

// Get the inode block number at an offset in block_size units.
uint32_t Ext2GetInodeBlock(Ext2Inode *pInode, Ext2FileSystem *pFS, uint32_t offset);

// Read a part of the inode's data.
void Ext2ReadFileSegment(Ext2FileSystem *pFS, Ext2InodeCacheUnit *pInode, uint32_t offset, uint32_t size, void *pMemOut);

// Write a part of the inode's data.
void Ext2WriteFileSegment(Ext2FileSystem *pFS, Ext2InodeCacheUnit *pInode, uint32_t offset, uint32_t size, const void *pMemIn);

// Grow an inode by 'byHowMuch' bytes.
void Ext2InodeExpand(Ext2FileSystem *pFS, Ext2InodeCacheUnit *pCacheUnit, uint32_t byHowMuch);

// Shrink an inode by 'byHowMuch' bytes.
void Ext2InodeShrink(Ext2FileSystem *pFS, Ext2InodeCacheUnit *pCacheUnit, uint32_t byHowMuch);

// Allocates a single block.
uint32_t Ext2AllocateBlock(Ext2FileSystem *pFS, uint32_t hint);

// Frees a single block.
void Ext2FreeBlock(Ext2FileSystem *pFS, uint32_t blockNo);

// Flush an inode.
void Ext2FlushInode(Ext2FileSystem* pFS, Ext2InodeCacheUnit* pUnit);

// Checks if a block spot is free right now.
bool Ext2CheckBlockFree(Ext2FileSystem *pFS, uint32_t block);

// Raw block based I/O functions.
DriveStatus Ext2ReadBlocks (Ext2FileSystem *pFS, uint32_t blockNo, uint32_t blockCount,       void *pMem);
DriveStatus Ext2WriteBlocks(Ext2FileSystem *pFS, uint32_t blockNo, uint32_t blockCount, const void *pMem);

// Flush the super block.
void Ext2FlushSuperBlock(Ext2FileSystem *pFS);

// Get a block at a specific offset from an inode.
uint32_t Ext2GetInodeBlock(Ext2Inode* pInode, Ext2FileSystem* pFS, uint32_t offset);

// Adds a directory entry pointing towards a certain inode and increases that inode's link count.
void Ext2AddDirectoryEntry(Ext2FileSystem *pFS, Ext2InodeCacheUnit* pUnit, const char* pName, uint32_t inodeNo, uint8_t typeIndicator);

#endif//_EXT2_H