#ifndef _FS_H
#define _FS_H

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

enum
{
	ENOTHING,
	EACCES,
	EEXIST,
	EINTR,
	EINVAL,
	EIO,
	EISDIR,
	ELOOP,
	EMFILE,
	ENAMETOOLONG,
	ENFILE,
	ENOENT,
	ENOSR,
	ENOSPC,
	ENOTDIR,
	ENXIO,
	EOVERFLOW,
	EROFS,
	EAGAIN,
	ENOMEM,
	ETXTBUSY,
	EBADF,
	ESPIPE,
};

// Opens a file and returns its descriptor.
int FiOpenD (const char* pFileName, int oflag, const char* srcFile, int srcLine);
#define FiOpen(pFileName, oflag)  FiOpenD(pFileName,oflag,__FILE__,__LINE__)

// Closes a file and frees its descriptor for use.
int FiClose (int fd);

// Reads from a file.
size_t FiRead (int fd, void *pBuf, int nBytes);

// Writes to a file.
size_t FiWrite (int fd, void *pBuf, int nBytes);

// Returns the current stream position of a file.
int FiTell (int fd);

// Returns the current size of a file.
int FiTellSize (int fd);

// Seeks around a file, if it's seekable.
int FiSeek (int fd, int offset, int whence);

#endif//_FS_H