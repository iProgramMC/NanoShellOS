// sys/mman.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __SYS__STAT_H
#define __SYS__STAT_H

#include <nanoshell/stat_types.h>

#define PERM_READ  (1)
#define PERM_WRITE (2)
#define PERM_EXEC  (4)

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

// loosely matches the FILE_TYPE_* struct:
#define S_IFMT  0170000
#define S_IFREG 0010000
#define S_IFCHR 0020000
#define S_IFBLK 0030000
#define S_IFIFO 0040000
#define S_IFLNK 0050000
#define S_IFDIR 0100000

// file mode bits:
#define S_IRWXU  00700
#define S_IRWXG  00070
#define S_IRWXO  00007
#define S_IXUSR  00400
#define S_IWUSR  00200
#define S_IRUSR  00100
#define S_IXGRP  00040
#define S_IWGRP  00020
#define S_IRGRP  00010
#define S_IXOTH  00004
#define S_IWOTH  00002
#define S_IROTH  00001
#define S_ISVTX  04000
#define S_ISUID  02000
#define S_ISGID  01000

#define S_ISBLK(m) ((m & S_IFMT) == S_IFBLK)
#define S_ISCHR(m) ((m & S_IFMT) == S_IFCHR)
#define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
#define S_ISREG(m) ((m & S_IFMT) == S_IFREG)
#define S_ISLNK(m) ((m & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m) ((m & S_IFMT) == S_IFIFO)

typedef struct stat
{
	dev_t  st_dev;
	ino_t  st_ino;
	mode_t st_mode;
	uid_t  st_uid;
	gid_t  st_gid;
	dev_t  st_rdev;
	off_t  st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	blksize_t st_blksize;
	blkcnt_t  st_blocks;
}
stat;

int FiStatAt   (int dd,         const char*pfn,  StatResult* pres);
int FiStat     (const char*pfn, StatResult* pres);

#endif//__SYS__STAT_H