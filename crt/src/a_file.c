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
static int g_OpenedDirDes [FIMAX];

static int FileSpotToFileHandle(int spot)
{
	if (spot < 0 || spot >= FIMAX) return -1;
	return g_OpenedFileDes[spot];
}

static int DirSpotToFileHandle(int spot)
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

// Check if a directory is opened here in this process
bool _I_IsDirectoryOpenedHere(int dd)
{
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedDirDes[i] == dd)
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
		return -EMFILE;
	}
	
	// FiOpenDebug now supports custom paths...
	int fd = _I_FiOpenDebug(path, oflag, "usertask", 1);
	
	if (fd < 0)
	{
		SetErrorNumber(fd);
		return fd;
	}
	
	g_OpenedFileDes[spot] = fd;
	return spot;
}

int close(int spot)
{
	int fd = FileSpotToFileHandle(spot);
	if (fd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsFileOpenedHere(fd))
		return SetErrorNumber(-EBADF);
	
	int rv = _I_FiClose (fd);
	
	// if closing was successful:
	if (rv >= 0)
	{
		g_OpenedFileDes[spot] = -1;
	}
	
	return rv;
}

size_t read_stdio(void* buf, unsigned int nbyte)
{
	char* bufchar = buf;
	for (unsigned i = 0; i < nbyte; i++)
	{
		char c = _I_ReadChar();
		*(bufchar++) = c;
		
		// If we got an 'end of transmission', instantly return
		if (c == EOT)
			return i;
	}
	
	return nbyte;
}

size_t write_stdio(const void* buf, unsigned int nbyte)
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

size_t read(int spot, void* buf, unsigned int nbyte)
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
	
	size_t result = _I_FiRead(filedes, buf, nbyte);
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
	return result;
}

size_t write(int spot, const void* buf, unsigned int nbyte)
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
	
	size_t result = _I_FiWrite(filedes, (void*)buf, nbyte);
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
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
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
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
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
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
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
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
	
	g_OpenedDirDes[spot] = fd;
	
	return spot;
}

int FiCloseDir(int spot)
{
	int dd = DirSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int closeRes = _I_FiCloseDir(dd);
	if (closeRes >= 0)
	{
		for (int i = 0; i < FIMAX; i++)
		{
			if (g_OpenedDirDes[i] == dd)
				g_OpenedDirDes[i] = -1;
		}
	}
	
	return closeRes;
}

DirEnt* FiReadDir(int spot)
{
	int dd = DirSpotToFileHandle(spot);
	if (dd < 0)
	{
		SetErrorNumber(-EBADF);
		return NULL;
	}
	
	if (!_I_IsDirectoryOpenedHere(dd))
	{
		SetErrorNumber(-EBADF);
		return NULL;
	}
	
	DirEnt* pResult = _I_FiReadDir(dd);
	
	if (!pResult)
		SetErrorNumber(0); // end of stream, I guess?
	
	return pResult;
}

int FiSeekDir(int spot, int loc)
{
	int dd = DirSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiSeekDir(dd, loc);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiRewindDir(int spot)
{
	int dd = DirSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiRewindDir(dd);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiTellDir(int spot)
{
	int dd = DirSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiTellDir(dd);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiStatAt(int spot, const char *pfn, StatResult* pres)
{
	int dd = DirSpotToFileHandle(spot);
	if (dd < 0)
		return SetErrorNumber(-EBADF);
	
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiStatAt(dd, pfn, pres);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiStat(const char *pfn, StatResult* pres)
{
	int result = _I_FiStat(pfn, pres);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiChDir(const char *pfn)
{
	int result = _I_FiChDir(pfn);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

// C Standard I/O
FILE* fdopen (int fd, UNUSED const char* type)
{
	FILE* pFile = malloc(sizeof(FILE));
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
	return read(stream->fd, ptr, size * nmemb);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t ultimate_size = size*nmemb;
	
	return write(stream->fd, ptr, ultimate_size);
}

int fseek(FILE* file, int offset, int whence)
{
	return lseek(file->fd, offset, whence);
}

int ftell(FILE* file)
{
	return tellf(file->fd);
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
		g_OpenedFileDes[i] = g_OpenedDirDes[i] = -1;
	
	g_OpenedFileDes[0] = FD_STDIO;
	g_OpenedFileDes[1] = FD_STDIO;
	g_OpenedFileDes[2] = FD_STDIO;
	
	//well, they're going to be the same file, which is fine.
	//Points to file description 0, which is handle FD_STDIO, should be ok.
	stdin = stdout = stderr = malloc(sizeof(FILE));
	memset(stdin, 0, sizeof(FILE));
	stdin->fd = 0;
}

int remove (const char* filename)
{
	return _I_FiRemoveFile(filename);
}

int getc (FILE* pFile)
{
	char chr = 0;
	size_t sz = fread(&chr, 1, 1, pFile);
	if (sz == 0)
		return EOF;
	
	return chr;
}

int feof(FILE* f)
{
	//TODO: kernel side implementation
	int chr = EOF;
	
	if (read(f->fd, &chr, 1) == 0)
		return EOF;
	
	//seek back
	fseek(f, ftell(f) - 1, SEEK_SET);
	
	return 0;
}

// empty for now. There's no actual flushing to be done.
int fflush(FILE* file)
{
	(void)file;
	return 0;
}

int rename(const char* old, const char* new)
{
	//TODO
	(void) old; (void) new;
	return -ENOTSUP;
}

int renameat(int olddirspot, const char* old, int newdirspot, const char* new)
{
	//TODO
	(void) old; (void) new;
	(void) olddirspot; (void) newdirspot;
	return -ENOTSUP;
}

