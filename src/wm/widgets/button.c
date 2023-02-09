/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

      Widget library: Button controls
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

extern bool g_GlowOnHover;

#define DARKEN(color) \
	((color >> 1) & 0xFF0000) | \
	(((color & 0xFF00) >> 1) & 0xFF00) | \
	(((color & 0xFF) >> 1) & 0xFF)
	
SAI UNUSED
uint32_t Blueify (uint32_t color)
{
	uint32_t red_component = color & 0xFF0000;
	red_component >>= 1;
	red_component &=  0xFF0000;
	
	uint32_t grn_component = color & 0xFF00;
	//grn_component <<= 1;
	//if (grn_component > 0xFF00) grn_component = 0xFF00;
	
	uint32_t blu_component = color & 0xFF;
	//blu_component <<= 1;
	//if (blu_component > 0xFF0000) blu_component = 0xFF;
	
	return red_component | grn_component | blu_component;
}

bool WidgetButton_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
				{
					//send a command event to the window:
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
				}
				this->m_buttonData.m_clicked = false;
				WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
				{
					this->m_buttonData.m_clicked = true;
					WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_KILLFOCUS:
			this->m_buttonData.m_hovered = false;
			if (g_GlowOnHover)
				WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		case EVENT_MOVECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p))
				{
					if (!this->m_buttonData.m_hovered)
					{
						this->m_buttonData.m_hovered = true;
						if (g_GlowOnHover)
							WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
					}
				}
				else if (this->m_buttonData.m_hovered)
				{
					this->m_buttonData.m_hovered = false;
					if (g_GlowOnHover)
						WidgetButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			else
			{
				this->m_buttonData.m_hovered = false;
			}
			break;
		}
		case EVENT_PAINT:
		{
			VidSetClipRect (&this->m_rect);
			if (this->m_buttonData.m_clicked)
			{
				Rectangle r = this->m_rect;
				//draw the button as slightly pushed in
				r.left++; r.right++; r.bottom++; r.top++;
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else if (this->m_buttonData.m_hovered && g_GlowOnHover)
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTON_HOVER_COLOR);
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				
				if (this->m_bDisabled)
				{
					//small hack
					uint8_t b[4];
					*((uint32_t*)b) = BUTTONMIDD;
					b[0] >>= 1; b[1] >>= 1; b[2] >>= 1; b[3] >>= 1;
					
					Rectangle r = this->m_rect;
					r.left++; r.right++; r.top++; r.bottom++;
					VidDrawText(this->m_text,       r     , TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR_LIGHT, TRANSPARENT);
					VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, *((uint32_t*)b),         TRANSPARENT);
				}
				else
				{
					VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				}
			}
			VidSetClipRect (NULL);
			
			break;
		}
	}
	return false;
}
bool WidgetButtonIcon_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
				{
					//send a command event to the window:
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
				}
				this->m_buttonData.m_clicked = false;
				WidgetButtonIcon_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
				{
					this->m_buttonData.m_clicked = true;
					WidgetButtonIcon_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_MOVECURSOR:
		{
			break;
		}
		case EVENT_PAINT:
		{
			int x = this->m_rect.left + (this->m_rect.right  - this->m_rect.left - this->m_parm2) / 2;
			int y = this->m_rect.top  + (this->m_rect.bottom - this->m_rect.top  - this->m_parm2) / 2;
			
			if (this->m_buttonData.m_clicked)
			{
				x++, y++;
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			}
			
			if (!this->m_bDisabled)
			{
				RenderIconForceSize (this->m_parm1, x, y, this->m_parm2);
			}
			else
			{
				//small hack
				uint8_t b[4];
				*((uint32_t*)b) = BUTTONMIDD;
				b[0] >>= 1; b[1] >>= 1; b[2] >>= 1; b[3] >>= 1;
				
				RenderIconForceSizeOutline (this->m_parm1, x + 1, y + 1, this->m_parm2, WINDOW_TEXT_COLOR_LIGHT);
				RenderIconForceSizeOutline (this->m_parm1, x + 0, y + 0, this->m_parm2, *((uint32_t*)b));
			}
			
			break;
		}
	}
	return false;
}
bool WidgetButtonIconBar_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
				{
					//send a command event to the window:
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
				}
				this->m_buttonData.m_clicked = false;
				WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_KILLFOCUS:
			this->m_buttonData.m_hovered = false;
			if (g_GlowOnHover)
				WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		case EVENT_MOVECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p))
				{
					if (!this->m_buttonData.m_hovered)
					{
						this->m_buttonData.m_hovered = true;
						if (g_GlowOnHover)
							WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
					}
				}
				else if (this->m_buttonData.m_hovered)
				{
					this->m_buttonData.m_hovered = false;
					if (g_GlowOnHover)
						WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
				{
					this->m_buttonData.m_clicked = true;
					WidgetButtonIconBar_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_PAINT:
		{
			uint32_t blue = SELECTED_MENU_ITEM_COLOR_B;
			
			int x = this->m_rect.left + (this->m_rect.right  - this->m_rect.left - this->m_parm2) / 2;
			int y = this->m_rect.top  + (this->m_rect.bottom - this->m_rect.top  - this->m_parm2) / 2;
			x++, y++;
			
			bool bRenderOutlineToo = false;
			
			if (this->m_buttonData.m_clicked)
			{
				VidFillRectangle(SELECTED_MENU_ITEM_COLOR_B, this->m_rect);
				VidDrawRectangle(SELECTED_MENU_ITEM_COLOR,   this->m_rect);
				
				x++, y++;
			}
			else if (this->m_buttonData.m_hovered && g_GlowOnHover)
			{
				VidFillRectangle(SELECTED_MENU_ITEM_COLOR_B, this->m_rect);
				VidDrawRectangle(SELECTED_MENU_ITEM_COLOR,   this->m_rect);
				
				x--, y--;
				bRenderOutlineToo = true;
				//RenderButtonShapeSmall (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
			}
			else
			{
				VidFillRectangle(WINDOW_BACKGD_COLOR, this->m_rect);
			}
			
			if (!this->m_bDisabled)
			{
				if (bRenderOutlineToo)
					RenderIconForceSizeOutline (this->m_parm1, x + 2, y + 2, this->m_parm2, DARKEN(blue));
				
				RenderIconForceSize (this->m_parm1, x, y, this->m_parm2);
			}
			else
			{
				//small hack
				uint8_t b[4];
				*((uint32_t*)b) = BUTTONMIDD;
				b[0] >>= 1; b[1] >>= 1; b[2] >>= 1; b[3] >>= 1;
				
				RenderIconForceSizeOutline(this->m_parm1, x+1, y+1, this->m_parm2, WINDOW_TEXT_COLOR_LIGHT);
				RenderIconForceSizeOutline(this->m_parm1, x+0, y+0, this->m_parm2, *((uint32_t*)b));
			}
			
			break;
		}
	}
	return false;
}

bool WidgetButtonList_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
				{
					//send a command event to the window:
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
				}
				this->m_buttonData.m_clicked = false;
				WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
				{
					this->m_buttonData.m_clicked = true;
					WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_KILLFOCUS:
			this->m_buttonData.m_hovered = false;
			if (g_GlowOnHover)
				WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			break;
		case EVENT_MOVECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p))
				{
					if (!this->m_buttonData.m_hovered)
					{
						this->m_buttonData.m_hovered = true;
						if (g_GlowOnHover)
							WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 1, pWindow);
					}
				}
				else if (this->m_buttonData.m_hovered)
				{
					this->m_buttonData.m_hovered = false;
					if (g_GlowOnHover)
						WidgetButtonList_OnEvent (this, EVENT_PAINT, 0, 1, pWindow);
				}
			}
			break;
		}
		case EVENT_PAINT:
		{
			Rectangle r = this->m_rect;
			
			int offs1 = 16;
			int offs = 20;
			if (this->m_parm1)
			{
				offs1 += 8;
				offs  += 10;
			}
			
			Rectangle r1 = this->m_rect;
			r1.right = r1.left + offs1;
			
			if (this->m_bDisabled)
			{
				Rectangle r2 = r;
				r2.left = r1.right;
				if (parm2)
				{
					VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, r);
					VidFillRectangle(WINDOW_BACKGD_COLOR, r1);
				}
				r.left += offs;
				r.top += 1;
				r.bottom += 1;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, BUTTON_MIDDLE_COLOR, TRANSPARENT);
				
				r.left -= offs;
				if (this->m_parm1)
					RenderIconForceSize (this->m_parm1, r.left + 4, r.top + (r.bottom - r.top - 16) / 2, 16);
			}
			else if (this->m_buttonData.m_clicked)
			{
				//draw the button as slightly pushed in
				VidFillRectangle(SELECTED_MENU_ITEM_COLOR_B, r);
				VidDrawRectangle(SELECTED_MENU_ITEM_COLOR,   r);
				
				r.top += 1;
				r.bottom += 1;
				
				r.left++; r.right++; r.bottom++; r.top++;
				
				if (this->m_parm1)
				{
					RenderIconForceSize (this->m_parm1, r.left + 4, r.top + (r.bottom - r.top - 16) / 2, 16);
				}
				
				r.left += offs;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else if (this->m_buttonData.m_hovered && g_GlowOnHover)
			{
				uint32_t col = SELECTED_MENU_ITEM_COLOR_B;
				VidFillRectangle(col, r);
				VidDrawRectangle(SELECTED_MENU_ITEM_COLOR, r);
				
				r.left += offs;
				r.top += 1;
				r.bottom += 1;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				
				r.left -= offs;
				if (this->m_parm1)
				{
					RenderIconForceSizeOutline (this->m_parm1, r.left + 5, r.top + (r.bottom - r.top - 16) / 2 + 1, 16, DARKEN(col));
					RenderIconForceSize        (this->m_parm1, r.left + 3, r.top + (r.bottom - r.top - 16) / 2 - 1, 16);
				}
			}
			else
			{
				Rectangle r2 = r;
				r2.left = r1.right;
				if (parm2)
				{
					VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, r);
					VidFillRectangle(WINDOW_BACKGD_COLOR, r1);
				}
				r.left += offs;
				r.top += 1;
				r.bottom += 1;
				VidDrawText(this->m_text, r, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				
				r.left -= offs;
				if (this->m_parm1)
					RenderIconForceSize (this->m_parm1, r.left + 4, r.top + (r.bottom - r.top - 16) / 2, 16);
			}
			
			break;
		}
	}
	return false;
}
bool WidgetButtonColor_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
				{
					//send a command event to the window:
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
				}
				this->m_buttonData.m_clicked = false;
				WidgetButtonColor_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
				{
					this->m_buttonData.m_clicked = true;
					WidgetButtonColor_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_PAINT:
		{
			if (this->m_buttonData.m_clicked)
			{
				Rectangle r = this->m_rect;
				//draw the button as slightly pushed in
				r.left++; r.right++; r.bottom++; r.top++;
				RenderButtonShape (this->m_rect, BUTTONMIDC, BUTTONDARK, this->m_parm2);
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, this->m_parm1, TRANSPARENT);
			}
			else
			{
				RenderButtonShape (this->m_rect, BUTTONDARK, BUTTONLITE, this->m_parm2);
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, this->m_parm1, TRANSPARENT);
			}
			
			break;
		}
	}
	return false;
}
//for the top bar of the window.  Uses this->m_parm1 as the event type.
bool WidgetActionButton_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && this->m_buttonData.m_clicked)
				{
					//send a command event to the window:
					//CallWindowCallback(pWindow, this->m_parm1, this->m_comboID, this->m_parm2);
					WindowRegisterEventUnsafe(pWindow, this->m_parm1, this->m_comboID, this->m_parm2);
				}
				this->m_buttonData.m_clicked = false;
				WidgetActionButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
			}
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p) && !this->m_buttonData.m_clicked)
				{
					this->m_buttonData.m_clicked = true;
					WidgetActionButton_OnEvent (this, EVENT_PAINT, 0, 0, pWindow);
				}
			}
			break;
		}
		case EVENT_PAINT:
		{
			//draw a green rectangle:
			if (this->m_buttonData.m_clicked)
			{
				Rectangle r = this->m_rect;
				//draw the button as slightly pushed in
				r.left++; r.right++; r.bottom++; r.top++;
				RenderButtonShapeSmall (this->m_rect, BUTTONMIDC, BUTTONDARK, BUTTONMIDC);
				VidDrawText(this->m_text, r, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			else
			{
				this->m_rect.right--;
				RenderButtonShapeSmall (this->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				this->m_rect.right++;//ugly hack
				VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
			}
			
			break;
		}
	}
	return false;
}
bool WidgetClickLabel_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
	//#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case EVENT_RELEASECURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p))
				{
					//send a command event to the window:
					//WindowRegisterEvent(pWindow, EVENT_COMMAND, this->m_parm1, this->m_parm2);
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, this->m_parm1);
				}
			}
		}
		//! fallthrough intentional - need the button to redraw itself as pushing back up
		case EVENT_PAINT:
	//#pragma GCC diagnostic pop
		{
			//then fill in the text:
			VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_VCENTERED, 0x1111FF, TRANSPARENT);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (!this->m_bDisabled)
			{
				Rectangle r = this->m_rect;
				Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
				if (RectangleContains (&r, &p))
				{
					//then fill in the text:
					VidDrawText(this->m_text, this->m_rect, TEXTSTYLE_VCENTERED, 0x11, TRANSPARENT);
				}
			}
			break;
		}
	}
	return false;
}
