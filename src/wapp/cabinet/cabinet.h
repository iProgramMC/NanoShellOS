/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

        Cabinet Application module
******************************************/
#ifndef __CABINET_H
#define __CABINET_H

#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>
#include <elf.h>
#include <wterm.h>
#include <resource.h>

IconType CabGetIconBasedOnName(const char *pName, int pType);
void CabinetChangeDirectory(Window* pWindow, const char * cwd, bool bTakeToRootOnFailure);
void FormatSize(uint32_t size, char* size_buf);
void FormatSizeDetailed(uint32_t size, char* size_buf);
void CreatePropertiesWindow(Window * parent, const char* path, const char* justName);
void PopupUserMountWindow(Window* pWindow);

#endif//__CABINET_H