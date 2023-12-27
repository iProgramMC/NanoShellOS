/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

            Help Applet module
******************************************/


#include <wbuiltin.h>
#include <resource.h>
#include <widget.h>
#include <vfs.h>

enum
{
	HELP_MENUBAR_ROOT,//=0, Start by adding to comboid 0
	HELP_TEXTVIEW,
	HELP_MENUBAR,
	HELP_MENUBAR_HELP,
	HELP_BTNABOUT,
};

typedef struct
{
	int  start, end;   //The start and end of the link. At the end there should be a TIST_UNLINK character.
	char resource[201];//Resource to launch when clicked
}
LinkEntry;
typedef struct
{
	int       nLinks;
	LinkEntry sLinks[32];//max!  Increase if necessary
}
LinkData;

// A lighter version of Markdown, with only the most basic of features
char* HelpMarkdownToInternalFmt (LinkData** pOutData, const char* pMd)
{
	LinkData* pData = MmAllocate(sizeof(LinkData));
	if (!pData) return NULL;
	
	*pOutData = pData;
	
	pData->nLinks = 0;
	
	char* pointer = MmAllocate (strlen (pMd) * 2);
	if (!pointer) return NULL;
	
	char* pHead = pointer;
	
	bool  header = false,
	      bold = false,
	      link = false;
	
	char* linkRangeStart = NULL, *linkRangeEnd = NULL;
	char  linkDest[201]; int linkDestLen = 0;
	
	while (*pMd)
	{
		if (*pMd == '\\')
		{
			// Character escape.
			pMd++;
			
			*pHead++ = *pMd;
		}
		else switch (*pMd)
		{
			case '#':
			{
				if (!header)
				{
					// Add a bold character
					*pHead++ = TIST_BOLD;
				}
				header = true;//A header block.
				
				while (*pMd == '#' || *pMd == ' ') pMd++;
				pMd--;//hack
				
				break;
			}
			case '[':
			{
				if (!link)
				{
					linkRangeStart = pHead;
					*pHead++ = TIST_LINK;
				}
				else
					*pHead++ = *pMd;
				link = true;
				break;
			}
			case ']':
			{
				if (link)
				{
					linkRangeEnd = pHead;
					*pHead++ = TIST_UNLINK;
					
					linkDestLen = 0;
					
					// If there's a ( right next to it...
					pMd++;
					if (*pMd == '(')
					{
						pMd++;
						
						//Read into linkDest until a ')'.
						while (*pMd != ')' && linkDestLen < 200)
							linkDest[linkDestLen++] = *pMd++;
						
						linkDest[linkDestLen] = 0;
						
						// Reached the ')', it's going to get skipped soon.
						// Add the link entry
						if (pData->nLinks < 32)
						{
							LinkEntry* pEntry = &pData->sLinks[pData->nLinks++];
							pEntry->start = linkRangeStart - pointer;
							pEntry->end   = linkRangeEnd   - pointer;
							strcpy (pEntry->resource, linkDest);
						}
					}
				}
				else
					*pHead++ = *pMd;
				link = false;
				break;
			}
			case '*':
			{
				if (bold)
				{
					// Add an unbold character
					*pHead++ = TIST_UNBOLD;
				}
				else
				{
					// Add a bold character
					*pHead++ = TIST_BOLD;
				}
				bold ^= true;
				break;
			}
			case '\r':
			{
				//Ignore character.
				break;
			}
			case '\n':
			{
				if (header)
				{
					header = false;
					
					*pHead++ = TIST_UNBOLD;
				}
				*pHead++ = *pMd;
				break;
			}
			default:
			{
				// Default: just put in the character
				*pHead++ = *pMd;
				break;
			}
		}
		pMd++;
	}
	*pHead = '\0';
	return pointer;
}

void HelpOpen(Window* pWindow, const char* pFileName)
{
	if (pWindow->m_data)
	{
		MmFree(pWindow->m_data);
		pWindow->m_data = NULL;
	}
	int fd = FiOpen (pFileName, O_RDONLY);
	if (fd < 0)
	{
		//no file
		char buff[1024];
		sprintf(buff, "Cannot find the help file '%s'.", pFileName);
		MessageBox(pWindow, buff, "Help", MB_OK | ICON_ERROR << 16);
		return;
	}
	else
	{
		// get the size
		int fileSize = FiTellSize (fd);
		
		// allocate a buffer
		char* buffer = MmAllocate (fileSize + 1);
		if (!buffer)
		{
			MessageBox(pWindow, "Not enough memory to perform this operation.", "Help", MB_OK | ICON_ERROR << 16);
			return;
		}
		
		buffer[fileSize] = 0;
		FiRead (fd, buffer, fileSize);
		
		// close the file
		FiClose (fd);
		
		LinkData* pLinkData = NULL;
		char* buffer2 = HelpMarkdownToInternalFmt(&pLinkData, buffer);
		
		pWindow->m_data = pLinkData;
		
		MmFree (buffer);
		if (!buffer2)
		{
			MessageBox(pWindow, "Not enough memory to perform this operation.", "Help", MB_OK | ICON_ERROR << 16);
			return;
		}
		
		// load the file into the box
		SetTextInputText(pWindow, HELP_TEXTVIEW, buffer2);
		
		// internally this function copies the text anyway, so free it here:
		MmFree (buffer2);
	}
}


#define HELP_WIDTH  500
#define HELP_HEIGHT 400

void CALLBACK HelpWndProc (Window* pWindow, int msg, long parm1, long parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			char* pOldData = (char*)pWindow->m_data;
			
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 4
			#define TOP_PADDING             (TITLE_BAR_HEIGHT)
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ PADDING_AROUND_LISTVIEW + TOP_PADDING, 
				/*X Size */ HELP_WIDTH - PADDING_AROUND_LISTVIEW * 2, 
				/*Y Size */ HELP_HEIGHT- PADDING_AROUND_LISTVIEW * 2 - TOP_PADDING
			);
			
			AddControlEx (pWindow, CONTROL_TEXTINPUT, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, HELP_TEXTVIEW, 1 | 4 | 8, 0);
			
			RECT(r, 0, 0, 0, 0);
			AddControl(pWindow, CONTROL_MENUBAR, r, NULL, HELP_MENUBAR, 0, 0);
			AddMenuBarItem(pWindow, HELP_MENUBAR, HELP_MENUBAR_ROOT, HELP_MENUBAR_HELP, "Help");
			AddMenuBarItem(pWindow, HELP_MENUBAR, HELP_MENUBAR_HELP, HELP_BTNABOUT,     "About Help");
			
			SetTextInputText (pWindow, HELP_TEXTVIEW, "NanoShell Help");
			
			pWindow->m_data = NULL;
			
			if (pOldData)
			{
				HelpOpen(pWindow, pOldData);
				MmFree(pOldData);
			}
			else
				HelpOpen(pWindow, "/Help/Main.md");
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == HELP_MENUBAR)
			{
				switch (parm2)
				{
					case HELP_BTNABOUT:
						ShellAbout("Help", ICON_HELP);
						break;
				}
			}
			break;
		}
		case EVENT_CLOSE:
		case EVENT_DESTROY:
		{
			if (pWindow->m_data)
			{
				MmFree(pWindow->m_data);
				pWindow->m_data = NULL;
			}
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
		}
		case EVENT_KEYRAW:
		{
			break;
		}
		case EVENT_CLICK_CHAR:
		{
			if (parm1 != HELP_TEXTVIEW) break;
			
			//SLogMsg("Clicked character %d in textview", parm2);
			LinkData* pData = (LinkData*)pWindow->m_data;
			if (pData)
			{
				for (int i = 0; i < pData->nLinks; i++)
				{
					if (pData->sLinks[i].start <= parm2 && parm2 <= pData->sLinks[i].end)
					{
						//Good!
						LaunchResource(pData->sLinks[i].resource);
						break;
					}
				}
			}
			
			break;
		}
		case EVENT_PAINT:
			
			break;
		default:
			DefaultWindowProc (pWindow, msg, parm1, parm2);
			break;
	}
}

void HelpEntry (long arg)
{
	Window *pWindow = CreateWindow ("Help", CW_AUTOPOSITION, CW_AUTOPOSITION, HELP_WIDTH, HELP_HEIGHT, HelpWndProc, WF_ALWRESIZ);
	
	if (!pWindow) {
		LogMsg("Could not create window.");
		return;
	}
	
	pWindow->m_iconID = ICON_HELP;
	pWindow->m_data   = (void*)arg;
	
	while (HandleMessages (pWindow));
}

RESOURCE_STATUS HelpOpenResource(const char* pResourceID)
{
	char* text = MmStringDuplicate (pResourceID);
	if (!text)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	int errorCode = 0;
	Task* pTask = KeStartTask(HelpEntry, (long)text, &errorCode);
	DebugLogMsg("Created help window. Pointer returned:%x, errorcode:%x", pTask, errorCode);
	
	if (!pTask)
		return RESOURCE_LAUNCH_OUT_OF_MEMORY;
	
	KeUnsuspendTask(pTask);
	KeDetachTask(pTask);
	
	return RESOURCE_LAUNCH_SUCCESS;
}
