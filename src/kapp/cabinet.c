/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Cabinet Application module
******************************************/

#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>
#include <elf.h>
#define CABINET_WIDTH  600
#define CABINET_HEIGHT 400

//TODO: Move this to its own space.
char g_cabinetCWD[PATH_MAX+2];

enum
{
	ZERO,
	MAIN_LISTVIEW,
	MAIN_MENU_BAR,
};
enum
{
	                     MENU$=0,
	//  +-------------------+-------------------+
	//  |                   |                   |
	MENU$FILE,          MENU$VIEW,          MENU$HELP,
	//  |                   |                   |
	//  v                   v                   v
	MENU$FILE$ROOT, MENU$VIEW$REFRESH,   MENU$HELP$ABOUT,
	MENU$FILE$EXIT, MENU$VIEW$CHVWMOD,
	
};

void UpdateDirectoryListing (Window* pWindow)
{
	ResetList        (pWindow, MAIN_LISTVIEW);
	AddElementToList (pWindow, MAIN_LISTVIEW, "..", ICON_FOLDER_PARENT);
	FileNode *pFolderNode = FsResolvePath (g_cabinetCWD);
	
	DirEnt* pEnt = NULL;
	int i = 0;
	
	while ((pEnt = FsReadDir (pFolderNode, i)) != 0)
	{
		FileNode* pNode = FsFindDir (pFolderNode, pEnt->m_name);
		
		if (!pNode)
		{
			AddElementToList (pWindow, MAIN_LISTVIEW, "<a NULL directory entry>", ICON_ERROR);
		}
		else
		{
			IconType icon = ICON_FILE;
			if (pNode->m_type & FILE_TYPE_DIRECTORY)
			{
				icon = ICON_FOLDER;
			}
			else if (EndsWith (pNode->m_name, ".nse"))
			{
				icon = ICON_EXECUTE_FILE;
			}
			else if (EndsWith (pNode->m_name, ".txt"))
			{
				icon = ICON_TEXT_FILE;
			}
			AddElementToList (pWindow, MAIN_LISTVIEW, pNode->m_name, icon);
			i++;
		}
	}
	
	RequestRepaint(pWindow);
}

char g_cabinetExecutableToExecute[PATH_MAX+4];

//TODO FIXME: Spawn a console window if the ELF file is not marked as using the GUI.
void LaunchExecutable (int argument)
{
	//The argument is assumed to point to a valid const char*.
	cli;
	const char* pFileNameUnsafe = (const char*)argument;
	char filename[1024];
	
	strcpy (filename, g_cabinetCWD);
	if (g_cabinetCWD[1] != 0)
		strcat (filename, "/");
	strcat (filename, pFileNameUnsafe);
	sti;
	
	KeTaskAssignTag(KeGetRunningTask(), filename);
	
	int fd = FiOpen (filename, O_RDONLY | O_EXEC);
	if (fd < 0)
	{
		//LogMsg("Got error %d while trying to open %s", fd, filename);
		char buffer[1024];
		sprintf (buffer, "Got error %d while trying to open %s", fd, filename);
		MessageBox(NULL, buffer, "File cabinet", MB_OK | ICON_ERROR << 16);
		return;
	}
	
	int length = FiTellSize (fd);
	//LogMsg("File Length: %d", length);
	char* pData = (char*)MmAllocate(length + 1);
	pData[length] = 0;
	
	FiRead(fd, pData, length);
	
	FiClose (fd);
	
	//LogMsg("Executing...");
	ElfExecute(pData, length);
	
	MmFree(pData);
}

//TODO FIXME
void CdBack(Window* pWindow)
{
	for (int i = PATH_MAX - 1; i >= 0; i--)
	{
		if (g_cabinetCWD[i] == PATH_SEP)
		{
			bool o = i == 0;
			g_cabinetCWD[i+o] = 0;
			FileNode* checkNode = FsResolvePath(g_cabinetCWD);
			if (!checkNode)
			{
				MessageBox(pWindow, "Cannot find parent directory.\n\nSince we can't go back, just exit.", pWindow->m_title, ICON_STOP << 16 | MB_OK);
				
				//fake a destroy call
				DestroyWindow(pWindow);
				return;
			}
			UpdateDirectoryListing (pWindow);
		}
	}
}

void CALLBACK CabinetWindowProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			VidTextOut (g_cabinetCWD, 8, 15 + TITLE_BAR_HEIGHT*2, 0, TRANSPARENT);
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == MAIN_LISTVIEW)
			{
				FileNode *pFolderNode = FsResolvePath (g_cabinetCWD);
				const char* pFileName = GetElementStringFromList (pWindow, parm1, parm2);
				//LogMsg("Double clicked element: %s", pFileName);
				
				FileNode* pFileNode = FsFindDir	(pFolderNode, pFileName);
				if (strcmp (pFileName, PATH_PARENTDIR) == 0)
				{
					CdBack(pWindow);
					break;
				}
				else if (pFileNode)
				{
					if (pFileNode->m_type & FILE_TYPE_DIRECTORY)
					{
						// Is a directory.  Navigate to it.
						char cwd_copy[sizeof(g_cabinetCWD)];
						memcpy(cwd_copy, g_cabinetCWD, sizeof(g_cabinetCWD));
						if (g_cabinetCWD[1] != 0)
							strcat (g_cabinetCWD, "/");
						strcat (g_cabinetCWD, pFileName);
						
						FileNode *pCurrent = FsResolvePath (g_cabinetCWD);
						if (!pCurrent)
						{
							memcpy(g_cabinetCWD, cwd_copy, sizeof(g_cabinetCWD));
							char buffer [256];
							sprintf (buffer, "Cannot find directory '%s'.  It may have been moved or deleted.\n\nTry clicking the 'Refresh' button in the top bar.", pFileName);
							MessageBox(pWindow, buffer, pWindow->m_title, ICON_ERROR << 16 | MB_OK);
						}
						else
							UpdateDirectoryListing (pWindow);
					}
					else if (EndsWith (pFileName, ".nse"))
					{
						// Executing file.  Might want to MessageBox the user about it?
						char buffer[256];
						sprintf(buffer, "This executable file might be unsafe for you to run.\n\nWould you like to run '%s' anyway?", pFileName);
						if (MessageBox (pWindow, buffer, pWindow->m_title, ICON_EXECUTE_FILE << 16 | MB_YESNO) == MBID_YES)
						{
							int errorCode = 0;
							strcpy (g_cabinetExecutableToExecute, pFileName);
							KeStartTask (LaunchExecutable, (int)g_cabinetExecutableToExecute, &errorCode);
							
							if (errorCode != TASK_SUCCESS)
							{
								sprintf (buffer, "Can not create thread to execute '%s'. Out of memory?", pFileName);
								MessageBox(pWindow, buffer, pWindow->m_title, ICON_STOP << 16 | MB_OK);
							}
						}
					}
					/*else if (EndsWith (pFileName, ".txt"))
					{
						// TODO!
					}*/
					else
					{
						char buffer[256];
						sprintf (buffer, "Don't know how to open '%s'.", pFileName);
						MessageBox(pWindow, buffer, pWindow->m_title, ICON_STOP << 16 | MB_OK);
					}
				}
				else
				{
					char buffer [256];
					sprintf (buffer, "Cannot find file '%s'.  It may have been moved or deleted.\n\nTry clicking the 'Refresh' button in the top bar.", pFileName);
					MessageBox(pWindow, buffer, pWindow->m_title, ICON_ERROR << 16 | MB_OK);
				}
			}
			else if (parm1 == MAIN_MENU_BAR)
			{
				switch (parm2)
				{
					case MENU$FILE$EXIT:
						//Exit.
						DestroyWindow (pWindow);
						break;
					case MENU$FILE$ROOT:
						strcpy (g_cabinetCWD, "/");
						UpdateDirectoryListing(pWindow);
						break;
					case MENU$VIEW$REFRESH:
						UpdateDirectoryListing(pWindow);
						break;
					case MENU$VIEW$CHVWMOD:
						//TODO
						//MessageBox(pWindow, "Not Implemented!", "File Cabinet", MB_OK|ICON_HELP<<16);
						break;
					case MENU$HELP$ABOUT:
						LaunchVersion();
						break;
				}
			}
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             36
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ CABINET_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ CABINET_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			AddControl (pWindow, CONTROL_ICONVIEW, r, NULL, MAIN_LISTVIEW, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR,  r, NULL, MAIN_MENU_BAR, 0, 0);
			
			// Initialize the menu-bar
			AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$, MENU$FILE, "File");
			AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$, MENU$VIEW, "View");
			AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$, MENU$HELP, "Help");
			
			// File:
			{
				AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$FILE, MENU$FILE$ROOT, "Back to root");
				AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$FILE, MENU$FILE$EXIT, "Exit");
			}
			// View:
			{
				AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$VIEW, MENU$VIEW$REFRESH, "Refresh");
				AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$VIEW, MENU$VIEW$CHVWMOD, "Change list view mode");
			}
			// Help:
			{
				AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$HELP, MENU$HELP$ABOUT, "About File Cabinet");
			}
			
			strcpy (g_cabinetCWD, "/");
			
			UpdateDirectoryListing (pWindow);
			
			break;
		}
		//TODO: Fix crash when shutting down cabinet?
		//TODO: SysMon crashes too? (c0103389)  Perhaps it's a widget dispose bug? (they both have listview widgets)
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void CabinetEntry (__attribute__((unused)) int argument)
{
	// create ourself a window:
	int xPos = (GetScreenSizeX() - CABINET_WIDTH)  / 2;
	int yPos = (GetScreenSizeY() - CABINET_HEIGHT) / 2;
	Window* pWindow = CreateWindow ("Cabinet", xPos, yPos, CABINET_WIDTH, CABINET_HEIGHT, CabinetWindowProc, 0);
	
	if (!pWindow)
	{
		MessageBox(NULL, "Hey, the window couldn't be created. File " __FILE__ ".", "Cabinet", MB_OK | ICON_STOP << 16);
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
