//  ***************************************************************
//  a_file.c - Creation date: 01/09/2022
//  -------------------------------------------------------------
//  NanoShell C Runtime Library
//  Copyright (C) 2022 iProgramInCpp - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include "crtlib.h"
#include "crtinternal.h"

// Max files open at once -- increase if necessary
#define FD_STDIO 65535
#define FIMAX    64
#define EOT      4      // end of transmission, aka Ctrl-D

static int g_OpenedFileDes[FIMAX];

static int FileSpotToFileHandle(int spot)
{
	if (spot < 0 || spot >= FIMAX) return -1;
	return g_OpenedFileDes[spot];
}

// Check if a file is opened here in this process
bool _I_IsFileOpenedHere(int fd)
{
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedFileDes[i] == fd)
			return true;
	}
	return false;
}


int open(const char* path, int oflag)
{
	int spot = -1;
	
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedFileDes[i] < 0)
		{
			spot = i; break;
		}
	}
	
	if (spot == -1)
	{
		SetErrorNumber(-EMFILE);
		return -1;
	}
	
	// FiOpenDebug now supports custom paths...
	int fd = _I_FiOpenDebug(path, oflag, "usertask", 1);
	
	if (fd < 0)
	{
		SetErrorNumber(fd);
		return -1;
	}
	
	g_OpenedFileDes[spot] = fd;
	return spot;
}

int close(int spot)
{
	int fd = FileSpotToFileHandle(spot);
	if (fd < 0)
	{
		SetErrorNumber(-EBADF);
		return -1;
	}
	
	if (!_I_IsFileOpenedHere(fd))
	{
		SetErrorNumber(-EBADF);
		return -1;
	}
	
	int rv = _I_FiClose (fd);
	
	// if closing was successful:
	if (rv >= 0)
	{
		g_OpenedFileDes[spot] = -1;
	}
	
	return rv;
}

// honestly, this needs to be redone too
int read_stdio(void* buf, unsigned int nbyte)
{
	char* bufchar = buf;
	for (unsigned i = 0; i < nbyte; i++)
	{
		char c = _I_ReadChar();
		*(bufchar++) = c;
		
		// If we got an 'end of transmission', instantly return
		if (c == EOT)
			return (int)i;
	}
	
	return nbyte;
}

int write_stdio(const void* buf, unsigned int nbyte)
{
	const char* bufchar = buf;
	char b[2];
	b[1] = 0;
	for (unsigned i = 0; i < nbyte; i++)
	{
		b[0] = *(bufchar++);
		_I_PutString(b);
	}
	
	return nbyte;
}

int read(int spot, void* buf, unsigned int nbyte)
{
	int filedes = FileSpotToFileHandle(spot);
	if (filedes < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	if (filedes == FD_STDIO)
	{
		return read_stdio(buf, nbyte);
	}
	
	int result = _I_FiRead(filedes, buf, nbyte);
	
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int write(int spot, const void* buf, unsigned int nbyte)
{
	int filedes = FileSpotToFileHandle(spot);
	if (filedes < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	if (filedes == FD_STDIO)
	{
		return write_stdio(buf, nbyte);
	}
	
	int result = _I_FiWrite(filedes, (void*)buf, nbyte);
	
	if (result < 0)
		return SetErrorNumber((int)result);
	
	return result;
}

int lseek(int spot, int offset, int whence)
{
	int filedes = FileSpotToFileHandle(spot);
	if (filedes < 0)
		return SetErrorNumber(-EBADF);
	
	if (filedes == FD_STDIO) return SetErrorNumber(-ESPIPE);
	
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiSeek(filedes, offset, whence);
	
	if (result < 0)
		return SetErrorNumber((int)result);
	
	return result;
}

int tellf(int spot)
{
	int filedes = FileSpotToFileHandle(spot);
	if (filedes < 0)
		return SetErrorNumber(-EBADF);
	
	if (filedes == FD_STDIO) return SetErrorNumber(-ESPIPE);
	
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiTell(filedes);
	
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int tellsz(int spot)
{
	int filedes = FileSpotToFileHandle(spot);
	if (filedes < 0)
		return SetErrorNumber(-EBADF);
	
	if (filedes == FD_STDIO) return SetErrorNumber(-ESPIPE);
	
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiTellSize(filedes);
	
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int ioctl(int spot, unsigned long request, void * argp)
{
	int filedes = FileSpotToFileHandle(spot);
	if (filedes < 0)
		return SetErrorNumber(-EBADF);
	
	if (filedes == FD_STDIO)
	{
		// process the IO request directly TODO
		return -ENOTTY;
	}
	
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiIoControl(filedes, request, argp);
	
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiOpenDir(const char* pFileName)
{
	int spot = -1;
	
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedFileDes[i] < 0)
		{
			spot = i; break;
		}
	}
	
	if (spot == -1)
	{
		SetErrorNumber(-EMFILE);
		return -EMFILE;
	}
	
	int fd = _I_FiOpenDirD(pFileName, "[Process]", 1);
	
	if (fd < 0)
		return SetErrorNumber(fd);
	
	g_OpenedFileDes[spot] = fd;
	
	return spot;
}

int FiCloseDir(int spot)
{
	int dd = FileSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int closeRes = _I_FiCloseDir(dd);
	if (closeRes >= 0)
	{
		for (int i = 0; i < FIMAX; i++)
		{
			if (g_OpenedFileDes[i] == dd)
				g_OpenedFileDes[i] = -1;
		}
	}
	else
		return SetErrorNumber(closeRes);
	
	return closeRes;
}

int FiReadDir(DirEnt* space, int spot)
{
	int dd = FileSpotToFileHandle(spot);
	if (dd < 0)
	{
		SetErrorNumber(-EBADF);
		return -1;
	}
	
	if (!_I_IsFileOpenedHere(dd))
	{
		SetErrorNumber(-EBADF);
		return -1;
	}
	
	int result = _I_FiReadDir(space, dd);
	
	if (result >= 0)
		return result;
	
	SetErrorNumber(result);
	return -1;
}

int FiSeekDir(int spot, int loc)
{
	int dd = FileSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiSeekDir(dd, loc);
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiRewindDir(int spot)
{
	int dd = FileSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiRewindDir(dd);
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiTellDir(int spot)
{
	int dd = FileSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiTellDir(dd);
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiStatAt(int spot, const char *pfn, StatResult* pres)
{
	int dd = FileSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiStatAt(dd, pfn, pres);
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiStat(const char *pfn, StatResult* pres)
{
	int result = _I_FiStat(pfn, pres);
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiLinkStat(const char *pfn, StatResult* pres)
{
	int result = _I_FiLinkStat(pfn, pres);
	if (result < 0)
		return SetErrorNumber(result);
	
	return result;
}

int FiChDir(const char *pfn)
{
	int result = _I_FiChDir(pfn);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiChangeDir(const char *pfn)
{
	return FiChDir(pfn);
}

int chdir(const char *pfn)
{
	return FiChDir(pfn);
}

// C Standard I/O
FILE* fdopen (int fd, UNUSED const char* type)
{
	if (fd < 0)
		return NULL;
	
	FILE* pFile = calloc(1, sizeof(FILE));
	if (!pFile)
	{
		close(fd);
		return NULL;
	}
	pFile->fd = fd;
	return pFile;
}

FILE* fopen (const char* file, const char* mode)
{
	const char* mode1 = mode;
	int flags = O_CREAT;
	
	while (*mode)
	{
		switch (*mode)
		{
		case'r':case'R':flags |= O_RDONLY;break;
		case'w':case'W':flags |= O_WRONLY;break;
		case'a':case'A':flags |= O_APPEND;break;
		case'+':        flags &=~O_CREAT; break;
		}
		mode++;
	}
	
	return fdopen(open(file, flags), mode1);
}

int fclose(FILE* file)
{
	if (file->fd == FD_STDIO) return SetErrorNumber(-ESPIPE);
	
	if (file)
	{
		int op = close(file->fd);
		
		if (op < 0)
		{
			SetErrorNumber(op);
			return op;
		}
		
		free (file);
		return op;
	}
	
	SetErrorNumber(-EBADF);
	return -EBADF;
}

size_t fread (void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	if (size == 0)
		return SetErrorNumber(-EBADF);
	
	SetErrorNumber(0);
	
	size_t nbyte = size * nmemb;
	size_t rd = read(stream->fd, ptr, nbyte);
	
	if (rd < nbyte)
	{
		// must have reached end of file
		stream->eof = true;
	}
	
	if (GetErrorNumber())
	{
		stream->error = true;
	}
	
	return rd / size;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	if (size == 0)
		return SetErrorNumber(-EBADF);
	
	SetErrorNumber(0);
	
	size_t nbyte = size * nmemb;
	size_t wr = write(stream->fd, ptr, nbyte);
	if (wr < nbyte)
	{
		stream->eof = true;
	}
	
	if (GetErrorNumber())
	{
		stream->error = true;
	}
	
	return wr / size;
}

int fseek(FILE* file, int offset, int whence)
{
	SetErrorNumber(0);
	int rv = lseek(file->fd, offset, whence);
	
	if (GetErrorNumber())
	{
		file->error = true;
	}
	
	return rv;
}

int ftell(FILE* file)
{
	SetErrorNumber(0);
	int rv = tellf(file->fd);
	
	if (GetErrorNumber())
	{
		file->error = true;
	}
	
	return rv;
}

void rewind(FILE* stream)
{
	fseek(stream, 0, SEEK_SET);
}

// called on exit
void _I_CloseOpenFiles()
{
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedFileDes[i] >= 0)
		{
			_I_FiClose(g_OpenedFileDes[i]);
			g_OpenedFileDes[i] = -1;
		}
	}
}

FILE *stdin, *stdout, *stderr;

// called on start
void _I_Setup()
{
	for (int i = 0; i < FIMAX; i++)
		g_OpenedFileDes[i] = g_OpenedFileDes[i] = -1;
	
	g_OpenedFileDes[0] = FD_STDIO;
	g_OpenedFileDes[1] = FD_STDIO;
	g_OpenedFileDes[2] = FD_STDIO;
	
	//well, they're going to be the same file, which is fine.
	//Points to file description 0, which is handle FD_STDIO, should be ok.
	stdin = stdout = stderr = calloc(1, sizeof(FILE));
	stdin->fd = 0;
}

int unlink (const char* filename)
{
	return _I_FiUnlinkFile(filename);
}

int getc (FILE* pFile)
{
	if (pFile->ungetc_buf_sz > 0)
	{
		return pFile->ungetc_buf[--pFile->ungetc_buf_sz];
	}
	
	char chr = 0;
	fread(&chr, 1, 1, pFile);
	if (pFile->eof)
		return EOF;
	
	return chr;
}

int fgetc(FILE* stream)
{
	return getc(stream);
}

int ungetc(int c, FILE * stream)
{
	if (stream->ungetc_buf_sz >= (int) sizeof stream->ungetc_buf)
		return EOF;
	
	stream->ungetc_buf[stream->ungetc_buf_sz++] = c;
	return 0;
}

int feof(FILE* f)
{
	return f->eof;
}

int ferror(FILE* f)
{
	return f->error;
}

void clearerr(FILE* f)
{
	f->error = 0;
}

// empty for now. There's no actual flushing to be done.
int fflush(FILE* file)
{
	(void)file;
	return 0;
}

int rename(const char* old, const char* new)
{
	return _I_FiRename(old, new);
}

int renameat(int olddirspot, const char* old, int newdirspot, const char* new)
{
	//TODO
	(void) old; (void) new;
	(void) olddirspot; (void) newdirspot;
	return -ENOTSUP;
}

int mkdir(const char * path, UNUSED int mode)
{
	return _I_FiMakeDir(path);
}

int rmdir(const char * path)
{
	return _I_FiRemoveDir(path);
}

int FiCreatePipe(const char * friendlyName, int pipefd[2], int flags)
{
	// look for two free spots.
	int j = 0;
	for (int i = 0; i < FIMAX && j < 2; i++)
	{
		if (g_OpenedFileDes[i] == -1)
			pipefd[j++] = i;
	}
	
	// if j didn't reach 2..
	if (j < 2)
	{
		//we have too many files open
		return -EMFILE;
	}
	
	int kshandles[2];
	int result = _I_FiCreatePipe(friendlyName, kshandles, flags);
	
	if (result < 0) return result; // actually don't do anything
	
	// assign the two pipefd's to the kernel handles
	g_OpenedFileDes[pipefd[0]] = kshandles[0];
	g_OpenedFileDes[pipefd[1]] = kshandles[1];
	
	return 0;
}

int pipe2(int pipefd[2], int flags)
{
	return FiCreatePipe("pipe", pipefd, flags);
}

int pipe(int pipefd[2])
{
	return pipe2(pipefd, 0);
}

int remove (const char* filename)
{
	StatResult res;
	int stat = FiStat(filename, &res);
	if (stat < 0) return stat;
	
	if (res.m_type == FILE_TYPE_DIRECTORY)
	{
		return _I_FiRemoveDir(filename);
	}
	else
	{
		return _I_FiUnlinkFile(filename);
	}
}

void* mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void* pMem = MAP_FAILED;
	
	int ec = MemoryMap(addr, length, prot, flags, fd, offset, &pMem);
	
	if (ec < 0)
	{
		SetErrorNumber(ec);
		return MAP_FAILED;
	}
	
	return pMem;
}

int munmap(void * addr, size_t sz)
{
	int ec = MemoryUnmap(addr, sz);
	if (ec < 0)
		SetErrorNumber(ec);
	return ec;
}

char* getcwd(char* buf, size_t sz)
{
	strncpy(buf, FiGetCwd(), sz);
	buf[sz - 1] = 0;
	return buf;
}

// Standard C wrappers for directory ops
DIR* opendir(const char* dirname)
{
	int dd = FiOpenDir(dirname);
	if (dd < 0)
		return NULL; // errno is already set
	
	DIR* dir = malloc(sizeof(DIR));
	dir->m_DirHandle = dd;
	return dir;
}

int closedir(DIR* dirp)
{
	int result = FiCloseDir(dirp->m_DirHandle);
	if (result < 0)
		return -1;
	
	free(dirp);
	return 0;
}

void rewinddir(DIR* dirp)
{
	// seek to the beginning, ie. 0
	FiSeekDir(dirp->m_DirHandle, 0);
}

int telldir(DIR* dirp)
{
	return FiTellDir(dirp->m_DirHandle);
}

void seekdir(DIR* dirp, int told)
{
	FiSeekDir(dirp->m_DirHandle, told);
}

struct dirent* readdir(DIR* dirp)
{
	// keep an offset that we can fill in.
	int offset = FiTellDir(dirp->m_DirHandle);
	
	// if we couldn't grab the telldir result, just bail. FiTellDir set an errno anyways
	if (offset < 0)
		return NULL;
	
	// note: It doesn't matter if we skip some entries (. and ..), those will be
	// skipped again if we seekdir() to dirent->d_off and readdir() again.
	
	// perform the actual read
	int result = FiReadDir(&dirp->m_NDirEnt, dirp->m_DirHandle);
	
	// if we got nothing (i.e. either hit the end, or had an error),
	// return null. FiReadDir sets an errno if something happened.
	if (result != 0)
		return NULL;
	
	// fill out the entries here:
	DirEnt* pDirEnt = &dirp->m_NDirEnt;
	struct dirent* ptr = &dirp->m_PDirEnt; // posix dirent
	
	strcpy(ptr->d_name, pDirEnt->m_name);
	
	ptr->d_reclen = sizeof(*ptr);
	ptr->d_ino    = pDirEnt->m_inode;
	ptr->d_type   = pDirEnt->m_type;
	ptr->d_off    = offset;
	
	return ptr;
}

// stat stuff
int FiFDStat(int spot, StatResult* pres)
{
	int fd = FileSpotToFileHandle(spot);
	if (fd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(fd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiFDStat(fd, pres);
	if (result < 0)
		return SetErrorNumber(result);
	
	return 0;
}

int FiFDChangeDir(int spot)
{
	int fd = FileSpotToFileHandle(spot);
	if (fd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(fd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiFDChangeDir(fd);
	if (result < 0)
		return SetErrorNumber(result);
	
	return 0;
}

int FiFDChangeMode(int spot, int mode)
{
	int fd = FileSpotToFileHandle(spot);
	if (fd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(fd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiFDChangeMode(fd, mode);
	if (result < 0)
		return SetErrorNumber(result);
	
	return 0;
}

int FiFDChangeTime(int spot, int atime, int mtime)
{
	int fd = FileSpotToFileHandle(spot);
	if (fd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(fd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiFDChangeTime(fd, atime, mtime);
	if (result < 0)
		return SetErrorNumber(result);
	
	return 0;
}

int FiChangeMode(const char* path, int mode)
{
	int res = _I_FiChangeMode(path, mode);
	if (res < 0)
		return SetErrorNumber(res);
	
	return 0;
}

int FiChangeTime(const char* path, int atime, int mtime)
{
	int res = _I_FiChangeTime(path, atime, mtime);
	if (res < 0)
		return SetErrorNumber(res);
	
	return 0;
}

void StatResultToStructStat(StatResult* pIn, struct stat* pOut)
{
	pOut->st_dev     = 0;
	pOut->st_rdev    = 0;
	pOut->st_uid     = 0;
	pOut->st_gid     = 0;
	pOut->st_size    = (off_t) pIn->m_size;
	pOut->st_atime   = (time_t) pIn->m_modifyTime; // todo?
	pOut->st_mtime   = (time_t) pIn->m_modifyTime;
	pOut->st_ctime   = (time_t) pIn->m_createTime;
	pOut->st_blksize = (blksize_t) 512; // the kernel uses this.
	pOut->st_blocks  = (blkcnt_t)  pIn->m_blocks;
	pOut->st_ino     = (ino_t) pIn->m_inode;
	
	// mode
	
	// so that FILE_TYPE_SYMBOLIC_LINK matches with S_IFLNK for instance
	pOut->st_mode = (mode_t) pIn->m_type << 12;
	
	if (pIn->m_perms & PERM_READ)  pOut->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
	if (pIn->m_perms & PERM_WRITE) pOut->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	if (pIn->m_perms & PERM_EXEC)  pOut->st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;
}

int stat(const char* path, struct stat* buf)
{
	StatResult sr;
	memset(&sr, 0, sizeof(sr));
	
	int res = FiStat(path, &sr);
	if (res < 0)
		return res; // it already set an errno
	
	// convert our stat result into a struct stat
	StatResultToStructStat(&sr, buf);
	
	return 0; // success!
}

int lstat(const char* path, struct stat* buf)
{
	StatResult sr;
	memset(&sr, 0, sizeof(sr));
	
	int res = FiLinkStat(path, &sr);
	if (res < 0)
		return res; // it already set an errno
	
	// convert our stat result into a struct stat
	StatResultToStructStat(&sr, buf);
	
	return 0; // success!
}

int fstat(int spot, struct stat* buf)
{
	StatResult sr;
	memset(&sr, 0, sizeof(sr));
	
	int res = FiFDStat(spot, &sr);
	if (res < 0)
		return res;
	
	// convert our stat result into a struct stat
	StatResultToStructStat(&sr, buf);
	
	return 0;
}

int fchdir(int fd)
{
	return FiFDChangeDir(fd);
}

int PosixModeToNSMode(mode_t mode)
{
	int nmode = 0;
	
	if (mode & (S_IRUSR | S_IRGRP | S_IROTH))
		nmode |= PERM_READ;
	
	if (mode & (S_IWUSR | S_IWGRP | S_IWOTH))
		nmode |= PERM_WRITE;
	
	if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
		nmode |= PERM_EXEC;
	
	return nmode;
}

int chmod(const char* path, mode_t mode)
{
	// convert the 'mode_t' to a NanoShell type mode. Note that type information will be thrown away
	int nmode = PosixModeToNSMode(mode);
	
	int res = _I_FiChangeMode(path, nmode);
	if (res < 0)
		return SetErrorNumber(res);
	
	return 0;
}
