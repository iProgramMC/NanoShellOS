/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

             Chess source file
******************************************/
#include "chess.h"

BoardState* g_History;
int g_HistorySize;
int g_HistoryCapacity;

BoardState* g_CurrentState;

void InitializeHistory()
{
	g_HistoryCapacity = 16;
	g_HistorySize = 0;
	g_History = calloc (sizeof(BoardState), g_HistoryCapacity);
}

// Adds a new frame to the move history.
// If there's nothing, set up the default state of the board.
// Otherwise, copy the last history frame.
void AddHistoryFrame()
{
	if (g_HistorySize == g_HistoryCapacity)
	{
		// need to expand. Realloc
		g_History = realloc(g_History, sizeof(BoardState) * g_HistoryCapacity * 2);
		g_HistoryCapacity *= 2;
	}
	
	BoardState *pState = &g_History[g_HistorySize++];
	
	if (g_HistorySize > 1)
	{
		*pState = g_History[g_HistorySize - 2];
	}
	else
	{
		memset(pState, 0, sizeof* pState);
		
		// generate a fresh new state.
		SetupBoard(pState);
	}
	
	g_CurrentState = pState;
}

PlayerState* GetCurrPlayerState(eColor col)
{
	return &g_CurrentState->m_PlayerState[col];
}

void AddCapture(BoardState* pState, eColor player, BoardPiece piece)
{
	pState->m_PlayerState[player].m_Captures[pState->m_PlayerState[player].m_nCaptures++] = piece;
}

BoardPiece* GetPiece(BoardState* pState, int row, int column)
{
	if (row < 0 || column < 0 || row >= BOARD_SIZE || column >= BOARD_SIZE) return NULL;
	return &pState->m_Pieces[row][column];
}

void SetPiece(BoardState* pState, int row, int column, ePiece pc, eColor col)
{
	BoardPiece* piece = GetPiece(pState, row, column);
	if (!piece) return;
	
	piece->piece = pc;
	piece->color = col;
	
	if (pc == PIECE_KING)
	{
		BoardPos pos = { row, column };
		pState->m_PlayerState[col].m_KingPos = pos;
	}
	
	if (g_CurrentState == pState)
		PaintTile(row, column);
}

BoardPiece g_emptyPiece;

// Returns a piece with an optional ignorable piece, and never returns NULL.
BoardPiece* GetPieceIgnore(BoardState* pState, int row, int col, int rowIgn, int colIgn)
{
	if (row == rowIgn && col == colIgn) return &g_emptyPiece;
	
	if (row < 0 || col < 0 || row >= BOARD_SIZE || col >= BOARD_SIZE) return &g_emptyPiece;
	
	return &pState->m_Pieces[row][col];
}

int GetHomeRow(eColor col)
{
	return (col == WHITE) ? 0 : (BOARD_SIZE - 1);
}

int Normalize(int x)
{
	if (x == 0) return x;
	return x / abs(x);
}

// note: this 'ray shooting' only works in 8 directions, and is only intended for ChessCheckLegalBasedOnRank.
bool ChessShootRay(BoardState* pState, int rowSrc, int colSrc, int rowDst, int colDst)
{
	int absRowDiff = abs(rowDst - rowSrc);
	int absColDiff = abs(colDst - colSrc);
	
	// we shouldn't be able to get here anyways
	if (absRowDiff == 0 && absColDiff == 0) return false;
	
	int rowDiffUnit = Normalize(rowDst - rowSrc);
	int colDiffUnit = Normalize(colDst - colSrc);
	
	// note: We add rowDiffUnit to skip our source square
	int rowCur = rowSrc + rowDiffUnit;
	int colCur = colSrc + colDiffUnit;
	
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		// if we have reached the piece we're in:
		if (rowCur == rowDst && colCur == colDst) return true;
		
		BoardPiece * pPiece = GetPiece(pState, rowCur, colCur);
		
		//LogMsg("checking piece: %c%d %c%d  %d  %d", colCur+'a', rowCur+1, colSrc+'a', rowSrc+1, rowDiffUnit, colDiffUnit);
		
		// we're OOB. seems like we overshot or something
		if (!pPiece)
		{
			return false;
		}
		
		// we're hitting a piece, that is DIFFERENT from our destination, on our way there. Get outta here
		if (pPiece->piece != PIECE_NONE)
		{
			//LogMsg("diff: %c%d %d", colCur+'a', rowCur+1, 0);
			return false;
		}
		
		rowCur += rowDiffUnit;
		colCur += colDiffUnit;
	}
	
	return false;
}

bool ChessIsBeingThreatened(BoardState* pState, int row, int col, eColor color, int rowIgn, int colIgn, bool bFlashTiles);

bool ChessCheckLegalBasedOnRank(BoardState* pState, int rowSrc, int colSrc, int rowDst, int colDst, eCastleType* pCastleTypeOut, bool* bWouldDoEP)
{
	*pCastleTypeOut = CASTLE_NONE;
	
	BoardPiece* pcSrc = GetPiece(pState, rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(pState, rowDst, colDst);
	
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
				// if there's a single step advance, instead of a double step, then pcDst should equal to pcInFront
				BoardPiece* pcInFront = GetPiece(pState, rowSrc + (pcSrc->color == WHITE ? (1) : (-1)), colSrc);
				
				return (pcDst->piece == PIECE_NONE && pcInFront->piece == PIECE_NONE);
			}
			// We're trying to capture something to the left or right, check if it's a valid capture
			else if (abs(colDst - colSrc) == 1)
			{
				// We can't capture
				if (pcDst->piece == PIECE_NONE)
				{
					for (int i = 0; i < NPLAYERS; i++)
					{
						if (pcSrc->color == (eColor)i) continue;
						if (pState->m_PlayerState[i].m_nEnPassantColumn == colDst)
						{
							// would do en passant
							*bWouldDoEP = true;
							return true;
						}
					}
					
					return false;
				}
				
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
			if (absRowDiff != absColDiff) return false;
			
			// shoot a 'ray' and see if we hit any pieces before this.
			return ChessShootRay(pState, rowSrc, colSrc, rowDst, colDst);
		}
		// A rook must move on a row or column.
		case PIECE_ROOK:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			if (absRowDiff != 0 && absColDiff != 0) return false;
			
			// shoot a 'ray' and see if we hit any pieces before this.
			return ChessShootRay(pState, rowSrc, colSrc, rowDst, colDst);
		}
		// A queen may move in diagonals, rows or columns.
		case PIECE_QUEEN:
		{
			int absRowDiff = abs(rowDst - rowSrc);
			int absColDiff = abs(colDst - colSrc);
			
			if (absRowDiff != absColDiff && absRowDiff != 0 && absColDiff != 0) return false;
			
			// shoot a 'ray' and see if we hit any pieces before this.
			return ChessShootRay(pState, rowSrc, colSrc, rowDst, colDst);
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
				
				if (!pState->m_PlayerState[pcSrc->color].m_bCastleAllowed[type]) return false;
				
				if (type == CASTLE_QUEENSIDE)
				{
					BoardPiece* pPiece = GetPiece(pState, homeRow, 0);
					if (pPiece->piece != PIECE_ROOK || pPiece->color != pcSrc->color) // we don't have a rook to castle with
						return false;
					
					for (int i = 1; i < BOARD_SIZE / 2; i++)
					{
						pPiece = GetPieceIgnore(pState, homeRow, i, -1, -1);
						if (pPiece->piece != PIECE_NONE) return false; // this square is blocked
						
						// check if this position would put us in check
						if (i >= 2)
						{
							if (ChessIsBeingThreatened(pState, homeRow, i, pcSrc->color, -1, -1, false))
								return false;
						}
					}
					
					// okay, we should be able to castle.
					return true;
				}
				else
				{
					BoardPiece* pPiece = GetPiece(pState, homeRow, BOARD_SIZE - 1);
					if (pPiece->piece != PIECE_ROOK || pPiece->color != pcSrc->color) // we don't have a rook to castle with
						return false;
					
					for (int i = BOARD_SIZE / 2 + 1; i < BOARD_SIZE - 1; i++)
					{
						pPiece = GetPieceIgnore(pState, homeRow, i, -1, -1);
						if (pPiece->piece != PIECE_NONE) return false; // this square is blocked
						
						// check if this position would put us in check
						if (i < BOARD_SIZE / 2 + 2)
						{
							if (ChessIsBeingThreatened(pState, homeRow, i, pcSrc->color, -1, -1, false))
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
bool ChessIsBeingThreatened(BoardState* pState, int row, int col, eColor color, int rowIgn, int colIgn, bool bFlashTiles)
{
	bool bFinalResult = false;
	
	if (pState != g_CurrentState)
		bFlashTiles = false;
	
	int rows[8], cols[8];
	rows[0] = row + 1, cols[0] = col - 1;
	rows[1] = row + 1, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col - 1;
	rows[3] = row - 1, cols[3] = col + 1;
	
	// Check for attacking pawns
	BoardPiece* pcs[8];
	for (int i = 0; i < 4; i++)
		pcs[i] = GetPieceIgnore(pState, rows[i], cols[i], rowIgn, colIgn);
	
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
			pcs[i] = GetPieceIgnore(pState, rows[i], cols[i], rowIgn, colIgn);
			
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
			pcs[i] = GetPieceIgnore(pState, rows[i], cols[i], rowIgn, colIgn);
			
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
		
		pcs[i] = GetPieceIgnore(pState, rows[i], cols[i], rowIgn, colIgn);
		
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
		pcs[i] = GetPieceIgnore(pState, rows[i], cols[i], rowIgn, colIgn);
		
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

eErrorCode ChessCheckMove(BoardState* pState, int rowSrc, int colSrc, int rowDst, int colDst, eCastleType* castleType, bool * bWouldDoEP, bool bFlashTiles)
{
	BoardPiece* pcSrc = GetPiece(pState, rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(pState, rowDst, colDst);
	
	if (!pcSrc || !pcDst)
		return ERROR_OUT_OF_BOUNDS;
	
	if (pState != g_CurrentState)
		bFlashTiles = false;
	
	// Check if it even makes sense to move here
	if (pcDst->piece == PIECE_KING)
		return ERROR_CANNOT_CAPTURE_KING;
	
	if (pcDst->piece != PIECE_NONE && pcDst->color == pcSrc->color)
		return ERROR_CANNOT_MOVE_ON_SAME_COLOR;
	
	if (pcSrc->color != pState->m_Player)
		return ERROR_NOT_YOUR_TURN;
	
	// Check if the move is valid, based on the rank.
	if (!ChessCheckLegalBasedOnRank(pState, rowSrc, colSrc, rowDst, colDst, castleType, bWouldDoEP))
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
		
		GetPiece(pState, rowSrc, colSrc)->piece = PIECE_NONE;
		
		BoardPos pos = pState->m_PlayerState[colr].m_KingPos;
		// If we're moving the king, see if _THAT_ position puts us in check
		if (piece == PIECE_KING)
		{
			BoardPos pos2 = { rowDst, colDst };
			pos = pos2;
		}
		bool wouldPutUsInCheck = ChessIsBeingThreatened(pState, pos.row, pos.col, colr, -1, -1, piece == PIECE_KING);
		
		if (piece != PIECE_KING && wouldPutUsInCheck)
		{
			// flash the king, since it's in check
			if (bFlashTiles) FlashTile(pos.row, pos.col);
		}
		
		*pcSrc = pcSrcT;
		*pcDst = pcDstT;
		
		if (wouldPutUsInCheck)
			return ERROR_MOVE_WOULD_PUT_US_IN_CHECK;
	}
	
	return ERROR_SUCCESS;
}

// Check if a color is in checkmate.
eMateType ChessCheckmateOrStalemate(BoardState* pState, eColor color)
{
	// if there's no king, it's the end of the game
	BoardPos kingPos = pState->m_PlayerState[color].m_KingPos;
	
	BoardPiece* pKing = GetPiece(pState, kingPos.row, kingPos.col);
	bool bIsKingThreatened = false;
	
	if (pKing)
		bIsKingThreatened = ChessIsBeingThreatened(pState, kingPos.row, kingPos.col, pKing->color, -1, -1, false);
	
	// set the turn as the color
	eColor backup = pState->m_Player;
	pState->m_Player = color;
	
	// check all 64*64 moves. There's only 4096 combinations to try.
	// This doesn't scale well, since ChessCheckMove() is O(N) or something,
	// and this would be like O(N^5), but this is reasonably quick to implement.
	for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		int iCol = i % BOARD_SIZE, iRow = i / BOARD_SIZE;
		
		BoardPiece* pPiece = GetPiece(pState, iRow, iCol);
		if (pPiece->piece == PIECE_NONE) continue;
		if (pPiece->color != color) continue;
		
		// TODO: Add moves depending on the piece that's being moved. Don't try every move ever, in a dumb way.
		for (int j = 0; j < BOARD_SIZE * BOARD_SIZE; j++)
		{
			int jCol = j % BOARD_SIZE, jRow = j / BOARD_SIZE;
			
			if (i == j) continue;
			
			// there's a legal move.
			// The explanation for this is:
			// If the king is in check, then ChessCheckMove only returns true if the move would take the king out of check.
			
			UNUSED eCastleType castleType;
			UNUSED bool bEnPassant;
			if (ChessCheckMove(pState, iRow, iCol, jRow, jCol, &castleType, &bEnPassant, false) == ERROR_SUCCESS)
			{
				//LogMsg("Move from %c%d to %c%d works", iCol+'a', iRow+1, jCol+'a', jRow+1);
				pState->m_Player = backup;
				return MATE_NONE;
			}
		}
	}
	
	pState->m_Player = backup;
	
	return bIsKingThreatened ? MATE_CHECK : MATE_STALE;
}

void ChessPerformMoveUnchecked(BoardState* pState, int rowSrc, int colSrc, int rowDst, int colDst, eCastleType castleType, bool * pbCapture, bool bEnPassant)
{
	BoardPiece* pcSrc = GetPiece(pState, rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(pState, rowDst, colDst);
	
	eColor col = pcSrc->color;
	
	int kingPos = BOARD_SIZE / 2;
	
	// Normally these would be the same, except in en passant
	int rowCap = rowDst, colCap = colDst;
	
	UNUSED int homeRow = (col == WHITE) ? 0 : 7;
	UNUSED int pawnRow = (col == WHITE) ? 1 : 6;
	
	switch (castleType)
	{
		// we checked for any checks/captures during castling. There may not be any pieces on the castled over tiles.
		// Additionally, the king may not castle into check, or through check.
		case CASTLE_KINGSIDE:
			
			// set the square right next to the king to the rook.
			SetPiece(pState, GetHomeRow(col), kingPos + 1, PIECE_ROOK, col);
			// and then, the king
			SetPiece(pState, GetHomeRow(col), kingPos + 2, PIECE_KING, col);
			
			// clear the rook's square
			SetPiece(pState, GetHomeRow(col), BOARD_SIZE - 1, PIECE_NONE, BLACK);
			// clear the king's square
			SetPiece(pState, GetHomeRow(col), kingPos, PIECE_NONE, BLACK);
			
			// no more castling!!
			pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_KINGSIDE ] = false;
			pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_QUEENSIDE] = false;
			
			break;
			
		case CASTLE_QUEENSIDE:
			
			// set the square right next to the king to the rook.
			SetPiece(pState, GetHomeRow(col), kingPos - 1, PIECE_ROOK, col);
			// and then, the king
			SetPiece(pState, GetHomeRow(col), kingPos - 2, PIECE_KING, col);
			
			// clear the rook's square
			SetPiece(pState, GetHomeRow(col), 0, PIECE_NONE, BLACK);
			// clear the king's square
			SetPiece(pState, GetHomeRow(col), kingPos, PIECE_NONE, BLACK);
			
			// no more castling!!
			pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_KINGSIDE ] = false;
			pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_QUEENSIDE] = false;
			
			break;
		
		default: // No castling
			
			if (bEnPassant)
			{
				// move the pcDst piece away from our home row, towards their home row
				int diff = col == WHITE ? -1 : +1;
				rowCap += diff;
				pcDst = GetPiece(pState, rowCap, colCap);
			}
			
			// if the king was moved:
			if (pcSrc->piece == PIECE_KING)
			{
				pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_KINGSIDE ] = false;
				pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_QUEENSIDE] = false;
			}
			
			// if the piece moved was a rook
			if (pcSrc->piece == PIECE_ROOK && rowSrc == GetHomeRow(col))
			{
				// if it was the queenside rook
				if (colSrc == 0)
					pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_QUEENSIDE] = false;
				else if (colDst == BOARD_SIZE - 1)
					pState->m_PlayerState[col].m_bCastleAllowed[CASTLE_KINGSIDE ] = false;
			}
			
			// if the piece moved was a pawn, and it moved two steps:
			if (pcSrc->piece == PIECE_PAWN && abs(rowDst - rowSrc) == 2)
				pState->m_PlayerState[pcSrc->color].m_nEnPassantColumn = colDst;
			
			// Capture whatever was here before.
			if (pcDst->piece != PIECE_NONE)
			{
				AddCapture(pState, col, *pcDst);
				*pbCapture = true;
			}
			
			SetPiece(pState, rowCap, colCap, PIECE_NONE, BLACK);
			SetPiece(pState, rowDst, colDst, pcSrc->piece, col);
			SetPiece(pState, rowSrc, colSrc, PIECE_NONE, BLACK);
			
			break;
	}
}

char GetPieceNotation(ePiece piece, int col)
{
	switch (piece)
	{
		case PIECE_PAWN:   return 'a' + col;
		case PIECE_ROOK:   return 'R';
		case PIECE_BISHOP: return 'B';
		case PIECE_KNIGHT: return 'N';
		case PIECE_KING:   return 'K';
		case PIECE_QUEEN:  return 'Q';
	}
	
	return '?';
}

// This formats a move into PGN and lets the UI know about it.
void ChessGeneratePGN(char* pgnOut, BoardState* pState, int rowSrc, int colSrc, int rowDst, int colDst, eCastleType castleType, bool bCheck, bool bCapture, bool bEnPassant, eMateType mateType)
{
	BoardPiece* pcDst = GetPiece(pState, rowDst, colDst);
	
	char moveList[sizeof(pState->m_MoveInfo.pgn)];
	strcpy(moveList, "???");
	
	if (castleType != CASTLE_NONE)
	{
		switch (castleType)
		{
			case CASTLE_KINGSIDE:
				strcpy(moveList, "O-O");
				break;
			case CASTLE_QUEENSIDE:
				strcpy(moveList, "O-O-O");
				break;
		}
	}
	else
	{
		ePiece pie = pcDst->piece;
		eColor col = pcDst->color;
		char piece = GetPieceNotation (pie, colSrc);
		
		// take out the piece for now
		BoardPiece pcCopy = *pcDst;
		pcDst->piece = PIECE_NONE;
		
		// Disambiguation notation. For example, Rae1 vs Rge1 if both rooks are able to move to e1
		char pieceDisamb[10];
		pieceDisamb[0] = 0;
		
		// Other flags. For example, 'e.p.' for en passant, '+' for check, '#' for checkmate..
		char otherFlags[10];
		otherFlags[0] = 0;
		
		// Capture flag.
		char captureFlag[2], pieceName[2];
		captureFlag[0] = captureFlag[1] = pieceName[1] = 0;
		pieceName[0] = piece;
		
		if (bCapture)
			captureFlag[0] = 'x';
		else if (pie == PIECE_PAWN && !bCapture)
			pieceName[0] = 0;
		
		if (mateType == MATE_CHECK)
			strcat(otherFlags, "#");
		else if (bCheck)
			strcat(otherFlags, "+");
		
		if (bEnPassant)
			strcat(otherFlags, " e.p.");
		
		int disambiguationFlags = 0;
		
		// Pawns and kings don't really need disambiguation, I don't think
		if (pie != PIECE_PAWN && pie != PIECE_KING)
		{
			// search through all 64 tiles, see if they can move here
			for (int ind = 0; ind < BOARD_SIZE * BOARD_SIZE; ind++)
			{
				int row = ind / BOARD_SIZE, clm = ind % BOARD_SIZE;
				
				BoardPiece* pPiece = GetPiece(pState, row, clm);
				//if (row == rowDst && clm == colDst) continue;
				
				if (pPiece->piece != pie || pPiece->color != col) continue;
				//LogMsg("Checking piece on %c%d", clm+'a', row+1);
				
				// see if the piece can move here
				UNUSED eCastleType castleType;
				UNUSED bool bEnPassant;
				if (ChessCheckMove(pState, row, clm, rowDst, colDst, &castleType, &bEnPassant, true) == ERROR_SUCCESS)
				{
					//LogMsg("ChessGeneratePGN: move from %c%d is also valid", clm+'a', row+1);
					
					// yes. Need disambiguation.
					if (row == rowDst)
						disambiguationFlags |= 1; // include the column letter
					if (clm == colDst)
						disambiguationFlags |= 2; // include the row number
					
					if (disambiguationFlags == 3) break; // we need FULL disambiguation, so it's not even worth checking other pieces out
				}
			}
		}
		
		if (disambiguationFlags)
		{
			char buff[10];
			
			if (disambiguationFlags & 1) // col
			{
				snprintf(buff, sizeof buff, "%c", colSrc + 'a');
				strcat(pieceDisamb, buff);
			}
			if (disambiguationFlags & 2) // row
			{
				snprintf(buff, sizeof buff, "%d", rowSrc + 1);
				strcat(pieceDisamb, buff);
			}
		}
		
		char colLet = 'a' + colDst;
		
		snprintf(moveList, sizeof moveList, "%s%s%s%c%d%s", pieceName, pieceDisamb, captureFlag, colLet, rowDst + 1, otherFlags);
		
		*pcDst = pcCopy;
	}
	
	strcpy(pgnOut, moveList);
}

eErrorCode ChessCommitMove(int rowSrc, int colSrc, int rowDst, int colDst)
{
	BoardState* pState = g_CurrentState;
	
	BoardPiece* pcSrc = GetPiece(pState, rowSrc, colSrc);
	
	eColor col = pcSrc->color;
	
	if (rowSrc == rowDst && colSrc == colDst)
		// don't treat it as a failure if they didn't move
		return ERROR_SUCCESS;
	
	eCastleType castleType = CASTLE_NONE;
	bool bEnPassant = false;
	
	eErrorCode errCode = ChessCheckMove(pState, rowSrc, colSrc, rowDst, colDst, &castleType, &bEnPassant, true);
	if (errCode != ERROR_SUCCESS)
		return errCode;
	
	if (g_CurrentState != &g_History[g_HistorySize - 1])
	{
		if (g_CurrentState < g_History || g_History + g_HistorySize <= g_CurrentState)
		{
			SLogMsg("Okay");
			return ERROR_CANT_OVERWRITE_HISTORY;
		}
		// Show a message box asking whether the user wants to overwrite the history.
		if (ChessMessageBox("Performing this move will overwrite the move history.\n\nDo you wish to perform the move?", "Chess", MB_YESNO | ICON_WARNING << 16) != MBID_YES)
		{
			return ERROR_CANT_OVERWRITE_HISTORY;
		}
		
		// this ptr diff should be fine, since we already check the bounds...
		g_HistorySize = (int)(g_CurrentState - g_History) + 1;
	}
	
	// commit the move!!
	AddHistoryFrame();
	
	pState = g_CurrentState;
	
	pState->m_PlayerState[pcSrc->color].m_nEnPassantColumn = -1;
	
	pState->m_PlayerState[pcSrc->color].m_bKingInCheck = false;
	
	bool bCapture = false;
	ChessPerformMoveUnchecked(pState, rowSrc, colSrc, rowDst, colDst, castleType, &bCapture, bEnPassant);
	
	bool bCheck = false;
	
	eMateType mateType = MATE_NONE;
	
	// Check if any other players are in check, and update their state.
	for (int i = FIRST_PLAYER; i < NPLAYERS; i++)
	{
		BoardPos pos = pState->m_PlayerState[i].m_KingPos;
		pState->m_PlayerState[i].m_bKingInCheck = ChessIsBeingThreatened(pState, pos.row, pos.col, i, -1, -1, false);
		
		if (pState->m_PlayerState[i].m_bKingInCheck)
		{
			bCheck = true;
			//LogMsg("%d's in check!", i);
		}
		
		// TODO: handle other players properly
		if ((eColor)i != col)
			mateType = ChessCheckmateOrStalemate(pState, i);
	}
	
	// fill in the move struct
	MoveInfo* pmi = &pState->m_MoveInfo;
	
	ChessGeneratePGN(pmi->pgn, pState, rowSrc, colSrc, rowDst, colDst, castleType, bCheck, bCapture, bEnPassant, mateType);
	
	ChessUpdateMoveList();
	
	pState->m_Player = GetNextPlayer(pState->m_Player);
	
	switch (mateType)
	{
		case MATE_CHECK: return ERROR_CHECKMATE;
		case MATE_STALE: return ERROR_STALEMATE;
		default:         return ERROR_SUCCESS;
	}
}

extern int g_nMoveNumber;
void SetupBoard(BoardState* pState)
{
	if (pState == g_CurrentState)
		ChessClearGUI();
	
	memset(&pState->m_Pieces, 0, sizeof pState->m_Pieces);
	
	// allow castling initially
	for (int i = 0; i < NPLAYERS; i++)
	{
		pState->m_PlayerState[i].m_bCastleAllowed[CASTLE_KINGSIDE]  = true;
		pState->m_PlayerState[i].m_bCastleAllowed[CASTLE_QUEENSIDE] = true;
	}
	
	pState->m_Player = WHITE;
	
	// clear other resources, such as checks, and captures
	for (int i = 0; i < NPLAYERS; i++)
	{
		BoardPos emptyPos = { 0, 0 };
		
		PlayerState* ppState = &pState->m_PlayerState[i];
		ppState->m_bKingInCheck     = false;
		ppState->m_nCaptures        = 0;
		ppState->m_KingPos          = emptyPos;
		ppState->m_nEnPassantColumn = -1;
	}
	
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		// set a2..g2 and a7..g7 to pawns. a2..g2 has the color white, a7..g7 has the color black.
		SetPiece(pState, 1, i, PIECE_PAWN, WHITE);
		SetPiece(pState, 6, i, PIECE_PAWN, BLACK);
	}
	
	// Rooks
	SetPiece(pState, 0, 0, PIECE_ROOK, WHITE);
	SetPiece(pState, 0, 7, PIECE_ROOK, WHITE);
	SetPiece(pState, 7, 0, PIECE_ROOK, BLACK);
	SetPiece(pState, 7, 7, PIECE_ROOK, BLACK);
	
	// Knights
	SetPiece(pState, 0, 1, PIECE_KNIGHT, WHITE);
	SetPiece(pState, 0, 6, PIECE_KNIGHT, WHITE);
	SetPiece(pState, 7, 1, PIECE_KNIGHT, BLACK);
	SetPiece(pState, 7, 6, PIECE_KNIGHT, BLACK);
	
	// Bishops
	SetPiece(pState, 0, 2, PIECE_BISHOP, WHITE);
	SetPiece(pState, 0, 5, PIECE_BISHOP, WHITE);
	SetPiece(pState, 7, 2, PIECE_BISHOP, BLACK);
	SetPiece(pState, 7, 5, PIECE_BISHOP, BLACK);
	
	// Queens
	SetPiece(pState, 0, 3, PIECE_QUEEN, WHITE);
	SetPiece(pState, 7, 3, PIECE_QUEEN, BLACK);
	
	// Kings
	SetPiece(pState, 0, 4, PIECE_KING, WHITE);
	SetPiece(pState, 7, 4, PIECE_KING, BLACK);
}

void ResetGame()
{
	//LogMsg("TODO: ResetGame!");
	InitializeHistory();
	AddHistoryFrame();
}

