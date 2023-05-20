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
#include <config.h>

#define SYSMON_WIDTH  486
#define SYSMON_HEIGHT 500

typedef struct
{
	int dataPointCPUUsage;
	int dataPointMemUsage;
	int dataPointWmFps;
	
	int xPos;
	
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
	FPS_LABEL,
	PFCOUNT_LABEL,
};

const char *GetTaskSuspendStateStr (int susp_type)
{
	switch (susp_type)
	{
		case SUSPENSION_NONE:                 return "Active";
		case SUSPENSION_TOTAL:                return "Suspended";
		case SUSPENSION_UNTIL_WM_UPDATE:      return "Wait WM";
		case SUSPENSION_UNTIL_TASK_EXPIRY:    return "Wait Task";
		case SUSPENSION_UNTIL_TIMER_EXPIRY:   return "Sleeping";
		case SUSPENSION_UNTIL_PROCESS_EXPIRY: return "Wait Process";
		case SUSPENSION_UNTIL_PIPE_READ:
		case SUSPENSION_UNTIL_PIPE_WRITE:     return "Wait Pipe";
		case SUSPENSION_UNTIL_OBJECT_EVENT:   return "Wait Object";
	}
	return "Unknown";
}

extern Task     g_runningTasks[];
extern Process  gProcesses[];
extern uint64_t g_kernelCpuTimeTotal;

static uint64_t s_cpu_time[C_MAX_TASKS], s_cpu_time_total = 0, s_idle_time_total;

// This allows multiple instances of SystemMonitor to run at the same time.
void MonitorSystem()
{
	//pause everything while we get timing information from these tasks
	KeVerifyInterruptsDisabled;
	s_cpu_time_total = s_idle_time_total = g_kernelCpuTimeTotal;
	
	g_kernelCpuTimeTotal = 0;
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		s_cpu_time[i] = g_runningTasks[i].m_cpuTimeTotal;
		g_runningTasks[i].m_cpuTimeTotal = 0;
		if (g_runningTasks[i].m_bExists)
			s_cpu_time_total += s_cpu_time[i];
	}
}

#define SUSPENSION_IDLE (-1)
#define TABLE_COLS      (6)

enum Column
{
	COL_TID,
	COL_PID,
	COL_IMAGE_NAME,
	COL_CPU_PERC,
	COL_STATUS,
	COL_PAGE_FAULTS,
};

bool SystemMonitorShowGraph()
{
	const char* p = CfgGetEntryValue("SystemMonitor::ShowGraph");
	
	if (!p) return true;
	
	// if it's different from 'no', show the graph.
	return strcmp(p, "no") != 0;
}

static void AddColumns(Window * pWindow)
{
	int tid_width = 30, pid_width = 30, image_name_width = 200, cpu_perc_width = 50, status_width = 75, page_faults_width = 75;
	
	if (IsLowResolutionMode())
	{
		tid_width = 30;
		pid_width = 30;
		image_name_width = 100;
		cpu_perc_width   = 30;
		status_width = 60;
		page_faults_width = 40;
	}
	
	AddTableColumn(pWindow, PROCESS_LISTVIEW, "TID",         tid_width);
	AddTableColumn(pWindow, PROCESS_LISTVIEW, "PID",         pid_width);
	AddTableColumn(pWindow, PROCESS_LISTVIEW, "Image Name",  image_name_width);
	AddTableColumn(pWindow, PROCESS_LISTVIEW, "CPU %",       cpu_perc_width);
	AddTableColumn(pWindow, PROCESS_LISTVIEW, "Status",      status_width);
	AddTableColumn(pWindow, PROCESS_LISTVIEW, "Page Faults", page_faults_width);
}

static void AddProcessToList(Window* pWindow, int tid, int pid, const char * name, int susp_type, int cpu_percent, int icon, ThreadStats* stats)
{
	const char* buf[TABLE_COLS] = { 0 };
	
	char pid_buf[16];
	sprintf(pid_buf, "%d", pid);
	char tid_buf[16];
	sprintf(tid_buf, "%d", tid);
	char cpu_percent_buf[16];
	sprintf(cpu_percent_buf, "%d%%", cpu_percent);
	
	buf[COL_TID] = tid_buf;
	buf[COL_PID] = pid_buf;
	buf[COL_IMAGE_NAME] = name;
	buf[COL_CPU_PERC] = cpu_percent_buf;
	
	switch (susp_type)
	{
		case SUSPENSION_IDLE:
			buf[COL_STATUS] = "Idle";
			break;
		case SUSPENSION_NONE:
		case SUSPENSION_UNTIL_OBJECT_EVENT:
		case SUSPENSION_UNTIL_WM_UPDATE:
			buf[COL_STATUS] = "Running";
			break;
		case SUSPENSION_TOTAL:
			buf[COL_STATUS] = "Suspended";
			break;
		case SUSPENSION_UNTIL_TIMER_EXPIRY:
			buf[COL_STATUS] = "Sleeping";
			break;
		case SUSPENSION_UNTIL_PIPE_WRITE:
		case SUSPENSION_UNTIL_PIPE_READ:
		case SUSPENSION_UNTIL_PROCESS_EXPIRY:
		case SUSPENSION_UNTIL_TASK_EXPIRY:
			buf[COL_STATUS] = "Blocked";
			break;
		default:
			buf[COL_STATUS] = "Unknown";
			break;
	}
	
	char pf_buf[16];
	pf_buf[0] = 0;
	
	if (stats)
	{
		sprintf(pf_buf, "%d", stats->m_pageFaults);
	}
	
	buf[COL_PAGE_FAULTS] = pf_buf;
	
	AddTableRow(pWindow, PROCESS_LISTVIEW, buf, icon);
}

static int ResolveTidFromTable(Window* pWindow, int selIndex)
{
	const char * things[TABLE_COLS] = { 0 };
	
	if (!GetRowStringsFromTable(pWindow, PROCESS_LISTVIEW, selIndex, things)) return -1;
	
	if (!things[COL_TID]) return -1;
	
	return atoi(things[COL_TID]);
}

//returns the amount of time the CPU spent idling
int UpdateSystemMonitorLists(Window* pWindow)
{
	int scroll = GetScrollTable(pWindow, PROCESS_LISTVIEW);
	int selind = GetSelectedIndexTable(pWindow, PROCESS_LISTVIEW);
	
	// Get the TID that should be selected.
	int tid = ResolveTidFromTable(pWindow, selind);
	selind = -1;
	
	// Update process list view.
	ResetTable(pWindow, PROCESS_LISTVIEW);
	AddColumns(pWindow);
	
	char buffer [1024];
	int cpu_usage_idle_percent = 0;
	// update kernel idle process
	{
		uint64_t cpu_usage_percent_64 = s_idle_time_total * 100 / s_cpu_time_total;
		cpu_usage_idle_percent = (int)cpu_usage_percent_64;
		
		AddProcessToList(pWindow, 0, 0, "System idle", SUSPENSION_IDLE, cpu_usage_idle_percent, ICON_APPLICATION, NULL);
	}
	
	int index = 1;
	
	for (int i = 0; i < C_MAX_TASKS; i++)
	{
		Task* pTask = g_runningTasks + i;
		if (pTask->m_bExists)
		{
			uint64_t time1 = s_cpu_time[i];
			
			//SLogMsg("TID %d  usage: %l out of %l", i, time1, s_cpu_time_total);
			
			uint64_t cpu_usage_percent_64 = time1 * 100 / s_cpu_time_total;
			int cpu_usage_percent = (int)cpu_usage_percent_64;
			
			const char * name;
			if (*pTask->m_tag == 0)
				name = pTask->m_authorFunc;
			else
				name = pTask->m_tag;
			
			int pid = 0;
			if (pTask->m_pProcess)
			{
				pid = (Process*)pTask->m_pProcess - gProcesses;
			}
			
			AddProcessToList(pWindow, i, pid, name, pTask->m_suspensionType, cpu_usage_percent, ICON_APPLICATION, KeGetTaskStats(pTask));
			
			if (i == tid) selind = index;
			
			index++;
		}
	}
	
	int npp = MpGetNumAvailablePages(), nfpp = MpGetNumFreePages();
	sprintf(buffer, "Memory: %d / %d KB (%d / %d pages)     ", (npp-nfpp)*4, npp*4, npp-nfpp, npp);
	SetLabelText(pWindow, MEMORY_LABEL, buffer);
	
	strcpy(buffer, "Uptime: ");
	FormatTime(buffer, FORMAT_TYPE_VAR, GetTickCount() / 1000);
	strcat (buffer, "      ");
	SetLabelText(pWindow, UPTIME_LABEL, buffer);
	
	sprintf(buffer, "FPS: %d        ", GetWindowManagerFPS());
	SetLabelText(pWindow, FPS_LABEL, buffer);
	
	sprintf(buffer, "Page Faults: %d        ", MmGetNumPageFaults());
	SetLabelText(pWindow, PFCOUNT_LABEL, buffer);
	
	SetScrollTable(pWindow, PROCESS_LISTVIEW, scroll);
	SetSelectedIndexTable(pWindow, PROCESS_LISTVIEW, selind);
	
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
	SystemMonitorInstance *pInst = (SystemMonitorInstance*)pWindow->m_data;
	Image *pImg = pInst->pImg;
	
	if (!pImg)
		return;
	
	int cpu_usage_total = 100 - cpu_idle_time;
	int npp = MpGetNumAvailablePages(), nfpp = MpGetNumFreePages();
	int memUsedKB = (npp - nfpp) * 4;
	int fps = 60 - GetWindowManagerFPS();
	if (fps < 0)  fps = 0;
	if (fps > 60) fps = 60;
	
#define GRAPH_QUANTUM_WIDTH (4)
	
	ShiftImageLeftBy(pImg, GRAPH_QUANTUM_WIDTH); // 4 pixels <--
	
	VBEData image_data, *backup;
	extern VBEData* g_vbeData;
	backup = g_vbeData;
	
	BuildGraphCtxBasedOnImage (&image_data, pImg);
	
	VidSetVBEData(&image_data);
	VidFillRect(0x000000, pImg->width - GRAPH_QUANTUM_WIDTH, 0, pImg->width, pImg->height);
	
	// draw the backdrop:
	for (int i = pImg->width - GRAPH_QUANTUM_WIDTH; i < pImg->width; i++)
	{
		if (pInst->xPos % 10 == 0)
		{
			VidDrawVLine(0x303030, 0, pImg->height, i);
		}
		else for (int j = 0; j < 100; j += 10)
		{
			VidPlotPixel(i, j, 0x303030);
		}
		
		pInst->xPos++;
	}
	
	// draw lines:
	
	// memory usage
	VidDrawLine(
		// color
		0xFF0000, 
		// pos 1
		pImg->width - GRAPH_QUANTUM_WIDTH, ConvertToGraphPos(pImg, pInst->dataPointWmFps, 60),
		// pos 2
		pImg->width - 1, ConvertToGraphPos(pImg, fps, 61));
	
	// memory usage
	VidDrawLine(
		// color
		0x0000FF, 
		// pos 1
		pImg->width - GRAPH_QUANTUM_WIDTH, ConvertToGraphPos(pImg, pInst->dataPointMemUsage, npp * 4),
		// pos 2
		pImg->width - 1, ConvertToGraphPos(pImg, memUsedKB, npp * 4));
	
	// CPU usage
	VidDrawLine(
		// color
		0x00FF00, 
		// pos 1
		pImg->width - GRAPH_QUANTUM_WIDTH, ConvertToGraphPos(pImg, pInst->dataPointCPUUsage, 100),
		// pos 2
		pImg->width - 1, ConvertToGraphPos(pImg, cpu_usage_total, 100));
	
	//update the data points
	pInst->dataPointCPUUsage = cpu_usage_total;
	pInst->dataPointMemUsage = memUsedKB;
	pInst->dataPointWmFps    = fps;
	
	VidSetVBEData(backup);
}

void CALLBACK SystemMonitorProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	int wnheight = SYSMON_HEIGHT, wnwidth = SYSMON_WIDTH;
	if (IsLowResolutionMode())
	{
		wnheight -= 180;
		wnwidth = 312;
	}
	
	switch (messageType)
	{
		#define PADDING_AROUND_LISTVIEW 8
		case EVENT_PAINT:
		{
			//paint the image
			
			SystemMonitorInstance *pInst = (SystemMonitorInstance*)pWindow->m_data;
			Image *pImg = pInst->pImg;
			
			if (!pImg) break;
			
			int listview_y = PADDING_AROUND_LISTVIEW;
			//int listview_width  = wnwidth   - PADDING_AROUND_LISTVIEW * 2;
			int listview_height = wnheight - 100 - pImg->height - PADDING_AROUND_LISTVIEW * 2;
			
			int x = (pWindow->m_vbeData.m_width - (wnwidth - 20)) / 2;
			int y = (pWindow->m_vbeData.m_height - wnheight) + listview_y + listview_height + 10;
			
			if (pImg)
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
			CallControlCallback(pWindow, FPS_LABEL, EVENT_PAINT, 0, 0);
			CallControlCallback(pWindow, PFCOUNT_LABEL, EVENT_PAINT, 0, 0);
			CallControlCallback(pWindow, PROCESS_LISTVIEW, EVENT_PAINT, 0, 0);
			SystemMonitorProc  (pWindow, EVENT_PAINT, 0, 0);
			
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r;
			// Add a list view control.
			
			AddTimer(pWindow, 1000, EVENT_UPDATE);
			
			int listview_y = PADDING_AROUND_LISTVIEW;
			int listview_width  = wnwidth   - PADDING_AROUND_LISTVIEW * 2;
			
			SystemMonitorInstance *pInst = (SystemMonitorInstance*)pWindow->m_data;
			Image *pImg = pInst->pImg;
			
			int image_height = 0;
			
			if (pImg)
				image_height = pImg->height;
			
			int listview_height = wnheight - 100 - image_height - PADDING_AROUND_LISTVIEW * 2;
			
			RECT(r, 
				/*X Coord*/ PADDING_AROUND_LISTVIEW, 
				/*Y Coord*/ listview_y, 
				/*X Size */ listview_width, 
				/*Y Size */ listview_height
			);
			
			AddControlEx (pWindow, CONTROL_TABLEVIEW, ANCHOR_RIGHT_TO_RIGHT | ANCHOR_BOTTOM_TO_BOTTOM, r, NULL, PROCESS_LISTVIEW, 0, 0);
			
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + image_height + 24, listview_width, 20);
			AddControlEx (pWindow, CONTROL_TEXTCENTER, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "Please wait...", MEMORY_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + image_height + 44, 150, 20);
			AddControlEx (pWindow, CONTROL_TEXTCENTER, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "Please wait...", FPS_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			
			RECT (r, PADDING_AROUND_LISTVIEW + 150, listview_y + listview_height + image_height + 44, listview_width - 150, 20);
			AddControlEx (pWindow, CONTROL_TEXTCENTER, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "Please wait...", UPTIME_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			
			RECT (r, PADDING_AROUND_LISTVIEW, listview_y + listview_height + image_height + 64, listview_width, 20);
			AddControlEx (pWindow, CONTROL_TEXTCENTER, ANCHOR_BOTTOM_TO_BOTTOM | ANCHOR_TOP_TO_BOTTOM, r, "Please wait...", PFCOUNT_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL);
			
			break;
		}
		
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void SystemMonitorEntry (__attribute__((unused)) int argument)
{
	int imheight = 100, wnheight = SYSMON_HEIGHT, wnwidth = SYSMON_WIDTH;
	if (IsLowResolutionMode())
	{
		imheight = 50;
		wnheight -= 180;
		wnwidth = 312;
	}
	
	int flags = WF_ALWRESIZ;
	
	const char* pVal = CfgGetEntryValue("SystemMonitor::AlwaysOnTop");
	if (!pVal  ||  strcmp(pVal, "yes") == 0)
	{
		flags |= WF_FOREGRND;
	}
	
	// create ourself a window:
	Window* pWindow = CreateWindow ("System Monitor", CW_AUTOPOSITION, CW_AUTOPOSITION, wnwidth, wnheight, SystemMonitorProc, flags);
	
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
	
	Image* pSystemImage = NULL;
	if (SystemMonitorShowGraph())
	{
		pSystemImage = BitmapAllocate (wnwidth - 16, imheight, 0x000020);
		if (!pInstance)
		{
			MmFree (pInstance);
			DestroyWindow (pWindow);
			while (HandleMessages (pWindow));
			return;
		}
		pInstance->pImg = pSystemImage;
	}
	
	pWindow->m_data = pInstance;
	
	while (HandleMessages(pWindow));

	if (pSystemImage)
		MmFree(pSystemImage);
	
	MmFree(pInstance);
}
