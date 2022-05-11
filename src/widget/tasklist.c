// NanoShell Operating System (C) 2022 iProgramInCpp
// Widget Library: Task list control

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

bool WidgetTaskList_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			VidFillRectangle (WINDOW_BACKGD_COLOR, this->m_rect);
			
			int nWindows = 0;
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (!nWindows) break;
			
			int btn_width = GetWidth (&this->m_rect) / nWindows;
			
			int btn_max_width = g_TaskListCompact ? 28 : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left;
			
			
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				
				
				Rectangle r;
				RECT (r, btn_x, this->m_rect.top, btn_width - 4, GetHeight(&this->m_rect));
				
				bool clicked = this->m_taskListData.m_clicked[i];
				bool hovered = this->m_taskListData.m_hovered[i];
				
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
					uint32_t bm = BUTTONMIDD;
					if (bm > 0xDFDFDF)//avoid overflow
						bm = 0xDFDFDF;
					bm += 0x202020;
					RenderButtonShape(r, BUTTONDARK, BUTTONLITE, bm);
				}
				else
					RenderButtonShape(r, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				
				g_windows[i].m_taskbarRect = r;
				
				Window *pWnd = &g_windows[i];
				
				int textX = btn_x + 4;
				// if this window has an icon:
				if (pWnd->m_iconID)
				{
					RenderIconForceSize(pWnd->m_iconID, textX + clicked, this->m_rect.top + 3 + clicked, 16);
					textX += 22; 
				}
				
				if (!g_TaskListCompact)
				{
					VidSetClipRect (&r);
					
					//VidTextOut (pWnd->m_title, textX, this->m_rect.top + 5, WINDOW_TEXT_COLOR, TRANSPARENT);
					
					RECT(r, textX + clicked, this->m_rect.top + 2 + clicked, btn_x + btn_width - textX - 2, GetHeight(&this->m_rect) - 4);
					
					VidDrawText (pWnd->m_title, r, TEXTSTYLE_VCENTERED, FLAGS_TOO(flgs, WINDOW_TEXT_COLOR), TRANSPARENT);
					
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
			
			int nWindows = 0;
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				nWindows++;
			}
			
			if (!nWindows) break;
			
			int btn_width = GetWidth (&this->m_rect) / nWindows;
			
			int btn_max_width = g_TaskListCompact ? 28 : 200;
			
			if (btn_width > btn_max_width)
				btn_width = btn_max_width;
			
			int btn_x = this->m_rect.left;
			
			for (int i = 0; i < WINDOWS_MAX; i++)
			{
				if (!g_windows[i].m_used) continue;
				if (g_windows[i].m_flags & WF_SYSPOPUP) continue;
				
				this->m_taskListData.m_hovered[i] = false;
				
				Rectangle r;
				RECT (r, btn_x, this->m_rect.top, btn_width - 4, GetHeight(&this->m_rect));
				
				if (RectangleContains (&r, &mouseClickPos))
				{
					if (eventType == EVENT_MOVECURSOR)
					{
						this->m_taskListData.m_hovered[i] = true;
					}
					else if (eventType == EVENT_CLICKCURSOR)
					{
						this->m_taskListData.m_clicked[i] = true;
					}
					else if (this->m_taskListData.m_clicked[i]) // EVENT_RELEASECURSOR
					{
						// switch to this window
						for (int j = 0; j < WINDOWS_MAX; j++)
							this->m_taskListData.m_clicked[j] = false;
						
						Window* pWindow = &g_windows[i];
						if (pWindow->m_hidden && pWindow->m_minimized) //TODO
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
