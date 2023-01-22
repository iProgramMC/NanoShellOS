/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

  Codename V-Builder - The form designer
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"

#define TEXT_BOX_HEIGHT (22)

#define DEF_VBUILD_WID 640
#define DEF_VBUILD_HEI (TITLE_BAR_HEIGHT*2 + 6 + TEXT_BOX_HEIGHT + 6)
#define DEF_FDESIGN_WID 500
#define DEF_FDESIGN_HEI 400
#define DEF_TOOLBOX_WID (24 * 2 + 6)
#define DEF_TOOLBOX_HEI (24 * 4 + 6 + TITLE_BAR_HEIGHT)

#define BUTTONDARK 0x808080
#define BUTTONMIDD BUTTON_MIDDLE_COLOR
#define BUTTONLITE 0xFFFFFF
#define BUTTONMIDC WINDOW_BACKGD_COLOR

#define GRABBER_SIZE (8)
#define GRID_WIDTH   (8)

enum eComboID
{
	CO_MENU_BAR = 1000,
	CO_EDITED_CTL,
	CO_EDITED_CHOOSE,
	CO_EDITED_FIELD_NAME,
	CO_EDITED_FIELD,
	CO_EDITED_FIELD_CHOOSE,
};

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

typedef struct tagDesCtl
{
	struct tagDesCtl *m_pPrev, *m_pNext;
	
	Rectangle m_rect;
	int       m_type;
	char      m_name[128];
	char      m_text[128];
	bool      m_sele;
	int       m_prm1, m_prm2;
	int       m_comboID;
}
DesignerControl;

// Data
DesignerControl* g_controlsFirst, *g_controlsLast;


DesignerControl* g_pEditedControl;


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

int  g_anchorX, g_anchorY;
int  g_curPosX, g_curPosY;
bool g_drawing;
int  g_selCtlType;
int  g_resizeKnob;

DesignerControl* g_controlResized;

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

void CALLBACK PrgFormBldProc (Window* pWindow, int messageType, int parm1, int parm2)
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
void CALLBACK VbPreviewWindow();


void VbSetEditedControl2(DesignerControl* pControl)
{
	if (pControl)
	{
		SetTextInputText(g_pMainWindow, CO_EDITED_CTL, pControl->m_name);
	}
	else
	{
		SetTextInputText(g_pMainWindow, CO_EDITED_CTL, "");
	}
	
	CallWindowCallbackAndControls(g_pMainWindow, EVENT_PAINT, 0, 0);
}

void VbSetEditedControl(DesignerControl* pControl)
{
	RegisterEvent(g_pMainWindow, EVENT_USER, (int)pControl, 0);
}

#define SELECT_CTL_WIDTH  (150)
#define SELECT_CTL_HEIGHT (200 + TITLE_BAR_HEIGHT)

void CALLBACK PrgVbSelectCtlProc(Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			
			RECT(r, 10, TITLE_BAR_HEIGHT + 10, SELECT_CTL_WIDTH - 20, SELECT_CTL_HEIGHT - 50 - TITLE_BAR_HEIGHT);
			
			AddControl(pWindow, CONTROL_LISTVIEW, r, NULL, 1000, 0, 0);
			
			RECT(r, SELECT_CTL_WIDTH - 80, SELECT_CTL_HEIGHT - 30, 70, 20);
			
			AddControl(pWindow, CONTROL_BUTTON, r, "OK", 1001, 0, 0);
			
			// Add whatever's in the list
			for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
			{
				AddElementToList(pWindow, 1000, C->m_name, ICON_FORWARD);
			}
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == 1001)
			{
				// Get the element.
				DesignerControl* element = NULL;
				
				int index = GetSelectedIndexList(pWindow, 1000);
				const char* str = GetElementStringFromList(pWindow, 1000, index);
				
				if (str)
				{
					for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
					{
						if (strcmp(C->m_name, str) == 0)
						{
							element = C;
							break;
						}
					}
					
					*((DesignerControl**)GetWindowData(pWindow)) = element;
				}
				
				DestroyWindow(pWindow);
			}
			break;
		}
		default:
		{
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
		}
	}
}

void VbSelectControlDialog()
{
	Rectangle mwr;
	GetWindowRect(g_pMainWindow, &mwr);
	
	DesignerControl* ctler = NULL;
	
	PopupWindowEx(g_pMainWindow, "Select Control", mwr.left + 50, mwr.top + 50, SELECT_CTL_WIDTH, SELECT_CTL_HEIGHT, PrgVbSelectCtlProc, WF_SYSPOPUP | WF_NOMINIMZ, &ctler);
	
	for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
		C->m_sele = false;
	
	if (ctler)
	{
		ctler->m_sele = true;
	}
	
	VbSetEditedControl2(ctler);
	
	RegisterEvent(g_pFormDesignerWindow, EVENT_PAINT, 0, 0);
}

void CALLBACK PrgVbMainWndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_USER:
		{
			VbSetEditedControl2((DesignerControl*)parm1);
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r = {0,0,0,0};
			
			AddControl (pWindow, CONTROL_MENUBAR, r, NULL, CO_MENU_BAR, 0, 0);
			
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 1, "File");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 2, "Edit");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 3, "View");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 4, "Window");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 0, 5, "Help");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 5, 6, "About Codename V-Builder...");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 3, 7, "Preview...");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 3, 8, "");
			AddMenuBarItem(pWindow, CO_MENU_BAR, 3, 9, "Exit");
			
			//#define DEF_VBUILD_HEI (TITLE_BAR_HEIGHT*2 + 6 + TEXT_BOX_HEIGHT + 6)
			int thingY = TITLE_BAR_HEIGHT * 2 + 6;
			
			RECT(r, 10, thingY, 120, TEXT_BOX_HEIGHT);
			
			AddControl(pWindow, CONTROL_TEXTINPUT, r, NULL, CO_EDITED_CTL, TEXTEDIT_READONLY, 0);
			
			RECT(r, 132, thingY, TEXT_BOX_HEIGHT - 1, TEXT_BOX_HEIGHT - 1);
			
			AddControl(pWindow, CONTROL_BUTTON_ICON_BAR, r, NULL, CO_EDITED_CHOOSE, ICON_FORWARD, 16);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 != CO_MENU_BAR)
			{
				switch (parm1)
				{
					case CO_EDITED_CHOOSE:
						VbSelectControlDialog();
						break;
				}
				
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
			int i = 0;
			for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
			{
				AddControl (pWindow, C->m_type, C->m_rect, C->m_text, 1000 + i, C->m_prm1, C->m_prm2);
				C->m_comboID = i;
				i++;
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

#define E(a) (1000 + (a))

int g_checkboxNums[] = {
	E(TOOL_CURSOR),
	E(TOOL_SELECT),
	E(CONTROL_TEXT),
	E(CONTROL_TEXTCENTER),
	E(CONTROL_BUTTON),
	E(CONTROL_TEXTINPUT),
	E(CONTROL_COUNT),
	E(CONTROL_CHECKBOX),
};

void VbToolkitOnSelect(int toolNum)
{
	for (int i = 0; i < (int)ARRAY_COUNT(g_checkboxNums); i++)
	{
		CheckboxSetChecked(g_pToolboxWindow, g_checkboxNums[i], false);
	}
	
	CheckboxSetChecked(g_pToolboxWindow, E(toolNum), true);
}

void CALLBACK PrgToolkitProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			//TODO: Clean up this function
			
			RECT(r, 3 + 0 * 24, 3 + TITLE_BAR_HEIGHT + 0 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(TOOL_CURSOR),        ICON_VB_CURSOR,      16);
			RECT(r, 3 + 1 * 24, 3 + TITLE_BAR_HEIGHT + 0 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(TOOL_SELECT),        ICON_VB_SELECT,      16);
			RECT(r, 3 + 0 * 24, 3 + TITLE_BAR_HEIGHT + 1 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_TEXT),       ICON_VB_TEXT,        16);
			RECT(r, 3 + 1 * 24, 3 + TITLE_BAR_HEIGHT + 1 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_TEXTCENTER), ICON_VB_TEXT_CEN,    16);
			RECT(r, 3 + 1 * 24, 3 + TITLE_BAR_HEIGHT + 2 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_TEXTINPUT),  ICON_VB_INPUT_1LINE, 16);
			RECT(r, 3 + 0 * 24, 3 + TITLE_BAR_HEIGHT + 2 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_COUNT),      ICON_VB_INPUT_MLINE, 16);
			RECT(r, 3 + 0 * 24, 3 + TITLE_BAR_HEIGHT + 3 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_BUTTON),     ICON_VB_BUTTON,      16);
			RECT(r, 3 + 1 * 24, 3 + TITLE_BAR_HEIGHT + 3 * 24, (DEF_TOOLBOX_WID - 6) / 2, 23);
			AddControl(pWindow, CONTROL_BUTTON_ICON_CHECKABLE, r, NULL, E(CONTROL_CHECKBOX),   ICON_VB_CHECKBOX,    16);
#undef E
			
			VbToolkitOnSelect(TOOL_CURSOR);
			
			break;
		}
		case EVENT_CHECKBOX:
		{
			// we ignore the second parameter
			VbToolkitOnSelect(parm1 - 1000);
			
			int *pType = &g_selCtlType;
			*pType = parm1 - 1000;
			
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
