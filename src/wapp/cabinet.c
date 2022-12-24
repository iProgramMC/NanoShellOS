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

#define TABLE_COLUMNS (2)

#define COOLBAR_BUTTON_HEIGHT (TITLE_BAR_HEIGHT - 6 + 8)

//TODO: Move this to its own space.
typedef struct
{
char m_cabinetCWD[PATH_MAX+2];
char m_cbntOldCWD[PATH_MAX+2];
bool m_bUsingTableView;
}
CabData;

#define g_cabinetCWD (((CabData*)pWindow->m_data)->m_cabinetCWD)
#define g_cbntOldCWD (((CabData*)pWindow->m_data)->m_cbntOldCWD)
#define g_bUsingTableView (((CabData*)pWindow->m_data)->m_bUsingTableView)

enum
{
	ZERO,
	MAIN_LISTVIEW,
	MAIN_MENU_BAR,
	MAIN_PATH_TEXT,
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
	
	//TODO: implement functionality for most of these
	//the coolbar is still in its planning phase :)
	CB$BACK,      //
	CB$FWD,       //
	CB$PROPERTIES,//pops out a 'properties' dialog
	CB$SEARCH,    //allows to search rescursively in a directory
	CB$COPY,      //copies a file path name to the clipboard
	CB$PASTE,     //
	CB$UNDO,      //
	CB$REDO,      //
	CB$VIEWICON,  //
	CB$VIEWLIST,  //
	CB$VIEWTABLE, //
	CB$WHATSTHIS, //pops out 'help' at, say, <root>/Help/Cabinet.md?
	CB$PACKAGER,  //zip archive?
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
		
		KeVerifyInterruptsEnabled;
		cli;
		
		FsMountRamDisk(pData);
		
		KeVerifyInterruptsDisabled;
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
	else if (EndsWith (pName, ".tga") || EndsWith (pName, ".bmp"))
	{
		icon = ICON_FILE_IMAGE;
	}
	else if (EndsWith (pName, ".tar") || EndsWith (pName, ".mrd"))
	{
		icon = ICON_TAR_ARCHIVE;//ICON_CABINET_COMBINE;
	}
	return icon;
}

void RequestTaskbarUpdate();

bool WidgetListView_OnEvent(Control* this, int eventType, int parm1, int parm2, Window* pWindow);
bool WidgetIconViewDrag_OnEvent(Control* this, int eventType, int parm1, int parm2, Window* pWindow);
void WidgetIconViewDrag_ArrangeIcons (Control *this);

static void CreateListView(Window* pWindow);
static void UpdateDirectoryListing (Window* pWindow);

static void ClearFileListing(Window * pWindow)
{
	if (g_bUsingTableView)
	{
		ResetTable(pWindow, MAIN_LISTVIEW);
		
		AddTableColumn(pWindow, MAIN_LISTVIEW, "Name", 200);
		AddTableColumn(pWindow, MAIN_LISTVIEW, "Size", 100);
	}
	else
	{
		ResetList (pWindow, MAIN_LISTVIEW);
	}
}

static void ChangeListViewMode(Window* pWindow)
{
	g_bUsingTableView ^= 1;
	RemoveControl(pWindow, MAIN_LISTVIEW);
	CreateListView(pWindow);
	ClearFileListing(pWindow);
	UpdateDirectoryListing(pWindow);
	RequestRepaintNew(pWindow);
}

static void FormatSize(uint32_t size, char* size_buf)
{
	uint32_t kb = size / 1024;
	if (size == ~0u)
		size_buf[0] = 0;
	else if (size < 1000)
		sprintf(size_buf, "%d B", size);
	else if (size < 1024)
		sprintf(size_buf, "%d,%03d B", size / 1000, size % 1000);
	else if (kb < 1000)
		sprintf(size_buf, "%d KB", kb);
	else if (kb < 1000000)
		sprintf(size_buf, "%d,%03d KB", kb / 1000, kb % 1000);
	else
		sprintf(size_buf, "%d,%03d,%03d KB", kb / 1000000, kb / 1000 % 1000, kb % 1000);
}

// size = -1, means don't show anything to the file
static void AddFileElementToList(Window* pWindow, const char * text, int icon, uint32_t file_size)
{
	// note: this is real crap
	if (g_bUsingTableView)
	{
		char size_buf[16];
		FormatSize(file_size, size_buf);
		
		const char* table[TABLE_COLUMNS] = { 0 };
		table[0] = text;
		table[1] = size_buf;
		
		AddTableRow(pWindow, MAIN_LISTVIEW, table, icon);
	}
	else
	{
		AddElementToList(pWindow, MAIN_LISTVIEW, text, icon);
	}
}

static void UpdateDirectoryListing (Window* pWindow)
{
reset:
	ClearFileListing(pWindow);
	
	if (strcmp (g_cabinetCWD, "/")) //if can go to parent, add a button
	{
		AddFileElementToList(pWindow, "..", ICON_FOLDER_PARENT, -1);
	}
	
	FileNode *pFolderNode = FsResolvePath (g_cabinetCWD);
	
	DirEnt* pEnt = NULL;
	uint32_t i = 0;
	
	if (!FsOpenDir(pFolderNode))
	{
		MessageBox(pWindow, "Could not load directory, taking you back to root.", "Cabinet", MB_OK | ICON_WARNING << 16);
		strcpy (g_cabinetCWD, "/");
		goto reset;
	}
	
	DirEnt ent;
	while ((pEnt = FsReadDir (pFolderNode, &i, &ent)) != 0)
	{
		FileNode* pNode = FsFindDir (pFolderNode, pEnt->m_name);
		
		if (!pNode)
		{
			AddFileElementToList(pWindow, "<a NULL directory entry>", ICON_ERROR, -1);
		}
		else
		{
			AddFileElementToList(pWindow, pNode->m_name, CabGetIconBasedOnName(pNode->m_name, pNode->m_type), pNode->m_type != FILE_TYPE_FILE ? (-1) : pNode->m_length);
			
			FsReleaseReference(pNode);
		}
	}
	
	int icon = CabGetIconBasedOnName(pFolderNode->m_name, pFolderNode->m_type);
	//SetWindowIcon (pWindow, icon);
	//SetWindowTitle(pWindow, pFolderNode->m_name);
	pWindow->m_iconID = icon;
	strcpy (pWindow->m_title, "Cabinet - ");
	strcat (pWindow->m_title, pFolderNode->m_name); //note: WINDOW_TITLE_MAX is 250, but file names are 127 max. 
	RequestTaskbarUpdate();
	
	SetLabelText(pWindow, MAIN_PATH_TEXT, g_cabinetCWD);
	
	FsReleaseReference(pFolderNode);
	
	//RequestRepaint(pWindow);
	CallControlCallback(pWindow, MAIN_LISTVIEW,  EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, MAIN_PATH_TEXT, EVENT_PAINT, 0, 0);
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
			FsReleaseReference(checkNode);
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
	PopupWindow(pWindow, "Mount a RAM drive", pWindow->m_rect.left + 50, pWindow->m_rect.top + 50, 450, 90-18+TITLE_BAR_HEIGHT, CabinetMountWindowProc, WF_NOCLOSE | WF_NOMINIMZ);
}

void CabinetDetermineResourceLaunchFailure(Window* pWindow, RESOURCE_STATUS status, const char* context, const char* filename)
{
	if (status == RESOURCE_LAUNCH_SUCCESS) return;
	
	char buffer[1024], buffes[1024];
	ASSERT(strlen(filename) < 512);
	
	sprintf(buffes, GetResourceErrorText(status), filename);
	sprintf(buffer, "The %s '%s' could not be loaded.\n\n%s", context, filename, buffes);
	
	MessageBox(pWindow, buffer, "Cabinet - Error Opening File", ICON_ERROR << 16 | MB_OK);
}

static const char * GetFileNameFromList(Window* pWindow, int index)
{
	if (g_bUsingTableView)
	{
		const char * table[TABLE_COLUMNS];
		
		if (!GetRowStringsFromTable(pWindow, MAIN_LISTVIEW, index, table)) return NULL;
		
		return table[0];
	}
	else
	{
		return GetElementStringFromList(pWindow, MAIN_LISTVIEW, index);
	}
}

void CALLBACK CabinetWindowProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			/*
			VidTextOut (g_cbntOldCWD, 8, 10 + TITLE_BAR_HEIGHT*2 + 28, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);
			VidTextOut (g_cabinetCWD, 8, 10 + TITLE_BAR_HEIGHT*2 + 28,  WINDOW_TEXT_COLOR , WINDOW_BACKGD_COLOR);
			strcpy(g_cbntOldCWD, g_cabinetCWD);
			*/
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == MAIN_LISTVIEW)
			{
				FileNode *pFolderNode = FsResolvePath (g_cabinetCWD);
				const char* pFileName = GetFileNameFromList(pWindow, parm2);//GetElementStringFromList (pWindow, parm1, parm2);
				//LogMsg("Double clicked element: %s", pFileName);
				
				if (strcmp (pFileName, PATH_PARENTDIR) == 0)
				{
					CdBack(pWindow);
					break;
				}
				else
				{
					FileNode* pFileNode = FsFindDir	(pFolderNode, pFileName);
					if (pFileNode)
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
							{
								UpdateDirectoryListing (pWindow);
								FsReleaseReference(pCurrent);
							}
							
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
								CabinetDetermineResourceLaunchFailure(pWindow, status, "executable file", filename);
								
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
								CabinetDetermineResourceLaunchFailure(pWindow, status, "script file", filename);
								
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
							CabinetDetermineResourceLaunchFailure(pWindow, status, "text file", filename);
							
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
							CabinetDetermineResourceLaunchFailure(pWindow, status, "Markdown help file", filename);
							
							OnNotBusy(pWindow);
						}
						else if (EndsWith (pFileName, ".tga") || EndsWith (pFileName, ".bmp"))
						{
							OnBusy(pWindow);
							
							// Get the file name.
							char filename[1024];
							strcpy (filename, "image:");
							strcat (filename, g_cabinetCWD);
							if (g_cabinetCWD[1] != 0)
								strcat (filename, "/");
							strcat (filename, pFileName);
							//CabinetExecute(pWindow, filename);
							RESOURCE_STATUS status = LaunchResource(filename);
							CabinetDetermineResourceLaunchFailure(pWindow, status, "image", filename);
							
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
					
					FsReleaseReference(pFileNode);
				}
				
				FsReleaseReference(pFolderNode);
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
						ChangeListViewMode(pWindow);
						break;
					case MENU$FILE$MRD:
						PopupUserMountWindow(pWindow);
						break;
					case MENU$HELP$ABOUT:
						LaunchVersion();
						break;
				}
			}
			else switch (parm1)
			{
				case CB$VIEWICON:
					if (!g_bUsingTableView) break;
					ChangeListViewMode(pWindow);
					break;
				case CB$VIEWTABLE:
					if (g_bUsingTableView) break;
					ChangeListViewMode(pWindow);
					break;
				default:
					MessageBox(pWindow, "Not implemented!  Check back later or something", "Cabinet", MB_OK | ICON_INFO << 16);
			}
			break;
		}
		case EVENT_CREATE:
		{
			strcpy (g_cbntOldCWD, "");
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             (TITLE_BAR_HEIGHT+18+COOLBAR_BUTTON_HEIGHT)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
				/*X Size */ CABINET_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ CABINET_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
			);
			
			CreateListView(pWindow);
			
			AddControlEx (pWindow, CONTROL_MENUBAR, ANCHOR_RIGHT_TO_RIGHT, r, NULL, MAIN_MENU_BAR, 0, 0);
			
			r.top -= 14;
			r.bottom = r.top + GetLineHeight();
			AddControl (pWindow, CONTROL_TEXTCENTER, r, "", MAIN_PATH_TEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			
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
			
			// Add the cool bar widgets
			int i = 0;
			int button_icons[] = {
				ICON_BACK, ICON_FORWARD,
				-1,
				ICON_COPY, ICON_PASTE,
				-1,
				ICON_UNDO, ICON_REDO,
				-1,
				ICON_VIEW_ICON, ICON_VIEW_TABLE,
				-1,
				ICON_PROPERTIES, ICON_FILE_SEARCH,
				ICON_WHATS_THIS, ICON_PACKAGER,
			};
			// TODO
			int button_actions[] = {
				CB$BACK, CB$FWD,
				-1,
				CB$COPY, CB$PASTE,
				-1,
				CB$UNDO, CB$REDO, 
				-1,
				CB$VIEWICON, CB$VIEWTABLE,
				-1,
				CB$PROPERTIES, CB$SEARCH,
				CB$WHATSTHIS, CB$PACKAGER,
			};
			int x_pos = PADDING_AROUND_LISTVIEW;
			for (i = 0; i < (int)ARRAY_COUNT(button_icons); i++)
			{
				if (button_icons[i] == 0)
					continue; // none
				if (button_icons[i] == -1)
				{
					RECT(r, x_pos, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT * 2, 5, COOLBAR_BUTTON_HEIGHT);
					//add a simple vertical line
					AddControl(pWindow, CONTROL_SIMPLE_VLINE, r, NULL, 0, 0, 0);
					x_pos += 5;
				}
				else
				{
					RECT(r, x_pos, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT * 2, COOLBAR_BUTTON_HEIGHT, COOLBAR_BUTTON_HEIGHT);
					AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, button_actions[i], button_icons[i], COOLBAR_BUTTON_HEIGHT > 36 ? 32 : 16);
					
					x_pos += (COOLBAR_BUTTON_HEIGHT + 2);
				}
			}
			
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

static void CreateListView(Window* pWindow)
{
	int CabinetWidth  = pWindow->m_rect.right - pWindow->m_rect.left;
	int CabinetHeight = pWindow->m_rect.bottom- pWindow->m_rect.top;
	
	Rectangle r;
	if (g_bUsingTableView)
	{
		RECT(r, 
			/*X Coord*/ PADDING_AROUND_LISTVIEW, 
			/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
			/*X Size */ CabinetWidth - PADDING_AROUND_LISTVIEW * 2, 
			/*Y Size */ CabinetHeight- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
		);
		
		AddControlEx (pWindow, CONTROL_TABLEVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, MAIN_LISTVIEW, 0, 0);
	}
	else
	{
		RECT(r, 
			/*X Coord*/ PADDING_AROUND_LISTVIEW, 
			/*Y Coord*/ PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT + TOP_PADDING, 
			/*X Size */ CabinetWidth - PADDING_AROUND_LISTVIEW * 2, 
			/*Y Size */ CabinetHeight- PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT - TOP_PADDING
		);
		
		AddControlEx (pWindow, CONTROL_ICONVIEWDRAG, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, MAIN_LISTVIEW, 0, 0);
	}
}

void CabinetEntry (__attribute__((unused)) int argument)
{
	// create ourself a window:
	int xPos = (GetScreenSizeX() - CABINET_WIDTH)  / 2;
	int yPos = (GetScreenSizeY() - CABINET_HEIGHT) / 2;
	Window* pWindow = CreateWindow ("Cabinet", xPos, yPos, CABINET_WIDTH, CABINET_HEIGHT, CabinetWindowProc, WF_ALWRESIZ);
	pWindow->m_iconID = ICON_CABINET;
	
	if (!pWindow)
	{
		// if you can't create the main window, what makes you think you can create a messagebox?!
		SLogMsg("The window could not be created");
		return;
	}
	
	pWindow->m_data = MmAllocate(sizeof(CabData));
	
	g_bUsingTableView = true;
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
