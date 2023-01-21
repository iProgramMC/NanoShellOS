/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

  Codename V-Builder - The form designer
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"

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

// Data
DesignerControl g_controls[64];

int  g_anchorX, g_anchorY;
int  g_curPosX, g_curPosY;
bool g_drawing;
int  g_selCtlType;
int  g_controlResized;
int  g_resizeKnob;

Rectangle g_lastDrawnSelArea;
Rectangle g_resizeRect;

Window* g_pFormDesignerWindow;
Window* g_pToolboxWindow;
Window* g_pMainWindow;

#define SWAPI(a,b)  do{int tmp; tmp=a; a=b; b=tmp; }while (0)

void VbDrawGrid(Rectangle* pWithinRect /*= NULL*/)
{
	int ox = 3, oy = TITLE_BAR_HEIGHT + 4;
	int windowWid = GetWidth (&g_pFormDesignerWindow->m_rect);
	int windowHei = GetHeight(&g_pFormDesignerWindow->m_rect);
	
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

void VbRenderCtls(Rectangle* pWithinRect /*= NULL*/)
{
	for (int i = 0; i < (int)ARRAY_COUNT(g_controls); i++)
	{
		if (!g_controls[i].m_used) continue;
		
		DesignerControl *C = &g_controls[i];
		
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
				RenderCheckbox(srect, false, C->m_text);
				
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

void VbRedraw()
{
	VbDrawGrid  (NULL);
	VbRenderCtls(NULL);
}

void FixUpCoords (int *L, int *T, int *R, int *B)
{
	if (*L > *R) SWAPI(*L,*R);
	if (*T > *B) SWAPI(*T,*B);
	
	//Ensure
	if (*L < 3)
		*L = 3;
	if (*T < 2 + TITLE_BAR_HEIGHT)
		*T = 2 + TITLE_BAR_HEIGHT;
	if (*R >= GetWidth (&g_pFormDesignerWindow->m_rect) - 3)
		*R  = GetWidth (&g_pFormDesignerWindow->m_rect) - 3;
	if (*B >= GetHeight(&g_pFormDesignerWindow->m_rect) - 3)
		*B  = GetHeight(&g_pFormDesignerWindow->m_rect) - 3;
	
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

void VbAddControl(int L, int T, int R, int B)
{
	Rectangle rect;
	rect.left = L, rect.top = T, rect.right = R, rect.bottom = B;
	
	for (int i = 0; i < (int)ARRAY_COUNT(g_controls); i++)
	{
		if (!g_controls[i].m_used) continue;
		g_controls[i].m_sele = false;
	}
	// Add a control
	for (int i = 0; i < (int)ARRAY_COUNT(g_controls); i++)
	{
		if (g_controls[i].m_used) continue;
		
		DesignerControl *C = &g_controls[i];
		
		strcpy (C->m_text, "Hello");
		C->m_used = true;
		C->m_type = g_selCtlType;
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
	
	MessageBox(g_pFormDesignerWindow, "There are too many controls in this form, delete some.", "Codename V-Builder", MB_OK | ICON_WARNING << 16);
}

void VbDelControl(int i)
{
	if (!g_controls[i].m_used) return;
	
	g_controls[i].m_used = false;
}

void VbSelect(int L, int T, int R, int B)
{
	Rectangle rect = {L,T,R,B};
	// go in reverse, because that's the order the controls are drawn in:
	for (int i = (int)ARRAY_COUNT(g_controls) - 1; i >= 0; i--)
	{
		if (!g_controls[i].m_used) continue;
		
		g_controls[i].m_sele = RectangleOverlap (&rect, &g_controls[i].m_rect);
	}
}

void CALLBACK PrgFormBldProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			g_anchorX = g_anchorY = g_curPosX = g_curPosY = -1;
			g_controlResized = -1;
			g_selCtlType = TOOL_CURSOR;
			
			break;
		}
		case EVENT_DESTROY:
		{
			break;
		}
		case EVENT_PAINT:
		{
			//Paint some dots
			VbRedraw();
			break;
		}
		case EVENT_COMMAND:
		{
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (g_selCtlType == TOOL_CURSOR)
			{
				int posX = GET_X_PARM (parm1), posY = GET_Y_PARM (parm1);
				Point p = { posX, posY };
				
				// Are we dragging a handle ?
				if (g_controlResized >= 0)
				{
					int deltaX = posX - g_curPosX;
					int deltaY = posY - g_curPosY;
					
					DesignerControl * C = &g_controls[g_controlResized];
					
					if (deltaX != 0 || deltaY != 0)
					{
						VidFillRectangle(WINDOW_BACKGD_COLOR, C->m_rect);
					}
					
					switch (g_resizeKnob)
					{
						case HANDLE_TL:
							g_resizeRect.left   += deltaX;
							g_resizeRect.top    += deltaY;
							g_resizeRect.right  += deltaX;
							g_resizeRect.bottom += deltaY;
							break;
							
						case HANDLE_BR:
							g_resizeRect.right  += deltaX;
							g_resizeRect.bottom += deltaY;
							
							if (g_resizeRect.right < g_resizeRect.left + GRID_WIDTH)
								g_resizeRect.right = g_resizeRect.left + GRID_WIDTH;
							
							if (g_resizeRect.bottom < g_resizeRect.top + GRID_WIDTH)
								g_resizeRect.bottom = g_resizeRect.top + GRID_WIDTH;
							break;
							
						case HANDLE_CR:
							g_resizeRect.right += deltaX;
							
							if (g_resizeRect.right < g_resizeRect.left + GRID_WIDTH)
								g_resizeRect.right = g_resizeRect.left + GRID_WIDTH;
							break;
							
						case HANDLE_BM:
							g_resizeRect.bottom += deltaY;
							
							if (g_resizeRect.bottom < g_resizeRect.top + GRID_WIDTH)
								g_resizeRect.bottom = g_resizeRect.top + GRID_WIDTH;
							break;
					}
					
					C->m_rect = g_resizeRect;
					
					if (g_resizeKnob == HANDLE_TL)
					{
						int width  = C->m_rect.right  - C->m_rect.left;
						int height = C->m_rect.bottom - C->m_rect.top;
						FixUpCoords(&C->m_rect.left, &C->m_rect.top, &C->m_rect.right, &C->m_rect.bottom);
						C->m_rect.right  = C->m_rect.left + width;
						C->m_rect.bottom = C->m_rect.top  + height;
					}
					else
					{
						FixUpCoords(&C->m_rect.left, &C->m_rect.top, &C->m_rect.right, &C->m_rect.bottom);
					}
					
					if (deltaX != 0 || deltaY != 0)
					{
						VbRedraw();
					}
				
					g_curPosX = posX;
					g_curPosY = posY;
					
					break;
				}
				
				// de-select everything
				int selected = -1;
				
				// go in reverse, because that's the order the controls are drawn in:
				for (int i = (int)ARRAY_COUNT(g_controls) - 1; i >= 0; i--)
				{
					if (!g_controls[i].m_used) continue;
					
					if (selected < 0 && RectangleContains(&g_controls[i].m_rect, &p))
					{
						selected = i;
						g_controls[i].m_sele = true;
					}
					else
					{
						g_controls[i].m_sele = false;
					}
				}
				
				// if we have selected something
				if (selected >= 0)
				{
					// Check if the cursor is within any of the handles
					
					for (int i = HANDLE_BR; i < HANDLE_COUNT; i++)
					{
						Rectangle sHandle = VbMakeHandleRect(i, g_controls[selected].m_rect);
						
						if (RectangleContains(&sHandle, &p))
						{
							g_controlResized = selected;
							g_resizeKnob     = i;
							g_resizeRect     = g_controls[selected].m_rect;
							break;
						}
					}
				}
				
				g_curPosX = posX;
				g_curPosY = posY;
				
				VbRedraw();
			}
			else
			{
				g_drawing = true;
				
				//Anchor
				int posX = GET_X_PARM (parm1), posY = GET_Y_PARM (parm1);
				if (g_anchorX == -1)
				{
					g_anchorX = posX;
					g_anchorY = posY;
				}
				
				//Draw rectangle on old position
				if (g_curPosX != -1)
				{
					int L = g_anchorX, T = g_anchorY, R = g_curPosX, B = g_curPosY;
					
					FixUpCoords(&L, &T, &R, &B);
					
					VidDrawRect(WINDOW_BACKGD_COLOR, L, T, R, B);
					Rectangle thing = { L - 2, T - 2, R + 2, B + 2 };
					VbDrawGrid  (&thing);
					VbRenderCtls(&thing);
				}
				
				//VbRedraw();
				
				g_curPosX = posX;
				g_curPosY = posY;
				
				int L = g_anchorX, T = g_anchorY, R = g_curPosX, B = g_curPosY;
				
				FixUpCoords(&L, &T, &R, &B);
				
				Rectangle thing = { L, T, R, B };
				g_lastDrawnSelArea = thing;
				VbDrawGrid  (&thing);
				VbRenderCtls(&thing);
				
				VidDrawRect(0x000000, L, T, R, B);
			}
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (parm1 == (0x80 | KEY_DELETE))
			{
				for (int i = 0; i < (int)ARRAY_COUNT(g_controls); i++)
				{
					DesignerControl *C = &g_controls[i];
					
					if (!C->m_sele) continue;
					
					//Delete
					VidFillRectangle(WINDOW_BACKGD_COLOR, g_controls[i].m_rect);
					
					VbDelControl (i);
					
					C->m_sele = false;
				}
				
				VbRedraw();
			}
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			int posX = GET_X_PARM (parm1), posY = GET_Y_PARM (parm1);
			
			g_curPosX = posX;
			g_curPosY = posY;
			
			g_drawing = false;
			
			int L = g_lastDrawnSelArea.left,
			    T = g_lastDrawnSelArea.top,
			    R = g_lastDrawnSelArea.right,
				B = g_lastDrawnSelArea.bottom;
			
			if (g_controlResized >= 0)
			{
				g_lastDrawnSelArea.left = -1;
				g_controlResized = -1;
			}
			
			if (L != -1)
			{
				VidDrawRect(WINDOW_BACKGD_COLOR, L, T, R, B);
				Rectangle thing = { L - 2, T - 2, R + 2, B + 2 };
				VbDrawGrid  (&thing);
				VbRenderCtls(&thing);
			}
			
			g_lastDrawnSelArea.left = -1;
			
			if (g_selCtlType > 0)
			{
				if (L != R && T != B) 
				{
					// Add the control
					VbAddControl (L, T, R, B);
					
					VbRedraw();
				}
				else
				{
					Rectangle thing = { L - 2, T - 2, R + 2, B + 2 };
					VbDrawGrid  (&thing);
					VbRenderCtls(&thing);
				}
			}
			else if (g_selCtlType == TOOL_SELECT)
			{
				VbSelect(L, T, R, B);
				VbRedraw();
			}
			
			g_anchorX = g_anchorY = g_curPosX = g_curPosY = -1;
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}
void CALLBACK VbPreviewWindow();

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
					VbPreviewWindow();
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
			for (int i = 0; i < (int)ARRAY_COUNT(g_controls); i++)
			{
				DesignerControl *C = &g_controls[i];
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

void CALLBACK VbPreviewWindow()
{
	char buffer[1024];
	strcpy(buffer, "Preview - ");
	strcat(buffer, GetWindowTitle(g_pFormDesignerWindow));
	
	Rectangle rect;
	GetWindowRect(g_pFormDesignerWindow, &rect);
	
	PopupWindow(
		g_pFormDesignerWindow,
		buffer,
		rect.left + 30,
		rect.top  + 30,
		GetWidth(&rect),
		GetHeight(&rect),
		PrgVbPreviewProc,
		0
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
			int *pType = &g_selCtlType;
			SetImageCtlMode(pWindow, *pType + 1000, BUTTON_MIDDLE_COLOR);
			
			*pType = parm1 - 1000;
			SetImageCtlMode(pWindow, *pType + 1000, WINDOW_TITLE_INACTIVE_COLOR_B);
			
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
			if (parm1 - 1000 == TOOL_CURSOR)
				ChangeCursor (g_pFormDesignerWindow, CURSOR_DEFAULT);
			else
				ChangeCursor (g_pFormDesignerWindow, CURSOR_CROSS);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void KillWindow(Window* pWindow)
{
	DestroyWindow(pWindow);
	while (HandleMessages(pWindow));
}

int NsMain(UNUSED int argc, UNUSED char** argv)
{
	// The form builder window
	Window* pMainWindow  = CreateWindow ("Codename V-Builder", 100, 100, DEF_VBUILD_WID, DEF_VBUILD_HEI, PrgVbMainWndProc, WF_ALWRESIZ);
	
	if (!pMainWindow)
	{
		LogMsg("Hey, the Main window couldn't be created");
		return 1;
	}
	
	SetWindowIcon(pMainWindow, ICON_DLGEDIT);
	
	Window* pFormWindow  = CreateWindow ("Form Designer", 240, 180-18+TITLE_BAR_HEIGHT, DEF_FDESIGN_WID, DEF_FDESIGN_HEI, PrgFormBldProc, WF_ALWRESIZ | WF_SYSPOPUP);
	
	if (!pFormWindow)
	{
		LogMsg("Hey, the Form window couldn't be created");
		KillWindow(pMainWindow);
		return 1;
	}
	
	SetWindowIcon(pFormWindow, ICON_DLGEDIT);
	
	// The tool box window
	Window* pToolsWindow = CreateWindow ("Toolbox", 100, 180-18+TITLE_BAR_HEIGHT, DEF_TOOLBOX_WID, DEF_TOOLBOX_HEI, PrgToolkitProc, WF_NOCLOSE | WF_NOMINIMZ | WF_SYSPOPUP);
	
	if (!pToolsWindow)
	{
		LogMsg("Hey, the Tools window couldn't be created");
		KillWindow(pMainWindow);
		KillWindow(pFormWindow);
		return 1;
	}
	
	SetWindowIcon(pToolsWindow, ICON_NULL);
	SetWindowData(pToolsWindow, pFormWindow);
	SetWindowData(pMainWindow,  pFormWindow);
	
	g_pMainWindow         = pMainWindow;
	g_pToolboxWindow      = pToolsWindow;
	g_pFormDesignerWindow = pFormWindow;
	
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
				KillWindow(pMainWindow);
			}
			if (pFormWindow)
			{
				KillWindow(pFormWindow);
			}
			if (pToolsWindow)
			{
				KillWindow(pToolsWindow);
			}
			break;
		}
	}
	
	return 0;
}
