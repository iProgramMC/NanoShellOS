// nanoshell/error_nums.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef __NANOSHELL_ERROR_NUMS_H
#define __NANOSHELL_ERROR_NUMS_H

// use with the negative prefix
enum
{
	ENOTHING,      // No Error
	EACCES,        // Permission denied
	EEXIST,        // File exists
	EINTR,         // Interrupted system call
	EINVAL,        // Invalid argument
	EIO,           // I/O error
	EISDIR,        // Is a directory
	ELOOP,         // Too many symbolic links
	EMFILE,        // Too many open files
	ENAMETOOLONG,  // File or path name too long
	ENFILE,        // Too many open files in system
	ENOENT,        // No such file or directory
	ENOSR,         // Out of stream resources
	ENOSPC,        // No space left on device
	ENOTDIR,       // Not a directory
	ENXIO,         // No such device or address
	EOVERFLOW,     // Value too large for defined data type
	EROFS,         // Read only file system
	EAGAIN,        // No more processes
	ENOMEM,        // Not enough memory
	ETXTBUSY,      // Text file busy
	EBADF,         // Bad file descriptor
	ESPIPE,        // Illegal seek
	EIEIO,         // Computer bought the farm
	ENOTSUP,       // Operation not supported
	EXDEV,         // Cross device operation not supported
	EBUSY,         // Resource is busy
	ENOTEMPTY,     // Directory is not empty
	ENOTTY,        // Invalid input/output control request
	ECOUNT,
};


#endif//__NANOSHELL_ERROR_NUMS_H
