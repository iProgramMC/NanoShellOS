/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Widget library: Checkbox control
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

#define CHECKBOX_SIZE 14
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);

bool CheckboxGetChecked(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return pWindow->m_pControlArray[i].m_checkBoxData.m_checked;
		}
	}
	return false;
}
void CheckboxSetChecked(Window* pWindow, int comboID, bool checked)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			pWindow->m_pControlArray[i].m_checkBoxData.m_checked = checked;
		}
	}
}
bool WidgetCheckbox_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	// this->rect only affects the top left position
	
	Rectangle check_rect = this->m_rect;
	check_rect.right  = check_rect.left + CHECKBOX_SIZE;
	check_rect.bottom = check_rect.top  + CHECKBOX_SIZE;
	
	Rectangle text_rect = this->m_rect;
	text_rect.left =  check_rect.right + 6;
	text_rect.top  += (check_rect.bottom - check_rect.top - GetLineHeight()) / 2 + 1;
	
	switch (eventType)
	{
		case EVENT_PAINT:
		{
			//VidFillRectangle(this->m_checkBoxData.m_clicked ? 0xcccccc : WINDOW_TEXT_COLOR_LIGHT, check_rect);
			//VidDrawRectangle(WINDOW_TEXT_COLOR,  
			//RenderButtonShapeSmallInsideOut (this->m_rect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
			check_rect.right--;
			//check_rect.bottom--;
			RenderButtonShapeSmallInsideOut (check_rect, 0xBFBFBF, BUTTONDARK, this->m_checkBoxData.m_clicked ? 0xcccccc : WINDOW_TEXT_COLOR_LIGHT);
			
			//if checked, mark it as "checked"
			if (this->m_checkBoxData.m_checked)
			{
				VidTextOut("\x15", check_rect.left + 3, check_rect.top + 3, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			
			VidDrawText(this->m_text, text_rect, TEXTSTYLE_WORDWRAPPED, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			if (RectangleContains (&check_rect, &p) || RectangleContains (&text_rect, &p))
			{
				this->m_checkBoxData.m_clicked = true;
				WidgetCheckbox_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			bool contains = (RectangleContains (&check_rect, &p) || RectangleContains (&text_rect, &p));
			if ( this->m_checkBoxData.m_clicked )
			{
				if (contains)
					this->m_checkBoxData.m_checked ^= 1;
				this->m_checkBoxData.m_clicked = false;
				
				CallWindowCallback(pWindow, EVENT_CHECKBOX, this->m_comboID, this->m_checkBoxData.m_checked);
				WidgetCheckbox_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			
			break;
		}
	}
	
	return false;
}
