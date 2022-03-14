/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     Command line kernel shell module
******************************************/
#include <main.h>
#include <shell.h>
#include <keyboard.h>
#include <string.h>
#include <console.h>
#include <vga.h>
#include <video.h>
#include <print.h>
#include <memory.h>
#include <misc.h>
#include <task.h>
#include <storabs.h>
#include <window.h>
#include <icon.h>
#include <vfs.h>
#include <elf.h>
#include <cinterp.h>
#include <fat.h>

char g_lastCommandExecuted[256] = {0};
extern Console* g_currentConsole;

void ShellTaskTest(int arg)
{
	while (1)	
	{
		SLogMsg("Task %d!", arg);
		for (int i = 0; i < 3; i++)
			hlt;
	}
}

void ShellTaskTest2(int arg)
{
	//This is the entry point of the new thread.
	//You can pass any 32-bit parm in the StartTask call. `arg` is one of them.
	//var represents the next color to set
	int var = 0;
	while (1)	
	{
		// set a 100 pixel tall column at that position:
		for (int y = 100; y < 200; y++)
			VidPlotPixel (arg%GetScreenSizeX(), y+arg/GetScreenSizeX(), var);
		
		//increment color by 32
		var += 32;
		
		//wait for 5 interrupts
		for (int i = 0; i < 5; i++)
			hlt;
	}
}

void TemporaryTask(__attribute__((unused)) int arg)
{
	for (int i = 0; i < 15; i++)
	{
		//for (int j = 0; j < 10000000; j++)
		//	;
		LogMsgNoCr("HI! %d",i);
		for (int i = 0; i < 30; i++)
			hlt;
	}
}
extern void KeTaskDone();

typedef void (*Pointer)(unsigned color, int left, int top, int right, int bottom);

void GraphicsTest()
{
	g_currentConsole->color = 0x2F;
	CoClearScreen(g_currentConsole);
	
	//demonstrate some of the apis that the kernel provides:
	//VidFillRect(0xFF0000, 10, 150, 210, 310);
	/**((uint32_t*)0xC0007CFC) = 14;
	
	Pointer ptr = (Pointer) 0xC0007C00;
	
	ptr(0xFF0000, 10, 150, 210, 310);return;*/
	VidDrawRect(0x00FF00, 100, 150, 250, 250);
	
	//lines, triangles, polygons, circles perhaps?
	VidDrawHLine (0xEE00FF, 100, 500, 400);
	VidDrawVLine (0xEE00FF, 150, 550, 15);
	
	VidPlotChar('A', 200, 100, 0xFF, 0xFF00);
	VidTextOut("Hello, opaque background world.\nI support newlines too!", 300, 150, 0xFFE, 0xAEBAEB);
	VidTextOut("Hello, transparent background world.\nI support newlines too!", 300, 182, 0xFFFFFF, TRANSPARENT);
	VidShiftScreen(10);
	
	LogMsg("Test complete.  Strike a key to exit.");
	CoGetChar();
	g_currentConsole->color = 0x1F;
}

int  g_nextTaskNum    = 0;
bool g_ramDiskMounted = true;
int  g_ramDiskID      = 0x00;//ATA: Prim Mas
int  g_lastReturnCode = 0;
bool CoPrintCharInternal (Console* this, char c, char next);

char g_cwd[PATH_MAX+1];
FileNode* g_pCwdNode = NULL;


void funnytest(UNUSED int argument)
{
	FileNode* pNode = g_pCwdNode;
	FileNode* pFile = FsFindDir(pNode, "main.nse");
	if (!pFile)
		LogMsg("No such file or directory");
	else
	{
		int length = pFile->m_length;
		char* pData = (char*)MmAllocate(length + 1);
		pData[length] = 0;
		
		FsRead(pFile, 0, length, pData);
		
		ElfExecute(pData, length, "");
		
		MmFree(pData);
	}
	LogMsg("");
}
extern Heap* g_pHeap;
extern bool  g_windowManagerRunning;
void WindowManagerShutdown ();

void ShellExecuteCommand(char* p)
{
	TokenState state;
	state.m_bInitted = 0;
	char* token = Tokenize (&state, p, " ");
	if (!token)
		return;
	if (*token == 0)
		return;
	if (strcmp (token, "help") == 0)
	{
		LogMsg("NanoShell Shell Help");
		LogMsg("cat        - prints the contents of a file");
		LogMsg("cls        - clear screen");
		LogMsg("cm         - character map");
		LogMsg("cd         - change directory");
		LogMsg("crash      - attempt to crash the kernel");
		LogMsg("color XX   - change the screen color");
		LogMsg("fd         - attempts to resolve a path, prints non-zero if found");
		LogMsg("ft         - attempts to write 'Hello, world\\n' to a file");
		LogMsg("e          - executes an ELF from the initrd");
		LogMsg("el         - prints the last returned value from an executed ELF");
		LogMsg("help       - shows this list");
		LogMsg("gt         - run a graphical test");
		LogMsg("lm         - list memory allocations");
		LogMsg("lr         - list the memory ranges provided by the bootloader");
		LogMsg("ls         - list the current working directory (right now just /)");
		LogMsg("lt         - list currently running threads (pauses them during the print)");
		LogMsg("mode X     - change the screen mode");
		LogMsg("mspy       - Memory Spy! (TM)");
		LogMsg("mrd        - mounts a RAM Disk from a file");
		
		//wait for new key
		LogMsg("Strike a key to print more.");
		CoGetChar();
		
		LogMsg("ph         - prints current heap's address in kernel address space (or NULL for the default heap)");
		LogMsg("rb         - reboots the system");
		LogMsg("sysinfo    - dump system information");
		LogMsg("sysinfoa   - dump advanced system information");
		LogMsg("time       - get timing information");
		LogMsg("stt        - spawns a single thread that doesn't last forever");
		LogMsg("st         - spawns a single thread that makes a random line forever");
		LogMsg("tt         - spawns 64 threads that makes random lines forever");
		LogMsg("tte        - spawns 1024 threads that makes random lines forever");
		LogMsg("ttte       - spawns 1024 threads that prints stuff");
		LogMsg("ver        - print system version");
		LogMsg("w          - start desktop manager");
	}
	else if (strcmp (token, "rb") == 0)
	{
		bool force = false;
		char* fileName = Tokenize (&state, NULL, " ");
		if (fileName)
		{
			if (strcmp (fileName, "--force") == 0) force = true;
		}
		if (KeGetRunningTask() == NULL)
			KeRestartSystem();
		else if (force)
		{
			WindowManagerShutdown ();
		}
		else
			LogMsg("Use the launcher's \"Shutdown computer\" option, shut down the computer, and click \"Restart\" to reboot, or use --force.");
	}
	else if (strcmp (token, "ph") == 0)
	{
		LogMsg("Current Heap: %x",g_pHeap);
	}
	else if (strcmp (token, "cd") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
			LogMsg("Expected filename");
		else if (*fileName == 0)
			LogMsg("Expected filename");
		else
		{
			if (strcmp (fileName, PATH_THISDIR) == 0) return;
			if (strcmp (fileName, "/") == 0)
			{
				strcpy(g_cwd, "/");
				g_pCwdNode = FsResolvePath(g_cwd);
			}
			if (strcmp (fileName, PATH_PARENTDIR) == 0)
			{
				for (int i = PATH_MAX - 1; i >= 0; i--)
				{
					if (g_cwd[i] == PATH_SEP)
					{
						g_cwd[i+(i==0)] = 0;
						g_pCwdNode = FsResolvePath(g_cwd);
						if (!g_pCwdNode)
						{
							LogMsg("Fatal: could not find parent directory?!");
							return;
						}
						break;
					}
				}
				return;
			}
			if (strlen (g_cwd) + strlen (fileName) < PATH_MAX - 3)
			{
				char cwd_copy[sizeof(g_cwd)];
				memcpy(cwd_copy, g_cwd, sizeof(g_cwd));
				if (g_cwd[1] != 0)
					strcat (g_cwd, "/");
				strcat (g_cwd, fileName);
				FileNode *pPrev = g_pCwdNode;
				g_pCwdNode = FsResolvePath(g_cwd);
				if (!g_pCwdNode)
				{
					memcpy(g_cwd, cwd_copy, sizeof(g_cwd));
					g_pCwdNode = pPrev;
					LogMsg("No such file or directory");
				}
			}
			else
				LogMsg("Path would be too large");
		}
	}
	else if (strcmp (token, "cm") == 0)
	{
		for (int y = 0; y < 16; y++)
			for (int x = 0; x < 16; x++)
			{
				CoPlotChar(g_currentConsole, x, y, (y<<4)|x);
			}
	}
	else if (strcmp (token, "el") == 0)
	{
		LogMsg("Last run ELF returned: %d", g_lastReturnCode);
	}
	else if (strcmp (token, "em") == 0)
	{
		int erc = 0;
		Task* pTask = KeStartTask(funnytest, 0, &erc);
		
		LogMsg("Started task!  Pointer: %x, errcode: %x", pTask, erc);
	}
	else if (strcmp (token, "fd") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else
		{
			LogMsg("Got: %x", FsResolvePath (fileName));
		}
	}
	else if (strcmp (token, "e") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else if (!g_windowManagerRunning)
		{
			LogMsg("Running apps outside the window manager is broken for now so don't.");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			int length = FiTellSize(fd);
			
			char* pData = (char*)MmAllocate(length + 1);
			pData[length] = 0;
			
			FiRead(fd, pData, length);
			
			FiClose(fd);
			
			ElfExecute(pData, length, g_lastCommandExecuted);
			
			MmFree(pData);
			
			LogMsg("");
		}
	}
	else if (strcmp (token, "ec") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			int length = FiTellSize(fd);
			
			char* pData = (char*)MmAllocate(length + 1);
			pData[length] = 0;
			
			FiRead(fd, pData, length);
			
			FiClose(fd);
			
			//ElfExecute(pData, length);
			
			CCSTATUS status = CcRunCCode(pData, length);
			LogMsg("Exited with status %d", status);
			
			MmFree(pData);
			
			LogMsg("");
		}
	}
	else if (strcmp (token, "cat") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			/*int length = FiTellSize (fd);
			char* pData = (char*)MmAllocate(length + 1);
			pData[length] = 0;
			*/
			FiSeek(fd, 0, SEEK_SET);
			
			int result; char data[2];
			while ((result = FiRead(fd, data, 1), result > 0))
			{
				CoPrintChar(g_currentConsole, data[0]);
			}
			
			FiClose (fd);
			LogMsg("");
		}
	}
	else if (strcmp (token, "ft") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_WRONLY);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			FiSeek(fd, 0, SEEK_END);
			
			char text[] = "Hello World from FiWrite!\n\n\n";
			
			FiWrite(fd, text, sizeof(text)-1);//do not also print the null terminator
			
			FiClose (fd);
			LogMsg("Done");
		}
	}
	else if (strcmp (token, "fce") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_WRONLY | O_CREAT);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			FiSeek(fd, 0, SEEK_END);
			
			char text[] = "Hello World from FiWrite!\n\n\n";
			
			FiWrite(fd, text, sizeof(text)-1);//do not also print the null terminator
			
			FiClose (fd);
			LogMsg("Done");
		}
	}
	else if (strcmp (token, "ffa") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("Expected filename");
		}
		else if (*fileName == 0)
		{
			LogMsg("Expected filename");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_WRONLY);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			FiSeek(fd, 0, SEEK_END);
			
			char text[] = "Hello World from FiWrite!\n\n\n";
			
			FiWrite(fd, text, sizeof(text)-1);//do not also print the null terminator
			
			FiClose (fd);
			LogMsg("Done");
		}
	}
	else if (strcmp (token, "lr") == 0)
	{
		KePrintMemoryMapInfo();
	}
	else if (strcmp (token, "ls") == 0)
	{
		FileNode* pNode = g_pCwdNode;
		LogMsg("Directory of %s", pNode->m_name, pNode);
		
		if (!FsOpenDir(pNode))
		{
			LogMsg("ERROR: Could not open '%s', try 'cd'-ing back?", pNode->m_name);
			return;
		}
		
		DirEnt* pDirEnt;
		int i = 0;
		while ((pDirEnt = FsReadDir(pNode, i)) != 0)
		{
			FileNode* pSubnode = FsFindDir(pNode, pDirEnt->m_name);
			if (!pSubnode)
				LogMsg("- [NULL?!]");
			else
				LogMsg("- %s\t%c%c%c\t%d\x02\x16\"%s\"", 
						(pSubnode->m_type & FILE_TYPE_DIRECTORY) ? "<DIR>" : "     ", 
						"-R"[!!(pSubnode->m_perms & PERM_READ )],
						"-W"[!!(pSubnode->m_perms & PERM_WRITE)],
						"-X"[!!(pSubnode->m_perms & PERM_EXEC )],
						pSubnode->m_length, 
						pSubnode->m_name);
			i++;
		}
		
		FsCloseDir(pNode);
	}
	else if (strcmp (token, "gt") == 0)
	{
		GraphicsTest();
	}
	else if (strcmp (token, "w") == 0)
	{
		if (VidIsAvailable())
		{
			/*int errorCode = 0;
			Task* pTask = KeStartTask (WindowManagerTask, 0, &errorCode);
			LogMsg("TASK: %x %x", pTask, errorCode);
			
			while (1) hlt;*/
			WindowManagerTask(0);
		}
		else
			LogMsg("Cannot run window manager in text mode.  Restart your computer, then make sure the gfxpayload is valid in GRUB.");
	}
	else if (strcmp (token, "mrd") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
		{
			LogMsg("You want to mount what, exactly?");
		}
		else if (*fileName == 0)
		{
			LogMsg("You want to mount what, exactly?");
		}
		else
		{
			char s[1024];
			if (*fileName != '/')
			{
				strcpy (s, g_cwd);
				if (g_cwd[1] != 0) //not just a '/'
					strcat(s, "/");
			}
			strcat (s, fileName);
			
			int fd = FiOpen (s, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("Got error code %d when opening file", fd);
				return;
			}
			
			int length = FiTellSize(fd);
			
			char* pData = (char*)MmAllocate(length + 1);
			pData[length] = 0;
			
			FiRead(fd, pData, length);
			
			FiClose(fd);
			
			FsMountRamDisk(pData);
			
			//Do not free as the file system now owns this pointer.
		}
	}
	else if (strcmp (token, "check") == 0)
	{
		char* fatNum = Tokenize (&state, NULL, " ");
		if (!fatNum)
		{
			goto print_usage1;
		}
		if (*fatNum == 0)
		{
			goto print_usage1;
		}
		
		int nFat = atoi (fatNum);
		
		CheckDiskFatMain (nFat);
		
		goto dont_print_usage1;
	print_usage1:
		LogMsg("Check Disk");
		LogMsg("Usage: check <FAT file system number>");
	dont_print_usage1:;
	}
	else if (strcmp (token, "mspy") == 0)
	{
		char* secNum = Tokenize (&state, NULL, " ");
		if (!secNum)
		{
			goto print_usage;
		}
		if (*secNum == 0)
		{
			goto print_usage;
		}
		
		char* nBytesS = Tokenize(&state, NULL, " ");
		if (!nBytesS)
		{
			goto print_usage;
		}
		if (*nBytesS == 0)
		{
			goto print_usage;
		}
		
		char* auxSwitch = Tokenize(&state, NULL, " ");
		bool as_bytes = false;
		if (auxSwitch && *auxSwitch != 0)
		{
			if (strcmp (auxSwitch, "/b") == 0)
				as_bytes = true;
		}
		
		int nAddr = atoi (secNum);
		int nBytes= atoi (nBytesS);
		
		int ints = nBytes/4;
		if (ints > 1024) ints = 1024;
		if (ints < 4) ints = 4;
		
		uint32_t* pAddr = (uint32_t*)(nAddr << 12);
		uint8_t* pAddrB = (uint8_t*) (nAddr << 12);
		for (int i = 0; i < ints; i += (8 >> as_bytes))
		{
			for (int j = 0; j < (8 >> as_bytes); j++)
			{
				if (as_bytes)
				{
					LogMsgNoCr("%b %b %b %b ", pAddrB[((i+j)<<2)+0], pAddrB[((i+j)<<2)+1], pAddrB[((i+j)<<2)+2], pAddrB[((i+j)<<2)+3]);
				}
				else
					LogMsgNoCr("%x ", pAddr[i+j]);
			}
			for (int j = 0; j < (8 >> as_bytes); j++)
			{
				#define FIXUP(c) ((c<32||c>126)?'.':c)
				char c1 = pAddrB[((i+j)<<2)+0], c2 = pAddrB[((i+j)<<2)+1], c3 = pAddrB[((i+j)<<2)+2], c4 = pAddrB[((i+j)<<2)+3];
				LogMsgNoCr("%c%c%c%c", FIXUP(c1), FIXUP(c2), FIXUP(c3), FIXUP(c4));
			}
			LogMsg("");
		}
		goto dont_print_usage;
	print_usage:
		LogMsg("Virtual Memory Spy (TM)");
		LogMsg("Usage: mspy <page number> <numBytes> [/b]");
		LogMsg("- bytes will be printed as groups of 4 unless [/b] is specified");
		LogMsg("- numBytes will be capped off at 4096 and rounded down to 32");
		LogMsg("- pageNumber must be a \x01\x0CVALID\x01\x0F and \x01\x0CMAPPED\x01\x0F address.");
		LogMsg("- if it's not valid or mapped then the system may CRASH or HANG!");
		LogMsg("- pageNumber is in\x01\x0C DECIMAL\x01\x0F");
		LogMsg("- note: cut off the last 3 digits of an address in hex and turn it to decimal to get a pageNumber");
	dont_print_usage:;
	}
	else if (strcmp (token, "cls") == 0)
	{
		CoClearScreen (g_currentConsole);
		g_currentConsole->curX = g_currentConsole->curY = 0;
	}
	else if (strcmp (token, "ver") == 0)
	{
		KePrintSystemVersion();
	}
	else if (strcmp (token, "lf") == 0)
	{
		FiDebugDump();
	}
	else if (strcmp (token, "lm") == 0)
	{
		MmDebugDump();
	}
	else if (strcmp (token, "lt") == 0)
	{
		KeTaskDebugDump();
	}
	/*else if (strcmp (token, "icon") == 0)
	{
		RenderIcon(ICON_NANOSHELL, 10, 10);
	}*/
	else if (strcmp (token, "stt") == 0)
	{
		int errorCode = 0;
		Task* task = KeStartTask(TemporaryTask, g_nextTaskNum++, &errorCode);
		LogMsg("Task %d (%x) spawned.  Error code: %x", g_nextTaskNum - 1, task, errorCode);
	}
	else if (strcmp (token, "st") == 0)
	{
		int errorCode = 0;
		Task* task = KeStartTask(ShellTaskTest2, g_nextTaskNum++, &errorCode);
		LogMsg("Task %d (%x) spawned.  Error code: %x", g_nextTaskNum - 1, task, errorCode);
	}
	else if (strcmp (token, "tt") == 0)
	{
		int errorCode = 0;
		for (int i = 0; i < 64; i++)
		{
			KeStartTask(ShellTaskTest2, g_nextTaskNum++, &errorCode);
		}
		LogMsg("Tasks have been spawned.");
		//LogMsg("Task %d (%x) spawned.  Error code: %x", g_nextTaskNum - 1, task, errorCode);
	}
	else if (strcmp (token, "tte") == 0)
	{
		int errorCode = 0;
		for (int i = 0; i < 1024; i++)
		{
			KeStartTask(ShellTaskTest2, g_nextTaskNum++, &errorCode);
		}
		LogMsg("Tasks have been spawned.");
	}
	else if (strcmp (token, "ttte") == 0)
	{
		int errorCode = 0;
		for (int i = 0; i < 128; i++)
		{
			KeStartTask(TemporaryTask, g_nextTaskNum++, &errorCode);
		}
		LogMsg("Tasks have been spawned.");
	}
	else if (strcmp (token, "crash") == 0)
	{
		LogMsg("OK");
		*((uint32_t*)0xFFFFFFFF) = 0;
	}
	/*else if (strcmp (token, "crashugly") == 0)
	{
		LogMsg("OK.  Crashing in a horrible, disgusting, and visceral way");
		
		//METHOD 1 doesn't work.  Just throws a page fault
		//__asm__ volatile ("movl 0x53673689, %eax\n\tmovl %eax, %esp");
		
		//METHOD 2: Stack underflow.  Pop shouldn't write to the stack -- Still throws a pagefault
		//while (1)
		//	__asm__ volatile ("pop %eax\n");
	
		//METHOD 3: Stack overflow. -- Threw a DIFFERENT error this time, a stack overflow
		//while (1)
		//	__asm__ volatile ("push %eax");
	
		//METHOD 4: Combine them -- Threw a pagefault this time ??
		//while (1)
		//{
		//	__asm__ volatile ("movl 0x28, %eax\n\tmovl %eax, %ss");
		//	__asm__ volatile ("push %eax");
		//}
	}*/
	else if (strcmp (token, "time") == 0)
	{
		int hi, lo;
		GetTimeStampCounter(&hi, &lo);
		LogMsg("Timestamp counter: %x%x (%d, %d)", hi, lo, hi, lo);
		
		//int tkc = GetTickCount(), rtkc = GetRawTickCount();
		//LogMsgNoCr("Tick count: %d, Raw tick count: %d", tkc, rtkc);
		LogMsg("Press any key to stop timing.");
		
		while (CoInputBufferEmpty())
		{
			int tkc = GetTickCount(), rtkc = GetRawTickCount();
			LogMsgNoCr("\rTick count: %d, Raw tick count: %d        ", tkc, rtkc);
			//for(int i=0; i<50; i++) 
			hlt;
		}
	}
	else if (strcmp (token, "mode") == 0)
	{
		if (VidIsAvailable())
		{
			LogMsg("Must use emergency text-mode shell to change mode.");
			return;
		}
		char* modeNum = Tokenize (&state, NULL, " ");
		if (!modeNum)
		{
			LogMsg("Expected mode number");
		}
		else if (*modeNum == 0)
		{
			LogMsg("Expected mode number");
		}
		else
		{
			SwitchMode (*modeNum - '0');
			//PrInitialize();
			CoInitAsText(g_currentConsole);
		}
	}
	else if (strcmp (token, "font") == 0)
	{
		char* fontNum = Tokenize (&state, NULL, " ");
		if (!fontNum)
		{
			LogMsg("Expected mode number");
		}
		else if (*fontNum == 0)
		{
			LogMsg("Expected font number");
		}
		else
		{
			VidSetFont (*fontNum - '0');
			LogMsg("the quick brown fox jumps over the lazy dog");
			LogMsg("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
			LogMsg("Font testing done.");
			//PrInitialize();
		}
	}
	else if (strcmp (token, "color") == 0)
	{
		char* colorNum = Tokenize (&state, NULL, " ");
		if (!colorNum)
		{
			LogMsg("Expected color hex");
		}
		else if (*colorNum == 0 || *(colorNum + 1) == 0)
		{
			LogMsg("Expected color hex");
		}
		else
		{
			//SwitchMode (*modeNum - '0');
			char c1 = colorNum[0], c2 = colorNum[1];
			
			/**/ if (c1 >= '0' && c1 <= '9') c1 -= '0';
			else if (c1 >= 'A' && c1 <= 'F') c1 -= 'A'-0xA;
			else if (c1 >= 'a' && c1 <= 'f') c1 -= 'a'-0xA;
			
			/**/ if (c2 >= '0' && c2 <= '9') c2 -= '0';
			else if (c2 >= 'A' && c2 <= 'F') c2 -= 'A'-0xA;
			else if (c2 >= 'a' && c2 <= 'f') c2 -= 'a'-0xA;
			
			g_currentConsole->color = c1 << 4 | c2;
		}
	}
	else if (strcmp (token, "sysinfo") == 0)
	{
		KePrintSystemInfo();
	}
	else if (strcmp (token, "sysinfoa") == 0)
	{
		KePrintSystemInfoAdvanced();
	}
	else
	{
		LogMsg("Unknown command.  Please type 'help'.");
	}
	
	//LogMsg("You typed: '%s'", p);
}

void ShellInit()
{
	strcpy (g_cwd, "/");
	g_pCwdNode = FsResolvePath (g_cwd);
}

void ShellRun(UNUSED int unused_arg)
{
	while (1) 
	{
		LogMsgNoCr("%s>", g_cwd);
		char buffer[256];
		CoGetString (buffer, 256);
		memcpy (g_lastCommandExecuted, buffer, 256);
		
		ShellExecuteCommand (buffer);
		
		hlt; hlt; hlt; hlt;
	}
}
