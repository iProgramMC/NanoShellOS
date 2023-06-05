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
	enum
	{
		SW_UNK  = (1 << 0),
		SW_BARE = (1 << 1),
		SW_DATE = (1 << 2),
		SW_INO  = (1 << 3),
		SW_HELP = (1 << 4),
		SW_CR   = (1 << 5),
	};
	
	int switches = 0;
	
	const char* pPathToList = FiGetCwd();
	
	for (int argn = 1; argn < argc; argn++)
	{
		char* parm = argv[argn];
		
		// if it starts with a dash, it's a switch or combination of switches.
		if (*parm == '-')
		{
			parm++;
			while (*parm)
			{
				switch (*parm)
				{
					case 'h': switches |= SW_HELP; break;
					case 'i': switches |= SW_INO;  break;
					case 'd': switches |= SW_DATE; break;
					case 'b': switches |= SW_BARE; break;
					case 'c': switches |= SW_CR;   break;
					default:  switches |= SW_UNK;  break;
				}
				parm++;
			}
		}
		
		// TODO: allow listing paths other than the cwd
	}
	
	if (switches & SW_UNK)
	{
		LogMsg("One or more unknown switches have been provided.");
	}
	
	if (switches & (SW_HELP | SW_UNK))
	{
		LogMsg("Usage: ls [-ibdh]");
		LogMsg("-h: Help. Shows this list.");
		LogMsg("-i: Display inode numbers next to files.");
		LogMsg("-b: List the directory in a bare format.");
		LogMsg("-d: Show human readable dates next to files.");
		LogMsg("-c: Show creation date instead of modification date. Use with -d.");
		return 0;
	}
	
	// specific color codes
	
	const char * colordire = "\x1b[91m";
	const char * colorfile = "\x1b[94m";
	const char * colorpipe = "\x1b[92m";
	const char * colorblkd = "\x1b[93m";
	const char * colorchrd = "\x1b[95m";
	const char * colormntp = "\x1b[96m";
	const char * colorunkf = "\x1b[98m";
	const char * normal    = "\x1b[97m";
	
	bool bareMode = switches & SW_BARE;
	
	int dd = FiOpenDir (pPathToList);
	if (dd < 0)
	{
		LogMsg("ls: cannot list '%s': %s", FiGetCwd(), strerror(errno));
		return 1;
	}
	
	LogMsg("%sDirectory of %s", normal, pPathToList);
	
	// rewind the dir
	FiRewindDir (dd);
	
	// scan through dir entries
	DirEnt ent, *pDirEnt = &ent;
	int err = 0;
	
	while ((err = FiReadDir(&ent, dd)) == 0)
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
			LogMsg("ls: cannot stat '%s': %s", pDirEnt->m_name, strerror(errno));
			continue;
		}
		
		const char* auxStr = "";
		char buffer [256];
		memset( buffer, 0, sizeof buffer );
		
		if (switches & SW_DATE)
		{
			// This shows the last modified date.
			uint32_t date = statResult.m_modifyTime;
			
			if (switches & SW_CR)
				date = statResult.m_createTime;
			
			TimeStruct ts;
			GetHumanTimeFromEpoch( date, &ts );
			
			sprintf( buffer, "%02d/%02d/%04d %02d:%02d:%02d  ", ts.day, ts.month, ts.year, ts.hours, ts.minutes, ts.seconds );
			
			auxStr = buffer;
		}
		
		if (switches & SW_INO)
		{
			char buf[20];
			
			sprintf( buf, " %9u ", statResult.m_inode );
			
			// hack
			if (buf[10] != ' ')
			{
				memmove(buf, buf + 1, sizeof buf);
				buf[10] = ' ';
			}
			
			
			strcat( buffer, buf );
			
			auxStr = buffer;
		}
		
		if (statResult.m_type & FILE_TYPE_MOUNTPOINT)
		{
			LogMsg("%s%c%c%c           %s%s%s",
				auxStr,
				"-r"[!!(statResult.m_perms & PERM_READ )],
				"-w"[!!(statResult.m_perms & PERM_WRITE)],
				"-x"[!!(statResult.m_perms & PERM_EXEC )],
				colormntp,
				pDirEnt->m_name,
				normal
			);
		}
		else
		if (statResult.m_type & FILE_TYPE_DIRECTORY)
		{
			LogMsg("%s%c%c%c           %s%s%s",
				auxStr,
				"-r"[!!(statResult.m_perms & PERM_READ )],
				"-w"[!!(statResult.m_perms & PERM_WRITE)],
				"-x"[!!(statResult.m_perms & PERM_EXEC )],
				colordire,
				pDirEnt->m_name,
				normal
			);
		}
		else
		{
			const char* color = colorunkf;
			switch (statResult.m_type)
			{
				case FILE_TYPE_FILE:   color = colorfile;
				case FILE_TYPE_PIPE:   color = colorpipe;
				case FILE_TYPE_CHAR_DEVICE:   color = colorchrd;
				case FILE_TYPE_BLOCK_DEVICE:  color = colorblkd;
			}
			
			LogMsg("%s%c%c%c %9d %s%s%s",
				auxStr,
				"-r"[!!(statResult.m_perms & PERM_READ )],
				"-w"[!!(statResult.m_perms & PERM_WRITE)],
				"-x"[!!(statResult.m_perms & PERM_EXEC )],
				statResult.m_size,
				color,
				pDirEnt->m_name,
				normal
			);
		}
		#undef THING
	}
	
	if (err < 0)
	{
		LogMsg("ls: %s: %s", FiGetCwd(), strerror(errno));
	}
	
	LogMsgNoCr("\x1B[0m");
	
	FiCloseDir (dd);
	
	return 0;
}
