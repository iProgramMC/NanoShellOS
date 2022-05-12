/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Cabinet Application module
******************************************/

#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>
#include <elf.h>
#include <wterm.h>
#include <resource.h>
#define CABINET_WIDTH  600
#define CABINET_HEIGHT 400

//TODO: Move this to its own space.
typedef struct
{
char m_cabinetCWD[PATH_MAX+2];
char m_cbntOldCWD[PATH_MAX+2];
}
CabData;

#define g_cabinetCWD (((CabData*)pWindow->m_data)->m_cabinetCWD)
#define g_cbntOldCWD (((CabData*)pWindow->m_data)->m_cbntOldCWD)

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
	MENU$FILE$MRD,
	
};

void CabinetMountRamDisk(Window *pwnd, const char *pfn)
{
	int fd = FiOpen (pfn, O_RDONLY);
	if (fd < 0)
	{
		MessageBox(pwnd, "Could not open that file!  Try another.", "Mount error", MB_OK | ICON_ERROR << 16);
	}
	else
	{
		int length = FiTellSize(fd);
		
		char* pData = (char*)MmAllocate(length + 1);
		pData[length] = 0;
		
		FiRead(fd, pData, length);
		
		FiClose(fd);
		
		cli;
		FsMountRamDisk(pData);
		sti;
	}
}

IconType CabGetIconBasedOnName(const char *pName, int pType)
{
	IconType icon = ICON_FILE;
	if (pType & FILE_TYPE_MOUNTPOINT)
	{
		icon = ICON_HARD_DRIVE; //- or ICON_DEVICE_BLOCK
	}
	else if (pType & FILE_TYPE_DIRECTORY)
	{
		icon = ICON_FOLDER;
	}
	else if (pType == FILE_TYPE_CHAR_DEVICE)
	{
		if (StartsWith (pName, "Com"))
			icon = ICON_SERIAL;
		else
			icon = ICON_DEVICE_CHAR;
	}
	else if (EndsWith (pName, ".nse"))
	{
		//icon = ICON_EXECUTE_FILE;
		icon = ICON_APPLICATION;
	}
	else if (EndsWith (pName, ".c"))
	{
		icon = ICON_FILE_CSCRIPT;
	}
	else if (EndsWith (pName, ".txt"))
	{
		icon = ICON_TEXT_FILE;
	}
	else if (EndsWith (pName, ".md"))
	{
		icon = ICON_FILE_MKDOWN;
	}
	else if (EndsWith (pName, ".tar") || EndsWith (pName, ".mrd"))
	{
		icon = ICON_TAR_ARCHIVE;//ICON_CABINET_COMBINE;
	}
	return icon;
}

void UpdateDirectoryListing (Window* pWindow)
{
reset:
	ResetList        (pWindow, MAIN_LISTVIEW);
	
	if (strcmp (g_cabinetCWD, "/")) //if can go to parent, add a button
		AddElementToList (pWindow, MAIN_LISTVIEW, "..", ICON_FOLDER_PARENT);
	
	FileNode *pFolderNode = FsResolvePath (g_cabinetCWD);
	
	DirEnt* pEnt = NULL;
	int i = 0;
	
	if (!FsOpenDir(pFolderNode))
	{
		MessageBox(pWindow, "Could not load directory, taking you back to root.", "Cabinet", MB_OK | ICON_WARNING << 16);
		strcpy (g_cabinetCWD, "/");
		goto reset;
	}
	
	while ((pEnt = FsReadDir (pFolderNode, i)) != 0)
	{
		FileNode* pNode = FsFindDir (pFolderNode, pEnt->m_name);
		
		if (!pNode)
		{
			AddElementToList (pWindow, MAIN_LISTVIEW, "<a NULL directory entry>", ICON_ERROR);
		}
		else
		{
			AddElementToList (pWindow, MAIN_LISTVIEW, pNode->m_name, CabGetIconBasedOnName(pNode->m_name, pNode->m_type));
			i++;
		}
	}
	
	int icon = CabGetIconBasedOnName(pFolderNode->m_name, pFolderNode->m_type);
	//SetWindowIcon (pWindow, icon);
	//SetWindowTitle(pWindow, pFolderNode->m_name);
	pWindow->m_iconID = icon;
	strcpy (pWindow->m_title, "Cabinet - ");
	strcat (pWindow->m_title, pFolderNode->m_name); //note: WINDOW_TITLE_MAX is 250, but file names are 127 max. 
	
	RequestRepaint(pWindow);
}

//TODO FIXME
void CdBack(Window* pWindow)
{
	for (int i = PATH_MAX - 1; i >= 0; i--)
	{
		if (g_cabinetCWD[i] == PATH_SEP)
		{
			g_cabinetCWD[i+(i == 0)] = 0;
			FileNode* checkNode = FsResolvePath(g_cabinetCWD);
			if (!checkNode)
			{
				MessageBox(pWindow, "Cannot find parent directory.\n\nGoing back to root.", pWindow->m_title, ICON_ERROR << 16 | MB_OK);
				
				strcpy (g_cabinetCWD, "/");
				return;
			}
			UpdateDirectoryListing (pWindow);
			break;
		}
	}
}

void CALLBACK CabinetMountWindowProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			
			RECT(r, 10, 10 + TITLE_BAR_HEIGHT, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, 1, ICON_CABINET_COMBINE, 0);
			
			RECT(r, 50, 15 + TITLE_BAR_HEIGHT, 150, 32);
			AddControl(pWindow, CONTROL_TEXT, r, "Type in the file name of a drive you want to mount.", 3, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECT(r, 450 - 80, 10 + TITLE_BAR_HEIGHT, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Mount", 2, 0, 0);
			RECT(r, 450 - 80, 40 + TITLE_BAR_HEIGHT, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", 5, 0, 0);
			
			RECT(r, 50, 30 + TITLE_BAR_HEIGHT,300, 20);
			AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, 4, 0, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == 2)
			{
				//Mount something
				
				const char* s = TextInputGetRawText(pWindow, 4);
				char buffer[2048];
				sprintf(buffer, "Would you like to mount the file '%s' as a read-only file system?", s);
				if (MessageBox (pWindow, buffer, pWindow->m_title, ICON_CABINET_COMBINE << 16 | MB_YESNO) == MBID_YES)
				{
					OnBusy (pWindow);
					CabinetMountRamDisk(pWindow, s);
					OnNotBusy(pWindow);
				}
			}
			if (parm1 == 2 || parm1 == 5)
				DestroyWindow(pWindow);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void PopupUserMountWindow(Window* pWindow)
{
	PopupWindow(pWindow, "Mount a RAM drive", pWindow->m_rect.left + 50, pWindow->m_rect.top + 50, 450, 90, CabinetMountWindowProc, WF_NOCLOSE | WF_NOMINIMZ);
}

void CALLBACK CabinetWindowProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			VidTextOut (g_cbntOldCWD, 8, 15 + TITLE_BAR_HEIGHT*2, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);
			VidTextOut (g_cabinetCWD, 8, 15 + TITLE_BAR_HEIGHT*2,  WINDOW_TEXT_COLOR , WINDOW_BACKGD_COLOR);
			strcpy(g_cbntOldCWD, g_cabinetCWD);
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
						OnBusy(pWindow);
						
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
							MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
						}
						else
							UpdateDirectoryListing (pWindow);
						
						OnNotBusy(pWindow);
					}
					else if (EndsWith (pFileName, ".nse"))
					{
						// Executing file.  Might want to MessageBox the user about it?
						char buffer[512];
						sprintf(buffer, "This executable file might be unsafe for you to run.\n\nWould you like to run '%s' anyway?", pFileName);
						if (MessageBox (pWindow, buffer, "Warning", ICON_INFO << 16 | MB_YESNO) == MBID_YES)
						{
							OnBusy(pWindow);
							
							// Get the file name.
							char filename[1024];
							strcpy (filename, "exwindow:");
							strcat (filename, g_cabinetCWD);
							if (g_cabinetCWD[1] != 0)
								strcat (filename, "/");
							strcat (filename, pFileName);
							//CabinetExecute(pWindow, filename);
							RESOURCE_STATUS status = LaunchResource(filename);
							SLogMsg("Resource launch status: %x", status);
							
							OnNotBusy(pWindow);
						}
					}
					else if (EndsWith (pFileName, ".c"))
					{
						// Executing file.  Might want to MessageBox the user about it?
						char buffer[512];
						sprintf(buffer, "This script file might be unsafe for you to run.\n\nWould you like to run the NanoShell script '%s'?", pFileName);
						if (MessageBox (pWindow, buffer, "Warning", ICON_INFO << 16 | MB_YESNO) == MBID_YES)
						{
							OnBusy(pWindow);
							
							// Get the file name.
							char filename[1024];
							strcpy (filename, "exscript:");
							strcat (filename, g_cabinetCWD);
							if (g_cabinetCWD[1] != 0)
								strcat (filename, "/");
							strcat (filename, pFileName);
							//CabinetExecute(pWindow, filename);
							RESOURCE_STATUS status = LaunchResource(filename);
							SLogMsg("Resource launch status: %x", status);
							
							OnNotBusy(pWindow);
						}
					}
					else if (EndsWith (pFileName, ".txt"))
					{
						OnBusy(pWindow);
						
						// Get the file name.
						char filename[1024];
						strcpy (filename, "ted:");
						strcat (filename, g_cabinetCWD);
						if (g_cabinetCWD[1] != 0)
							strcat (filename, "/");
						strcat (filename, pFileName);
						//CabinetExecute(pWindow, filename);
						RESOURCE_STATUS status = LaunchResource(filename);
						SLogMsg("Resource launch status: %x", status);
						
						OnNotBusy(pWindow);
					}
					else if (EndsWith (pFileName, ".tar") || EndsWith (pFileName, ".mrd"))
					{
						// Executing file.  Might want to MessageBox the user about it?
						char buffer[512];
						sprintf(buffer, "Would you like to mount the file '%s' as a read-only file system?", pFileName);
						if (MessageBox (pWindow, buffer, pWindow->m_title, ICON_CABINET_COMBINE << 16 | MB_YESNO) == MBID_YES)
						{
							OnBusy(pWindow);
							
							char filename[1024];
							strcpy (filename, g_cabinetCWD);
							if (g_cabinetCWD[1] != 0)
								strcat (filename, "/");
							strcat (filename, pFileName);
							
							CabinetMountRamDisk(pWindow, filename);
							
							OnNotBusy(pWindow);
						}
					}
					else if (EndsWith (pFileName, ".md"))
					{
						OnBusy(pWindow);
						
						// Get the file name.
						char filename[1024];
						strcpy (filename, "help:");
						strcat (filename, g_cabinetCWD);
						if (g_cabinetCWD[1] != 0)
							strcat (filename, "/");
						strcat (filename, pFileName);
						//CabinetExecute(pWindow, filename);
						RESOURCE_STATUS status = LaunchResource(filename);
						SLogMsg("Resource launch status: %x", status);
						
						OnNotBusy(pWindow);
					}
					else
					{
						char buffer[256];
						sprintf (buffer, "Don't know how to open '%s'.", pFileName);
						MessageBox(pWindow, buffer, "Error", ICON_INFO << 16 | MB_OK);//ICON_QUESTION
					}
				}
				else
				{
					char buffer [256];
					sprintf (buffer, "Cannot find file '%s'.  It may have been moved or deleted.\n\nTry clicking the 'Refresh' button in the top bar.", pFileName);
					MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
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
					case MENU$FILE$MRD:
						PopupUserMountWindow(pWindow);
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
			strcpy (g_cbntOldCWD, "");
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
			
			AddControl (pWindow, CONTROL_ICONVIEWDRAG, r, NULL, MAIN_LISTVIEW, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR,  r, NULL, MAIN_MENU_BAR, 0, 0);
			
			// Initialize the menu-bar
			AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$, MENU$FILE, "File");
			AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$, MENU$VIEW, "View");
			AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$, MENU$HELP, "Help");
			
			// File:
			{
				AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$FILE, MENU$FILE$MRD,  "Mount a ram-drive");
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
		case EVENT_DESTROY:
		{
			if (pWindow->m_data)
			{
				MmFree(pWindow->m_data);
				pWindow->m_data = NULL;
			}
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
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
	pWindow->m_iconID = ICON_CABINET;
	
	if (!pWindow)
	{
		MessageBox(NULL, "Hey, the window couldn't be created. File " __FILE__ ".", "Cabinet", MB_OK | ICON_STOP << 16);
		return;
	}
	
	pWindow->m_data = MmAllocate(sizeof(CabData));
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
