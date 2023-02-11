/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Task List controls
******************************************/

#include <widget.h>
#include <video.h>
#include <image.h>
#include <icon.h>
#include <clip.h>
#include <print.h>
#include <misc.h>
#include <keyboard.h>
#include <wmenu.h>
#include <string.h>

extern Window g_windows[WINDOWS_MAX];
extern short  g_windowDrawOrder[WINDOWS_MAX];

void SelectWindow (Window *pWnd);
void ShowWindow (Window *pWnd);

bool g_TaskListCompact = false;

extern bool g_GlowOnHover;

typedef struct
{
	bool m_clicked[WINDOWS_MAX];
	bool m_hovered[WINDOWS_MAX];
	WindowQuery queries[WINDOWS_MAX];
	char titles[WINDOWS_MAX][256];
}
TaskListData;

bool WidgetTaskList_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			TaskListData* pData = MmAllocate(sizeof(TaskListData));
			memset(pData, 0, sizeof *pData);
			this->m_dataPtr = pData;
			
			if (!pData) break;
			
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				pData->titles[i][0] = 0;
				
				WindowQuery* pQuery = &pData->queries[i];
				pQuery->titleOut     = pData->titles[i];
				pQuery->titleOutSize = sizeof (pData->titles[i]);
			}
			
			break;
		}
		case EVENT_DESTROY:
		{
			MmFree(this->m_dataPtr);
			break;
		}
		case EVENT_PAINT:
		{
			TaskListData* pData = (TaskListData*)this->m_dataPtr;
			
			size_t nWindowsSt = 0;
			QueryWindows(pData->queries, ARRAY_COUNT(pData->queries), &nWindowsSt);
			VidFillRectangle (WINDOW_BACKGD_COLOR, this->m_rect);
			
			int nWindows = 0;
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (!nWindows) break;
			
			int btn_width = GetWidth (&this->m_rect) / nWindows;
			
			int btn_max_width = g_TaskListCompact ? 28 : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left;
			
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				
				Rectangle r;
				RECT (r, btn_x, this->m_rect.top, btn_width - 4, GetHeight(&this->m_rect));
				
				bool clicked = pData->m_clicked[pQuery->windowID];
				bool hovered = pData->m_hovered[pQuery->windowID];
				
				uint32_t flgs = 0;
				if (g_windows[i].m_isSelected)
				{
					RenderButtonShape(r, BUTTONLITE, BUTTONDARK, BUTTONMIDD);
					
					flgs |= TEXT_RENDER_BOLD;
				}
				else if (clicked)
				{
					RenderButtonShape(r, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				}
				else if (hovered)
				{
					RenderButtonShape(r, BUTTONDARK, BUTTONLITE, BUTTON_HOVER_COLOR);
				}
				else
					RenderButtonShape(r, BUTTONDARK, BUTTONLITE, BUTTON_MIDDLE_COLOR);
				
				// hmm.
				g_windows[pQuery->windowID].m_taskbarRect = r;
				
				int textX = btn_x + 4;
				// if this window has an icon:
				if (pQuery->iconID)
				{
					RenderIconForceSize(pQuery->iconID, textX + clicked, this->m_rect.top + 3 + (((this->m_rect.bottom - this->m_rect.top - 6) - 16) / 2) + clicked, 16);
					textX += 22; 
				}
				
				if (!g_TaskListCompact)
				{
					VidSetClipRect (&r);
					
					//VidTextOut (pWnd->m_title, textX, this->m_rect.top + 5, WINDOW_TEXT_COLOR, TRANSPARENT);
					
					RECT(r, textX + clicked, this->m_rect.top + 2 + clicked, btn_x + btn_width - textX - 2, GetHeight(&this->m_rect) - 4);
					
					VidDrawText (pQuery->titleOut, r, TEXTSTYLE_VCENTERED, FLAGS_TOO(flgs, WINDOW_TEXT_COLOR), TRANSPARENT);
					
					VidSetClipRect (NULL);
				}
				
				btn_x += btn_width;
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		case EVENT_CLICKCURSOR:
		case EVENT_MOVECURSOR:
		{
			Point mouseClickPos  = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			TaskListData* pData = (TaskListData*)this->m_dataPtr;
			
			size_t nWindowsSt = 0;
			QueryWindows(pData->queries, ARRAY_COUNT(pData->queries), &nWindowsSt);
			
			int nWindows = 0;
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (!nWindows) break;
			
			int btn_width = GetWidth (&this->m_rect) / nWindows;
			
			int btn_max_width = g_TaskListCompact ? 28 : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left;
			
			for (size_t i = 0; i < nWindowsSt; i++)
			{
				WindowQuery* pQuery = &pData->queries[i];
				if (pQuery->flags & WF_SYSPOPUP) continue;
				
				pData->m_hovered[pQuery->windowID] = false;
				
				Rectangle r;
				RECT (r, btn_x, this->m_rect.top, btn_width - 4, GetHeight(&this->m_rect));
				
				if (RectangleContains (&r, &mouseClickPos))
				{
					if (eventType == EVENT_MOVECURSOR)
					{
						pData->m_hovered[pQuery->windowID] = true;
					}
					else if (eventType == EVENT_CLICKCURSOR)
					{
						pData->m_clicked[pQuery->windowID] = true;
					}
					else if (pData->m_clicked[pQuery->windowID]) // EVENT_RELEASECURSOR
					{
						// switch to this window
						for (int j = 0; j < WINDOWS_MAX; j++)
							pData->m_clicked[j] = false;
						
						//hmm??
						Window* pWindow = &g_windows[pQuery->windowID];
						if (pWindow->m_hidden && (pWindow->m_flags & WF_MINIMIZE)) //TODO
						{
							//Unhide and unminimize
							WindowRegisterEvent (pWindow, EVENT_UNMINIMIZE, 0, 0);
							//TODO: wait for the animation to finish :)
							ShowWindow(pWindow);
						}
						
						SelectWindow (pWindow);
					}
					
					if (eventType != EVENT_MOVECURSOR || g_GlowOnHover)
						WidgetTaskList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
					
					if (eventType != EVENT_MOVECURSOR)
						break;
				}
				
				btn_x += btn_width;
			}
			
			break;
		}
	}
	
	return false;
}
