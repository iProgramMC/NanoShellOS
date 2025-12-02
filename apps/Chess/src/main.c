/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

             Main source file
******************************************/
#include "chess.h"

#define CHESS_WIDTH  (PIECE_SIZE * BOARD_SIZE + SIDE_BAR_WIDTH + 20)
#define CHESS_HEIGHT (PIECE_SIZE * BOARD_SIZE + TOP_BAR_HEIGHT + 20)

#define ICON_RESIGN  (902)
#define ICON_SCALE16 (903)
#define ICON_QUES    (904)
#define ICON_TROPHY  (900)
#define ICON_SCALE   (901)
#define BMP_PIECES   (1000)

Window* g_pWindow;
int g_BoardX = 0, g_BoardY = TOP_BAR_HEIGHT;

bool g_bGameOver = false;

extern BoardState* g_CurrentState;

// note: The row number is flipped, so 1 is the bottom-most row.

Image* g_pPieceImages[14];
Image* g_pPieceSheet;

enum
{
	CHESS_TURN_LABEL = 1000,
	CHESS_MOVE_LIST,
	CHESS_CAPTURES_BLACK,
	CHESS_CAPTURES_WHITE,
	CHESS_RESIGN_BLACK,
	CHESS_RESIGN_WHITE,
	CHESS_DRAW_BLACK,
	CHESS_DRAW_WHITE,
};

void DrawRectangleFrame(Rectangle rect)
{
	rect.left   -= 2;
	rect.top    -= 2;
	rect.right  += 2;
	rect.bottom += 2;
	
	DrawEdge(rect, DRE_SUNKEN, TRANSPARENT);
}

void DrawFrameAroundBoard()
{
	Rectangle rect = { g_BoardX, g_BoardY, g_BoardX + BOARD_SIZE * PIECE_SIZE, g_BoardY + BOARD_SIZE * PIECE_SIZE };
	DrawRectangleFrame(rect);
}

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

void BuildPieceImages()
{
	int pieceSize = g_pPieceSheet->width;
	
	for (int pieceID = PIECE_NONE + 1; pieceID < PIECE_MAX; pieceID++)
	{
		for (int color = FIRST_PLAYER; color < NPLAYERS; color++)
		{
			int yPos = pieceSize * (pieceID - 1 + 6 * (1 - color));
			int placeInMem = pieceID * 2 + color;
			
			Image* img = calloc(1, sizeof(Image));
			img->width = img->height = pieceSize;
			img->framebuffer = &g_pPieceSheet->framebuffer[yPos * img->width];
			
			g_pPieceImages[placeInMem] = img;
		}
	}
}

void RemovePieceImages()
{
	for (int i = 0; i < (int)ARRAY_COUNT(g_pPieceImages); i++)
	{
		if (g_pPieceImages[i])
		{
			free(g_pPieceImages[i]);
			g_pPieceImages[i] = NULL;
		}
	}
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
	
	BoardPiece* pPiece = GetPiece(g_CurrentState, row, column);
	
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
		VidBlitImageResize(pImg, x + offsetX, y + offsetY, PIECE_SIZE, PIECE_SIZE);
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

void SetupBoard(BoardState* pState);

// Mouse.
bool g_bDragging = false;

int g_MouseX = -1, g_MouseY = -1;

void ChessStartDrag(int x, int y)
{
	// start dragging the piece
	
	// Determine its coordinates on the board.
	int boardRow = -1, boardCol = -1;
	GetBoardCoords(x, y, &boardRow, &boardCol);
	
	BoardPiece* pPc = GetPiece(g_CurrentState, boardRow, boardCol);
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
	if (g_bGameOver) return;
	
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

void SetGameOver(bool bGameOver)
{
	g_bGameOver = bGameOver;
}

void ClearFlashingTiles()
{
	for (int row = 0; row < BOARD_SIZE; row++)
	{
		for (int col = 0; col < BOARD_SIZE; col++)
		{
			g_FlashingTiles[row][col] = 0;
			PaintTile(row, col);
		}
	}
}

void ChessOnGameEnd(eErrorCode errCode, eColor col)
{
	char buffer[2048], buffer2[100];
	SetGameOver(true);
	UpdatePlayerTurn();
	
	switch (errCode)
	{
		case ERROR_RESIGNATION_BLACK:
		case ERROR_RESIGNATION_WHITE:
		{
			eColor won = errCode == ERROR_RESIGNATION_BLACK ? WHITE : BLACK;
			
			snprintf(buffer,  sizeof buffer,  "%s wins by resignation.\n\nWould you like to reset the board, and play another game?", GetPlayerName(won));
			snprintf(buffer2, sizeof buffer2, "%s wins", GetPlayerName(won));
			
			if (MessageBox(
				g_pWindow,
				buffer,
				buffer2,
				MB_YESNO | ICON_TROPHY << 16
			) == MBID_YES)
			{
				//SetupBoard(g_CurrentState);
				ResetGame();
				RegisterEvent(g_pWindow, EVENT_PAINT, 0, 0);
			}
			
			break;
		}
		case ERROR_STALEMATE:
		{
			if (MessageBox(
				g_pWindow,
				"Stalemate!\n\nThis game is a draw.\n\nWould you like to reset the board, and play another game?",
				"Game Draw",
				MB_YESNO | ICON_SCALE << 16
			) == MBID_YES)
			{
				ResetGame();
				RegisterEvent(g_pWindow, EVENT_PAINT, 0, 0);
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
				MB_YESNO | ICON_TROPHY << 16
			) == MBID_YES)
			{
				ResetGame();
				RegisterEvent(g_pWindow, EVENT_PAINT, 0, 0);
			}
			
			break;
		}
	}
}

int g_nMoveNumber = 0;

void ChessClearGUI()
{
	g_nMoveNumber = 0;
	ResetList(g_pWindow, CHESS_MOVE_LIST);
	ResetList(g_pWindow, CHESS_CAPTURES_BLACK);
	ResetList(g_pWindow, CHESS_CAPTURES_WHITE);
}

void ChessReleaseCursor(int x, int y)
{
	if (g_bGameOver)
		return;
	
	eErrorCode err = ERROR_SUCCESS;
	
	//bool bPromotePawn = false;
	//int nPawnPromotedFile = 0;
	eColor col = BLACK;
	
	if (g_DraggedPieceRow != -1 && g_DraggedPieceCol != -1)
	{
		int boardRow = -1, boardCol = -1;
		GetBoardCoords(x, y, &boardRow, &boardCol);
		
		BoardPiece* pPc = GetPiece(g_CurrentState, g_DraggedPieceRow, g_DraggedPieceCol);
		if (pPc)
		{
			//nPawnPromotedFile = boardCol;
			//bPromotePawn = true;
			
			col = pPc->color;
			err = ChessCommitMove(g_CurrentState, g_DraggedPieceRow, g_DraggedPieceCol, boardRow, boardCol);
		}
	}
	
	g_MouseX = g_MouseY = -1;
	g_bDragging = false;
	int odpr = g_DraggedPieceRow, odpc = g_DraggedPieceCol;
	g_DraggedPieceRow = -1;
	g_DraggedPieceCol = -1;
	
	if (odpc != -1 && odpr != -1)
		PaintTile(odpr, odpc);
	
	ChangeCursor(g_pWindow, CURSOR_DEFAULT);
	
	if (err != ERROR_SUCCESS) switch (err)
	{
		case ERROR_STALEMATE:
		case ERROR_CHECKMATE:
			ChessOnGameEnd(err, col);
			break;
		default:
			LogMsg("Cannot commit move: %d", err);
	}
	
	UpdatePlayerTurn();
}

void UpdatePlayerTurn()
{
	char buffer[100];
	
	if (g_bGameOver)
		strcpy(buffer, "Game Over");
	else
		sprintf(buffer, "It's %s's turn", GetPlayerName(g_CurrentState->m_Player));
	
	VidSetClipRect(NULL);
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

void ChessUpdateMoveList()
{
	CallControlCallback(g_pWindow, CHESS_MOVE_LIST, EVENT_UPDATE_MOVE_LIST, 0, 0);
}

void AddMoveList(Window* pWindow, Rectangle rect, int comboID);

void CALLBACK ChessWndProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_UPDATE_FLASHING:
		{
			UpdateFlashingTiles();
			break;
		}
		case EVENT_CREATE:
		{
			BuildPieceImages();
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			// Load the cursors.
			for (int i = PIECE_NONE + 1; i < PIECE_MAX; i++)
			{
				SetPieceCursorID((ePiece)i, BLACK, UploadCursor(GetPieceImage((ePiece)i, BLACK), PIECE_SIZE / 2, PIECE_SIZE / 2));
				SetPieceCursorID((ePiece)i, WHITE, UploadCursor(GetPieceImage((ePiece)i, WHITE), PIECE_SIZE / 2, PIECE_SIZE / 2));
			}
			
			// Create some controls letting the players know about their status in the game.
			
			g_BoardY = (CHESS_HEIGHT - BOARD_SIZE * PIECE_SIZE) / 2;
			g_BoardX = LEFT_BAR_WIDTH;
			ResetGame();
			
			Rectangle rect;
			RECT(rect, g_BoardX, g_BoardY - 15, BOARD_SIZE * PIECE_SIZE, 10);
			
			AddControl(pWindow, CONTROL_TEXTCENTER, rect, "", CHESS_TURN_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_FORCEBGCOL | TEXTSTYLE_VCENTERED | TEXTSTYLE_HCENTERED);
			UpdatePlayerTurn();
			
			RECT(rect, g_BoardX + BOARD_SIZE * PIECE_SIZE + 10, g_BoardY, RIGHT_BAR_WIDTH, BOARD_SIZE * PIECE_SIZE);
			//AddControl(pWindow, CONTROL_LISTVIEW, rect, NULL, CHESS_MOVE_LIST, 0, 0);
			AddMoveList(pWindow, rect, CHESS_MOVE_LIST);
			
			RECT(rect, 10, g_BoardY, 40, (BOARD_SIZE * PIECE_SIZE / 2) - 5);
			AddControl(pWindow, CONTROL_LISTVIEW, rect, NULL, CHESS_CAPTURES_BLACK, 0, 0);
			RECT(rect, 10, g_BoardY + (BOARD_SIZE * PIECE_SIZE / 2) + 5, 40, (BOARD_SIZE * PIECE_SIZE / 2) - 5);
			AddControl(pWindow, CONTROL_LISTVIEW, rect, NULL, CHESS_CAPTURES_BLACK, 0, 0);
			
			int btnWidth = RIGHT_BAR_WIDTH;
			RECT(rect, CHESS_WIDTH - btnWidth - 5, 10, btnWidth - 5, 20);
			//AddControl(pWindow, CONTROL_BUTTON, rect, "Resign", CHESS_RESIGN_BLACK, ICON_RESIGN, 0);
			AddControl(pWindow, CONTROL_BUTTON, rect, "Find Best Move", CHESS_RESIGN_BLACK, ICON_RESIGN, 0);
			RECT(rect, CHESS_WIDTH - btnWidth * 2 - 10, 10, btnWidth - 5, 20);
			AddControl(pWindow, CONTROL_BUTTON, rect, "Draw",   CHESS_DRAW_BLACK,   ICON_SCALE16, 0);
			SetControlDisabled(pWindow, CHESS_DRAW_BLACK, true);
			
			RECT(rect, CHESS_WIDTH - btnWidth - 5, CHESS_HEIGHT - 30, btnWidth - 5, 20);
			//AddControl(pWindow, CONTROL_BUTTON, rect, "Resign", CHESS_RESIGN_WHITE, ICON_RESIGN, 0);
			AddControl(pWindow, CONTROL_BUTTON, rect, "Find Best Move", CHESS_RESIGN_WHITE, ICON_RESIGN, 0);
			RECT(rect, CHESS_WIDTH - btnWidth * 2 - 10, CHESS_HEIGHT - 30, btnWidth - 5, 20);
			AddControl(pWindow, CONTROL_BUTTON, rect, "Draw",   CHESS_DRAW_WHITE,   ICON_SCALE16, 0);
			SetControlDisabled(pWindow, CHESS_DRAW_WHITE, true);
			
			g_FlashTimerID = AddTimer(pWindow, 250, EVENT_UPDATE_FLASHING);
			
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
			switch (parm1)
			{
				case CHESS_RESIGN_WHITE:
				{
					if (g_CurrentState->m_Player != WHITE)
					{
						MessageBox(pWindow, "You can't resign. It's not your turn!", "Chess", MB_OK | ICON_WARNING << 16);
						break;
					}
					
					/*
					if (MessageBox(pWindow, "Are you sure you want to resign as white?", "Chess", ICON_QUES << 16 | MB_YESNO) == MBID_YES)
					{
						ChessOnGameEnd(ERROR_RESIGNATION_WHITE, BLACK);
					}
					*/
					PerformBestMove();
					break;
				}
				case CHESS_RESIGN_BLACK:
				{
					if (g_CurrentState->m_Player != BLACK)
					{
						MessageBox(pWindow, "You can't resign. It's not your turn!", "Chess", MB_OK | ICON_WARNING << 16);
						break;
					}
					
					/*
					if (MessageBox(pWindow, "Are you sure you want to resign as black?", "Chess", ICON_QUES << 16 | MB_YESNO) == MBID_YES)
					{
						ChessOnGameEnd(ERROR_RESIGNATION_BLACK, WHITE);
					}
					*/
					PerformBestMove();
					break;
				}
			}
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

void ChessUpdateMoveList();

int main()
{
	for (int i = 2; i < 14; i++)
	{
		//g_pPieceImages[i] = GetImage(GetResource(ICON_PIECES_START + i));
		g_pPieceSheet = GetImage(GetResource(BMP_PIECES));
	}
	
	g_pWindow = CreateWindow ("Chess", CW_AUTOPOSITION, CW_AUTOPOSITION, CHESS_WIDTH, CHESS_HEIGHT, ChessWndProc, 0);
	
	if (!g_pWindow)
		return 1;
	
	while (HandleMessages (g_pWindow));
	
	RemovePieceImages();
	
	return 0;
}

int ChessMessageBox(const char* text, const char* caption, int flags)
{
	return MessageBox(g_pWindow, text, caption, flags);
}
