/*****************************************
		NanoShell Operating System
	   (C) 2022-2023 iProgramInCpp

        Cabinet Application module
******************************************/
#include "cabinet.h"

#define CABINET_PROPS_WIDTH  (300)
#define CABINET_PROPS_HEIGHT (300)

enum
{
	PROPS_NOTHING,
	PROPS_TABPICKER,
	PROPS_BUTTON_OK,
	PROPS_LABEL,
	PROPS_ICON,
	PROPS_LABEL_LOCATION,
	PROPS_LABEL_FILE_SIZE,
	PROPS_LABEL_DISK_SIZE,
	PROPS_LABEL_CREATE_TIME,
	PROPS_LABEL_MODIFY_TIME,
	PROPS_LABEL_ATTRIBUTES,
	PROPS_LABEL_INODE,
	PROPS_LOCATION,
	PROPS_FILE_SIZE,
	PROPS_DISK_SIZE,
	PROPS_CREATE_TIME,
	PROPS_MODIFY_TIME,
	PROPS_ATTRIBUTES,
	PROPS_INODE,
	PROPS_DETAILS_TABLE,
};

enum
{
	PROPS_TAB_GENERAL,
	PROPS_TAB_DETAILS,
};

static const int s_CidsToHideGeneral[] = {
	PROPS_ICON, PROPS_LABEL, PROPS_LABEL_ATTRIBUTES, PROPS_LABEL_CREATE_TIME, PROPS_LABEL_DISK_SIZE,
	PROPS_LABEL_FILE_SIZE, PROPS_LABEL_INODE, PROPS_LABEL_LOCATION, PROPS_LABEL_MODIFY_TIME,
	PROPS_LOCATION, PROPS_FILE_SIZE, PROPS_DISK_SIZE, PROPS_CREATE_TIME, PROPS_MODIFY_TIME,
	PROPS_ATTRIBUTES, PROPS_INODE,
};

static const int s_CidsToHideDetails[] = { PROPS_DETAILS_TABLE };

static void CabinetPropertiesSetTabVisible(Window* pWindow, int tabID, bool visible)
{
	const int *cidsToHide = NULL;
	int nCidsToHide = 0;
	switch (tabID)
	{
		case PROPS_TAB_GENERAL:
			cidsToHide = s_CidsToHideGeneral, nCidsToHide = (int)ARRAY_COUNT(s_CidsToHideGeneral);
			break;
		case PROPS_TAB_DETAILS:
			cidsToHide = s_CidsToHideDetails, nCidsToHide = (int)ARRAY_COUNT(s_CidsToHideDetails);
			break;
	}
	
	for (int i = 0; i < nCidsToHide; i++)
	{
		SetControlVisibility(pWindow, cidsToHide[i], visible);
	}
}

#define RECTA(r, x, y, w, h) RECT(r, x, y + TITLE_BAR_HEIGHT, w, h)

static void CALLBACK CabinetPropertiesProc(Window * pWindow, int eventType, int parm1, int parm2)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			RECT(r, CABINET_PROPS_WIDTH - 62, CABINET_PROPS_HEIGHT - 22, 60, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "OK", PROPS_NOTHING, 0, 0);
			
			RECT(r, 2, 2, CABINET_PROPS_WIDTH - 4, CABINET_PROPS_HEIGHT - 32);
			AddControl(pWindow, CONTROL_TAB_PICKER, r, NULL, PROPS_TABPICKER, 0, 0);
			
			TabViewAddTab(pWindow, PROPS_TABPICKER, PROPS_TAB_GENERAL, "General", 200);
			
			const char** ptrs = (const char**)pWindow->m_data;
			const char* path     = ptrs[0];
			const char* justName = ptrs[1];
			
			if (path == NULL || justName == NULL)
			{
				RECTA(r, 52, 10, CABINET_PROPS_WIDTH - 62, CABINET_PROPS_HEIGHT - 20);
				AddControl(pWindow, CONTROL_TEXTHUGE, r, NULL, PROPS_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
				SetHugeLabelText(pWindow, PROPS_LABEL, "There is no file whose properties should be checked.");
				
				RECTA(r, 10, 10, 32, 32);
				AddControl(pWindow, CONTROL_ICON, r, NULL, PROPS_ICON, ICON_ERROR, 0);
				
				break;
			}
			
			StatResult res;
			int stat = FiStat(path, &res);
			
			if (stat < 0)
			{
				char buffer[1024];
				
				snprintf(buffer, 1024, "The file '%s' could not have its properties checked.\n\n%s", path, GetErrNoString(stat));
				
				RECTA(r, 52, 10, CABINET_PROPS_WIDTH - 62, CABINET_PROPS_HEIGHT - 20);
				AddControl(pWindow, CONTROL_TEXTHUGE, r, NULL, PROPS_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
				SetHugeLabelText(pWindow, PROPS_LABEL, buffer);
				
				RECTA(r, 10, 10, 32, 32);
				AddControl(pWindow, CONTROL_ICON, r, NULL, PROPS_ICON, ICON_ERROR, 0);
				
				break;
			}
			
			FileAssociation* pAssoc = ResolveAssociation(justName, res.m_type);
			int icon = pAssoc->icon;
			
			// *** Add other tabs based on the type of file this is.
			
			// If we are an executable.
			if (icon == ICON_APPLICATION)
			{
				ProgramInfo* pPrgInfo = RstRetrieveProgramInfoFromFile(path);
				if (pPrgInfo)
				{
					TabViewAddTab(pWindow, PROPS_TABPICKER, PROPS_TAB_DETAILS, "Details", 200);
					
					// add a table control, but hide it
					RECTA(r, 10, 10, CABINET_PROPS_WIDTH - 20, CABINET_PROPS_HEIGHT - 50 - TITLE_BAR_HEIGHT);
					AddControl(pWindow, CONTROL_TABLEVIEW, r, NULL, PROPS_DETAILS_TABLE, 0, TABLEVIEW_NOICONCOLUMN);
					
					AddTableColumn(pWindow, PROPS_DETAILS_TABLE, "Property", 75);
					AddTableColumn(pWindow, PROPS_DETAILS_TABLE, "Value", CABINET_PROPS_WIDTH - 100 - 20);
					
					const char* text[] = {NULL, NULL};
					
					text[0] = "Project name";
					text[1] = pPrgInfo->m_info.m_ProjName;
					AddTableRow(pWindow, PROPS_DETAILS_TABLE, text, ICON_NULL);
					text[0] = "File description";
					text[1] = pPrgInfo->m_info.m_AppName;
					AddTableRow(pWindow, PROPS_DETAILS_TABLE, text, ICON_NULL);
					text[0] = "Author";
					text[1] = pPrgInfo->m_info.m_AppAuthor;
					AddTableRow(pWindow, PROPS_DETAILS_TABLE, text, ICON_NULL);
					text[0] = "Copyright";
					text[1] = pPrgInfo->m_info.m_AppCopyright;
					AddTableRow(pWindow, PROPS_DETAILS_TABLE, text, ICON_NULL);
					text[0] = "Architecture";
					text[1] = ElfGetArchitectureString(pPrgInfo->m_machine, pPrgInfo->m_word_size);
					AddTableRow(pWindow, PROPS_DETAILS_TABLE, text, ICON_NULL);
					text[0] = "System ABI";
					text[1] = ElfGetOSABIString(pPrgInfo->m_os_abi);
					AddTableRow(pWindow, PROPS_DETAILS_TABLE, text, ICON_NULL);
					
					SetControlVisibility(pWindow, PROPS_DETAILS_TABLE, false);
					
					MmFree(pPrgInfo);
				}
			}
			
			int lineHeight = GetLineHeight();
			
			RECTA(r, 10, 10, 32, 32);
			AddControl(pWindow, CONTROL_ICON, r, NULL, PROPS_ICON, icon, 0);
			
			// add the hint text
			RECTA(r, 52, 10 + (32 - lineHeight) / 2, CABINET_PROPS_WIDTH - 62, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXTHUGE, r, NULL, PROPS_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			SetHugeLabelText(pWindow, PROPS_LABEL, justName);
			
			RECTA(r, 10, 20 + 32 + 0 * 20, CABINET_PROPS_WIDTH - 20, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, "Location:", PROPS_LABEL_LOCATION, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECTA(r, 10, 20 + 32 + 1 * 20, CABINET_PROPS_WIDTH - 20, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, "Size:", PROPS_LABEL_FILE_SIZE, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECTA(r, 10, 20 + 32 + 2 * 20, CABINET_PROPS_WIDTH - 20, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, "Size on disk:", PROPS_LABEL_DISK_SIZE, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECTA(r, 10, 20 + 32 + 4 * 20, CABINET_PROPS_WIDTH - 20, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, "Created:", PROPS_LABEL_CREATE_TIME, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECTA(r, 10, 20 + 32 + 5 * 20, CABINET_PROPS_WIDTH - 20, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, "Modified:", PROPS_LABEL_MODIFY_TIME, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECTA(r, 10, 20 + 32 + 7 * 20, CABINET_PROPS_WIDTH - 20, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, "Attributes:", PROPS_LABEL_ATTRIBUTES, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			// Add the actual information itself
			char buf[512];
			RECTA(r, 80, 20 + 32 + 0 * 20, CABINET_PROPS_WIDTH - 90, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXTHUGE, r, NULL, PROPS_LOCATION, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			SetHugeLabelText(pWindow, PROPS_LOCATION, path);
			
			FormatSizeDetailed(res.m_size, buf);
			RECTA(r, 80, 20 + 32 + 1 * 20, CABINET_PROPS_WIDTH - 90, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, buf, PROPS_FILE_SIZE, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			FormatSizeDetailed(res.m_blocks * 512, buf);
			RECTA(r, 80, 20 + 32 + 2 * 20, CABINET_PROPS_WIDTH - 90, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, buf, PROPS_DISK_SIZE, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			TimeStruct str;
			GetHumanTimeFromEpoch(res.m_createTime, &str);
			sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d", str.day, str.month, str.year, str.hours, str.minutes, str.seconds);
			
			RECTA(r, 80, 20 + 32 + 4 * 20, CABINET_PROPS_WIDTH - 90, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, buf, PROPS_CREATE_TIME, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			GetHumanTimeFromEpoch(res.m_modifyTime, &str);
			sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d", str.day, str.month, str.year, str.hours, str.minutes, str.seconds);
			
			RECTA(r, 80, 20 + 32 + 5 * 20, CABINET_PROPS_WIDTH - 90, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, buf, PROPS_MODIFY_TIME, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			sprintf(buf, "%s, %s, %s",
				(res.m_perms & PERM_READ)    ? "Read"    : "No Read",
				(res.m_perms & PERM_WRITE)   ? "Write"   : "No Write",
				(res.m_perms & PERM_EXEC)    ? "Execute" : "No Execute");
			
			RECTA(r, 80, 20 + 32 + 7 * 20, CABINET_PROPS_WIDTH - 90, lineHeight + 4);
			AddControl(pWindow, CONTROL_TEXT, r, buf, PROPS_ATTRIBUTES, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == PROPS_NOTHING)
				DestroyWindow(pWindow);
			
			break;
		}
		case EVENT_TABCHANGED:
		{
			if (parm1 != PROPS_TABPICKER) break;
			UNUSED int oldTab = GET_X_PARM(parm2), newTab = GET_Y_PARM(parm2);
			
			CabinetPropertiesSetTabVisible(pWindow, oldTab, false);
			CabinetPropertiesSetTabVisible(pWindow, newTab, true);
			
			break;
		}
		case EVENT_DESTROY:
		{
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
			break;
		}
		default:
			DefaultWindowProc(pWindow, eventType, parm1, parm2);
	}
}

void CreatePropertiesWindow(Window * parent, const char* path, const char* justName)
{
	char thing[1024];
	snprintf(thing, sizeof thing, "Properties of %s", justName);
	
	const char*  ptrs[2];
	ptrs[0] = path;
	ptrs[1] = justName;
	
	PopupWindowEx(parent, thing, parent->m_rect.left + 100, parent->m_rect.top + 100, CABINET_PROPS_WIDTH, CABINET_PROPS_HEIGHT, CabinetPropertiesProc, WF_NOCLOSE | WF_NOMINIMZ, ptrs);
}


