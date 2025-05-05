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
#include <time.h>
#include <task.h>
#include <storabs.h>
#include <window.h>
#include <icon.h>
#include <vfs.h>
#include <elf.h>
#include <cinterp.h>
#include <sb.h>
#include <fat.h>
#include <pci.h>
#include <config.h>
#include <clip.h>
#include <image.h>
#include <ansi.h>
#define MOVEDATA_PRECISION 4096
char g_lastCommandExecuted[256] = {0};

void FsPipeTest();
void MemorySpy();
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

void ElfOnExecuteFail(int errorCode, const char* fileName, bool bAsync)
{
	char buffer[1024 + PATH_MAX];
	
	snprintf(buffer, sizeof buffer, ElfGetErrorMsg(errorCode), fileName);
	
	if (bAsync)
	{
		MessageBox(NULL, buffer, "Application Execution Error", MB_OK | ICON_ERROR << 16);
		return;
	}
	
	// we assume the shell handles this.
}

extern void KeTaskDone();

typedef void (*Pointer)(unsigned color, int left, int top, int right, int bottom);

void ShellClearScreen()
{
	LogMsgNoCr(ANSI_CLEAR_SCREEN ANSI_GO_TO(1, 1));
}

static int s_ShellSetColorLutBg[] = { 40, 44, 42, 46, 41, 45, 43, 47,100,104,102,106,101,105,103,107 };
static int s_ShellSetColorLutFg[] = { 30, 34, 32, 36, 31, 35, 33, 37, 90, 94, 92, 96, 91, 95, 93, 97 };

void ShellSetColor(uint8_t fgbg)
{
	char ansiCmd [64];
	int bgColorCode = s_ShellSetColorLutBg[fgbg >> 4];
	int fgColorCode = s_ShellSetColorLutFg[fgbg & 0xF];
	
	sprintf(ansiCmd, "\e[%d;%dm" ANSI_PERMANENTIZE, bgColorCode, fgColorCode);
	
	LogMsgNoCr(ansiCmd);
}

void ShellExecutableInfo(const char* pFileName)
{
	ProgramInfo *pinfo = RstRetrieveProgramInfoFromFile(pFileName);
	if (pinfo == NULL)
	{
		LogMsg("ei: %s: Cannot retrieve executable info.", pFileName);
		return;
	}
	
	LogMsg("%s Properties",          pFileName);
	LogMsg("Project Name: %s",       pinfo->m_info.m_ProjName);
	LogMsg("File Desc   : %s",       pinfo->m_info.m_AppName);
	LogMsg("Version     : %d.%d.%d", pinfo->m_info.m_Version.Major, pinfo->m_info.m_Version.Minor, pinfo->m_info.m_Version.BuildNum);
	LogMsg("Author      : %s",       pinfo->m_info.m_AppAuthor);
	LogMsg("Copyright   : %s",       pinfo->m_info.m_AppCopyright);
	LogMsg("Architecture: %s",       ElfGetArchitectureString(pinfo->m_machine, pinfo->m_word_size));
	LogMsg("OS ABI      : %s",       ElfGetOSABIString(pinfo->m_os_abi));
	
	MmFree(pinfo);
}

int  g_nextTaskNum    = 0;
bool g_ramDiskMounted = true;
int  g_ramDiskID      = 0x00;//ATA: Prim Mas
int  g_lastReturnCode = 0;
bool CoPrintCharInternal (Console* this, char c, char next);

//extern Heap* g_pHeap;
extern bool  g_windowManagerRunning;
void WindowManagerShutdown ();
uint64_t ReadTSC();

void ShellMinicom(const char* pFile)
{
	if (strcmp(pFile, "--help") == 0)
	{
		LogMsg("mc help");
		LogMsg("Usage: mc <device file name>");
		LogMsg("To exit, press Ctrl-A, then Ctrl-D.");
		LogMsg("Functionality:");
		LogMsg("   MC is a terminal simulator. It allows both input and output through "
		       "character devices, such as serial ports.");
	}
	
	StatResult sr;
	int ec = FiStat(pFile, &sr);
	if (ec < 0)
	{
		LogMsg("mc: cannot stat '%s': %s", pFile, GetErrNoString(ec));
		return;
	}
	
	if (sr.m_type != FILE_TYPE_CHAR_DEVICE)
	{
		LogMsg("mc: '%s' is not a character device");
		return;
	}
	
	int fd = FiOpen(pFile, O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		LogMsg("mc: cannot open '%s': %s", pFile, GetErrNoString(ec));
		return;
	}
	
	// clear the screen
	LogMsg(ANSI_CLEAR_SCREEN);
	
	bool ctrl_a = false;
	
	while (true)
	{
		char buffer[256];
		size_t read = FiRead(fd, buffer, sizeof buffer);
		if ((int)read < 0) break;
		
		if (read == 0)
		{
			WaitMS(10);
		}
		
		for (size_t i = 0; i < read; i++)
		{
			CoPrintChar(GetCurrentConsole(), buffer[i]);
			//LogMsgNoCr("%b ",buffer[i]);
		}
		
		if (CoAnythingOnInputQueue(GetCurrentConsole()))
		{
			char chr = CoGetChar();
			
			// check if the user hit ctrl+A and then ctrl+D
			if (chr == '\1') // Ctrl-A
				ctrl_a = true;
			else if (chr == '\4' && ctrl_a) // Ctrl-D
				break;
			else
				ctrl_a = false;
			
			// not sure if we should echo.. probably not
			FiWrite(fd, &chr, sizeof chr);
		}
	}
	
	FiClose(fd);
}

void ShellExecuteFile(const char* fileName, const char* arguments)
{
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
		int er = 0;
		int ec = ElfRunProgram(fileName, arguments, false, false, 0, &er);
		
		if (ec != ELF_ERROR_NONE)
		{
			LogMsgNoCr("e: %s: ", fileName);
			LogMsg(ElfGetErrorMsg(ec), fileName);
		}
		
		LogMsg("");
	}
}

int ComparisonInts(const void* a, const void* b, UNUSED void* ctx)
{
	return *(const int*)a - *(const int*)b;
}

void ShellExecuteCommand(char* p, bool* pbExit)
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
		LogMsg("cat <file>   - prints the contents of a file");
		LogMsg("cls          - clear screen");
		LogMsg("cm           - character map");
		LogMsg("cd <dir>     - change directory");
		LogMsg("cfg          - list all the kernel configuration parameters");
		LogMsg("crash        - attempt to crash the kernel");
		LogMsg("color <hex>  - change the screen color");
		LogMsg("ft           - attempts to write 'Hello World from FiWrite!\\n' to a file");
		LogMsg("e <elf>      - executes an ELF from the initrd");
		LogMsg("ec <script>  - executes an NSScript file");
		LogMsg("el           - prints the last returned value from an executed ELF");
		LogMsg("exit         - exits the shell");
		LogMsg("export       - loads some configuration data from the provided arguments");
		LogMsg("hat          - performs a hex dump of a file");
		LogMsg("help         - shows this list");
		LogMsg("kill <pid>   - kill a thread with an id");
		LogMsg("image        - displays an image in the top left of the current graphics context");
		LogMsg("lf           - list debugging information about the file system");
		LogMsg("lc           - list clipboard contents");
		LogMsg("lm           - list memory allocations");
		LogMsg("lspci        - list currently installed PCI devices");
		LogMsg("lr           - list the memory ranges provided by the bootloader");
		
		//wait for new key
		LogMsg("Strike a key to print more.");
		CoGetChar();
		
		LogMsg("ls           - list the current working directory (right now just /)");
		LogMsg("lt           - list currently running threads (pauses them during the print)");
		LogMsg("movedata     - copies data from one file to another");
		LogMsg("mkdir        - creates a new empty directory");
		LogMsg("mspy         - Memory Spy! (TM)");
		LogMsg("mrd <file>   - mounts a RAM Disk from a file");
		LogMsg("ph           - prints current heap's address in kernel address space (or NULL for the default heap)");
		LogMsg("pipetest     - tests the pipe functionality");
		LogMsg("pwd          - prints the current working directory");
		LogMsg("rb <--force> - reboots the system");
		LogMsg("rm           - removes a file or link");
		LogMsg("rmdir        - removes an empty directory");
		LogMsg("rename       - renames a file or link");
		LogMsg("sysinfo      - dump system information");
		LogMsg("sysinfoa     - dump advanced system information");
		LogMsg("ta           - test ANSI escape codes");
		LogMsg("tc           - copy 'Hello, world!' to the clipboard");
		LogMsg("tm           - print the current date and time");
		LogMsg("time         - time a shell command");
		LogMsg("ver          - print system version");
		LogMsg("w            - start desktop manager");
	}
	else if (strcmp (token, "exit") == 0)
	{
		*pbExit = true;
	}
	else if (strcmp (token, "fac") == 0)
	{
		StFlushAllCaches();
	}
	else if (strcmp (token, "lh") == 0)
	{
		StDebugDumpAll();
	}
	else if (strcmp (token, "pipetest") == 0)
	{
		FsPipeTest();
	}
	else if (strcmp (token, "tm") == 0)
	{
		int e = GetEpochTime();
		TimeStruct str = *TmReadTime();//, stt;
		//GetHumanTimeFromEpoch(e, &stt);
		
		LogMsg("Current date/time:   %02d/%02d/%04d %02d:%02d:%02d", str.day,str.month,str.year,str.hours,str.minutes,str.seconds);
		//only here for debugging. Let me know if something is wrong
		//LogMsg("Epoch to human time: %02d/%02d/%04d %02d:%02d:%02d", stt.day,stt.month,stt.year,stt.hours,stt.minutes,stt.seconds);
		LogMsg("Epoch time: %d", e);
	}
	else if (strcmp (token, "ta") == 0) // Test ANSI
	{
		LogMsg(
		//"\e[15T" // Scroll 15 lines up
		"You can't see this!"
		ANSI_CLEAR_LINE_TO_BEGIN    // Erase from beginning of the line to the cursor's position.
		ANSI_GO_IN_LINE(30)         // Move cursor horizontally to x=30
		"Testing at 30 characters!"
		ANSI_GO_TO(2, 2)            // Move cursor at (1, 1) - coordinates are 1 based
		"Testing at (1, 1)!"
		"\e[94m"   // Set foreground color to bright red
		" Now in blue."
		"\e[39m"   // Reset foreground color to default
		);
	}
	else if (strcmp (token, "pwd") == 0)
	{
		LogMsg(FiGetCwd());
	}
	else if (strcmp (token, "rb") == 0)
	{
		bool force = false;
		char* fileName = Tokenize (&state, NULL, " ");
		if (fileName)
		{
			if (strcmp (fileName, "--force") == 0) force = true;
		}
		if (!IsWindowManagerRunning())
		{
			KeRestartSystem();
		}
		else if (force)
		{
			WindowManagerShutdown ();
		}
		else
			LogMsg("Use the launcher's \"Shutdown computer\" option, shut down the computer, and click \"Restart\" to reboot, or use --force.");
	}
	else if (strcmp (token, "ph") == 0)
	{
		LogMsg("Current Heap: %p", MuGetCurrentHeap());
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
			int result = FiChangeDir(fileName);
			if (result)
			{
				LogMsg("cd: %s: %s", fileName, GetErrNoString(result) );
			}
		}
	}
	else if (strcmp (token, "cm") == 0)
	{
		for (int y = 0; y < 16; y++)
			for (int x = 0; x < 16; x++)
			{
				char chr = (y<<4)|x;
				if (chr == '\n' || chr == '\b' || chr == '\e' || chr == '\r' || chr == '\0' || chr == '\1') chr = '.';
				LogMsg("\e[%d;%dH%c", x+1, y+1, chr);
			}
	}
	else if (strcmp (token, "el") == 0)
	{
		LogMsg("Last run ELF returned: %d", g_lastReturnCode);
	}
	else if (strcmp (token, "time") == 0)
	{
		int timeThen = GetTickCount();
		bool bExit = false;
		ShellExecuteCommand(state.m_pContinuation, &bExit);
		int timeNow  = GetTickCount();
		
		LogMsg("Real time: %d ms", timeNow - timeThen);
	}
	else if (strcmp (token, "e") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		ShellExecuteFile(fileName, state.m_pContinuation);
	}
	else if (strcmp (token, "ei") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName || !*fileName)
			LogMsg("ei: Expected file name");
		else
			ShellExecutableInfo(fileName);
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
			int fd = FiOpen (fileName, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("ec: %s: %s", fileName, GetErrNoString(fd));
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
	else if (strcmp (token, "image") == 0)
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
			int fd = FiOpen (fileName, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("image: %s: %s", fileName, GetErrNoString(fd));
				return;
			}
			
			int length = FiTellSize(fd);
			
			char* pData = (char*)MmAllocate(length + 1);
			pData[length] = 0;
			
			FiRead(fd, pData, length);
			
			FiClose(fd);
			
			// try to load an image
			int error = 0;
			Image* pImg;
			
			pImg = LoadImageFile(pData, length, &error);
			if (error)
			{
				// can't
				LogMsg("Could not load the image (%d).", error);
			}
			else
			{
				LogMsg("Image %dx%d", pImg->width, pImg->height);
				VidBlitImage(pImg, 0, 0);
				
				MmFree(pImg);
			}
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
			int fd = FiOpen (fileName, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("cat: %s: %s", fileName, GetErrNoString(fd));
				return;
			}
			
			FiSeek(fd, 0, SEEK_SET);
			
			int result; char data[2];
			while ((result = FiRead(fd, data, 1), result > 0))
			{
				LogMsgNoCr("%c", data[0]);
			}
			
			FiClose (fd);
			LogMsg("");
		}
	}
	else if (strcmp (token, "hat") == 0)
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
			int fd = FiOpen (fileName, O_RDONLY);
			if (fd < 0)
			{
				LogMsg("hat: %s: %s", fileName, GetErrNoString(fd));
				return;
			}
			
			FiSeek(fd, 0, SEEK_SET);
			
			int offset = 0, currentLineOffset = 0;
			
			int result; char data[2];
			char buffer[16];
			
			while ((result = FiRead(fd, data, 1), result > 0))
			{
				if (offset % 16 == 0)
				{
					currentLineOffset = offset;
					if (offset != 0)
					{
						LogMsgNoCr("   ");
						// Also print the ASCII representation of the data. We've placed that inside 'buffer'.
						for (int i = 0; i < 16; i++)
						{
							char c = buffer[i];
							if (c < 0x20 || c > 0x7E) c = '.';
							LogMsgNoCr("%c", c);
						}
						LogMsg("");
					}
					LogMsgNoCr("%x: ", offset);
				}
				
				buffer[offset % 16] = data[0];
				LogMsgNoCr("%B ", data[0]);
				
				offset++;
			}
			
			LogMsgNoCr(ANSI_GO_IN_LINE(60));
			
			for (int i = currentLineOffset; i < offset; i++)
			{
				char c = buffer[i % 16];
				if (c < 0x20 || c > 0x7E) c = '.';
				LogMsgNoCr("%c", c);
			}
			LogMsg("");
			
			FiClose (fd);
		}
	}
	else if (strcmp (token, "rm") == 0)
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
			// Get rid of the file.
			int io = FiUnlinkFile (fileName);
			if (io < 0)
			{
				LogMsg("rm: %s: %s", fileName, GetErrNoString(io));
				return;
			}
			
			LogMsg("Done");
		}
	}
	else if (strcmp (token, "rename") == 0)
	{
		char* pSrcFileName = Tokenize (&state, NULL, " ");
		if (!pSrcFileName)      goto fail_rename;
		if (*pSrcFileName == 0) goto fail_rename;
		
		char* pDstFileName = Tokenize (&state, NULL, " ");
		if (!pDstFileName)      goto fail_rename;
		if (*pDstFileName == 0) goto fail_rename;
		
		int status = FiRename(pSrcFileName, pDstFileName);
		
		if (status < 0)
		{
			LogMsg("rename: %s -> %s: %s", pSrcFileName, pDstFileName, GetErrNoString(status));
		}
		else
		{
			LogMsg("Done");
		}
		
		goto no_fail_rename;
	fail_rename:;
		LogMsg("Usage: rename <source> <destination>");
		LogMsg("Moves a file from 'source' to 'destination'. Cross file system 'rename' is not allowed for now.");
	no_fail_rename:;
	}
	else if (strcmp (token, "movedata") == 0)
	{
		LogMsg("Starting to copy...");
		char* fileNameOut = Tokenize (&state, NULL, " ");
		if (!fileNameOut)
		{
			LogMsg("Usage: movedata <output> <input>");
			goto fail_movedata;
		}
		if (*fileNameOut == 0)
		{
			LogMsg("Usage: movedata <output> <input>");
			goto fail_movedata;
		}
		
		char* fileNameIn = Tokenize (&state, NULL, " ");
		if (!fileNameIn)
		{
			LogMsg("Usage: movedata <output> <input>");
			goto fail_movedata;
		}
		if (*fileNameIn == 0)
		{
			LogMsg("Usage: movedata <output> <input>");
			goto fail_movedata;
		}
		
		int fd_in = FiOpen (fileNameIn, O_RDONLY);
		if (fd_in < 0)
		{
			LogMsg("movedata: Could not open %s for reading: %s", fileNameIn, GetErrNoString(fd_in));
			goto fail_movedata;
		}
		
		int fd_out = FiOpen (fileNameOut, O_WRONLY | O_CREAT | O_TRUNC);
		if (fd_out < 0)
		{
			LogMsg("movedata: Could not open %s for writing: %s", fileNameOut, GetErrNoString(fd_out));
			FiClose(fd_in);
			goto fail_movedata;
		}
		
		LogMsg("Progress...");
		
		
		
		uint8_t chunk_of_data[MOVEDATA_PRECISION];
		size_t sz = FiTellSize(fd_in);
		
		//write 512 byte blocks
		for (size_t i = 0; i < sz; i += MOVEDATA_PRECISION)
		{
			size_t read_in = FiRead(fd_in, chunk_of_data, MOVEDATA_PRECISION);
			if ((int)read_in < 0)
			{
				LogMsg("movedata: Could not write all %d bytes to %s - only wrote %d: %s", MOVEDATA_PRECISION, fileNameOut, read_in, GetErrNoString((int)read_in));
				FiClose(fd_in);
				FiClose(fd_out);
				goto fail_movedata;
			}
			
			FiWrite(fd_out, chunk_of_data, read_in);
			
			LogMsgNoCr("\rProgress: %d/%d", i, sz);
		}
		
		// write the last few bytes
		int last_x_block_size = (sz % MOVEDATA_PRECISION);
		for (int i = 0; i < last_x_block_size; i++)
		{
			size_t read_in = FiRead(fd_in, chunk_of_data, 1);
			if ((int)read_in < 0)
			{
				LogMsg("movedata: Could not write 1 byte to %s - only wrote %d: %s", fileNameOut, read_in, GetErrNoString((int)read_in));
				FiClose(fd_in);
				FiClose(fd_out);
				goto fail_movedata;
			}
			
			FiWrite(fd_out, chunk_of_data, read_in);
			
			LogMsgNoCr("\rProgress: %d/%d", i, sz);
		}
		
		LogMsg("");
		
		FiClose(fd_out);
		FiClose(fd_in);
		fail_movedata:;
		LogMsg("Done");
	}
	else if (strcmp (token, "rmdir") == 0)
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
			int status = FiRemoveDir(fileName);
			if (status < 0)
			{
				LogMsg("rmdir: %s: %s", fileName, GetErrNoString(status));
			}
			else
			{
				LogMsg("Done");
			}
		}
	}
	else if (strcmp (token, "mkdir") == 0)
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
			int status = FiMakeDir(fileName);
			if (status < 0)
			{
				LogMsg("mkdir: %s: %s", fileName, GetErrNoString(status));
			}
			else
			{
				LogMsg("Done");
			}
		}
	}
	else if (strcmp (token, "fts") == 0)
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
			int fd = FiOpen (fileName, O_WRONLY);
			if (fd < 0)
			{
				LogMsg("fts: %s: %s", fileName, GetErrNoString(fd));
				return;
			}
			
			FiSeek(fd, 0, SEEK_END);
			
			size_t   sz;
			void* data = SbTestGenerateSound(&sz);
			
			FiWrite(fd, data, sz);//do not also print the null terminator
			
			MmFree(data);
			
			FiClose (fd);
			LogMsg("Done");
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
			int fd = FiOpen (fileName, O_WRONLY | O_CREAT | O_APPEND);
			if (fd < 0)
			{
				LogMsg("ft: %s: %s", fileName, GetErrNoString(fd));
				return;
			}
			
			FiSeek(fd, 0, SEEK_END);
			
			const char* text; uint32_t sz;
			if (state.m_pContinuation && *state.m_pContinuation)
			{
				text = state.m_pContinuation;
				sz   = strlen(text);
			}
			else
			{
				text = "Hello World from FiWrite!\n\n\n";
				sz   = 28;
			}
			
			FiWrite(fd, (void*)text, sz);//do not also print the null terminator
			
			FiClose (fd);
			LogMsg("Done");
		}
	}
	else if (strcmp (token, "mc") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		if (!fileName)
			LogMsg("mc: expected filename");
		else if (*fileName == 0)
			LogMsg("mc: expected filename");
		else
			ShellMinicom(fileName);
	}
	else if (strcmp (token, "lr") == 0)
	{
		KePrintMemoryMapInfo();
	}
	else if (strcmp (token, "cp") == 0)
	{
		LogMsg("Starting to copy...");
		char* fileNameIn = Tokenize (&state, NULL, " ");
		if (!fileNameIn)
		{
			LogMsg("Usage: cp <input> <output>");
			return;
		}
		if (*fileNameIn == 0)
		{
			LogMsg("Usage: cp <input> <output>");
			return;
		}
		
		char* fileNameOut = Tokenize (&state, NULL, " ");
		if (!fileNameOut)
		{
			LogMsg("Usage: cp <input> <output>");
			return;
		}
		if (*fileNameOut == 0)
		{
			LogMsg("Usage: cp <input> <output>");
			return;
		}
		
		int fd_in = FiOpen (fileNameIn, O_RDONLY);
		if (fd_in < 0)
		{
			LogMsg("cp: Could not open %s for reading: %s", fileNameIn, GetErrNoString(fd_in));
			return;
		}
		
		int fd_out = FiOpen (fileNameOut, O_WRONLY | O_CREAT | O_TRUNC);
		if (fd_out < 0)
		{
			LogMsg("cp: Could not open %s for writing: %s", fileNameOut, GetErrNoString(fd_out));
			FiClose(fd_in);
			return;
		}
		
		LogMsg("Progress...");
		
		
		
		uint8_t chunk_of_data[MOVEDATA_PRECISION];
		size_t sz = FiTellSize(fd_in);
		
		//write 512 byte blocks
		for (size_t i = 0; i < sz; i += MOVEDATA_PRECISION)
		{
			size_t read_in = FiRead(fd_in, chunk_of_data, MOVEDATA_PRECISION);
			if ((int)read_in < 0)
			{
				LogMsg("cp: Could not write all %d bytes to %s - only wrote %d: %s", MOVEDATA_PRECISION, fileNameOut, read_in, GetErrNoString((int)read_in));
				FiClose(fd_in);
				FiClose(fd_out);
				return;
			}
			
			FiWrite(fd_out, chunk_of_data, read_in);
			
			LogMsgNoCr("\rProgress: %d/%d", i, sz);
		}
		
		// write the last few bytes
		int last_x_block_size = (sz % MOVEDATA_PRECISION);
		for (int i = 0; i < last_x_block_size; i++)
		{
			size_t read_in = FiRead(fd_in, chunk_of_data, 1);
			if ((int)read_in < 0)
			{
				LogMsg("cp: Could not write 1 byte to %s - only wrote %d: %s", fileNameOut, read_in, GetErrNoString((int)read_in));
				FiClose(fd_in);
				FiClose(fd_out);
				return;
			}
			
			FiWrite(fd_out, chunk_of_data, read_in);
			
			LogMsgNoCr("\rProgress: %d/%d", i, sz);
		}
		
		LogMsg("");
		
		FiClose(fd_out);
		FiClose(fd_in);
		LogMsg("Done");
	}
	
	else if (strcmp (token, "mv") == 0)
	{
		char* fileNameIn = Tokenize (&state, NULL, " ");
		if (!fileNameIn)
		{
			LogMsg("Usage: mv <origin> <dest>");
			return;
		}
		if (*fileNameIn == 0)
		{
			LogMsg("Usage: mv <origin> <dest>");
			return;
		}
		
		char* fileNameOut = Tokenize (&state, NULL, " ");
		if (!fileNameOut)
		{
			LogMsg("Usage: mv <origin> <dest>");
			return;
		}
		if (*fileNameOut == 0)
		{
			LogMsg("Usage: mv <origin> <dest>");
			return;
		}
		int result = FiRename(fileNameIn, fileNameOut);
		if(result == -ENXIO) {
			LogMsg("FiRename failed, trying move+delete");
			// Get rid of the file.
					int fd_in = FiOpen (fileNameIn, O_RDONLY);
			if (fd_in < 0)
			{
				LogMsg("mv: Could not open %s for reading: %s", fileNameIn, GetErrNoString(fd_in));
				return;
			}
			
			int fd_out = FiOpen (fileNameOut, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd_out < 0)
			{
				LogMsg("mv: Could not open %s for writing: %s", fileNameOut, GetErrNoString(fd_out));
				FiClose(fd_in);
				return;
			}
			
			LogMsg("Progress...");
			
			
			
			uint8_t chunk_of_data[MOVEDATA_PRECISION];
			size_t sz = FiTellSize(fd_in);
			
			//write 512 byte blocks
			for (size_t i = 0; i < sz; i += MOVEDATA_PRECISION)
			{
				size_t read_in = FiRead(fd_in, chunk_of_data, MOVEDATA_PRECISION);
				if ((int)read_in < 0)
				{
					LogMsg("mv: Could not write all %d bytes to %s - only wrote %d: %s", MOVEDATA_PRECISION, fileNameOut, read_in, GetErrNoString((int)read_in));
					FiClose(fd_in);
					FiClose(fd_out);
					return;
				}
				
				FiWrite(fd_out, chunk_of_data, read_in);
				
				LogMsgNoCr("\rProgress: %d/%d", i, sz);
			}
			
			// write the last few bytes
			int last_x_block_size = (sz % MOVEDATA_PRECISION);
			for (int i = 0; i < last_x_block_size; i++)
			{
				size_t read_in = FiRead(fd_in, chunk_of_data, 1);
				if ((int)read_in < 0)
				{
					LogMsg("mv: Could not write 1 byte to %s - only wrote %d: %s", fileNameOut, read_in, GetErrNoString((int)read_in));
					FiClose(fd_in);
					FiClose(fd_out);
					return;
				}
				
				FiWrite(fd_out, chunk_of_data, read_in);
				
				LogMsgNoCr("\rProgress: %d/%d", i, sz);
			}
			
			LogMsg("");
			
			FiClose(fd_out);
			FiClose(fd_in);
			int io = FiUnlinkFile (fileNameIn);
			if (io < 0)
			{
				LogMsg("mv: couldn't remove %s: %s", fileNameOut, GetErrNoString(io));
				return;
			}
		}
	}
	else if (strcmp (token, "ls") == 0)
	{
		enum
		{
			SW_UNK  = (1 << 0),
			SW_BARE = (1 << 1),
			SW_DATE = (1 << 2),
			SW_INO  = (1 << 3),
			SW_HELP = (1 << 4),
			SW_CR   = (1 << 5),
			SW_TYPE = (1 << 6),
		};
		
		int switches = 0;
		
		const char* pPathToList = FiGetCwd();
		bool bUseCWD = true;
		
		// parse other parameters if applicable
		char* parm = Tokenize (&state, NULL, " ");
		while (parm)
		{
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
						case 't': switches |= SW_TYPE; break;
						default:  switches |= SW_UNK;  break;
					}
					parm++;
				}
			}
			
			// TODO: allow listing paths other than the cwd
			
			parm = Tokenize (&state, NULL, " ");
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
			LogMsg("-t: Show file type numbers. These are specific to NanoShell.");
			return;
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
		
		int dd = FiOpenDir(bUseCWD ? "." : pPathToList);
		if (dd < 0)
		{
			LogMsg("ls: cannot list '%s': %s", pPathToList, GetErrNoString(dd));
			return;
		}
		
		LogMsg("%sDirectory of %s", normal, pPathToList);
		
		FiRewindDir(dd);
		
		DirEnt ent, *pDirEnt = &ent;
		memset(pDirEnt, 0, sizeof ent);
		int err = 0;
		
		while (!(err = FiReadDir(pDirEnt, dd)))
		{
			if (bareMode)
			{
				LogMsg("%s", pDirEnt->m_name);
				continue;
			}
			
			StatResult statResult;
			//int res = FiStatAt (dd, pDirEnt->m_name, &statResult);
			
			char path[2048];
			path[0] = 0;
			
			if (!bUseCWD)
			{
				strcpy(path, pPathToList);
				if (strcmp(path, "/"))
					strcat(path, "/");
			}
			
			strcat(path, pDirEnt->m_name);
			
			int res = FiStat(path, &statResult);
			
			if (res < 0)
			{
				LogMsg("ls: cannot stat '%s': %s", pDirEnt->m_name, GetErrNoString(res));
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
				
				buf[10] = ' ';
				
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
			
			if (switches & SW_TYPE)
			{
				char buf[20];
				
				buf[10] = ' ';
				
				sprintf( buf, " %3u ", statResult.m_type );
				
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
			LogMsg("ls: %s: %s", FiGetCwd(), GetErrNoString(err));
		}
		
		LogMsgNoCr("\x1B[0m");
		
		FiCloseDir(dd);
	}
	else if (strcmp (token, "xyzzy") == 0)
	{
		LogMsg("Huzzah!");
	}
	else if (strcmp (token, "w") == 0)
	{
		if (VidIsAvailable())
		{
			FiChangeDir("/");
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
			int fd = FiOpen (fileName, O_RDONLY);
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
	else if (strcmp (token, "cfg") == 0)
	{
		CfgPrintEntries ();
	}
	else if (strcmp (token, "export") == 0)
	{
		char *parms = state.m_pContinuation;
		if (!parms)
			LogMsg("No parms provided");
		else
			CfgLoadFromParms (parms);
	}
	else if (strcmp (token, "kill") == 0)
	{
		char* procNum = Tokenize (&state, NULL, " ");
		if (!procNum)
		{
			LogMsg( "No pid provided" );
		}
		if (*procNum == 0)
		{
			LogMsg( "No pid provided" );
		}
		
		int proc = atoi (procNum);
		
		if (KeKillThreadByPID (proc))
			LogMsg("Killed task with pid %d", proc);
		else
			LogMsg("Can't find task with pid %d", proc);
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
		
//		int nFat = atoi (fatNum);
		
//		CheckDiskFatMain (nFat);
		
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
		
		if (strcmp (secNum, "/f") == 0)
		{
			MemorySpy();
			return;
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
		
		int nAddr = atoihex (secNum);
		int nBytes= atoi (nBytesS);
		
		uint32_t* pAddr = (uint32_t*)((uintptr_t)nAddr);
		
		extern bool MmIsMapped(uintptr_t addr);
		
		DumpBytesAsHex ((void*)pAddr, nBytes, as_bytes);
		
		goto dont_print_usage;
	print_usage:
		LogMsg("Virtual Memory Spy (TM)");
		LogMsg("Usage: mspy <address hex> <numBytes> [/b]");
		LogMsg("  OR   mspy /f");
		LogMsg("- If [/f] is specified, a full-screen menu will popup");
		LogMsg("- bytes will be printed as groups of 4 unless [/b] is specified");
		LogMsg("- numBytes will be capped off at 4096 and rounded down to 32");
		LogMsg("- pageNumber must represent a VALID and MAPPED address.");
		LogMsg("- if it's not valid or mapped then the system may CRASH or HANG!");
		LogMsg("- pageNumber is in HEXADECIMAL");
	dont_print_usage:;
	}
	else if (strcmp (token, "cls") == 0)
	{
		ShellClearScreen();
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
	else if (strcmp (token, "lc") == 0)
	{
		CbDump();
	}
	else if (strcmp (token, "tc") == 0)
	{
		CbCopyText("Hello, world!");
	}
	else if (strcmp (token, "lspci") == 0)
	{
		PciDump();
	}
	else if (strcmp (token, "lt") == 0)
	{
		KeTaskDebugDump();
	}
	else if (strcmp (token, "lp") == 0)
	{
		ExProcessDebugDump();
	}
	else if (strcmp (token, "crash") == 0)
	{
		LogMsg("OK");
		*((uint32_t*)0xFFFFFFFF) = 0;
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
			
			ShellSetColor(c1 << 4 | c2);
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
		// This could be an executable
		/*
		
	else if (strcmp (token, "e") == 0)
	{
		char* fileName = Tokenize (&state, NULL, " ");
		ShellExecuteFile(fileName, state.m_pContinuation);
	}
		*/
		
		char buffer[PATH_MAX];
		// does this start with a dot or a slash?
		if (*token == '/' || *token == '.')
		{
			// we can just execute it as-is
			if (EndsWith(token, ".nse"))
				snprintf(buffer, sizeof buffer - 1, "%s", token);
			else
				snprintf(buffer, sizeof buffer - 1, "%s.nse", token);
		}
		else
		{
			// might be a thing in the 'Bin' folder
			snprintf(buffer, sizeof buffer - 1, "/Bin/%s.nse", token);
		}
		
		buffer[sizeof buffer-1] = 0;
		
		// does this file exist?
		int fd = FiOpen(buffer, O_RDONLY);
		if (fd == -ENOENT)
		{
			LogMsg("The name specified (%s) is not recognized as an internal or external\ncommand, operable program or batch file. Type 'help' for a list of commands.", buffer);
		}
		else if (fd < 0)
		{
			LogMsg("%s: %s", fd, GetErrNoString(fd));
		}
		else
		{
			// the file is there, try to run it
			FiClose(fd);
			ShellExecuteFile(buffer, state.m_pContinuation);
		}
	}
	
	//LogMsg("You typed: '%s'", p);
}

void ShellInit()
{
	//strcpy (g_cwd, "/");
	
	/*
	bool b = CbCopyText("movedata /Device/Sb16 /Fat0/sup/crap.raw\n");
	if (!b)
	{
		LogMsg("Error copying text");
	}
	*/
}

void ShellPrintMotd()
{
	if (!CfgEntryMatches("Shell::ShowMotd", "yes")) return;
	
	//TODO: Hmm, maybe we should allow multiline MOTD
	const char *pValue = CfgGetEntryValue("Shell::Motd");
	
	//bool bCenter = CfgEntryMatches ("Shell::MotdCenter", "yes");
	
	LogMsg("%s", pValue);
}

void ShellRun(UNUSED long unused_arg)
{
	LogMsgNoCr("\e]2;Command Prompt\a");
	ShellPrintMotd();
	
	bool bExit = false;
	
	while (!bExit)
	{
		//LogMsgNoCr("\e]2;Command Prompt: %s\a", FiGetCwd());
		
		LogMsgNoCr("%s>", FiGetCwd());
		char buffer[256];
		CoGetString (buffer, 256);
		memcpy (g_lastCommandExecuted, buffer, 256);
		
		//LogMsgNoCr("\e]2;Command Prompt: %s - %s\a", FiGetCwd(), buffer);
		
		ShellExecuteCommand (buffer, &bExit);
		
		//WaitMS (1);
	}
}
