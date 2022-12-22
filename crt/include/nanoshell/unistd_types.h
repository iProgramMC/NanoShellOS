// nanoshell/unistd_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _NANOSHELL_UNISTD_TYPES__H
#define _NANOSHELL_UNISTD_TYPES__H

#define PATH_MAX (260)
#define PATH_SEP ('/')
#define PATH_THISDIR (".")
#define PATH_PARENTDIR ("..")

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

// mmap() flags
#define PROT_NONE  (0 << 0)
#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2) //not applicable here

#define MAP_FAILED ((void*) -1) //not NULL

#define MAP_FILE      (0 << 0) //retroactive, TODO
#define MAP_SHARED    (1 << 0) //means changes in the mmapped region will be written back to the file on unmap/close
#define MAP_PRIVATE   (1 << 1) //means changes won't be committed back to the source file
#define MAP_FIXED     (1 << 4) //fixed memory mapping means that we really want it at 'addr'.
#define MAP_ANONYMOUS (1 << 5) //anonymous mapping, means that there's no file backing this mapping :)
#define MAP_ANON      (1 << 5) //synonymous with "MAP_ANONYMOUS"
#define MAP_NORESERVE (0 << 0) //don't reserve swap space, irrelevent here

#define MAP_DONTREPLACE (1 << 30) //don't clobber preexisting fixed mappings there. Used with MAP_FIXED to create...
#define MAP_FIXED_NOREPLACE (MAP_DONTREPLACE | MAP_FIXED)

#define O_RDONLY (1)
#define O_WRONLY (2)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (4)
#define O_CREAT  (8)
#define O_EXEC   (1024)

//lseek whences
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct DirEntS
{
	char     m_name[128]; //+nullterm, so 127 concrete chars
	uint32_t m_inode;     //device specific
	uint32_t m_type;
}
DirEnt;

typedef struct
{
	uint32_t m_type;
	uint32_t m_size;
	uint32_t m_blocks;
	uint32_t m_inode;
	
	uint32_t m_perms;
	uint32_t m_modifyTime;
	uint32_t m_createTime;
}
StatResult;

enum
{
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FILE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_PIPE,
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16 //to be OR'd into the other flags
};

enum
{
	IOCTL_NO_OP,                     // This can be used to test if the device actually supports I/O control. Does nothing.
	
	// Define the starting places of I/O controls for each device.
	IOCTL_TERMINAL_START = 10000,
	IOCTL_SOUNDDEV_START = 20000,
	//...
	
	IOCTL_TERMINAL_GET_SIZE = IOCTL_TERMINAL_START, // argp points to a Point structure, which will get filled in.
	IOCTL_TERMINAL_SET_ECHO_INPUT,                  // enable or disable echoing input in CoGetString()
	
	IOCTL_SOUNDDEV_SET_SAMPLE_RATE = IOCTL_SOUNDDEV_START,  // Set the sample rate of an audio playback device.
};

#endif//_NANOSHELL_UNISTD_TYPES__H