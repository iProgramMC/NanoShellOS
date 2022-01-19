/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

    Userspace syscalls enum header file
******************************************/
#ifndef _USERSPACE_SYSCALLS_H
#define _USERSPACE_SYSCALLS_H

enum {
	LOGMSG = 1,
	MALLOC = 2,
	FREE   = 3,
	DUMPMEM= 4,
};

#endif//_USERSPACE_SYSCALLS_H