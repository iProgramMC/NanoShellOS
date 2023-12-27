/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

        Promotion Popup source file
******************************************/
#include "chess.h"

extern Window* g_pWindow;

// TODO: Show icons instead.

ePiece g_PromotedPiecePlace;

void CALLBACK PromoPopupWndProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			g_PromotedPiecePlace = PIECE_NONE;
			
			Rectangle r;
			RECT(r, 0, 0, 200, 25);
			
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "Choose a piece to promote, or cancel", 100000, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL | TEXTSTYLE_HCENTERED | TEXTSTYLE_VCENTERED);
			
			// The buttons.
			static const char* const promoText[] = { "Queen", "Rook", "Knight", "Bishop" };
			static const int promoBtns[] = { PIECE_QUEEN, PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP };
			
			for (int i = 0; i < 4; i++)
			{
				RECT(r, i * 70 + 3, 30, 70 - 6, 30);
				
				// the parm1 is the icon associated with the button. I added it for testing
				AddControl(pWindow, CONTROL_BUTTON, r, promoText[i], promoBtns[i], promoBtns[i] + 42, 0);
			}
			
			// The cancel button
			RECT(r, (280-80)/2, 70, 70, 20);
			AddControl(pWindow, CONTROL_BUTTON, r, "Cancel", PIECE_NONE, ICON_BACK, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			g_PromotedPiecePlace = parm1;
			RegisterEventInsideWndProc(pWindow, EVENT_DESTROY, 0, 0);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

ePiece PromotionPopup(eColor color, int row, int col)
{
	Rectangle rect;
	GetWindowRect(g_pWindow, &rect);
	
	char buffer[50];
	sprintf(buffer, "Promote Pawn on %c%d (%s)", 'a'+col, row+1, color ? "Black": "White");
	
	PopupWindow(g_pWindow, buffer, rect.left + 50, rect.top + 50, 280, 100, PromoPopupWndProc, WF_NOMINIMZ | WF_SYSPOPUP | WF_NOCLOSE);
	
	return g_PromotedPiecePlace;
}
