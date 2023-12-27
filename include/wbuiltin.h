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
#include <time.h>

#define DebugLogMsg  SLogMsg

#define RECT(rect,x,y,w,h) do {\
	rect.left = x, rect.top = y, rect.right = x+w, rect.bottom = y+h;\
} while (0)

// ALT TAB
void UpdateAltTabWindow();
void OnPressAltTabOnce();
void KillAltTab();

// MISC WINDOWS
void VersionProgramTask(long argument);
void PrgMagnifyTask(long argument);
void PrgMineTask  (long argument);
void PrgPaintTask (long argument);
void PrgVBldTask  (long argument);
void IconTestTask (long argument);
void ListTestTask (long argument);
void CabinetEntry (long argument);
void BigTextEntry (long argument);
void ControlEntry (long argument);
void HelpEntry    (long argument);
void LaunchVersion();//ShellAbout
void SystemMonitorEntry(long argument);
void LauncherEntry(long arg);
void TaskbarEntry(long arg);
void CrashReporterCheck();
void ShellAbout (const char *pText, int iconID);

#endif//_WBUILTIN_H