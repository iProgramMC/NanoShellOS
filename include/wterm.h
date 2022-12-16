/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

  Console Window Host module header file
******************************************/
#ifndef _WTERM_H
#define _WTERM_H

#include <window.h>
#include <console.h>
#include <task.h>
#include <shell.h>

void TerminalHostTask(int arg);
int TerminalHostStart(int arg);

#endif//_WTERM_H