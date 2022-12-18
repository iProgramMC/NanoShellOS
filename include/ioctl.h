//  ***************************************************************
//  ioctl.h - Creation date: 18/12/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#ifndef _IOCTL_H
#define _IOCTL_H

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

#endif//_IOCTL_H
