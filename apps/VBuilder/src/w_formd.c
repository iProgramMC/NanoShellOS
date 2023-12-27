/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

    Codename V-Builder - Form Designer
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "w_defs.h"

extern int g_selCtlType;

int  g_anchorX, g_anchorY;
int  g_curPosX, g_curPosY;
bool g_drawing;
int  g_resizeKnob;

DesignerControl* g_controlResized;

Rectangle g_lastDrawnSelArea;
Rectangle g_resizeRect;

Window* g_pFormDesignerWindow;

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

// Data
DesignerControl* g_controlsFirst, *g_controlsLast;
extern DesignerControl* g_pEditedControl;

void VbAddControlToList(DesignerControl* pCtl)
{
	if (g_controlsLast == NULL)
	{
		g_controlsFirst = g_controlsLast = pCtl;
		pCtl->m_pNext = pCtl->m_pPrev = NULL;
	}
	else
	{
		pCtl->m_pPrev = g_controlsLast;
		g_controlsLast->m_pNext = pCtl;
		g_controlsLast = pCtl;
	}
}

#define SWAPI(a,b)  do{int tmp; tmp=a; a=b; b=tmp; }while (0)

void VbDrawGrid(Rectangle* pWithinRect /*= NULL*/)
{
	int ox = 0, oy = 0;
	int windowWid = GetWidth (&g_pFormDesignerWindow->m_rect);
	int windowHei = GetHeight(&g_pFormDesignerWindow->m_rect);
	
	for (; ox <= windowWid; ox += GRID_WIDTH)
	{
		for (oy = 0; oy <= windowHei; oy += GRID_WIDTH)
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
	for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
	{
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
	if (*L < 0)
		*L = 0;
	if (*T < 0)
		*T = 0;
	if (*R >= GetWidth (&g_pFormDesignerWindow->m_rect))
		*R  = GetWidth (&g_pFormDesignerWindow->m_rect);
	if (*B >= GetHeight(&g_pFormDesignerWindow->m_rect))
		*B  = GetHeight(&g_pFormDesignerWindow->m_rect);
	
	// Round to the nearest GRID_WIDTH
	
	*L = (*L / GRID_WIDTH) * GRID_WIDTH;
	*T = (*T / GRID_WIDTH) * GRID_WIDTH;
	*R = ((*R + GRID_WIDTH - 1) / GRID_WIDTH) * GRID_WIDTH;
	*B = ((*B + GRID_WIDTH - 1) / GRID_WIDTH) * GRID_WIDTH;
}

int g_ctlNameNums[CONTROL_COUNT];

const char* const g_ctlNames[] = {
	"None",
	"Text",
	"Icon",
	"Button",
	"TextBox",
	"CheckBox",
	"ClickLabel",
	"TextCen",
	"ButtonEvent",
	"ListView",
	"VScrollBar",
	"HScrollBar",
	"MenuBar",
	"TextHuge",
	"IconView",
	"SurroundRect",
	"ButtonColor",
	"ButtonList",
	"ButtonIcon",
	"ButtonIconBar",
	"HLine",
	"Image",
	"TaskList",
	"IconViewDrag",
};

const char* VbGetCtlName(int type)
{
	if (type >= (int)ARRAY_COUNT(g_ctlNames)) return "Unknown";
	
	return g_ctlNames[type];
}

void VbSetEditedControl(DesignerControl* pControl);

void VbAddControl(int L, int T, int R, int B)
{
	Rectangle rect;
	rect.left = L, rect.top = T, rect.right = R, rect.bottom = B;
	
	for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
	{
		C->m_sele = false;
	}
	
	// Add a control
	DesignerControl *C = calloc(1, sizeof(DesignerControl));
	
	if (!C) return;
	
	strcpy (C->m_text, "Hello");
	sprintf(C->m_name, "%s%d", VbGetCtlName(g_selCtlType), ++g_ctlNameNums[g_selCtlType]);
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
	
	VbAddControlToList(C);
	VbSetEditedControl(C);
}

void VbDelControl(DesignerControl* C)
{
	if (C->m_pNext)
	{
		C->m_pNext->m_pPrev = C->m_pPrev;
	}
	if (C->m_pPrev)
	{
		C->m_pPrev->m_pNext = C->m_pNext;
	}
	
	if (g_controlsFirst == C)
	{
		g_controlsFirst = g_controlsFirst->m_pNext;
	}
	if (g_controlsLast  == C)
	{
		g_controlsLast  = g_controlsLast->m_pPrev;
	}
	
	if (g_pEditedControl == C)
	{
		VbSetEditedControl(NULL);
	}
	
	free(C);
}

void VbSelect(int L, int T, int R, int B)
{
	Rectangle rect = {L,T,R,B};
	
	int selectedNum = 0;
	DesignerControl* pCtl = NULL;
	
	// go in reverse, because that's the order the controls are drawn in:
	for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
	{
		C->m_sele = RectangleOverlap (&rect, &C->m_rect);
		if (C->m_sele)
		{
			selectedNum++;
			pCtl = C;
		}
	}
	
	if (selectedNum != 1)
		VbSetEditedControl (NULL);
	else
		VbSetEditedControl (pCtl);
}

void CALLBACK PrgFormBldProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			g_anchorX = g_anchorY = g_curPosX = g_curPosY = -1;
			g_controlResized = NULL;
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
				if (g_controlResized)
				{
					int deltaX = posX - g_curPosX;
					int deltaY = posY - g_curPosY;
					
					DesignerControl * C = g_controlResized;
					
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
						VbRenderCtls(NULL);
					}
				
					g_curPosX = posX;
					g_curPosY = posY;
					
					break;
				}
				
				// de-select everything
				DesignerControl* selected = NULL;
				
				// go in reverse, because that's the order the controls are drawn in:
				for (DesignerControl* C = g_controlsLast; C; C = C->m_pPrev)
				{
					if (!selected && RectangleContains(&C->m_rect, &p))
					{
						selected = C;
						C->m_sele = true;
						VbSetEditedControl (C);
					}
					else
					{
						C->m_sele = false;
					}
				}
				
				// if we have selected something
				if (selected)
				{
					// Check if the cursor is within any of the handles
					
					for (int i = HANDLE_BR; i < HANDLE_COUNT; i++)
					{
						Rectangle sHandle = VbMakeHandleRect(i, selected->m_rect);
						
						if (RectangleContains(&sHandle, &p))
						{
							g_controlResized = selected;
							g_resizeKnob     = i;
							g_resizeRect     = selected->m_rect;
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
				Rectangle thing2 = { L - 2, T - 2, R + 2, B + 2 };
				VbDrawGrid  (&thing2);
				VbRenderCtls(&thing2);
				
				VidDrawRect(0x000000, L, T, R, B);
			}
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (parm1 == (0x80 | KEY_DELETE))
			{
				for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
				{
					if (!C->m_sele) continue;
					
					//Delete
					VidFillRectangle(WINDOW_BACKGD_COLOR, C->m_rect);
					
					VbDelControl (C);
					
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
			
			if (g_controlResized)
			{
				g_lastDrawnSelArea.left = -1;
				g_controlResized = NULL;
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

void VbCreateFormDesignerWindow()
{
	Window* pFormWindow  = CreateWindow ("Form Designer", 240 - D_OFFSET, 180 - D_OFFSET, DEF_FDESIGN_WID, DEF_FDESIGN_HEI, PrgFormBldProc, WF_ALWRESIZ | WF_SYSPOPUP);
	
	if (!pFormWindow)
	{
		exit(1);
	}
	
	SetWindowIcon(pFormWindow, ICON_DLGEDIT);
	
	g_pFormDesignerWindow = pFormWindow;
}


