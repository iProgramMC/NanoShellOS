// stdio.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _STDIO__H
#define _STDIO__H

#include <nanoshell/stdio_types.h>

// POSIX file access functions
int     open  (const char* path, int oflag);
int     close (int fd);
size_t  read  (int fd,       void* buf,    unsigned int nbyte);
size_t  write (int fd, const void* buf,    unsigned int nbyte);
int     lseek (int fd,       int   offset,          int whence);
int     tellf (int fd);
int     tellsz(int fd);

// Standard C file access functions

FILE*  fopen (const char* file, const char* mode);
int    fclose(FILE* file);
size_t fread (      void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int    fseek (FILE* file, int offset, int whence);
int    ftell (FILE* file);
int    fflush(FILE* file);
int    fputs (const char* s, FILE* stream);
int    fputc (int c, FILE* stream);
int    putc  (int c, FILE* stream);
int    feof  (FILE* file);
int    getc  (FILE* file);
void   rewind(FILE* file);
int    vfprintf (FILE* file, const char* fmt, va_list list);
int    fprintf  (FILE* file, const char* fmt, ...);

// Operations on files
int remove(const char* filename);
int rename(const char* oldname, const char* newname);
int renameat(int olddirdd, const char* oldname, int newdirdd, const char* newname);

// External definitions to stdin, stdout and stderr
extern FILE *stdin, *stdout, *stderr;

// NanoShell specifics that have yet to have POSIX names assigned to them. (TODO)
int     FiOpenDir  (const char* pFileName);
int     FiCloseDir (int dd);
DirEnt* FiReadDir  (int dd);
int     FiSeekDir  (int dd,          int loc);
int     FiRewindDir(int dd);
int     FiTellDir  (int dd);
int     FiStatAt   (int dd,         const char*pfn,  StatResult* pres);
int     FiStat     (const char*pfn, StatResult* pres);
int     FiChDir    (const char*pfn);
const char* FiGetCwd();
const char* ErrNoStr(int errno);

// Formatted input/output
int vfprintf(FILE* file, const char* fmt, va_list list);
int fprintf (FILE* file, const char* fmt, ...);
int vsnprintf(char* OutBuffer, size_t BufferSize, const char* FormatType, va_list list);
int vsprintf (char* OutBuffer, const char* FormatType, va_list list);
int snprintf (char* OutBuffer, size_t BufferSize, const char* fmt, ...);
int sprintf  (char* OutBuffer, const char* FormatType, ...);
void LogMsg    (const char* Format, ...);
void LogMsgNoCr(const char* Format, ...);
int  printf    (const char* Format, ...);

// Character input/output
int fputs(const char* s, FILE * stream);
int fputc(int c, FILE * stream);
int putc(int c, FILE* stream);
int puts(const char * s);
int putchar(int c);

#endif//_STDIO__H