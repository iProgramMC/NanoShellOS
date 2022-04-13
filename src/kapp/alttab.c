/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

          Alt-Tab display module
******************************************/

#include <wbuiltin.h>


//DEFINES//
#define ALT_TAB_WIDTH 400
#define ALT_TAB_HEITE  70

//EXTERN FUNCTIONS//
void NukeWindow (Window* pWindow);
void SelectWindow (Window* pWindow);
void PaintWindowBackgroundAndBorder (Window* pWindow);

//EXTERNS//
extern Window  g_windows        [WINDOWS_MAX];
extern short   g_windowDrawOrder[WINDOWS_MAX];

//INTERNAL VARIABLES//
static Window* g_pAltTab = NULL;
static int     g_windowDrawOrderIndex = -1;
static short   g_windowIndexOrder[WINDOWS_MAX], g_windowIndexOrderSize;
static bool    g_temporaryArray[WINDOWS_MAX];

//ENUMS//
enum{
	ALTTAB_ICON=10000,
	ALTTAB_TEXT
};

void AltTab$ResetWindowIndexOrder()
{
	memset (g_windowIndexOrder, 0, sizeof(g_windowIndexOrder));
	memset (g_temporaryArray,   0, sizeof(g_temporaryArray));
	g_windowIndexOrderSize = 0;
}
void AltTab$CalcWindowIndexOrder()
{
	AltTab$ResetWindowIndexOrder();
	for (int i = WINDOWS_MAX-1; i >= 0; i--)
	{
		if (g_windowDrawOrder[i] < 0) continue;
		if (g_windowDrawOrder[i] >= WINDOWS_MAX) continue;
		if (g_temporaryArray[g_windowDrawOrder[i]] == 0)
		{
			if (!g_windows[g_windowDrawOrder[i]].m_used) continue;
			g_windowIndexOrder[g_windowIndexOrderSize++] = g_windowDrawOrder[i];
			if (g_windowIndexOrderSize >= WINDOWS_MAX) return;
			g_temporaryArray[g_windowDrawOrder[i]]++;
			if (g_windows[g_windowDrawOrder[i]].m_isSelected)
			{
				g_windowDrawOrderIndex = g_windowIndexOrderSize;
			}
		}
	}
	if (g_windowDrawOrderIndex >= g_windowIndexOrderSize)
		g_windowDrawOrderIndex = 0;
}

void AltTabProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			//add an icon and a text control:
			Rectangle rect;
			RECT(rect, 16, 16, 32, 32);
			AddControl(pWindow, CONTROL_ICON, rect, NULL, ALTTAB_ICON, ICON_ERROR, 0);
			RECT(rect, 64, 16, ALT_TAB_WIDTH-96, 32);
			AddControl(pWindow, CONTROL_TEXTCENTER, rect, "Text goes here.", ALTTAB_TEXT, 0, TEXTSTYLE_VCENTERED | TEXTSTYLE_HCENTERED);
			
			break;
		}
		case EVENT_PAINT:
		{
			PaintWindowBackgroundAndBorder(pWindow);
			
			//get info about the selected application:
			Window* pSelWnd = NULL;
			if (!g_windows[g_windowIndexOrder[g_windowDrawOrderIndex]].m_used) break;
			
			pSelWnd = &g_windows[g_windowIndexOrder[g_windowDrawOrderIndex]];
			SetLabelText(pWindow, ALTTAB_TEXT, pSelWnd->m_title);
			SetIcon     (pWindow, ALTTAB_ICON, pSelWnd->m_iconID);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void UpdateAltTabWindow()
{
	if (g_pAltTab)
		HandleMessages(g_pAltTab);
}
void ProgressAltTab()
{
	g_windowDrawOrderIndex++;
	if (g_windowDrawOrderIndex >= g_windowIndexOrderSize)
		g_windowDrawOrderIndex = 0;
	
	if (g_pAltTab)
		RequestRepaint(g_pAltTab);
}
void OnPressAltTabOnce()
{
	if (!g_pAltTab)
	{
		AltTab$CalcWindowIndexOrder();
		if (g_windowIndexOrderSize <= 1) return;
		
		int screenWidth = GetScreenWidth(), screenHeite = GetScreenHeight();
		g_pAltTab = CreateWindow(
			"Alt-tab handler",
			(screenWidth - ALT_TAB_WIDTH) / 2,
			(screenHeite - ALT_TAB_HEITE) / 2,
			ALT_TAB_WIDTH,
			ALT_TAB_HEITE,
			AltTabProc,
			WF_NOCLOSE|WF_NOMINIMZ|WF_NOTITLE|WF_SYSPOPUP
		);
	}
	else
		ProgressAltTab();
}
void KillAltTab()
{
	if (g_pAltTab)
	{
		NukeWindow(g_pAltTab);
		
		//select the window user requested.
		if (g_windowIndexOrder[g_windowDrawOrderIndex] != -1)
		{
			if (g_windows[g_windowIndexOrder[g_windowDrawOrderIndex]].m_used)
				SelectWindow(g_windows + g_windowIndexOrder[g_windowDrawOrderIndex]);
		}
	}
	
	g_pAltTab = NULL;
}
