/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

  Codename V-Builder - The form designer
******************************************/

#include <wbuiltin.h>
#include <keyboard.h>

#define DEF_VBUILD_WID 640
#define DEF_VBUILD_HEI (TITLE_BAR_HEIGHT*2+6)
#define DEF_FDESIGN_WID 500
#define DEF_FDESIGN_HEI 400
#define DEF_TOOLBOX_WID (96+6)
#define DEF_TOOLBOX_HEI (24*8+6+TITLE_BAR_HEIGHT)

#define BUTTONDARK 0x808080
#define BUTTONMIDD BUTTON_MIDDLE_COLOR
#define BUTTONLITE 0xFFFFFF
#define BUTTONMIDC WINDOW_BACKGD_COLOR

#define GRABBER_SIZE (8)
#define GRID_WIDTH   (8)

enum eResizeHandle
{
	HANDLE_NONE,
	HANDLE_BR, // bottom right
	HANDLE_BM, // bottom middle
	HANDLE_CR, // center right
	HANDLE_TL, // top left
	HANDLE_COUNT,
};

static Rectangle VbMakeHandleRect(int handleType, Rectangle ctlRect)
{
	Rectangle rect = ctlRect;
	switch (handleType)
	{
		case HANDLE_TL:
			rect.right  = rect.left + GRABBER_SIZE - 1;
			rect.bottom = rect.top  + GRABBER_SIZE - 1;
			break;
		case HANDLE_BR:
			rect.left = rect.right  - GRABBER_SIZE + 1;
			rect.top  = rect.bottom - GRABBER_SIZE + 1;
			break;
		case HANDLE_CR:
			rect.left = rect.right  - GRABBER_SIZE + 1;
			rect.top  = rect.bottom = (rect.bottom + rect.top) / 2;
			rect.top    -= GRABBER_SIZE / 2;
			rect.bottom += GRABBER_SIZE / 2 - 1;
			break;
		case HANDLE_BM:
			rect.top  = rect.bottom - GRABBER_SIZE + 1;
			rect.left = rect.right  = (rect.left + rect.right) / 2;
			rect.left  -= GRABBER_SIZE / 2;
			rect.right += GRABBER_SIZE / 2 - 1;
			break;
	}
	return rect;
}

enum
{
	TOOL_CURSOR = -100,
	TOOL_SELECT,
};

void RenderButtonShapeNoRounding(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);
void RenderButtonShapeSmall(Rectangle rectb, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);
void RenderButtonShape(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);

typedef struct
{
	bool      m_used;
	Rectangle m_rect;
	int       m_type;
	char      m_text[128];
	bool      m_sele;
	int       m_prm1, m_prm2;
}
DesignerControl;

typedef struct
{
	DesignerControl m_controls[64];
	
	int  m_anchorX, m_anchorY;
	int  m_curPosX, m_curPosY;
	bool m_drawing;
	int  m_selCtlType;
	int  m_controlResized;
	int  m_resizeKnob;
	
	Rectangle m_lastDrawnSelArea;
	Rectangle m_resizeRect;
}
VBuilderData;

#define V ((VBuilderData*)pWindow->m_data)
#define SWAPI(a,b)  do{int tmp; tmp=a; a=b; b=tmp; }while (0)

void VbDrawGrid(Window* pWindow, Rectangle* pWithinRect /*= NULL*/)
{
	int ox = 3, oy = TITLE_BAR_HEIGHT + 4;
	int windowWid = GetWidth (&pWindow->m_rect);
	int windowHei = GetHeight(&pWindow->m_rect);
	
	for (; ox <= windowWid; ox += GRID_WIDTH)
	{
		for (oy = TITLE_BAR_HEIGHT + 2; oy <= windowHei; oy += GRID_WIDTH)
		{
			if (pWithinRect)
			{
				Point p = { ox, oy };
				if (!RectangleContains(pWithinRect, &p))
					continue;
			}
			
			VidPlotPixel (ox, oy, 0x000000);
		}
	}
}
bool WidgetCheckbox_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow);
void VbRenderCtls(Window* pWindow, Rectangle* pWithinRect /*= NULL*/)
{
	for (int i = 0; i < (int)ARRAY_COUNT(V->m_controls); i++)
	{
		if (!V->m_controls[i].m_used) continue;
		
		DesignerControl *C = &V->m_controls[i];
		
		if (pWithinRect && !RectangleOverlap(pWithinRect, &C->m_rect)) continue;
		
		Rectangle rect = C->m_rect;
		rect.left += 5;
		rect.top  += 5;
		
		Rectangle srect = C->m_rect;
		srect.right --;
		srect.bottom--;
		
		switch (C->m_type)
		{
			case CONTROL_TEXTCENTER:
				VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, srect);
				VidDrawText (C->m_text, rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				break;
			case CONTROL_BUTTON:
				RenderButtonShape (C->m_rect, BUTTONDARK, BUTTONLITE, BUTTONMIDD);
				VidDrawText (C->m_text, rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				break;
			case CONTROL_TEXTINPUT:
				VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, srect);
				RenderButtonShapeSmallInsideOut (srect, 0xBFBFBF, BUTTONDARK, TRANSPARENT);
				break;
			case CONTROL_CHECKBOX:
			{
				VidFillRectangle(WINDOW_BACKGD_COLOR, srect);
				// Since I am too lazy to replicate that code, just construct a fake control and
				// call its render event
				Control c;
				memset (&c, 0, sizeof c);
				c.m_rect = srect;
				c.m_checkBoxData.m_checked = false;
				strcpy (c.m_text, C->m_text);
				
				WidgetCheckbox_OnEvent(&c, EVENT_PAINT, 0, 0, pWindow);
				
				break;
			}
			default:
				VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, srect);
				VidDrawText (C->m_text, rect, TEXTSTYLE_WORDWRAPPED, WINDOW_TEXT_COLOR, TRANSPARENT);
				if (!C->m_sele)
					VidDrawRectangle(0x0000FF, srect);
				break;
		}
		
		if (C->m_sele)
		{
			// Draw a red rectangle and some handles.
			VidDrawRectangle(0xFF0000, srect);
			
			for (int i = HANDLE_BR; i < HANDLE_COUNT; i++)
			{
				Rectangle sHandle = VbMakeHandleRect(i, srect);
				VidFillRectangle(0x000000, sHandle);
			}
		}
	}
}

void VbRedraw(Window *pWindow)
{
	VbDrawGrid  (pWindow, NULL);
	VbRenderCtls(pWindow, NULL);
}

void FixUpCoords (Window* pWindow, int *L, int *T, int *R, int *B)
{
	if (*L > *R) SWAPI(*L,*R);
	if (*T > *B) SWAPI(*T,*B);
	
	//Ensure
	if (*L < 3)
		*L = 3;
	if (*T < 2 + TITLE_BAR_HEIGHT)
		*T = 2 + TITLE_BAR_HEIGHT;
	if (*R >= GetWidth (&pWindow->m_rect) - 3)
		*R  = GetWidth (&pWindow->m_rect) - 3;
	if (*B >= GetHeight(&pWindow->m_rect) - 3)
		*B  = GetHeight(&pWindow->m_rect) - 3;
	
	// Round to the nearest GRID_WIDTH
	*L -= 3;
	*R -= 3;
	*T -= TITLE_BAR_HEIGHT + 2;
	*B -= 4;
	
	*L = (*L / GRID_WIDTH) * GRID_WIDTH;
	*T = (*T / GRID_WIDTH) * GRID_WIDTH;
	*R = ((*R + GRID_WIDTH - 1) / GRID_WIDTH) * GRID_WIDTH;
	*B = ((*B + GRID_WIDTH - 1) / GRID_WIDTH) * GRID_WIDTH;
	
	*L += 3;
	*R += 3;
	*T += TITLE_BAR_HEIGHT + 2;
	*B += 4;
}

void VbAddControl(Window *pWindow, int L, int T, int R, int B)
{
	Rectangle rect;
	rect.left = L, rect.top = T, rect.right = R, rect.bottom = B;
	
	for (int i = 0; i < (int)ARRAY_COUNT(V->m_controls); i++)
	{
		if (!V->m_controls[i].m_used) continue;
		V->m_controls[i].m_sele = false;
	}
	// Add a control
	for (int i = 0; i < (int)ARRAY_COUNT(V->m_controls); i++)
	{
		if (V->m_controls[i].m_used) continue;
		
		DesignerControl *C = &V->m_controls[i];
		
		strcpy (C->m_text, "Hello");
		C->m_used = true;
		C->m_type = V->m_selCtlType;
		C->m_rect = rect;
		C->m_sele = true;
		C->m_prm1 = 0;
		C->m_prm2 = 0;
		
		if (C->m_type == CONTROL_TEXTCENTER)
		{
			C->m_prm2 = TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED;
		}
		else if (C->m_type == CONTROL_TEXT)//fake
		{
			C->m_type = CONTROL_TEXTCENTER;
			C->m_prm2 = TEXTSTYLE_WORDWRAPPED;
		}
		else if (C->m_type == CONTROL_TEXTINPUT)//fake
		{
			C->m_type = CONTROL_TEXTINPUT;
			C->m_prm1 = 0;
		}
		else if (C->m_type == CONTROL_COUNT)//fake - textinputmultiline
		{
			C->m_type = CONTROL_TEXTINPUT;
			C->m_prm1 = TEXTEDIT_MULTILINE;
		}
		
		return;
	}
	
	MessageBox(pWindow, "There are too many controls in this form, delete some.", "Codename V-Builder", MB_OK | ICON_WARNING << 16);
}

void VbDelControl(Window *pWindow, int i)
{
	if (!V->m_controls[i].m_used) return;
	
	V->m_controls[i].m_used = false;
}

void VbSelect(Window* pWindow, int L, int T, int R, int B)
{
	Rectangle rect = {L,T,R,B};
	// go in reverse, because that's the order the controls are drawn in:
	for (int i = (int)ARRAY_COUNT(V->m_controls) - 1; i >= 0; i--)
	{
		if (!V->m_controls[i].m_used) continue;
		
		V->m_controls[i].m_sele = RectangleOverlap (&rect, &V->m_controls[i].m_rect);
	}
}

void CALLBACK PrgFormBldProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			//Create VBuilderData
			pWindow->m_data = MmAllocate(sizeof(VBuilderData));
			
			memset(V, 0, sizeof *V);
			
			V->m_anchorX = V->m_anchorY = V->m_curPosX = V->m_curPosY = -1;
			V->m_controlResized = -1;
			V->m_selCtlType = TOOL_CURSOR;
			
			break;
		}
		case EVENT_DESTROY:
		{
			if (pWindow->m_data) MmFree(pWindow->m_data);
			
			pWindow->m_data = NULL;
			break;
		}
		case EVENT_PAINT:
		{
			//Paint some dots
			VbRedraw(pWindow);
			break;
		}
		case EVENT_COMMAND:
		{
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (V->m_selCtlType == TOOL_CURSOR)
			{
				int posX = GET_X_PARM (parm1), posY = GET_Y_PARM (parm1);
				Point p = { posX, posY };
				
				// Are we dragging a handle ?
				if (V->m_controlResized >= 0)
				{
					int deltaX = posX - V->m_curPosX;
					int deltaY = posY - V->m_curPosY;
					
					DesignerControl * C = &V->m_controls[V->m_controlResized];
					
					if (deltaX != 0 || deltaY != 0)
					{
						VidFillRectangle(WINDOW_BACKGD_COLOR, C->m_rect);
					}
					
					switch (V->m_resizeKnob)
					{
						case HANDLE_TL:
							V->m_resizeRect.left   += deltaX;
							V->m_resizeRect.top    += deltaY;
							V->m_resizeRect.right  += deltaX;
							V->m_resizeRect.bottom += deltaY;
							break;
							
						case HANDLE_BR:
							V->m_resizeRect.right  += deltaX;
							V->m_resizeRect.bottom += deltaY;
							
							if (V->m_resizeRect.right < V->m_resizeRect.left + GRID_WIDTH)
								V->m_resizeRect.right = V->m_resizeRect.left + GRID_WIDTH;
							
							if (V->m_resizeRect.bottom < V->m_resizeRect.top + GRID_WIDTH)
								V->m_resizeRect.bottom = V->m_resizeRect.top + GRID_WIDTH;
							break;
							
						case HANDLE_CR:
							V->m_resizeRect.right += deltaX;
							
							if (V->m_resizeRect.right < V->m_resizeRect.left + GRID_WIDTH)
								V->m_resizeRect.right = V->m_resizeRect.left + GRID_WIDTH;
							break;
							
						case HANDLE_BM:
							V->m_resizeRect.bottom += deltaY;
							
							if (V->m_resizeRect.bottom < V->m_resizeRect.top + GRID_WIDTH)
								V->m_resizeRect.bottom = V->m_resizeRect.top + GRID_WIDTH;
							break;
					}
					
					C->m_rect = V->m_resizeRect;
					
					if (V->m_resizeKnob == HANDLE_TL)
					{
						int width  = C->m_rect.right  - C->m_rect.left;
						int height = C->m_rect.bottom - C->m_rect.top;
						FixUpCoords(pWindow, &C->m_rect.left, &C->m_rect.top, &C->m_rect.right, &C->m_rect.bottom);
						C->m_rect.right  = C->m_rect.left + width;
						C->m_rect.bottom = C->m_rect.top  + height;
					}
					else
					{
						FixUpCoords(pWindow, &C->m_rect.left, &C->m_rect.top, &C->m_rect.right, &C->m_rect.bottom);
					}
					
					if (deltaX != 0 || deltaY != 0)
					{
						VbRedraw(pWindow);
					}
				
					V->m_curPosX = posX;
					V->m_curPosY = posY;
					
					break;
				}
				
				// de-select everything
				int selected = -1;
				
				SLogMsg("Eh!");
				
				// go in reverse, because that's the order the controls are drawn in:
				for (int i = (int)ARRAY_COUNT(V->m_controls) - 1; i >= 0; i--)
				{
					if (!V->m_controls[i].m_used) continue;
					
					if (selected < 0 && RectangleContains(&V->m_controls[i].m_rect, &p))
					{
						selected = i;
						V->m_controls[i].m_sele = true;
					}
					else
					{
						V->m_controls[i].m_sele = false;
					}
				}
				
				// if we have selected something
				if (selected >= 0)
				{
					// Check if the cursor is within any of the handles
					
					for (int i = HANDLE_BR; i < HANDLE_COUNT; i++)
					{
						Rectangle sHandle = VbMakeHandleRect(i, V->m_controls[selected].m_rect);
						
						if (RectangleContains(&sHandle, &p))
						{
							V->m_controlResized = selected;
							V->m_resizeKnob     = i;
							V->m_resizeRect     = V->m_controls[selected].m_rect;
							break;
						}
					}
				}
				
				V->m_curPosX = posX;
				V->m_curPosY = posY;
				
				VbRedraw(pWindow);
			}
			else
			{
				V->m_drawing = true;
				
				//Anchor
				int posX = GET_X_PARM (parm1), posY = GET_Y_PARM (parm1);
				if (V->m_anchorX == -1)
				{
					V->m_anchorX = posX;
					V->m_anchorY = posY;
				}
				
				//Draw rectangle on old position
				if (V->m_curPosX != -1)
				{
					int L = V->m_anchorX, T = V->m_anchorY, R = V->m_curPosX, B = V->m_curPosY;
					
					FixUpCoords(pWindow, &L, &T, &R, &B);
					
					VidDrawRect(WINDOW_BACKGD_COLOR, L, T, R, B);
					Rectangle thing = { L - 2, T - 2, R + 2, B + 2 };
					VbDrawGrid  (pWindow, &thing);
					VbRenderCtls(pWindow, &thing);
				}
				
				//VbRedraw(pWindow);
				
				V->m_curPosX = posX;
				V->m_curPosY = posY;
				
				int L = V->m_anchorX, T = V->m_anchorY, R = V->m_curPosX, B = V->m_curPosY;
				
				FixUpCoords(pWindow, &L, &T, &R, &B);
				
				Rectangle thing = { L, T, R, B };
				V->m_lastDrawnSelArea = thing;
				VbDrawGrid  (pWindow, &thing);
				VbRenderCtls(pWindow, &thing);
				
				VidDrawRect(0x000000, L, T, R, B);
			}
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (parm1 == (0x80 | KEY_DELETE))
			{
				for (int i = 0; i < (int)ARRAY_COUNT(V->m_controls); i++)
				{
					DesignerControl *C = &V->m_controls[i];
					
					if (!C->m_sele) continue;
					
					//Delete
					VidFillRectangle(WINDOW_BACKGD_COLOR, V->m_controls[i].m_rect);
					
					VbDelControl (pWindow, i);
					
					C->m_sele = false;
				}
				
				VbRedraw(pWindow);
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			int posX = GET_X_PARM (parm1), posY = GET_Y_PARM (parm1);
			
			V->m_curPosX = posX;
			V->m_curPosY = posY;
			
			V->m_drawing = false;
			
			int L = V->m_lastDrawnSelArea.left,
			    T = V->m_lastDrawnSelArea.top,
			    R = V->m_lastDrawnSelArea.right,
				B = V->m_lastDrawnSelArea.bottom;
			
			if (V->m_controlResized >= 0)
			{
				V->m_lastDrawnSelArea.left = -1;
				V->m_controlResized = -1;
			}
			
			if (L != -1)
			{
				VidDrawRect(WINDOW_BACKGD_COLOR, L, T, R, B);
				Rectangle thing = { L - 2, T - 2, R + 2, B + 2 };
				VbDrawGrid  (pWindow, &thing);
				VbRenderCtls(pWindow, &thing);
			}
			
			V->m_lastDrawnSelArea.left = -1;
			
			SLogMsg("Tool: %d", V->m_selCtlType);
			
			if (V->m_selCtlType > 0)
			{
				if (L != R && T != B) 
				{
					// Add the control
					VbAddControl (pWindow, L, T, R, B);
					
					VbRedraw(pWindow);
				}
				else
				{
					Rectangle thing = { L - 2, T - 2, R + 2, B + 2 };
					VbDrawGrid  (pWindow, &thing);
					VbRenderCtls(pWindow, &thing);
				}
			}
			else if (V->m_selCtlType == TOOL_SELECT)
			{
				SLogMsg("Selecting L%d T%d R%d B%d",L,T,R,B);
				VbSelect(pWindow, L, T, R, B);
				VbRedraw(pWindow);
			}
			
			V->m_anchorX = V->m_anchorY = V->m_curPosX = V->m_curPosX = -1;
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}
void CALLBACK VbPreviewWindow(Window *pWindow);

void CALLBACK PrgVbMainWndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r = {0,0,0,0};
			
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, 1000, 0, 0);
			
			AddMenuBarItem(pWindow, 1000, 0, 1, "File");
			AddMenuBarItem(pWindow, 1000, 0, 2, "Edit");
			AddMenuBarItem(pWindow, 1000, 0, 3, "View");
			AddMenuBarItem(pWindow, 1000, 0, 4, "Window");
			AddMenuBarItem(pWindow, 1000, 0, 5, "Help");
			AddMenuBarItem(pWindow, 1000, 5, 6, "About Codename V-Builder...");
			AddMenuBarItem(pWindow, 1000, 3, 7, "Preview...");
			AddMenuBarItem(pWindow, 1000, 3, 8, "");
			AddMenuBarItem(pWindow, 1000, 3, 9, "Exit");
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 != 1000)
			{
				break;
			}
			
			switch (parm2)
			{
				case 9:
					DefaultWindowProc(pWindow, EVENT_DESTROY, 0, 0);
					break;
				case 6:
					ShellAbout("Codename V-Builder", ICON_DLGEDIT);
					break;
				case 7:
					VbPreviewWindow((Window*)pWindow->m_data);
					break;
			}
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void CALLBACK PrgVbPreviewProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			//Create the controls
			Window* pSubWnd = ((Window*)pWindow->m_data);
			
			VBuilderData* pData = (VBuilderData*)(pSubWnd->m_data);
			
			for (int i = 0; i < (int)ARRAY_COUNT(pData->m_controls); i++)
			{
				DesignerControl *C = &pData->m_controls[i];
				if (!C->m_used) continue;
				
				AddControl (pWindow, C->m_type, C->m_rect, C->m_text, 1000 + i, C->m_prm1, C->m_prm2);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void CALLBACK VbPreviewWindow(Window *pWindow)
{
	char buffer[1024];
	strcpy(buffer, "Preview - ");
	strcat(buffer, pWindow->m_title);
	PopupWindowEx(
		pWindow,
		buffer,
		pWindow->m_rect.left + 30,
		pWindow->m_rect.top  + 30,
		GetWidth(&pWindow->m_rect),
		GetHeight(&pWindow->m_rect),
		PrgVbPreviewProc,
		0,
		pWindow
	);
}

void CALLBACK PrgToolkitProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			
			#define E(a) (1000 + (a))
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 0 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Cursor",       E(TOOL_CURSOR),        WINDOW_TEXT_COLOR, WINDOW_TITLE_INACTIVE_COLOR_B);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 1 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Select",       E(TOOL_SELECT),        WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 2 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Text",         E(CONTROL_TEXT),       WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 3 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Text Cen",     E(CONTROL_TEXTCENTER), WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 4 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Button",       E(CONTROL_BUTTON),     WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 5 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Input 1-Ln",   E(CONTROL_TEXTINPUT),  WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 6 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Input M-Ln",   E(CONTROL_COUNT),      WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			RECT(r, 3, 3 + TITLE_BAR_HEIGHT + 7 * 24, 96, 23);
			AddControl(pWindow, CONTROL_BUTTON_COLORED, r, "Checkbox",     E(CONTROL_CHECKBOX),   WINDOW_TEXT_COLOR, BUTTON_MIDDLE_COLOR);
			#undef E
			
			break;
		}
		case EVENT_COMMAND:
		{
			//gotta love this
			//int *pType = &(((VBuilderData*)(((Window*)pWindow->m_data)->m_data)->m_selCtlType);
			
			Window* pFormBldWindow = (Window*)pWindow->m_data;
			
			VBuilderData* pData = (VBuilderData*)(pFormBldWindow->m_data);
			
			int *pType = &pData->m_selCtlType;
			SetImageCtlMode(pWindow, *pType + 1000, BUTTON_MIDDLE_COLOR);
			
			*pType = parm1 - 1000;
			SetImageCtlMode(pWindow, *pType + 1000, WINDOW_TITLE_INACTIVE_COLOR_B);
			
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
			if (parm1 - 1000 == TOOL_CURSOR)
				ChangeCursor (pFormBldWindow, CURSOR_DEFAULT);
			else
				ChangeCursor (pFormBldWindow, CURSOR_CROSS);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void PrgVBldTask (__attribute__((unused)) int argument)
{
	// The form builder window
	Window* pMainWindow  = CreateWindow ("Codename V-Builder", 100, 100, DEF_VBUILD_WID, DEF_VBUILD_HEI, PrgVbMainWndProc, WF_ALWRESIZ);
	pMainWindow->m_iconID = ICON_DLGEDIT;
	
	if (!pMainWindow)
	{
		DebugLogMsg("Hey, the Main window couldn't be created");
		return;
	}
	
	Window* pFormWindow  = CreateWindow ("Form Designer", 240, 180-18+TITLE_BAR_HEIGHT, DEF_FDESIGN_WID, DEF_FDESIGN_HEI, PrgFormBldProc, WF_ALWRESIZ | WF_SYSPOPUP);
	pFormWindow->m_iconID = ICON_DLGEDIT;
	
	if (!pFormWindow)
	{
		DebugLogMsg("Hey, the Form window couldn't be created");
		DestroyWindow(pMainWindow);
		return;
	}
	
	// The tool box window
	Window* pToolsWindow = CreateWindow ("Toolbox", 100, 180-18+TITLE_BAR_HEIGHT, DEF_TOOLBOX_WID, DEF_TOOLBOX_HEI, PrgToolkitProc, WF_NOCLOSE | WF_NOMINIMZ | WF_SYSPOPUP);
	pToolsWindow->m_iconID = ICON_NULL;
	pToolsWindow->m_data   = pFormWindow;
	pMainWindow ->m_data   = pFormWindow;
	
	if (!pToolsWindow)
	{
		DebugLogMsg("Hey, the Tools window couldn't be created");
		DestroyWindow(pMainWindow);
		DestroyWindow(pFormWindow);
		return;
	}
	
	// event loop:
	while (true)
	{
		bool b1 = HandleMessages (pMainWindow);
		bool b2 = HandleMessages (pFormWindow);
		bool b3 = HandleMessages (pToolsWindow);
		
		if (!b1)
		{
			pMainWindow = NULL;
		}
		
		if (!b2)
		{
			pFormWindow = NULL;
		}
		
		if (!b3)
		{
			pToolsWindow = NULL;
		}
		
		if (!b1 || !b2 || !b3)
		{
			if (pMainWindow)
			{
				DestroyWindow(pMainWindow);
				while (HandleMessages(pMainWindow));
			}
			if (pFormWindow)
			{
				DestroyWindow(pFormWindow);
				while (HandleMessages(pFormWindow));
			}
			if (pToolsWindow)
			{
				DestroyWindow(pToolsWindow);
				while (HandleMessages(pToolsWindow));
			}
			break;
		}
	}
}
