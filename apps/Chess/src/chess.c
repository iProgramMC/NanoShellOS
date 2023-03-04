/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

             Chess source file
******************************************/
#include "chess.h"

typedef enum
{
	CASTLE_NONE,
	CASTLE_KINGSIDE,
	CASTLE_QUEENSIDE,
	CASTLE_TYPES,
}
eCastleType;

bool g_bCastleAllowed[CASTLE_TYPES][NPLAYERS]; // king side = 0, queen side = 1

BoardPiece g_Captures[NPLAYERS][BOARD_SIZE * BOARD_SIZE];
int g_nCaptures[NPLAYERS];

bool g_bIsKingInCheck[NPLAYERS];

BoardPos g_KingPositions[NPLAYERS];

extern eColor g_playingPlayer;

BoardPiece g_pieces[BOARD_SIZE][BOARD_SIZE];

void AddCapture(eColor player, BoardPiece piece)
{
	g_Captures[player][g_nCaptures[player]++] = piece;
}

BoardPiece* GetPiece(int row, int column)
{
	if (row < 0 || column < 0 || row >= BOARD_SIZE || column >= BOARD_SIZE) return NULL;
	return &g_pieces[row][column];
}

void SetPiece(int row, int column, ePiece pc, eColor col)
{
	BoardPiece* piece = GetPiece(row, column);
	piece->piece = pc;
	piece->color = col;
	
	if (pc == PIECE_KING)
	{
		BoardPos pos = { row, column };
		g_KingPositions[col] = pos;
	}
	
	PaintTile(row, column);
}

BoardPiece g_emptyPiece;

// Returns a piece with an optional ignorable piece, and never returns NULL.
BoardPiece* GetPieceIgnore(int row, int col, int rowIgn, int colIgn)
{
	if (row == rowIgn && col == colIgn) return &g_emptyPiece;
	
	if (row < 0 || col < 0 || row >= BOARD_SIZE || col >= BOARD_SIZE) return &g_emptyPiece;
	
	return &g_pieces[row][col];
}

int GetHomeRow(eColor col)
{
	return (col == WHITE) ? 0 : (BOARD_SIZE - 1);
}

bool ChessIsBeingThreatened(int row, int col, eColor color, int rowIgn, int colIgn, bool bFlashTiles);

bool ChessCheckLegalBasedOnRank(int rowSrc, int colSrc, int rowDst, int colDst, eCastleType* pCastleTypeOut)
{
	*pCastleTypeOut = CASTLE_NONE;
	
	BoardPiece* pcSrc = GetPiece(rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(rowDst, colDst);
	
	int homeRow = GetHomeRow(pcSrc->color);
	
	switch (pcSrc->piece)
	{
		// A pawn may only move forward.
		case PIECE_PAWN:
		{
			// Check if we're moving in the correct direction.
			if (pcSrc->color == WHITE)
			{
				if (rowDst - 1 != rowSrc)
				{
					if (rowSrc != 1 || rowDst - 2 != rowSrc)
						return false;
				}
			}
			else
			{
				if (rowDst + 1 != rowSrc)
				{
					if (rowSrc != 6 || rowDst + 2 != rowSrc)
						return false;
				}
			}
			
			// We're trying to move forward. Check if there's anybody ahead.
			if (colDst == colSrc)
			{
				return (pcDst->piece == PIECE_NONE);
			}
			// We're trying to capture something to the left or right, check if it's a valid capture
			else if (abs(colDst - colSrc) == 1)
			{
				// We can't capture
				if (pcDst->piece == PIECE_KING || pcDst->piece == PIECE_NONE)
					return false;
				
				if (pcDst->color == pcSrc->color)
					return false;
				
				return true;
			}
			else return false;
			break;
		}
		// A knight must move in an L shape.
		case PIECE_KNIGHT:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			
			return (absColDiff == 2 && absRowDiff == 1) || (absColDiff == 1 && absRowDiff == 2);
		}
		// A bishop must move on a diagonal.
		case PIECE_BISHOP:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			return absRowDiff == absColDiff;
		}
		// A rook must move on a row or column.
		case PIECE_ROOK:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			return absRowDiff == 0 || absColDiff == 0;
		}
		// A queen may move in diagonals, rows or columns.
		case PIECE_QUEEN:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			return absRowDiff == absColDiff || absRowDiff == 0 || absColDiff == 0;
		}
		// A king may move in any of the 8 neighboring tiles, so long as his distance is no less than 1.
		case PIECE_KING:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			if (absRowDiff > 1 || absColDiff > 1)
			{
				// are we trying to move on a row?
				if (absRowDiff != 0) return false;
				
				// are we in the home row?
				if (rowDst != homeRow) return false;
				
				eCastleType type = CASTLE_NONE; // undefined
				
				if (colSrc > colDst) // castle queenside
					type = CASTLE_QUEENSIDE;
				else
					type = CASTLE_KINGSIDE;
				
				if (type == CASTLE_NONE) return false;
				
				*pCastleTypeOut = type;
				
				if (!g_bCastleAllowed[type][pcSrc->color]) return false;
				
				if (type == CASTLE_QUEENSIDE)
				{
					BoardPiece* pPiece = GetPiece(homeRow, 0);
					if (pPiece->piece != PIECE_ROOK || pPiece->color != pcSrc->color) // we don't have a rook to castle with
						return false;
					
					for (int i = 1; i < BOARD_SIZE / 2; i++)
					{
						pPiece = GetPieceIgnore(homeRow, i, -1, -1);
						if (pPiece->piece != PIECE_NONE) return false; // this square is blocked
						
						// check if this position would put us in check
						if (i >= 2)
						{
							if (ChessIsBeingThreatened(homeRow, i, pcSrc->color, -1, -1, false))
								return false;
						}
					}
					
					// okay, we should be able to castle.
					return true;
				}
				else
				{
					BoardPiece* pPiece = GetPiece(homeRow, BOARD_SIZE - 1);
					if (pPiece->piece != PIECE_ROOK || pPiece->color != pcSrc->color) // we don't have a rook to castle with
						return false;
					
					for (int i = BOARD_SIZE / 2 + 1; i < BOARD_SIZE - 1; i++)
					{
						pPiece = GetPieceIgnore(homeRow, i, -1, -1);
						if (pPiece->piece != PIECE_NONE) return false; // this square is blocked
						
						// check if this position would put us in check
						if (i < BOARD_SIZE / 2 + 2)
						{
							if (ChessIsBeingThreatened(homeRow, i, pcSrc->color, -1, -1, false))
								return false;
						}
					}
					
					// okay, we should be able to castle.
					return true;
				}
				
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	
	return false;
}

// row, col - The position of a king
bool ChessIsBeingThreatened(int row, int col, eColor color, int rowIgn, int colIgn, bool bFlashTiles)
{
	bool bFinalResult = false;
	
	int rows[8], cols[8];
	rows[0] = row + 1, cols[0] = col - 1;
	rows[1] = row + 1, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col - 1;
	rows[3] = row - 1, cols[3] = col + 1;
	
	// Check for attacking pawns
	BoardPiece* pcs[8];
	for (int i = 0; i < 4; i++)
		pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
	
	// king is white, so check for black pawns above
	if (color == WHITE)
	{
		if (pcs[0]->piece == PIECE_PAWN && pcs[0]->color == BLACK)
		{
			if (bFlashTiles)
				FlashTile(rows[0], cols[0]);
			bFinalResult = true;
		}
		if (pcs[1]->piece == PIECE_PAWN && pcs[1]->color == BLACK)
		{
			if (bFlashTiles)
				FlashTile(rows[1], cols[1]);
			bFinalResult = true;
		}
	}
	// king is black, so check for white pawns above
	else if (color == BLACK)
	{
		if (pcs[2]->piece == PIECE_PAWN && pcs[2]->color == WHITE)
		{
			if (bFlashTiles)
				FlashTile(rows[2], cols[2]);
			bFinalResult = true;
		}
		if (pcs[3]->piece == PIECE_PAWN && pcs[3]->color == WHITE)
		{
			if (bFlashTiles)
				FlashTile(rows[3], cols[3]);
			bFinalResult = true;
		}
	}
	
	// Check for attacking bishops or queen diagonals
	rows[0] = row + 1, cols[0] = col - 1;
	rows[1] = row + 1, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col - 1;
	rows[3] = row - 1, cols[3] = col + 1;
	
	const static int deltaBishopRow[] = {  1,  1, -1, -1 };
	const static int deltaBishopCol[] = { -1, +1, -1,  1 };
	
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int i = 0; i < 4; i++)
		{
			pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
			
			// this is a different-color bishop or queen. In check
			if ((pcs[i]->piece == PIECE_BISHOP || pcs[i]->piece == PIECE_QUEEN) && pcs[i]->color != color)
			{
				if (bFlashTiles)
					FlashTile(rows[i], cols[i]);
				bFinalResult = true;
			}
			
			// we hit a piece, so stop "shooting a ray" here.
			if (pcs[i]->piece != PIECE_NONE)
				rows[i] = cols[i] = -1;
			
			if (rows[i] != -1 || cols[i] != -1)
			{
				rows[i] += deltaBishopRow[i];
				cols[i] += deltaBishopCol[i];
			}
		}
	}
	
	// Check for attacking rooks or queen axes
	rows[0] = row, cols[0] = col - 1;
	rows[1] = row, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col;
	rows[3] = row + 1, cols[3] = col;
	
	const static int deltaRookRow[] = {  0,  0, -1, +1 };
	const static int deltaRookCol[] = { -1, +1,  0,  0 };
	
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int i = 0; i < 4; i++)
		{
			pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
			
			// this is a different-color rook or queen. In check
			if ((pcs[i]->piece == PIECE_ROOK || pcs[i]->piece == PIECE_QUEEN) && pcs[i]->color != color)
			{
				if (bFlashTiles)
					FlashTile(rows[i], cols[i]);
				bFinalResult = true;
			}
			
			// we hit a piece, so stop "shooting a ray" here.
			if (pcs[i]->piece != PIECE_NONE)
				rows[i] = cols[i] = -1;
			
			if (rows[i] != -1 || cols[i] != -1)
			{
				rows[i] += deltaRookRow[i];
				cols[i] += deltaRookCol[i];
			}
		}
	}
	
	const static int deltaKingRow[] = {  0,  0, -1, +1,  1,  1, -1, -1 };
	const static int deltaKingCol[] = { -1, +1,  0,  0, -1, +1, -1,  1 };
	for (int i = 0; i < 8; i++)
	{
		rows[i] = row + deltaKingRow[i];
		cols[i] = col + deltaKingCol[i];
		
		pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
		
		// this is a different-color rook or queen. You can't move two kings right next to each other.
		if (pcs[i]->piece == PIECE_KING && pcs[i]->color != color)
		{
			if (bFlashTiles)
				FlashTile(rows[i], cols[i]);
			bFinalResult = true;
		}
	}
	
	// Check for attacking knights.
	rows[0] = row - 1, cols[0] = col + 2;
	rows[1] = row + 1, cols[1] = col + 2;
	rows[2] = row - 2, cols[2] = col + 1;
	rows[3] = row + 2, cols[3] = col + 1;
	rows[4] = row - 1, cols[4] = col - 2;
	rows[5] = row + 1, cols[5] = col - 2;
	rows[6] = row - 2, cols[6] = col - 1;
	rows[7] = row + 2, cols[7] = col - 1;
	for (int i = 0; i < 8; i++)
	{
		pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
		
		if (pcs[i]->piece == PIECE_KNIGHT && pcs[i]->color != color)
		{
			if (bFlashTiles)
				FlashTile(rows[i], cols[i]);
			bFinalResult = true;
		}
	}
	
	// no checks found. We're good
	return bFinalResult;
}

eErrorCode ChessCheckMove(int rowSrc, int colSrc, int rowDst, int colDst, eCastleType* castleType)
{
	BoardPiece* pcSrc = GetPiece(rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(rowDst, colDst);
	
	// Check if it even makes sense to move here
	if (pcDst->piece == PIECE_KING)
		return ERROR_CANNOT_CAPTURE_KING;
	
	if (pcDst->piece != PIECE_NONE && pcDst->color == pcSrc->color)
		return ERROR_CANNOT_MOVE_ON_SAME_COLOR;
	
	if (pcSrc->color != g_playingPlayer)
		return ERROR_NOT_YOUR_TURN;
	
	// Check if the move is valid, based on the rank.
	if (!ChessCheckLegalBasedOnRank(rowSrc, colSrc, rowDst, colDst, castleType))
		return ERROR_MOVE_ILLEGAL;
	
	// Check if the move would put us in check.
	// There are at least two situations where a move can put the king in check:
	// 1. The king moves directly into check
	// 2. A piece reveals the king to an enemy piece, checking it
	{
		// Do the swap very temporarily
		BoardPiece pcSrcT = *pcSrc;
		BoardPiece pcDstT = *pcDst;
		
		eColor colr  = pcSrc->color;
		ePiece piece = pcSrc->piece;
		
		*pcDst = pcSrcT;
		SetPiece(rowSrc, colSrc, PIECE_NONE, BLACK);
		
		BoardPos pos = g_KingPositions[colr];
		// If we're moving the king, see if _THAT_ position puts us in check
		if (piece == PIECE_KING)
		{
			BoardPos pos2 = { rowDst, colDst };
			pos = pos2;
		}
		bool wouldPutUsInCheck = ChessIsBeingThreatened(pos.row, pos.col, colr, -1, -1, piece == PIECE_KING);
		
		if (piece != PIECE_KING && wouldPutUsInCheck)
		{
			// flash the king, since it's in check
			FlashTile(pos.row, pos.col);
		}
		
		*pcSrc = pcSrcT;
		*pcDst = pcDstT;
		
		if (wouldPutUsInCheck)
			return ERROR_MOVE_WOULD_PUT_US_IN_CHECK;
	}
	
	return ERROR_SUCCESS;
}

void ChessPerformMoveUnchecked(int rowSrc, int colSrc, int rowDst, int colDst, eCastleType castleType)
{
	BoardPiece* pcSrc = GetPiece(rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(rowDst, colDst);
	
	eColor col = pcSrc->color;
	
	int kingPos = BOARD_SIZE / 2;
	
	switch (castleType)
	{
		// we checked for any checks/captures during castling. There may not be any pieces on the castled over tiles.
		// Additionally, the king may not castle into check, or through check.
		case CASTLE_KINGSIDE:
			
			// set the square right next to the king to the rook.
			SetPiece(GetHomeRow(col), kingPos + 1, PIECE_ROOK, col);
			// and then, the king
			SetPiece(GetHomeRow(col), kingPos + 2, PIECE_KING, col);
			
			// clear the rook's square
			SetPiece(GetHomeRow(col), BOARD_SIZE - 1, PIECE_NONE, BLACK);
			// clear the king's square
			SetPiece(GetHomeRow(col), kingPos, PIECE_NONE, BLACK);
			
			// no more castling!!
			g_bCastleAllowed[CASTLE_KINGSIDE ][col] = false;
			g_bCastleAllowed[CASTLE_QUEENSIDE][col] = false;
			
			break;
			
		case CASTLE_QUEENSIDE:
			
			// set the square right next to the king to the rook.
			SetPiece(GetHomeRow(col), kingPos - 1, PIECE_ROOK, col);
			// and then, the king
			SetPiece(GetHomeRow(col), kingPos - 2, PIECE_KING, col);
			
			// clear the rook's square
			SetPiece(GetHomeRow(col), 0, PIECE_NONE, BLACK);
			// clear the king's square
			SetPiece(GetHomeRow(col), kingPos, PIECE_NONE, BLACK);
			
			// no more castling!!
			g_bCastleAllowed[CASTLE_KINGSIDE ][col] = false;
			g_bCastleAllowed[CASTLE_QUEENSIDE][col] = false;
			
			break;
		
		default: // No castling
			
			// if the king was moved:
			if (pcSrc->piece == PIECE_KING)
			{
				g_bCastleAllowed[CASTLE_KINGSIDE ][col] = false;
				g_bCastleAllowed[CASTLE_QUEENSIDE][col] = false;
			}
			
			// if the piece moved was a rook
			if (pcSrc->piece == PIECE_ROOK && rowSrc == GetHomeRow(col))
			{
				// if it was the queenside rook
				if (colSrc == 0)
					g_bCastleAllowed[CASTLE_QUEENSIDE][col] = false;
				else if (colDst == BOARD_SIZE - 1)
					g_bCastleAllowed[CASTLE_KINGSIDE][col] = false;
			}
			
			// Capture whatever was here before.
			if (pcDst->piece != PIECE_NONE)
				AddCapture(col, *pcDst);
			
			SetPiece(rowDst, colDst, pcSrc->piece, col);
			SetPiece(rowSrc, colSrc, PIECE_NONE, BLACK);
			
			break;
	}
}

eErrorCode ChessCommitMove(int rowSrc, int colSrc, int rowDst, int colDst)
{
	BoardPiece* pcSrc = GetPiece(rowSrc, colSrc);
	
	if (rowSrc == rowDst && colSrc == colDst)
		// don't treat it as a failure if they didn't move
		return ERROR_SUCCESS;
	
	eCastleType castleType = CASTLE_NONE;
	
	eErrorCode errCode = ChessCheckMove(rowSrc, colSrc, rowDst, colDst, &castleType);
	if (errCode != ERROR_SUCCESS)
		return errCode;
	
	g_bIsKingInCheck[pcSrc->color] = false;
	
	ChessPerformMoveUnchecked(rowSrc, colSrc, rowDst, colDst, castleType);
	
	// Check if any other players are in check, and update their state.
	for (int i = FIRST_PLAYER; i < NPLAYERS; i++)
	{
		BoardPos pos = g_KingPositions[i];
		g_bIsKingInCheck[i] = ChessIsBeingThreatened(pos.row, pos.col, i, -1, -1, false);
		
		if (g_bIsKingInCheck[i])
		{
			LogMsg("%d's in check!", i);
		}
	}
	
	g_playingPlayer = GetNextPlayer(g_playingPlayer);
	
	
	
	return ERROR_SUCCESS;
}

void SetupBoard()
{
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		// set a2..g2 and a7..g7 to pawns. a2..g2 has the color white, a7..g7 has the color black.
		SetPiece(1, i, PIECE_PAWN, WHITE);
		SetPiece(6, i, PIECE_PAWN, BLACK);
	}
	
	// Rooks
	SetPiece(0, 0, PIECE_ROOK, WHITE);
	SetPiece(0, 7, PIECE_ROOK, WHITE);
	SetPiece(7, 0, PIECE_ROOK, BLACK);
	SetPiece(7, 7, PIECE_ROOK, BLACK);
	
	// Knights
	SetPiece(0, 1, PIECE_KNIGHT, WHITE);
	SetPiece(0, 6, PIECE_KNIGHT, WHITE);
	SetPiece(7, 1, PIECE_KNIGHT, BLACK);
	SetPiece(7, 6, PIECE_KNIGHT, BLACK);
	
	// Bishops
	SetPiece(0, 2, PIECE_BISHOP, WHITE);
	SetPiece(0, 5, PIECE_BISHOP, WHITE);
	SetPiece(7, 2, PIECE_BISHOP, BLACK);
	SetPiece(7, 5, PIECE_BISHOP, BLACK);
	
	// Queens
	SetPiece(0, 3, PIECE_QUEEN, WHITE);
	SetPiece(7, 3, PIECE_QUEEN, BLACK);
	
	// Kings
	SetPiece(0, 4, PIECE_KING, WHITE);
	SetPiece(7, 4, PIECE_KING, BLACK);
	
	for (int i = 0; i < NPLAYERS; i++)
	{
		g_bCastleAllowed[CASTLE_KINGSIDE] [i] = true;
		g_bCastleAllowed[CASTLE_QUEENSIDE][i] = true;
	}
}
