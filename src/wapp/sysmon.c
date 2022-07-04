/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        SysMon  Application module
******************************************/

#include <wbuiltin.h>
#include <widget.h>
#include <vfs.h>
#include <elf.h>
#include <image.h>
#include <task.h>

#define SYSMON_WIDTH  400
#define SYSMON_HEIGHT 400

typedef struct
{
	int dataPointCPUUsage;
	int dataPointMemUsage;
	int dataPointWmFps;
	
	Image* pImg;
}
SystemMonitorInstance;

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
		case SUSPENSION_UNTIL_WM_UPDATE: return "Wait WM";
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

//returns the amount of time the CPU spent idling
int UpdateSystemMonitorLists(Window* pWindow)
{
	// Update process list view.
	ResetList(pWindow, PROCESS_LISTVIEW);
	
	char buffer [1024];
	int cpu_usage_idle_percent = 0;
	// update kernel idle process
	{
		uint64_t cpu_usage_percent_64 = s_idle_time_total * 100 / s_cpu_time_total;
		cpu_usage_idle_percent = (int)cpu_usage_percent_64;
		sprintf (buffer, "[Idle]  [%d%%] System idle", cpu_usage_idle_percent);
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
	
	return cpu_usage_idle_percent;
}
void ShiftImageLeftBy(Image *pImg, int by)
{
	uint32_t* pFB = (uint32_t*)pImg->framebuffer;
	for (int i = 0; i < pImg->height; i++)
	{
		// this'll probably work, but not for the opposite direction
		memcpy_ints(&pFB[pImg->width * i], &pFB[pImg->width * i + by], pImg->width - by);
	}
}
SAI int ConvertToGraphPos(Image *pImg, int y, int ymax)
{
	return (int)((uint64_t)(ymax - y) * (uint64_t)pImg->height / (uint64_t)ymax);
}
void UpdateSystemMonitorGraph(Window* pWindow, int cpu_idle_time)
{
	int cpu_usage_total = 100 - cpu_idle_time;
	int npp = GetNumPhysPages(), nfpp = GetNumFreePhysPages();
	int memUsedKB = (npp - nfpp) * 4;
	int fps = 60 - GetWindowManagerFPS();
	if (fps < 0)  fps = 0;
	if (fps > 60) fps = 60;
	
	SystemMonitorInstance *pInst = (SystemMonitorInstance*)pWindow->m_data;
	Image *pImg = pInst->pImg;
	
	ShiftImageLeftBy(pImg, 4); // 4 pixels <--
	
	VBEData image_data, *backup;
	extern VBEData* g_vbeData;
	backup = g_vbeData;
	
	BuildGraphCtxBasedOnImage (&image_data, pImg);
	
	VidSetVBEData(&image_data);
	VidFillRect(0x000000, pImg->width - 4, 0, pImg->width, pImg->height);
	
	// draw lines:
	
	// memory usage
	VidDrawLine(
		// color
		0xFF0000, 
		// pos 1
		pImg->width - 4, ConvertToGraphPos(pImg, pInst->dataPointWmFps, 60),
		// pos 2
		pImg->width - 1, ConvertToGraphPos(pImg, fps,                   61));
	
	// memory usage
	VidDrawLine(
		// color
		0x0000FF, 
		// pos 1
		pImg->width - 4, ConvertToGraphPos(pImg, pInst->dataPointMemUsage, npp * 4),
		// pos 2
		pImg->width - 1, ConvertToGraphPos(pImg, memUsedKB,                npp * 4));
	
	// CPU usage
	VidDrawLine(
		// color
		0x00FF00, 
		// pos 1
		pImg->width - 4, ConvertToGraphPos(pImg, pInst->dataPointCPUUsage, 100),
		// pos 2
		pImg->width - 1, ConvertToGraphPos(pImg, cpu_usage_total,          100));
	
	//update the data points
	pInst->dataPointCPUUsage = cpu_usage_total;
	pInst->dataPointMemUsage = memUsedKB;
	pInst->dataPointWmFps    = fps;
	
	VidSetVBEData(backup);
}

void CALLBACK SystemMonitorProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		#define PADDING_AROUND_LISTVIEW 8
		case EVENT_PAINT:
		{
			//paint the image
			
			int listview_y = PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT;
			int listview_width  = SYSMON_WIDTH   - PADDING_AROUND_LISTVIEW * 2;
			int listview_height = (SYSMON_HEIGHT) / 2  - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT;
			
			int x = (pWindow->m_vbeData.m_width - (SYSMON_WIDTH - 20)) / 2;
			int y = (pWindow->m_vbeData.m_height - SYSMON_HEIGHT) + listview_y + listview_height + 10;
			
			SystemMonitorInstance *pInst = (SystemMonitorInstance*)pWindow->m_data;
			Image *pImg = pInst->pImg;
			
			VidBlitImage(pImg, x, y);
			
			break;
		}
		case EVENT_COMMAND:
		{
			break;
		}
		case EVENT_UPDATE:
		{
			UpdateSystemMonitorGraph (pWindow, UpdateSystemMonitorLists (pWindow));
			
			CallControlCallback(pWindow, MEMORY_LABEL, EVENT_PAINT, 0, 0);
			CallControlCallback(pWindow, UPTIME_LABEL, EVENT_PAINT, 0, 0);
			CallControlCallback(pWindow, PROCESS_LISTVIEW, EVENT_PAINT, 0, 0);
			SystemMonitorProc  (pWindow, EVENT_PAINT, 0, 0);
			
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			
			int listview_y = PADDING_AROUND_LISTVIEW + TITLE_BAR_HEIGHT;
			int listview_width  = SYSMON_WIDTH   - PADDING_AROUND_LISTVIEW * 2;
			int listview_height = (SYSMON_HEIGHT) / 2  - PADDING_AROUND_LISTVIEW * 2 - TITLE_BAR_HEIGHT;
			
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ listview_y, 
				/*X Size */ listview_width, 
				/*Y Size */ listview_height
			);
			
			AddControlEx (pWindow, CONTROL_LISTVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, PROCESS_LISTVIEW, 0, 0);
			
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + 124, listview_width, 20);
			AddControlEx (pWindow, CONTROL_TEXT, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "Please wait...", MEMORY_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + 144, listview_width, 20);
			AddControlEx (pWindow, CONTROL_TEXT, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "Please wait...", UPTIME_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			break;
		}
		
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void SystemMonitorEntry (__attribute__((unused)) int argument)
{
	// create ourself a window:
	Window* pWindow = CreateWindow ("System Monitor", CW_AUTOPOSITION, CW_AUTOPOSITION, SYSMON_WIDTH, SYSMON_HEIGHT, SystemMonitorProc, WF_ALWRESIZ);
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	pWindow->m_iconID = ICON_SYSMON;
	
	SystemMonitorInstance* pInstance = (SystemMonitorInstance*)MmAllocate(sizeof(SystemMonitorInstance));
	if (!pInstance)
	{
		DestroyWindow (pWindow);
		while (HandleMessages (pWindow));
		return;
	}
	memset (pInstance, 0, sizeof *pInstance);
	
	Image* pSystemImage = BitmapAllocate (SYSMON_WIDTH - 20, 100, 0x0000FF);
	if (!pInstance)
	{
		MmFree (pInstance);
		DestroyWindow (pWindow);
		while (HandleMessages (pWindow));
		return;
	}
	pInstance->pImg = pSystemImage;
	
	pWindow->m_data = pInstance;
	
	// event loop:
#if THREADING_ENABLED
	int next_tick_in = GetTickCount();
	while (HandleMessages (pWindow))
	{
		if (GetTickCount() >= next_tick_in)
		{
			next_tick_in += 1000;
			WindowRegisterEvent(pWindow, EVENT_UPDATE, 0, 0);
		}
	}
#endif

	MmFree(pSystemImage);
}
