/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
          Minesweeper application

             Main source file
******************************************/
#include <nsstandard.h>

#include "queue.h"

#define FINDER_WIDTH  400
#define FINDER_HEIGHT 300

enum
{
	F_SQUERYLABEL,
	F_SQUERYTEXT,
	F_SQUERYSEARCHBTN,
	F_SPATHLABEL,
	F_SPATHTEXT,
	F_SEARCHSTOPBTN,
	F_SEARCHTABLE,
	F_SEARCHTEXT,
};

Window* g_pMainWindow;

int array[5] = { 42, 69, 100, 200, 400 };

Queue* g_pSearchQueue = NULL;
char * g_pSearchQuery = NULL;

int g_nFound, g_nBrowsed, g_nWhenStartedSearch;

void HaltSearchImmediately();

void ResetSearchResultTable()
{
	ResetTable(g_pMainWindow, F_SEARCHTABLE);
	AddTableColumn(g_pMainWindow, F_SEARCHTABLE, "File Name", 200);
	AddTableColumn(g_pMainWindow, F_SEARCHTABLE, "Full Path", 400);
}

bool EndsWith(const char* pText, const char* pCheck)
{
	int slt = strlen (pText), slc = strlen (pCheck);
	if (slt < slc) return false; //obviously, it can't.
	
	const char* pTextEnd = pText + slt;
	pTextEnd -= slc;
	return (strcmp (pTextEnd, pCheck) == 0);
}

bool StartsWith(const char* pText, const char* pCheck)
{
	int slt = strlen (pText), slc = strlen (pCheck);
	if (slt < slc) return false; //obviously, it can't.
	
	char text[slc+1];
	memcpy (text, pText, slc);
	text[slc] = 0;
	
	return (strcmp (text, pCheck) == 0);
}

int CabGetIconBasedOnName(const char *pName, int pType)
{
	int icon = ICON_FILE;
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

int GetFileIcon(const char* file_name)
{
	StatResult sr;
	int res = FiStat(file_name, &sr);
	if (res < 0) {
		return ICON_ERROR;
	}
	
	return CabGetIconBasedOnName(file_name, sr.m_type);
}

void AddSearchResult(const char * full_path, const char * file_name)
{
	const char * buf[2] = { 0 };
	
	buf[0] = file_name;
	buf[1] = full_path;
	
	AddTableRow(g_pMainWindow, F_SEARCHTABLE, buf, GetFileIcon(full_path));
	CallControlCallback(g_pMainWindow, F_SEARCHTABLE, EVENT_PAINT, 0, 0);
}

void StartSearch(const char* path, const char* query)
{
	// halt the pre-existing search
	HaltSearchImmediately();
	
	g_pSearchQueue = QueueCreate();
	
	// push the path to the search queue
	QueuePush(g_pSearchQueue, strdup(path));
	
	// enable the 'stop search' button
	SetControlDisabled(g_pMainWindow, F_SEARCHSTOPBTN, false);
	
	ResetSearchResultTable();
	
	g_pSearchQuery = strdup(query);
	
	g_nBrowsed = g_nFound = 0;
	g_nWhenStartedSearch = GetTickCount();
}

// note: This is an inefficient algorithm, and will lead to a stack overflow if the strings are too long.
// Make sure to cap their lengths.
bool QueryMatches(const char* query, const char* thing)
{
	// if we have gone through all the characters in the pattern...
	if (*query == '\0')
	{
		// check if we have also matched all of the thing's characters.
		return (*thing == 0);
	}
	
	// if we have a wildcard...
	if (*query == '*')
	{
		query++;
		
		for (; *thing; thing++)
		{
			if (QueryMatches(query, thing))
				return true;
		}
		
		// also check the case where there's no string to match.
		return QueryMatches(query, thing);
	}
	
	// If this isn't a single character wildcard or a regular wildcard, this means that we should
	// check if the current character in the query matches the current character in the thing.
	// If they don't match, we can easily discard the entire match as false.
	if (*query != '?')
	{
		if (*query != *thing) return false;
	}
	// If it is, we shouldn't have a null character.
	else
	{
		if (*thing == '\0')
			return false;
	}
	
	return QueryMatches(query + 1, thing + 1);
}

char* ConcatenateFileName(const char* path, const char* dename)
{
	bool root = false;
	
	if (strcmp(path, "/") == 0) root = true;
	
	size_t denamelen = strlen(dename), pathlen = strlen(path);
	
	char* fn = malloc(pathlen + 1 + denamelen + 1);
	if (!fn) return fn;
	
	strcpy(fn, path);
	
	if (!root)
	{
		strcpy(fn + pathlen, "/");
		pathlen++;
	}
	
	strcpy(fn + pathlen, dename);
	return fn;
}

void SetProcessingText(const char * text)
{
	SetLabelText(g_pMainWindow, F_SEARCHTEXT, text);
	CallControlCallback(g_pMainWindow, F_SEARCHTEXT, EVENT_PAINT, 0, 0);
}

void SetProcessingTextWithFormatting(const char* dir)
{
	char buffer[1024];
	snprintf(buffer, sizeof buffer, "Processing %s...", dir);
	SetProcessingText(buffer);
}

void ProcessOneSearchQueueItem()
{
	if (!g_pSearchQueue) return;
	if (QueueEmpty(g_pSearchQueue)) 
	{
		HaltSearchImmediately();
		return;
	}
	
	char* ptr = QueuePop(g_pSearchQueue);
	
	// try to open the directory
	int dd = FiOpenDir(ptr);
	if (dd < 0)
	{
		char buffer[1024];
		snprintf(buffer, sizeof buffer, "During search, the directory '%s' could not be accessed.\n\n%s", ptr, ErrNoStr(dd));
		MessageBox(g_pMainWindow, buffer, "Finder", MB_OK | ICON_ERROR << 16);
		free(ptr);
		return;
	}
	
	//LogMsg("Processing dir '%s'.", ptr);
	
	DirEnt ent, *pDirEnt = &ent;
	int err = 0;
	
	while ((err = FiReadDir(&ent, dd)) == 0)
	{
		SetProcessingTextWithFormatting(ptr);
		
		// inspect this directory entry.
		if (pDirEnt->m_type & FILE_TYPE_DIRECTORY)
		{
			// Add this file's name to the queue.
			char* fn = ConcatenateFileName(ptr, pDirEnt->m_name);
			QueuePush(g_pSearchQueue, fn);
		}
		
		if (QueryMatches(g_pSearchQuery, pDirEnt->m_name))
		{
			char* fn = ConcatenateFileName(ptr, pDirEnt->m_name);
			AddSearchResult(fn, pDirEnt->m_name);
			free(fn);
		}
	}
	
	FiCloseDir(dd);
	
	free(ptr);
}

void HaltSearchImmediately()
{
	if (g_pSearchQueue)
	{
		// pop everything from the queue and free it
		while (!QueueEmpty(g_pSearchQueue))
			free(QueuePop(g_pSearchQueue)); // note: we can free NULL here. It's fine
		
		QueueFree(g_pSearchQueue);
		g_pSearchQueue = NULL;
	}
	
	if (g_pSearchQuery)
	{
		// disable the stop search button
		SetControlDisabled(g_pMainWindow, F_SEARCHSTOPBTN, true);
		
		int timeLeft = GetTickCount() - g_nWhenStartedSearch;
		
		char buf[1024];
		snprintf(buf, sizeof buf, "Search done. %d matches, from %d total files. (%d.%03d s)", g_nFound, g_nBrowsed, timeLeft / 1000, timeLeft % 1000);
		
		SetProcessingText(buf);
	}
	
	free(g_pSearchQuery);
	g_pSearchQuery = NULL;
}

int GetSelectedFileIndex()
{
	return GetSelectedIndexTable(g_pMainWindow, F_SEARCHTABLE);
}

const char * GetFileNameFromList(int index)
{
	const char * table[2];
	if (!GetRowStringsFromTable(g_pMainWindow, F_SEARCHTABLE, index, table)) return NULL;
	return table[1];
}

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			// add a label
			Rectangle rect;
			
			RECT(rect, 10, 10, 70, 20);
			AddControl(pWindow, CONTROL_TEXTCENTER, rect, "Search Query:", F_SQUERYLABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED | TEXTSTYLE_FORCEBGCOL);
			
			RECT(rect, 80, 10, FINDER_WIDTH - 90 - 75, 20);
			AddControlEx(pWindow, CONTROL_TEXTINPUT, ANCHOR_RIGHT_TO_RIGHT, rect, "", F_SQUERYTEXT, 0, 0);
			
			RECT(rect, 10, 40, 70, 20);
			AddControl(pWindow, CONTROL_TEXTCENTER, rect, "Path:", F_SPATHLABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_VCENTERED | TEXTSTYLE_FORCEBGCOL);
			
			RECT(rect, 80, 40, FINDER_WIDTH - 90 - 75, 20);
			AddControlEx(pWindow, CONTROL_TEXTINPUT, ANCHOR_RIGHT_TO_RIGHT, rect, "", F_SPATHTEXT, 0, 0);
			SetTextInputText(pWindow, F_SPATHTEXT, "/");
			
			RECT(rect, FINDER_WIDTH - 80, 10, 70, 20);
			AddControlEx(pWindow, CONTROL_BUTTON, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT, rect, "Search!", F_SQUERYSEARCHBTN, 0, 0);
			
			RECT(rect, FINDER_WIDTH - 80, 40, 70, 20);
			AddControlEx(pWindow, CONTROL_BUTTON, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT, rect, "Stop Search", F_SEARCHSTOPBTN, 0, 0);
			SetControlDisabled(pWindow, F_SEARCHSTOPBTN, true);
			
			RECT(rect, 10, 65, FINDER_WIDTH, 14);
			AddControlEx(pWindow, CONTROL_TEXTCENTER, ANCHOR_RIGHT_TO_RIGHT, rect, "Search not running.", F_SEARCHTEXT, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			
			RECT(rect, 5, 80, FINDER_WIDTH - 10, FINDER_HEIGHT - 85);
			AddControlEx(pWindow, CONTROL_TABLEVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, rect, NULL, F_SEARCHTABLE, 0, 0);
			ResetSearchResultTable();
			
			// time every 10 ms
			AddTimer(pWindow, 10, EVENT_USER);
			
			break;
		}
		case EVENT_USER:
		{
			for (int i = 0; i < 8; i++)
			{
				ProcessOneSearchQueueItem();
			}
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == F_SQUERYSEARCHBTN)
			{
				const char* textQuery = TextInputGetRawText(pWindow, F_SQUERYTEXT);
				const char* textPath  = TextInputGetRawText(pWindow, F_SPATHTEXT);
				
				StartSearch(textPath, textQuery);
				
				break;
			}
			if (parm1 == F_SEARCHSTOPBTN)
			{
				HaltSearchImmediately();
				
				break;
			}
			if (parm1 == F_SEARCHTABLE)
			{
				const char* fn = GetFileNameFromList(GetSelectedFileIndex());
				if (!fn) break;
				
				//TODO
				LogMsg("Hey! You just clicked %s.", fn);
				
				break;
			}
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

int main()
{
	Window* pWindow = g_pMainWindow = CreateWindow ("Finder", CW_AUTOPOSITION, CW_AUTOPOSITION, FINDER_WIDTH, FINDER_HEIGHT, WndProc, WF_ALWRESIZ);
	
	if (!pWindow)
	{
		LogMsg("Hey, the window couldn't be created");
		return 0;
	}
	
	SetWindowIcon (pWindow, ICON_FILE_SEARCH);
	
	// event loop:
	while (HandleMessages (pWindow));
	
	return 0;
}
