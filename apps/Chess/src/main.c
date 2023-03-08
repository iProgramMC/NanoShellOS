/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

             Main source file
******************************************/
#include "chess.h"

#define CHESS_WIDTH  (PIECE_SIZE * BOARD_SIZE + SIDE_BAR_WIDTH + 20)
#define CHESS_HEIGHT (PIECE_SIZE * BOARD_SIZE + TOP_BAR_HEIGHT + 20)

// Include the game icons.
#include "../icons/black_pawn.h"
#include "../icons/black_king.h"
#include "../icons/black_queen.h"
#include "../icons/black_rook.h"
#include "../icons/black_knight.h"
#include "../icons/black_bishop.h"
#include "../icons/white_pawn.h"
#include "../icons/white_king.h"
#include "../icons/white_queen.h"
#include "../icons/white_rook.h"
#include "../icons/white_knight.h"
#include "../icons/white_bishop.h"

Window* g_pWindow;
int g_BoardX = 0, g_BoardY = TOP_BAR_HEIGHT;

// note: The row number is flipped, so 1 is the bottom-most row.

Image* const g_pPieceImages[] =
{
	NULL,
	NULL,
	&g_black_pawn_icon,
	&g_white_pawn_icon,
	&g_black_king_icon,
	&g_white_king_icon,
	&g_black_queen_icon,
	&g_white_queen_icon,
	&g_black_rook_icon,
	&g_white_rook_icon,
	&g_black_knight_icon,
	&g_white_knight_icon,
	&g_black_bishop_icon,
	&g_white_bishop_icon,
};

enum
{
	CHESS_TURN_LABEL = 1000,
	
};

void DrawFrameAroundBoard()
{
	Rectangle rect = { g_BoardX, g_BoardY, g_BoardX + BOARD_SIZE * PIECE_SIZE - 1, g_BoardY + BOARD_SIZE * PIECE_SIZE - 1 };
	for (int i = 0; i < BOARD_THICKNESS; i++)
	{
		rect.left--;
		rect.top--;
		rect.right++;
		rect.bottom++;
		
		VidDrawHLine(BUTTON_SHADOW_COLOR, rect.left, rect.right, rect.top);
		VidDrawHLine(BUTTON_HILITE_COLOR, rect.left, rect.right, rect.bottom);
		VidDrawVLine(BUTTON_SHADOW_COLOR, rect.top, rect.bottom, rect.left);
		VidDrawVLine(BUTTON_HILITE_COLOR, rect.top, rect.bottom, rect.right);
	}
}

// white gets the first turn every time
eColor g_playingPlayer = WHITE;

void GetBoardCoords(int mouseX, int mouseY, int * boardRow, int * boardCol)
{
	mouseX -= g_BoardX;
	mouseY -= g_BoardY;
	
	*boardRow = BOARD_SIZE - 1 - mouseY / PIECE_SIZE;
	*boardCol = mouseX / PIECE_SIZE;
}

int g_CursorIDs[PIECE_MAX * 2];

int g_FlashingTiles[BOARD_SIZE][BOARD_SIZE];

void FlashTile(int row, int col)
{
	g_FlashingTiles[row][col] = TILE_FLASH_COUNT;
	PaintTile(row, col);
}

Image* GetPieceImage(ePiece piece, eColor color)
{
	return g_pPieceImages[piece * 2 + color];
}

int GetPieceCursorID(ePiece piece, eColor color)
{
	return g_CursorIDs[piece * 2 + color];
}

void SetPieceCursorID(ePiece piece, eColor color, int id)
{
	if (id < 0)
	{
		MessageBox(g_pWindow, "Cannot load Chess. Too many cursors are loaded into the system. Please close some applications before playing.", "Chess", MB_OK | ICON_ERROR << 16);
		DestroyWindow(g_pWindow);
		while (HandleMessages(g_pWindow));
		exit(0);
	}
	g_CursorIDs[piece * 2 + color] = id;
}

eColor GetNextPlayer(eColor curPlr)
{
	switch (curPlr)
	{
		case WHITE: return BLACK;
		case BLACK: return WHITE;
		
		case NPLAYERS: return BLACK;
	}
	return BLACK;
}

const uint32_t g_boardColors[] = { 0xCBAC8B, 0x7D5834 };
const uint32_t g_boardFlashColors[] = { 0xFF0000, 0xAF0000 };

BoardPiece g_DraggedPiece;
int g_DraggedPieceRow = -1, g_DraggedPieceCol = -1;

void PaintTile(int row, int column)
{
	eColor color = ((row + column) % 2) ? BLACK : WHITE;
	
	int x = g_BoardX + column * PIECE_SIZE;
	int y = g_BoardY + (BOARD_SIZE - 1 - row) * PIECE_SIZE;
	
	bool bFlashed = g_FlashingTiles[row][column] % 2;
	
	// Fill the rectangle.
	uint32_t bc = g_boardColors[color], oc = g_boardColors[!color];
	
	if (bFlashed)
		bc = g_boardFlashColors[color], oc = g_boardFlashColors[!color];
	
	Rectangle rect = { x, y, x + PIECE_SIZE - 1, y + PIECE_SIZE - 1 };
	
	VidFillRectangle(bc, rect);
	
	BoardPiece* pPiece = GetPiece(row, column);
	
	Image* pImg = GetPieceImage(pPiece->piece, pPiece->color);
	
	bool bIsBeingDragged = (g_DraggedPieceRow == row && g_DraggedPieceCol == column);
	
	rect.left   += 2;
	rect.top    += 2;
	rect.right  -= 2;
	rect.bottom -= 2;
	
	// If this is part of the first row or column, draw the text.
	if (row == 0)
	{
		char buf[2];
		buf[0] = 'a' + column;
		buf[1] = 0;
		VidDrawText(buf, rect, TEXTSTYLE_DJUSTIFY | TEXTSTYLE_RJUSTIFY, oc, TRANSPARENT);
	}
	if (column == 0)
	{
		char buf[10];
		snprintf(buf, sizeof buf, "%d", row + 1);
		VidDrawText(buf, rect, 0, oc, TRANSPARENT);
	}
	
	if (pImg && !bIsBeingDragged)
	{
		int offsetX = (PIECE_SIZE - pImg->width) / 2;
		int offsetY = (PIECE_SIZE - pImg->height) / 2;
		VidBlitImage(pImg, x + offsetX, y + offsetY);
	}
}

void PaintBoard()
{
	for (int row = 0; row < BOARD_SIZE; row++)
	{
		for (int column = 0; column < BOARD_SIZE; column++)
		{
			PaintTile(row, column);
		}
	}
}

void SetupBoard();

// Mouse.
bool g_bDragging = false;

int g_MouseX = -1, g_MouseY = -1;

void ChessStartDrag(int x, int y)
{
	// start dragging the piece
	
	// Determine its coordinates on the board.
	int boardRow = -1, boardCol = -1;
	GetBoardCoords(x, y, &boardRow, &boardCol);
	
	BoardPiece* pPc = GetPiece(boardRow, boardCol);
	if (!pPc) return;
	
	if (pPc->piece == PIECE_NONE)
		return;
	
	g_bDragging = true;
	g_DraggedPiece = *pPc;
	g_DraggedPieceRow = boardRow;
	g_DraggedPieceCol = boardCol;
	
	PaintTile(boardRow, boardCol);
	
	ChangeCursor(g_pWindow, GetPieceCursorID(g_DraggedPiece.piece, g_DraggedPiece.color));
}

void ChessClickCursor(int x, int y)
{
	if (g_bDragging) return;
	
	//if (g_MouseX != -1)
	{
		ChessStartDrag(x, y);
	}
	
	g_MouseX = x, g_MouseY = y;
}

void UpdatePlayerTurn();

const char* const g_playerNames[] =
{
	"Black",
	"White",
};

const char* GetPlayerName(eColor player)
{
	return g_playerNames[player];
}

void ChessReleaseCursor(int x, int y)
{
	if (g_DraggedPieceRow != -1 && g_DraggedPieceCol != -1)
	{
		int boardRow = -1, boardCol = -1;
		GetBoardCoords(x, y, &boardRow, &boardCol);
		
		BoardPiece* pPc = GetPiece(boardRow, boardCol);
		if (pPc)
		{
			eColor col = pPc->color;
			eErrorCode err = ChessCommitMove(g_DraggedPieceRow, g_DraggedPieceCol, boardRow, boardCol);
			
			char buffer[2048], buffer2[100];
			
			if (err != ERROR_SUCCESS) switch (err)
			{
				case ERROR_STALEMATE:
				{
					if (MessageBox(
						g_pWindow,
						"Stalemate!\n\nThis game is a draw.\n\nWould you like to reset the board, and play another game?",
						"Game Draw",
						MB_YESNO | ICON_SWEEP_SMILE << 16 // TODO ICON_SCALE << 16
					) == MBID_YES)
					{
						//TODO
					}
					break;
				}
				case ERROR_CHECKMATE:
				{
					snprintf(buffer,  sizeof buffer,  "Checkmate!\n\n%s Wins.\n\nWould you like to reset the board, and play another game?", GetPlayerName(col));
					snprintf(buffer2, sizeof buffer2, "%s Wins", GetPlayerName(col));
					
					if (MessageBox(
						g_pWindow,
						buffer,
						buffer2,
						MB_YESNO | ICON_SWEEP_CARET << 16 // TODO ICON_TROPHY << 16
					) == MBID_YES)
					{
						//TODO
					}
					
					break;
				}
				default:
					LogMsg("Cannot commit move: %d", err);
			}
		}
	}
	
	UpdatePlayerTurn();
	
	g_MouseX = g_MouseY = -1;
	g_bDragging = false;
	int odpr = g_DraggedPieceRow, odpc = g_DraggedPieceCol;
	g_DraggedPieceRow = -1;
	g_DraggedPieceCol = -1;
	
	if (odpc != -1 && odpr != -1)
		PaintTile(odpr, odpc);
	
	ChangeCursor(g_pWindow, CURSOR_DEFAULT);
}

void UpdatePlayerTurn()
{
	char buffer[100];
	sprintf(buffer, "It's %s's turn", GetPlayerName(g_playingPlayer));
	SetLabelText(g_pWindow, CHESS_TURN_LABEL, buffer);
	CallControlCallback(g_pWindow, CHESS_TURN_LABEL, EVENT_PAINT, 0, 0);
}

void UpdateFlashingTiles()
{
	for (int row = 0; row < BOARD_SIZE; row++)
	{
		for (int col = 0; col < BOARD_SIZE; col++)
		{
			int oldFlashingTiles = g_FlashingTiles[row][col];
			if (g_FlashingTiles[row][col] > 0)
				g_FlashingTiles[row][col]--;
			
			if (oldFlashingTiles % 2 != g_FlashingTiles[row][col] % 2)
				PaintTile(row, col);
		}
	}
}

int g_FlashTimerID = -1;

ePiece PromotionPopup(eColor color, int row, int col);

void CALLBACK ChessWndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_USER:
		{
			UpdateFlashingTiles();
			break;
		}
		case EVENT_CREATE:
		{
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			// Load the cursors.
			for (int i = PIECE_PAWN; i < PIECE_MAX; i++)
			{
				SetPieceCursorID((ePiece)i, BLACK, UploadCursor(GetPieceImage((ePiece)i, BLACK), PIECE_SIZE / 2, PIECE_SIZE / 2));
				SetPieceCursorID((ePiece)i, WHITE, UploadCursor(GetPieceImage((ePiece)i, WHITE), PIECE_SIZE / 2, PIECE_SIZE / 2));
			}
			
			// Create some controls letting the players know about their status in the game.
			
			g_BoardY = TOP_BAR_HEIGHT + 10;
			g_BoardX = 10;
			SetupBoard();
			
			Rectangle rect;
			RECT(rect, PIECE_SIZE * BOARD_SIZE + 10, 10, CHESS_WIDTH - PIECE_SIZE*BOARD_SIZE-10, 20);
			
			AddControl(pWindow, CONTROL_TEXTCENTER, rect, "", CHESS_TURN_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL | TEXTSTYLE_VCENTERED | TEXTSTYLE_HCENTERED);
			UpdatePlayerTurn();
			
			g_FlashTimerID = AddTimer(pWindow, 250, EVENT_USER);
			
			break;
		}
		
		case EVENT_PAINT:
		{
			DrawFrameAroundBoard();
			PaintBoard();
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			ChessClickCursor(GET_X_PARM(parm1), GET_Y_PARM(parm1));
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			ChessReleaseCursor(GET_X_PARM(parm1), GET_Y_PARM(parm1));
			break;
		}
		case EVENT_COMMAND:
		{
			break;
		}
		case EVENT_CLOSE:
		{
			// TODO: Ask the user if they would like to resign
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
		}
		default:
		{
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
		}
	}
}

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	g_pWindow = CreateWindow ("Chess", CW_AUTOPOSITION, CW_AUTOPOSITION, CHESS_WIDTH, CHESS_HEIGHT, ChessWndProc, 0);
	
	if (!g_pWindow)
		return 1;
	
	while (HandleMessages (g_pWindow));
	
	return 0;
}
