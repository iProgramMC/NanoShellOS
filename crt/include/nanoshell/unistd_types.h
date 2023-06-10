// nanoshell/unistd_types.h
// Copyright (C) 2022 iProgramInCpp
// The NanoShell Standard C Library
#ifndef _NANOSHELL_UNISTD_TYPES__H
#define _NANOSHELL_UNISTD_TYPES__H

#include <sys/types.h>

#define PATH_MAX (260)
#define PATH_SEP ('/')
#define PATH_THISDIR (".")
#define PATH_PARENTDIR ("..")

#define O_RDONLY   (1)
#define O_WRONLY   (2)
#define O_RDWR     (O_RDONLY | O_WRONLY)
#define O_APPEND   (4)
#define O_CREAT    (8)
#define O_NONBLOCK (16)
#define O_TRUNC    (32)
#define O_EXEC     (1024)

//lseek whences
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

enum
{
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FILE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_PIPE,
	FILE_TYPE_SYMBOLIC_LINK,
	FILE_TYPE_DIRECTORY  = 8,
	FILE_TYPE_MOUNTPOINT = 16, //to be OR'd into the other flags
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