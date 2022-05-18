/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        SysMon  Application module
******************************************/

#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>
#include <elf.h>
#include <task.h>
#define SYSMON_WIDTH  300
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

const char *GetTaskSuspendStateStr (int susp_type)
{
	switch (susp_type)
	{
		case SUSPENSION_NONE:  return "Active";
		case SUSPENSION_TOTAL: return "Suspended";
		case SUSPENSION_UNTIL_TASK_EXPIRY:    return "Wait Task";
		case SUSPENSION_UNTIL_TIMER_EXPIRY:   return "Sleeping";
		case SUSPENSION_UNTIL_PROCESS_EXPIRY: return "Wait Process";
	}
	return "Unknown";
}

extern Task g_runningTasks[];
extern uint64_t g_kernelCpuTimeTotal;

static uint64_t s_cpu_time[C_MAX_TASKS], s_cpu_time_total = 0, s_idle_time_total;

// This allows multiple instances of SystemMonitor to run at the same time.
void MonitorSystem()
{
	//pause everything while we get timing information from these tasks
	cli;
	s_cpu_time_total = s_idle_time_total = g_kernelCpuTimeTotal;
	
	g_kernelCpuTimeTotal = 0;
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		s_cpu_time[i] = g_runningTasks[i].m_cpuTimeTotal;
		g_runningTasks[i].m_cpuTimeTotal = 0;
		if (g_runningTasks[i].m_bExists)
			s_cpu_time_total += s_cpu_time[i];
	}
	sti;
}

int GetNumPhysPages(void);
int GetNumFreePhysPages(void);
void UpdateSystemMonitorLists(Window* pWindow)
{
	// Update process list view.
	ResetList(pWindow, PROCESS_LISTVIEW);
	
	char buffer [1024];
	// update kernel idle process
	{
		uint64_t cpu_usage_percent_64 = s_idle_time_total * 100 / s_cpu_time_total;
		int cpu_usage_percent = (int)cpu_usage_percent_64;
		sprintf (buffer, "[Idle]  [%d%%] System idle", cpu_usage_percent);
		AddElementToList(pWindow, PROCESS_LISTVIEW, buffer, ICON_APPLICATION);
	}
	
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		Task* pTask = g_runningTasks + i;
		if (pTask->m_bExists)
		{
			uint64_t time1 = s_cpu_time[i];
			
			uint64_t cpu_usage_percent_64 = time1 * 100 / s_cpu_time_total;
			int cpu_usage_percent = (int)cpu_usage_percent_64;
			
			if (strlen(pTask->m_tag) == 0)
				sprintf (buffer, "[%s]  [%d%%] %s [%s:%d]", GetTaskSuspendStateStr (pTask->m_suspensionType), cpu_usage_percent, pTask->m_authorFunc, pTask->m_authorFile, pTask->m_authorLine);
			else
				sprintf (buffer, "[%s]  [%d%%] %s",         GetTaskSuspendStateStr (pTask->m_suspensionType), cpu_usage_percent, pTask->m_tag);
			AddElementToList(pWindow, PROCESS_LISTVIEW, buffer, ICON_APPLICATION);
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
			
			AddControlEx (pWindow, CONTROL_LISTVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, PROCESS_LISTVIEW, 0, 0);
			
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + 4, listview_width, 20);
			AddControlEx (pWindow, CONTROL_TEXT, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "placeholder", MEMORY_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + 24, listview_width, 20);
			AddControlEx (pWindow, CONTROL_TEXT, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "placeholder", UPTIME_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
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
	Window* pWindow = CreateWindow ("System Monitor", xPos, yPos, SYSMON_WIDTH, SYSMON_HEIGHT, SystemMonitorProc, WF_ALWRESIZ);
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
	int next_tick_in = GetTickCount();
	while (HandleMessages (pWindow))
	{
		if (GetTickCount() >= next_tick_in)
		{
			next_tick_in += 1000;
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
			WindowRegisterEvent(pWindow, EVENT_PAINT,  0, 0);
		}
	}
#endif
}
