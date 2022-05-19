/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
        Directory list application

             Main source file
******************************************/
#include <nsstandard.h>

Console* GetConsole();

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	bool bareMode = false;
	
	//allow reverting to old color
	Console* pCon = GetConsole();
	uint8_t old_color = pCon->color;
	
	char revert_color[3] = { 0x1, 0xF, 0x0 };
	revert_color[1] = 0x10 + (old_color & 0xF);
	
	
	int dd = FiOpenDir (FiGetCwd());
	if (dd < 0)
	{
		LogMsg("ls: cannot list '%s': %s", FiGetCwd(), ErrNoStr(dd));
		return 1;
	}
	
	// rewind the dir
	FiRewindDir (dd);
	
	// scan through dir entries
	DirEnt* pDirEnt;
	while ((pDirEnt = FiReadDir(dd)) != NULL)
	{
		if (bareMode)
		{
			LogMsg("%s", pDirEnt->m_name);
			continue;
		}
		
		StatResult statResult;
		int res = FiStatAt (dd, pDirEnt->m_name, &statResult);
		
		if (res < 0)
		{
			LogMsg("ls: cannot stat '%s': %s", pDirEnt->m_name, ErrNoStr(res));
			continue;
		}
		#define THING "\x10"
		if (statResult.m_type & FILE_TYPE_DIRECTORY)
		{
			LogMsg("%c%c%c\x02" THING "\x01\x0C%s\x01\x0F",
				"-r"[!!(statResult.m_perms & PERM_READ )],
				"-w"[!!(statResult.m_perms & PERM_WRITE)],
				"-x"[!!(statResult.m_perms & PERM_EXEC )],
				pDirEnt->m_name
			);
		}
		else
		{
			LogMsg("%c%c%c %d\x02" THING "%s",
				"-r"[!!(statResult.m_perms & PERM_READ )],
				"-w"[!!(statResult.m_perms & PERM_WRITE)],
				"-x"[!!(statResult.m_perms & PERM_EXEC )],
				statResult.m_size,
				pDirEnt->m_name
			);
		}
		#undef THING
	}
	
	FiCloseDir (dd);
	
	pCon->color = old_color;
	
	return 0;
}
