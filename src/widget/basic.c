/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

      Widget library: Basic controls
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

bool WidgetNone_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	return false;
}
bool WidgetText_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
			VidTextOut(this->m_text, this->m_rect.left, this->m_rect.top, this->m_parm1, this->m_parm2);
			break;
	}
	return false;
}
bool WidgetTextCenter_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			if (this->m_parm2 & TEXTSTYLE_FORCEBGCOL)
			{
				VidFillRectangle(WINDOW_BACKGD_COLOR, this->m_rect);
			}
			VidDrawText(this->m_text, this->m_rect, this->m_parm2, this->m_parm1, TRANSPARENT);
			break;
		}
	}
	return false;
}
bool WidgetTextHuge_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
			this->m_dataPtr = NULL;
			break;
		case EVENT_PAINT:
			if (this->m_dataPtr)
				VidDrawText((const char*)this->m_dataPtr, this->m_rect, this->m_parm2, this->m_parm1, TRANSPARENT);
			break;
		case EVENT_DESTROY:
			if (this->m_dataPtr)
				MmFreeK(this->m_dataPtr);
			this->m_dataPtr = NULL;
			break;
	}
	return false;
}
bool WidgetIcon_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
			RenderIcon(this->m_parm1, this->m_rect.left + (this->m_rect.right - this->m_rect.left - 32) / 2, this->m_rect.top + (this->m_rect.bottom - this->m_rect.top - 32) / 2);
			break;
	}
	return false;
}
bool WidgetSurroundRect_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			// Draw a rectangle to surround the things we put inside
			Rectangle r = this->m_rect;
			r.top += GetLineHeight() / 2;
			
			VidDrawRectangle(WINDOW_TEXT_COLOR, r);
			
			// Draw the text
			VidTextOut(this->m_text, this->m_rect.left + 10, this->m_rect.top, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);
			VidTextOut(this->m_text, this->m_rect.left + 12, this->m_rect.top, 0x00000,             WINDOW_BACKGD_COLOR);
			
			break;
		}
	}
	return false;
}
bool WidgetSimpleLine_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			// Draw a rectangle to surround the things we put inside
			VidDrawHLine(WINDOW_BACKGD_COLOR - 0x0F0F0F, this->m_rect.left + 8, this->m_rect.right - 8, (this->m_rect.top + this->m_rect.bottom) / 2);
			
			break;
		}
	}
	return false;
}



void SetHugeLabelText (Window *pWindow, int comboID, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			//strcpy(pWindow->m_pControlArray[i].m_text, pText);
			char* ptr = (char*)pWindow->m_pControlArray[i].m_dataPtr;
			if (ptr)
				MmFreeK(ptr);
			ptr = MmAllocateK(strlen(pText)+1);
			pWindow->m_pControlArray[i].m_dataPtr = ptr;
			strcpy (ptr, pText);
			
			return;
		}
	}
}

void SetLabelText (Window *pWindow, int comboID, const char* pText)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			strcpy(pWindow->m_pControlArray[i].m_text, pText);
			return;
		}
	}
}
void SetIcon (Window *pWindow, int comboID, int icon)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_parm1 = icon;
			return;
		}
	}
}

