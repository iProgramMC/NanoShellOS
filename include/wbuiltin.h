/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

 Window Built-in Application module h file
******************************************/
#ifndef _WBUILTIN_H
#define _WBUILTIN_H

#include <window.h>
#include <icon.h>
#include <print.h>
#include <task.h>
#include <widget.h>
#include <misc.h>

#define DebugLogMsg  SLogMsg

#define RECT(rect,x,y,w,h) do {\
	rect.left = x, rect.top = y, rect.right = x+w, rect.bottom = y+h;\
} while (0)

void VersionProgramTask (int argument);
void PrgPaintTask (int argument);
void IconTestTask (int argument);
void ListTestTask (int argument);
void CabinetEntry (int argument);
void SystemMonitorEntry(int argument);
void LauncherEntry(int arg);

#endif//_WBUILTIN_H