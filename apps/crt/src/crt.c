//
// crt.c
//
// Copyright (C) 2022 iProgramInCpp.
//
// The standard NanoShell library internal implementation
// -- Basic runtime and cleanup
//

#include "crtlib.h"
#include "crtinternal.h"

// Max memory allocations at once -- increase if necessary
#define MMMAX 1024
// Max files open at once -- increase if necessary
#define FIMAX 1024

// Memory management

#ifdef USE_MEMORY
static void*  g_AllocatedMemoryList[MMMAX];
static size_t g_AllocatedMemorySize[MMMAX];

void *malloc (size_t size)
{
	int fs = -1;
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i] == NULL)
		{
			fs = i;
			break;
		}
	}
	if (fs == -1) return NULL; //Too many!
	
	g_AllocatedMemoryList[fs] = _I_AllocateDebug (size, "usertask", 1);
	
	if (!g_AllocatedMemoryList[fs]) return NULL;//Can't allocate?
	
	g_AllocatedMemorySize[fs] = size;
	
	return g_AllocatedMemoryList[fs];
}

void free (void* ptr)
{
	_I_Free(ptr);
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i] == ptr)
		{
			g_AllocatedMemoryList[i]  = NULL;
			g_AllocatedMemorySize[i]  = 0;
		}
	}
}

void sleep (int ms)
{
	_I_TmSleep (ms);
}
#endif
void _I_FreeEverything()
{
#ifdef USE_MEMORY
	for (int i = 0; i < MMMAX; i++)
	{
		if (g_AllocatedMemoryList[i])
		{
			_I_Free(g_AllocatedMemoryList[i]);
			g_AllocatedMemoryList[i] = NULL;
		}
	}
#endif
}


// File management

static int g_OpenedFileDes[FIMAX];

#ifdef USE_FILE
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
		LogMsg("Can't open ! Too many files!");
		return -ETXTBUSY;//-ETXTBUSY
	}
	
	char abspath[1024];
	if (path[0] == '/')
		abspath[0] = 0;
	else
		strcpy (abspath, "/Fat0/");//TODO!
	
	strcat (abspath, path);
	
	int fd = _I_FiOpenDebug(abspath, oflag, "usertask", 1);
	if (fd < 0) return fd;
	
	g_OpenedFileDes[spot] = fd;
	return fd;
}
int close(int fd)
{
	int rv = _I_FiClose (fd);
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedFileDes[i] == fd)
			g_OpenedFileDes[i]  = -1;
	}
	return rv;
}
size_t read(int filedes, void* buf, unsigned int nbyte)
{
	return _I_FiRead(filedes, buf, nbyte);
}
size_t write(int filedes, const void* buf, unsigned int nbyte)
{
	return _I_FiWrite(filedes, (void*)buf, nbyte);
}
int lseek(int filedes, int offset, int whence)
{
	return _I_FiSeek(filedes, offset, whence);
}
int tellf(int filedes)
{
	return _I_FiTell(filedes);
}
int tellsz(int filedes)
{
	return _I_FiTellSize(filedes);
}

int FiOpenDir(const char* pFileName)
{
	return _I_FiOpenDirD(pFileName, "some process",2022);//TODO
}
int FiCloseDir(int dd)
{
	return _I_FiCloseDir(dd);
}
DirEnt* FiReadDir(int dd)
{
	return _I_FiReadDir(dd);
}
int FiSeekDir(int dd,int loc)
{
	return _I_FiSeekDir(dd, loc);
}
int FiRewindDir(int dd)
{
	return _I_FiRewindDir(dd);
}
int FiTellDir(int dd)
{
	return _I_FiTellDir(dd);
}
int FiStatAt(int dd,const char*pfn, StatResult* pres)
{
	return _I_FiStatAt(dd,pfn,pres);
}
int FiStat(const char*pfn, StatResult* pres)
{
	return _I_FiStat(pfn,pres);
}
const char* FiGetCwd()
{
	return _I_FiGetCwd();
}
int FiChDir(const char* pfn)
{
	return _I_FiChDir(pfn);
}
#endif

void _I_CloseOpenFiles()
{
#ifdef USE_FILE
	for (int i = 0; i < FIMAX; i++)
	{
		if (g_OpenedFileDes[i] >= 0)
		{
			_I_FiClose(g_OpenedFileDes[i]);
			g_OpenedFileDes[i] = -1;
		}
	}
#endif
}


// C Standard I/O

#ifdef USE_FILE
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
		free (file);
		return op;
	}
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

#endif


void _I_CloseOpenWindows()
{
	
}

void _I_Setup()
{
	for (int i = 0; i < FIMAX; i++)
		g_OpenedFileDes[i] = -1;
}