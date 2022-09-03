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
#define FIMAX 64

static int g_OpenedFileDes[FIMAX];
static int g_OpenedDirDes[FIMAX];

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
	return fd;
}

int close(int fd)
{
	if (!_I_IsFileOpenedHere(fd))
		return SetErrorNumber(-EBADF);
	
	int rv = _I_FiClose (fd);
	
	// if closing was successful:
	if (rv >= 0)
	{
		for (int i = 0; i < FIMAX; i++)
		{
			if (g_OpenedFileDes[i] == fd)
				g_OpenedFileDes[i]  = -1;
		}
	}
	
	return rv;
}

size_t read(int filedes, void* buf, unsigned int nbyte)
{
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	size_t result = _I_FiRead(filedes, buf, nbyte);
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
	return result;
}

size_t write(int filedes, const void* buf, unsigned int nbyte)
{
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	size_t result = _I_FiWrite(filedes, (void*)buf, nbyte);
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
	return result;
}

int lseek(int filedes, int offset, int whence)
{
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiSeek(filedes, offset, whence);
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
	return result;
}

int tellf(int filedes)
{
	if (!_I_IsFileOpenedHere(filedes))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiTell(filedes);
	
	if ((int)result < 0)
	{
		SetErrorNumber((int)result);
	}
	
	return result;
}

int tellsz(int filedes)
{
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
	{
		SetErrorNumber(fd);
	}
	else
	{
		g_OpenedDirDes[spot] = fd;
	}
	
	return fd;
}

int FiCloseDir(int dd)
{
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

DirEnt* FiReadDir(int dd)
{
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

int FiSeekDir(int dd, int loc)
{
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiSeekDir(dd, loc);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiRewindDir(int dd)
{
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiRewindDir(dd);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiTellDir(int dd)
{
	if (!_I_IsDirectoryOpenedHere(dd))
		return SetErrorNumber(-EBADF);
	
	int result = _I_FiTellDir(dd);
	if (result < 0)
		SetErrorNumber(result);
	
	return result;
}

int FiStatAt(int dd, const char *pfn, StatResult* pres)
{
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
	if (file <= stderr)
		return 0;
	
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
	if (stream <= stderr)
		return 0;
	
	return read(stream->fd, ptr, size * nmemb);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t ultimate_size = size*nmemb;
	
	if (stream <= stderr)
	{
		for (size_t i = 0; i < ultimate_size; i++)
			LogMsgNoCr("%c", ((const char*)ptr)[i]);
	}
	
	return write(stream->fd, ptr, ultimate_size);
}

int fseek(FILE* file, int offset, int whence)
{
	if (file <= stderr)
		return 0;
	
	return lseek(file->fd, offset, whence);
}

int ftell(FILE* file)
{
	if (file <= stderr)
		return 0;
	
	return tellf(file->fd);
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

// called on start
void _I_Setup()
{
	for (int i = 0; i < FIMAX; i++)
		g_OpenedFileDes[i] = -1;
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
	int chr = getc(f);
	if (chr == EOF)
		return EOF;
	
	//seek back
	fseek(f, ftell(f) - 1, SEEK_SET);
	
	return 0;
}
