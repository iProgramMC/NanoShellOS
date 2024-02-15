/*****************************************
		NanoShell Operating System
	   (C) 2022-2023 iProgramInCpp

        Cabinet Application module
******************************************/
#include "cabinet.h"

#define CABINET_WIDTH  600
#define CABINET_HEIGHT 400

// The maximum amount of stat() calls before we quit. This is to speed up loading large folders.
// This is still a work in progress.
#define C_MAX_STATS_BEFORE_QUIT (256)

#define TABLE_COLUMNS (4)

//TODO: Move this to its own space.
typedef struct
{
	char m_cabinetCWD[PATH_MAX+2];
	char m_cbntOldCWD[PATH_MAX+2];
	bool m_bUsingTableView;
	char*m_pDestCWD;
}
CabData;

void FormatSize(uint32_t size, char* size_buf)
{
	uint32_t kb = (size + 1023) / 1024;
	if (size == ~0u)
		size_buf[0] = 0;
	else if (size < 1024)
		sprintf(size_buf, "%,d B", size);
	else
		sprintf(size_buf, "%,d KB", kb);
}

void FormatSizeDetailed(uint32_t size, char* size_buf)
{
	char buffer2[64];
	sprintf(buffer2, " (%,d bytes)", size);
	FormatSize(size, size_buf);
	strcat(size_buf, buffer2);
}

#define g_cabinetCWD (((CabData*)pWindow->m_data)->m_cabinetCWD)
#define g_cbntOldCWD (((CabData*)pWindow->m_data)->m_cbntOldCWD)
#define g_pDestCWD   (((CabData*)pWindow->m_data)->m_pDestCWD)
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
	CB$DELETE,    //delete the file
};

IconType CabGetIconBasedOnName(const char *pName, int pType)
{
	return ResolveAssociation(pName, pType)->icon;
}

void RequestTaskbarUpdate();

static void CreateListView(Window* pWindow);
static void UpdateDirectoryListing (Window* pWindow);

static void ClearFileListing(Window * pWindow)
{
	if (g_bUsingTableView)
	{
		ResetTable(pWindow, MAIN_LISTVIEW);
		
		AddTableColumn(pWindow, MAIN_LISTVIEW, "Name", 200);
		AddTableColumn(pWindow, MAIN_LISTVIEW, "Size", 100);
		AddTableColumn(pWindow, MAIN_LISTVIEW, "Last modified date", 150);
		AddTableColumn(pWindow, MAIN_LISTVIEW, "File type", 150);
	}
	else
	{
		ResetList (pWindow, MAIN_LISTVIEW);
	}
}

static void ChangeListViewMode(Window* pWindow)
{
	g_bUsingTableView ^= 1;
	OnBusy(pWindow);
	RemoveControl(pWindow, MAIN_LISTVIEW);
	CreateListView(pWindow);
	ClearFileListing(pWindow);
	UpdateDirectoryListing(pWindow);
	RequestRepaintNew(pWindow);
	OnNotBusy(pWindow);
}

// size = -1, means don't show anything to the file
static void AddFileElementToList(Window* pWindow, const char * text, int icon, uint32_t file_size, int last_modified_date, bool is_symlink, const char * description)
{
	if (is_symlink)
		icon |= ICON_SHORTCUT_FLAG;
	
	// note: this is real crap
	if (g_bUsingTableView)
	{
		char size_buf[16];
		FormatSize(file_size, size_buf);
		
		char date_buf[64];
		date_buf[0] = 0;
		
		if (last_modified_date != -1)
		{
			TimeStruct str;
			GetHumanTimeFromEpoch(last_modified_date, &str);
			sprintf(date_buf, "%02d/%02d/%04d %02d:%02d:%02d", str.day, str.month, str.year, str.hours, str.minutes, str.seconds);
		}
		
		const char* table[TABLE_COLUMNS] = { 0 };
		table[0] = text;
		table[1] = size_buf;
		table[2] = date_buf;
		table[3] = description;
		
		AddTableRow(pWindow, MAIN_LISTVIEW, table, icon);
	}
	else
	{
		AddElementToList(pWindow, MAIN_LISTVIEW, text, icon);
	}
}

static void UpdateDirectoryListing (Window* pWindow)
{
	ClearFileListing(pWindow);
	
	OnBusy(pWindow);
	
	if (strcmp (g_cabinetCWD, "/")) //if can go to parent, add a button
	{
		AddFileElementToList(pWindow, "..", ICON_FOLDER_PARENT, -1, -1, false, "Up one level");
	}
	
	DirEnt ent, *pEnt = &ent;
	int err = 0;
	
	int dd = FiOpenDir (g_cabinetCWD);
	if (dd < 0)
	{
		CabinetChangeDirectory(pWindow, "/", false);
		return;
	}
	
	int filesDone = 0;
	
	while ((err = FiReadDir(pEnt, dd)) == 0)
	{
		StatResult statResult;
		int res = 0;
		
		const char* pName = pEnt->m_name;
		
		if (filesDone < C_MAX_STATS_BEFORE_QUIT && g_bUsingTableView)
		{
			FiStatAt (dd, pEnt->m_name, &statResult);
		}
		else
		{
			// TODO: Allow user to choose.
			statResult.m_size       = -1;
			statResult.m_modifyTime = -1;
			statResult.m_type       = pEnt->m_type;
		}
		filesDone++;
		
		bool bIsSymLink = false;
		
		if (res < 0)
		{
			char buf[512];
			sprintf(buf, "%s (cannot stat)", pEnt->m_name, GetErrNoString(res));
			AddFileElementToList(pWindow, buf, ICON_ERROR, -1, -1, false, GetErrNoString(res));
		}
		else if (pEnt->m_type == FILE_TYPE_SYMBOLIC_LINK && filesDone < C_MAX_STATS_BEFORE_QUIT)
		{
			// stat the actual file using FiStat. This works since the cabinet uses the system CWD as its own CWD.
			int res = FiStat(pEnt->m_name, &statResult);
			if (res < 0)
			{
				// TODO: ICON_CHAIN_BROKEN
				AddFileElementToList(pWindow, pEnt->m_name, ICON_CHAIN_BROKEN, (pEnt->m_type != FILE_TYPE_FILE) ? (-1) : statResult.m_size, statResult.m_modifyTime, true, "Broken symbolic link");
			}
			else
			{
				pEnt->m_type = statResult.m_type;
				bIsSymLink = true;
				goto stat_done;
			}
		}
		else
		{
			FileAssociation* pAssoc;
		stat_done:
			pAssoc = ResolveAssociation(pName, pEnt->m_type);
			
			AddFileElementToList(pWindow, pEnt->m_name, pAssoc->icon, (pEnt->m_type != FILE_TYPE_FILE) ? (-1) : statResult.m_size, statResult.m_modifyTime, bIsSymLink, pAssoc->description);
		}
	}
	
	if (err < 0)
	{
		SLogMsg("Couldn't fully read directory: %s", GetErrNoString(err));
	}
	
	FiCloseDir(dd);
	dd = 0;
	
	int icon = CabGetIconBasedOnName(g_cabinetCWD, FILE_TYPE_DIRECTORY);
	//SetWindowIcon (pWindow, icon);
	//SetWindowTitle(pWindow, pFolderNode->m_name);
	pWindow->m_iconID = icon;
	
	char* title = MmAllocate(WINDOW_TITLE_MAX);
	strcpy (title, "Cabinet - ");
	strcat (title, g_cabinetCWD); //note: WINDOW_TITLE_MAX is 250, but file names are 127 max. 
	SetWindowTitle(pWindow, title);
	MmFree(title);
	RequestTaskbarUpdate();
	
	SetLabelText(pWindow, MAIN_PATH_TEXT, g_cabinetCWD);
	
	//RequestRepaint(pWindow);
	CallControlCallback(pWindow, MAIN_LISTVIEW,  EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, MAIN_PATH_TEXT, EVENT_PAINT, 0, 0);
	
	OnNotBusy(pWindow);
}

void CdBack(Window* pWindow)
{
	CabinetChangeDirectory(pWindow, "..", true);
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

static int GetSelectedFileIndex(Window* pWindow)
{
	if (g_bUsingTableView)
		return GetSelectedIndexTable(pWindow, MAIN_LISTVIEW);
	else
		return GetSelectedIndexList(pWindow, MAIN_LISTVIEW);
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

void CALLBACK CabinetWindowProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_COMMAND:
		{
			if (parm1 == MAIN_LISTVIEW)
			{
				int dd = FiOpenDir(g_cabinetCWD);
				if (dd < 0)
				{
					CabinetChangeDirectory(pWindow, "/", 0);
					break;
				}
				
				const char* pFileName = GetFileNameFromList(pWindow, parm2);//GetElementStringFromList (pWindow, parm1, parm2);
				//LogMsg("Double clicked element: %s", pFileName);
				
				if (!pFileName)
				{
					SLogMsg("pFileName is NULL!");
					FiCloseDir(dd);
					break;
				}
				
				if (strcmp (pFileName, PATH_PARENTDIR) == 0)
				{
					CdBack(pWindow);
				}
				else
				{
					OnBusy(pWindow);
					StatResult sr;
					int res = FiStatAt(dd, pFileName, &sr);
					
					if (res >= 0)
					{
						if (sr.m_type & FILE_TYPE_DIRECTORY)
						{
							CabinetChangeDirectory(pWindow, pFileName, false);
						}
						else if (sr.m_type & FILE_TYPE_SYMBOLIC_LINK)
						{
							// first, try to chdir there.
							int res = FiChangeDir(pFileName);
							if (res < 0)
							{
								goto _regular_file;
							}
							else
							{
								CabinetChangeDirectory(pWindow, ".", false);
							}
						}
						else
						{
						_regular_file:;
							RESOURCE_STATUS status = LaunchFileOrResource(pFileName);
							if (status == RESOURCE_LAUNCH_INVALID_PROTOCOL)
							{
								char ext[PATH_MAX];
								ext[0] = 0;
								char* x = strrchr(pFileName, '.');
								if (x)
								{
									strncpy(ext, x, sizeof ext - 1);
									ext[sizeof ext - 1] = 0;
								}
								
								char buffer [512];
								snprintf (buffer, sizeof buffer, "There is no file association set up for the file type \"%s\".\n\nNanoShell cannot open this file.", *ext ? ext : "None");
								MessageBox(pWindow, buffer, "Error", ICON_INFO << 16 | MB_OK);
							}
							else if (status != RESOURCE_LAUNCH_SUCCESS)
							{
								char buffer [512];
								snprintf (buffer, sizeof buffer, GetResourceErrorText(status), pFileName);
								MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
							}
						}
					}
					else
					{
						OnNotBusy(pWindow);
						char buffer [512];
						snprintf(buffer, sizeof buffer, "Cannot access file '%s'.  It may have been moved or deleted.\n\n%s\n\nTry clicking the 'Refresh' button in the top bar.", GetErrNoString(res), pFileName);
						MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
					}
				}
				
				OnNotBusy(pWindow);
				
				FiCloseDir(dd);
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
						CabinetChangeDirectory(pWindow, "/", false);
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
				case CB$PROPERTIES:
				{
					const char* pFileName = GetFileNameFromList(pWindow, GetSelectedFileIndex(pWindow));
					
					if (strcmp(pFileName, "..") == 0) break;
					
					char buffer[1024];
					strcpy(buffer, g_cabinetCWD);
					if (g_cabinetCWD[1] != 0) // not just /
					{
						strcat(buffer, "/");
					}
					strcat(buffer, pFileName);
					
					CreatePropertiesWindow(pWindow, buffer, pFileName);
					
					break;
				}
				case CB$DELETE:
				{
					const char* pFileName = GetFileNameFromList(pWindow, GetSelectedFileIndex(pWindow));
					if (!pFileName) break;
					if (strcmp(pFileName, "..") == 0) break;
					
					char buffer[1024];
					
					// check what it is
					StatResult sr;
					int res = FiStat(pFileName, &sr);
					if (res < 0)
					{
						snprintf(buffer, sizeof buffer, "The file '%s' is not accessible. It may have been moved or deleted.\n\nClick the 'Refresh' button in the top toolbar.", pFileName);
						MessageBox(pWindow, buffer, "Cabinet", MB_OK | ICON_ERROR << 16);
						break;
					}
					
					snprintf(buffer, sizeof buffer, "Are you sure you want to delete '%s'?", pFileName);
					
					if (MessageBox(pWindow, buffer, "Confirm File Deletion", MB_YESNO | ICON_WARNING << 16) == MBID_YES)
					{
						// is this a directory?
						if (sr.m_type & FILE_TYPE_DIRECTORY)
						{
							// TODO
							snprintf(buffer, sizeof buffer, "The directory '%s' cannot be deleted for now, recursive deletion is still in the works.", pFileName);
							MessageBox(pWindow, buffer, "Cabinet", MB_OK | ICON_WARNING << 16);
						}
						else
						{
							int res = FiUnlinkFile(pFileName);
							if (res < 0)
							{
								snprintf(buffer, sizeof buffer, "The file '%s' cannot be deleted.\n\n%s", pFileName, GetErrNoString(res));
								MessageBox(pWindow, buffer, "Cabinet", MB_OK | ICON_WARNING << 16);
							}
							
							UpdateDirectoryListing(pWindow);
						}
					}
					
					break;
				}
				default:
					MessageBox(pWindow, "Not implemented! Check back later or something.", "Cabinet", MB_OK | ICON_INFO << 16);
			}
			break;
		}
		case EVENT_CREATE:
		{
			strcpy (g_cbntOldCWD, "");
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			#define TOP_PADDING             (TITLE_BAR_HEIGHT+COOLBAR_BUTTON_HEIGHT)
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
				//AddMenuBarItem(pWindow, MAIN_MENU_BAR, MENU$FILE, MENU$FILE$MRD,  "Mount a ram-drive");
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
			
			if (g_pDestCWD)
			{
				CabinetChangeDirectory(pWindow, g_pDestCWD, true);
				MmFree(g_pDestCWD);
				g_pDestCWD = NULL;
			}
			else
			{
				CabinetChangeDirectory(pWindow, "/", false);
			}
			
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
				ICON_DELETE,
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
				CB$DELETE,
			};
			int x_pos = PADDING_AROUND_LISTVIEW;
			for (i = 0; i < (int)ARRAY_COUNT(button_icons); i++)
			{
				if (button_icons[i] == 0)
					continue; // none
				if (button_icons[i] == -1)
				{
					RECT(r, x_pos, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT, 5, COOLBAR_BUTTON_HEIGHT);
					//add a simple vertical line
					AddControl(pWindow, CONTROL_SIMPLE_VLINE, r, NULL, 0, 0, 0);
					x_pos += 5;
				}
				else
				{
					RECT(r, x_pos, PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT, COOLBAR_BUTTON_HEIGHT, COOLBAR_BUTTON_HEIGHT);
					AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, "", button_actions[i], button_icons[i], COOLBAR_BUTTON_HEIGHT > 36 ? 32 : 16);
					
					x_pos += (COOLBAR_BUTTON_HEIGHT + 1);
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

void CabinetChangeDirectory(Window* pWindow, const char * cwd, bool bTakeToRootOnFailure)
{
	if (strlen(cwd) >= sizeof g_cabinetCWD - 1) return;
	
	if (FiChangeDir(cwd) < 0)
	{
		char buffer [256];
		sprintf (buffer, "Cannot find directory '%s'.  It may have been moved or deleted.\n\n%s", cwd,
			bTakeToRootOnFailure ? "Changing back to the root directory." : "Try clicking the 'Refresh' button in the top bar.");
		MessageBox(pWindow, buffer, "Error", ICON_ERROR << 16 | MB_OK);
		
		if (bTakeToRootOnFailure)
			CabinetChangeDirectory(pWindow, "/", false);
		
		return;
	}
	
	//note: the appropriate length checks were done above
	strncpy(g_cabinetCWD, FiGetCwd(), sizeof g_cabinetCWD - 1);
	g_cabinetCWD[sizeof g_cabinetCWD - 1] = 0;
	
	UpdateDirectoryListing(pWindow);
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
		
		AddControlEx (pWindow, CONTROL_ICONVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, MAIN_LISTVIEW, 0, 0);
	}
}

void CabinetEntry (long argument)
{
	// create ourself a window:
	int xPos = (GetScreenSizeX() - CABINET_WIDTH)  / 2;
	int yPos = (GetScreenSizeY() - CABINET_HEIGHT) / 2;
	Window* pWindow = CreateWindow ("Cabinet", xPos, yPos, CABINET_WIDTH, CABINET_HEIGHT, CabinetWindowProc, WF_ALWRESIZ);
	
	if (!pWindow)
	{
		// if you can't create the main window, what makes you think you can create a messagebox?!
		SLogMsg("The window could not be created");
		return;
	}
	
	pWindow->m_iconID = ICON_CABINET;
	
	CabData* pData = pWindow->m_data = MmAllocate(sizeof(CabData));
	if (!pData)
	{
		if (argument)
			MmFree((char*)argument);
		
		SLogMsg("Cabinet data couldn't be allocated!!");
		DestroyWindow(pWindow);
		while (HandleMessages(pWindow));
		return;
	}
	
	g_pDestCWD = NULL;
	if (argument)
		g_pDestCWD = (char*) argument;
	
	g_bUsingTableView = true;
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
