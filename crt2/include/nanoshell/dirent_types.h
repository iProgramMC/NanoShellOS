// nanoshell/dirent_types.h
// Copyright (C) 2023 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _DIRENT_TYPES_H
#define _DIRENT_TYPES_H

// matches the FILE_TYPE enum (unistd_types.h)
#define DT_UNKNOWN (0)
#define DT_REG     (1)
#define DT_CHR     (2)
#define DT_BLK     (3)
#define DT_FIFO    (4)
#define DT_LNK     (5)
#define DT_DIR     (8)
#define DT_SOCK    (32) // fantasy value. Will never be set unless we implement unix sockets.

typedef struct DirEntS
{
	char     m_name[128]; //+nullterm, so 127 concrete chars
	uint32_t m_inode;     //device specific
	uint32_t m_type;
}
DirEnt;

struct dirent
{
	ino_t    d_ino;       // Inode number
	off_t    d_off;       // Offset in the file (i.e. the value of telldir() for this dirent)
	uint16_t d_reclen;    // Record length
	uint8_t  d_type;      // Optional type of file, may be DT_UNKNOWN if not supported.
	char     d_name[128]; // Null terminated file name. The size matches that of the DirEnt structure's
};

typedef struct dirent dirent;

typedef struct
{
	int m_DirHandle;
	DirEnt m_NDirEnt; // nanoshell dirent
	dirent m_PDirEnt; // posix dirent
}
DIR; // pointer to a DIR returned by opendir

#endif//_DIRENT_TYPES_H