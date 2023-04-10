/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Widget library: Progress Bar control
******************************************/
#include <widget.h>
#include <print.h>

void CtlSetProgressBarProg(Control *this, int progress)
{
	this->m_parm1 = progress;
}

void CtlSetProgressBarMax(Control *this, int max)
{
	this->m_parm2 = max;
}

void ProgBarSetProgress(Window* pWindow, int comboID, int prog)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetProgressBarProg(&pWindow->m_pControlArray[i], prog);
		}
	}
}

void ProgBarSetMaxProg(Window* pWindow, int comboID, int prog)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlSetProgressBarMax(&pWindow->m_pControlArray[i], prog);
		}
	}
}

bool WidgetProgressBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			VidSetClipRect(&this->m_rect);
			
			DrawEdge(this->m_rect, DRE_SUNKEN, 0);
			
			int max = this->m_parm2;
			int pro = this->m_parm1;
			if (max < 0)   max = 1;
			if (pro > max) pro = max;
			
			int width = (this->m_rect.right - this->m_rect.left - 4) * pro / max;
			
			int progress_text_num = 100 * pro / max;
			char buffer[20];
			snprintf(buffer, sizeof buffer, "%d%%", progress_text_num);
			
			Rectangle rect = { this->m_rect.left + 2, this->m_rect.top + 2, this->m_rect.left + 2 + width, this->m_rect.bottom - 2 };
			VidSetClipRect(&rect);
			VidFillRect(WINDOW_TITLE_ACTIVE_COLOR, this->m_rect.left + 2, this->m_rect.top + 2, this->m_rect.left + 2 + width, this->m_rect.bottom - 3);
			
			VidDrawText(buffer, this->m_rect, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR_LIGHT | TEXT_RENDER_BOLD, WINDOW_TITLE_ACTIVE_COLOR);
			
			Rectangle rect1 = { this->m_rect.left + 2 + width, this->m_rect.top + 2, this->m_rect.right - 2, this->m_rect.bottom - 2 };
			VidSetClipRect(&rect1);
			VidFillRect(WINDOW_TEXT_COLOR_LIGHT, this->m_rect.left + 2 + width, this->m_rect.top + 2, this->m_rect.right - 2, this->m_rect.bottom - 3);
			VidDrawText(buffer, this->m_rect, TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR | TEXT_RENDER_BOLD, WINDOW_TEXT_COLOR_LIGHT);
			
			VidSetClipRect(NULL);
			break;
		}
	}
	
	return false;
}