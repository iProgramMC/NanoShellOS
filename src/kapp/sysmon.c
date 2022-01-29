/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Cabinet Application module
******************************************/

#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>
#include <elf.h>
#include <task.h>
#define SYSMON_WIDTH  600
#define SYSMON_HEIGHT 300

enum
{
	ZERO,
	PROCESS_LISTVIEW,
	CPU_GRAPHVIEW,
	MEMORY_LABEL,
	TASKSOPEN_LABEL,
	CPUPERCENT_LABEL,
	UPTIME_LABEL,
};

extern Task g_runningTasks[];

int GetNumPhysPages(void);
int GetNumFreePhysPages(void);
void UpdateSystemMonitorLists(Window* pWindow)
{
	// Update process list view.
	ResetList(pWindow, PROCESS_LISTVIEW);
	
	char buffer [1024];
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		Task* pTask = g_runningTasks + i;
		if (pTask->m_bExists)
		{
			sprintf (buffer, "THREAD '%s': %s:%d (%s) : F:0x%x",/* V:0x%x H:0x%x S:0x%x A:0x%x", */
				pTask->m_tag,
				pTask->m_authorFile,
				pTask->m_authorLine,
				pTask->m_authorFunc,
				pTask->m_pFunction/*,
				pTask->m_pVBEContext,
				pTask->m_pCurrentHeap,
				pTask->m_pStack,
				pTask->m_argument*/
			);
			AddElementToList(pWindow, PROCESS_LISTVIEW, buffer, ICON_BOMB);
		}
	}
	
	
	int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	sprintf(buffer, "Memory: %d / %d KB (%d / %d pages)     ", (npp-nfpp)*4, npp*4, npp-nfpp, npp);
	SetLabelText(pWindow, MEMORY_LABEL, buffer);
	
	sprintf(buffer, "FPS: %d           Uptime: ", GetWindowManagerFPS());
	FormatTime(buffer, FORMAT_TYPE_VAR, GetTickCount() / 1000);
	strcat (buffer, "      ");
	SetLabelText(pWindow, UPTIME_LABEL, buffer);
}

void CALLBACK SystemMonitorProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_PAINT:
		{
			break;
		}
		case EVENT_COMMAND:
		{
			break;
		}
		case EVENT_UPDATE:
		{
			UpdateSystemMonitorLists (pWindow);
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			#define PADDING_AROUND_LISTVIEW 8
			
			int listview_y = PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT;
			int listview_width  = SYSMON_WIDTH   - PADDING_AROUND_LISTVIEW * 2;
			int listview_height = SYSMON_HEIGHT*3/4  - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT;
			
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ listview_y, 
				/*X Size */ listview_width, 
				/*Y Size */ listview_height
			);
			
			AddControl (pWindow, CONTROL_LISTVIEW, r, NULL, PROCESS_LISTVIEW, 0, 0);
			
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + 4, listview_width, 20);
			AddControl (pWindow, CONTROL_TEXT, r, "placeholder", MEMORY_LABEL, 0, WINDOW_BACKGD_COLOR);
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + 24, listview_width, 20);
			AddControl (pWindow, CONTROL_TEXT, r, "placeholder", UPTIME_LABEL, 0, WINDOW_BACKGD_COLOR);
			
			break;
		}
		
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void SystemMonitorEntry (__attribute__((unused)) int argument)
{
	// create ourself a window:
	int xPos = (GetScreenSizeX() - SYSMON_WIDTH)  / 2;
	int yPos = (GetScreenSizeY() - SYSMON_HEIGHT) / 2;
	Window* pWindow = CreateWindow ("System Monitor", xPos, yPos, SYSMON_WIDTH, SYSMON_HEIGHT, SystemMonitorProc, 0);
	pWindow->m_iconID = ICON_RESMON;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	int timeout = 0;
	while (HandleMessages (pWindow))
	{
		if (timeout == 0)
		{
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			WindowRegisterEvent(pWindow, EVENT_PAINT,  0, 0);
			timeout = 100;
		}
		timeout--;
	}
#endif
}
