/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

      Control panel - Taskbar Applet
******************************************/

#include <wbuiltin.h>

enum
{
	TASKBAR_POPUP_DATECHECK = 1,
	TASKBAR_POPUP_TIMECHECK,
	TASKBAR_POPUP_BUTTON_COMPACT,
	TASKBAR_POPUP_BUTTON_GLOW,
	TASKBAR_POPUP_SOLIDBG,
	TASKBAR_POPUP_SHOWMOVINGWINS,
	TASKBAR_POPUP_SHOWCPUUSAGE,
	TASKBAR_POPUP_SHOWMEMUSAGE,
	TASKBAR_POPUP_USELARGEICONS,
	TASKBAR_POPUP_OK,
	TASKBAR_POPUP_APPLY,
	TASKBAR_POPUP_CANCEL,
};

#define TSKBR_POPUP_WIDTH 350
#define TSKBR_POPUP_HEITE 420

extern bool g_GlowOnHover;
extern bool g_TaskListCompact;
extern bool g_bShowDate, g_bShowTimeSeconds;
extern bool g_bShowCPUUsage, g_bShowMemUsage;
extern bool g_bUseLargeIcons;
extern bool     g_BackgroundSolidColorActive, g_RenderWindowContents;
extern uint32_t g_BackgroundSolidColor;

void RefreshEverything();

void TaskbarSetProperties(bool bShowDate, bool bShowTimeSecs, bool bCompactTaskList, bool bShowCPUUsage, bool bShowMEMUsage, bool bUseLargeIcons);

void CALLBACK CplTaskbarWndProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			pWindow->m_iconID = ICON_HOME;
			
			Rectangle r;
			
			const int horzOffs = 40;
			
			int m = 16, h = 40;
			if (m < GetLineHeight())
				m = GetLineHeight();
			if (h < GetLineHeight())
				h = GetLineHeight();
			
			RECT(r, 10 + horzOffs, 10 + h * 0 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show the date on the task bar", TASKBAR_POPUP_DATECHECK, g_bShowDate, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 1 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show the seconds on the clock on the task bar", TASKBAR_POPUP_TIMECHECK, g_bShowTimeSeconds, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 2 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Make task bar buttons compact", TASKBAR_POPUP_BUTTON_COMPACT, g_TaskListCompact, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 3 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Make buttons glow when hovered over", TASKBAR_POPUP_BUTTON_GLOW, g_GlowOnHover, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 4 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Use solid color instead of background image", TASKBAR_POPUP_SOLIDBG, g_BackgroundSolidColorActive, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 5 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show window contents while moving", TASKBAR_POPUP_SHOWMOVINGWINS, g_RenderWindowContents, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 6 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show CPU usage graph", TASKBAR_POPUP_SHOWCPUUSAGE, g_RenderWindowContents, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 7 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Show memory usage graph", TASKBAR_POPUP_SHOWMEMUSAGE, g_RenderWindowContents, 0);
			
			RECT(r, 10 + horzOffs, 10 + h * 8 + (h - m) / 2, TSKBR_POPUP_WIDTH - 20, 20);
			AddControl(pWindow, CONTROL_CHECKBOX, r, "Double the size of icons on the task bar", TASKBAR_POPUP_USELARGEICONS, g_bUseLargeIcons, 0);
			
			// also add the icons
			static const int icons[] = {
				ICON_SETTING_TB_SHOW_DATE,
				ICON_SETTING_TB_SHOW_SECS,
				ICON_SETTING_COMPACT_ICONS,
				ICON_SETTING_HOVER_GLOW,
				ICON_SETTING_SOLID_BG,
				ICON_SETTING_WINDOW_DRAG,
				ICON_SETTING_GRAPH_CPU,
				ICON_SETTING_GRAPH_MEM,
				ICON_SETTING_ICON_SIZE,
			};
			
			int iconOffset = (h - 32) / 2;
			if (iconOffset < 0)
				iconOffset = 0;
			
			for (int i = 0; i <= 8; i++)
			{
				RECT(r, 10, 10 + h * i + iconOffset, 32, 32);
				AddControl(pWindow, CONTROL_ICON, r, NULL, 9999, icons[i], 0);
			}
			
			RECT(r, TSKBR_POPUP_WIDTH - 60 * 3, TSKBR_POPUP_HEITE - 30, 50, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "OK", TASKBAR_POPUP_OK, 0, 0);
			RECT(r, TSKBR_POPUP_WIDTH - 60 * 2, TSKBR_POPUP_HEITE - 30, 50, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", TASKBAR_POPUP_CANCEL, 0, 0);
			RECT(r, TSKBR_POPUP_WIDTH - 60 * 1, TSKBR_POPUP_HEITE - 30, 50, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Apply", TASKBAR_POPUP_APPLY, 0, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
				case TASKBAR_POPUP_CANCEL:
				{
					DestroyWindow(pWindow);
					break;
				}
				case TASKBAR_POPUP_OK:
				case TASKBAR_POPUP_APPLY:
				{
					// apply the OG taskbar properties.
					TaskbarSetProperties(
						CheckboxGetChecked(pWindow, TASKBAR_POPUP_DATECHECK),
						CheckboxGetChecked(pWindow, TASKBAR_POPUP_TIMECHECK),
						CheckboxGetChecked(pWindow, TASKBAR_POPUP_BUTTON_COMPACT),
						CheckboxGetChecked(pWindow, TASKBAR_POPUP_SHOWCPUUSAGE),
						CheckboxGetChecked(pWindow, TASKBAR_POPUP_SHOWMEMUSAGE),
						CheckboxGetChecked(pWindow, TASKBAR_POPUP_USELARGEICONS)
					);
					
					// apply the new properties too
					g_GlowOnHover          = CheckboxGetChecked(pWindow, TASKBAR_POPUP_BUTTON_GLOW);
					g_RenderWindowContents = CheckboxGetChecked(pWindow, TASKBAR_POPUP_SHOWMOVINGWINS);
					
					bool bSolidBGChecked = CheckboxGetChecked(pWindow, TASKBAR_POPUP_SOLIDBG);
					if (g_BackgroundSolidColorActive != bSolidBGChecked)
					{
						g_BackgroundSolidColorActive  = bSolidBGChecked;
						RefreshEverything();
					}
					
					if (parm1 == TASKBAR_POPUP_OK)
						DestroyWindow(pWindow);
					break;
				}
			}
			break;
		}
		case EVENT_PAINT:
		{
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

void CplTaskbar(Window* pWindow)
{
	PopupWindow(
		pWindow,
		"Desktop & Task Bar",
		pWindow->m_rect.left + 50,
		pWindow->m_rect.top  + 50,
		TSKBR_POPUP_WIDTH,
		TSKBR_POPUP_HEITE,
		CplTaskbarWndProc,
		WF_NOMINIMZ
	);
}
