/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp
             Chess application

             Chess source file
******************************************/
#include "chess.h"

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

bool ChessCheckLegalBasedOnRank(int rowSrc, int colSrc, int rowDst, int colDst)
{
	BoardPiece* pcSrc = GetPiece(rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(rowDst, colDst);
	
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
			return absRowDiff <= 1 && absColDiff <= 1;
		}
	}
	
	return false;
}

BoardPiece g_emptyPiece;

// Returns a piece with an optional ignorable piece, and never returns NULL.
BoardPiece* GetPieceIgnore(int row, int col, int rowIgn, int colIgn)
{
	if (row == rowIgn && col == colIgn) return &g_emptyPiece;
	
	if (row < 0 || col < 0 || row >= BOARD_SIZE || col >= BOARD_SIZE) return &g_emptyPiece;
	
	return &g_pieces[row][col];
}

// row, col - The position of a king
bool ChessIsKingInCheck(int row, int col, eColor color, int rowIgn, int colIgn)
{
	LogMsg("ChessIsKingInCheck(%d,%d,%d)",row,col,color);
	
	int rows[4], cols[4];
	rows[0] = row + 1, cols[0] = col - 1;
	rows[1] = row + 1, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col - 1;
	rows[3] = row - 1, cols[3] = col + 1;
	
	// Check for attacking pawns
	BoardPiece* pcs[4];
	for (int i = 0; i < 4; i++)
		pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
	
	// king is white, so check for black pawns above
	if (color == WHITE)
	{
		if (pcs[0]->piece == PIECE_PAWN && pcs[0]->color == BLACK) return true;
		if (pcs[1]->piece == PIECE_PAWN && pcs[1]->color == BLACK) return true;
	}
	// king is black, so check for white pawns above
	else if (color == BLACK)
	{
		if (pcs[2]->piece == PIECE_PAWN && pcs[2]->color == WHITE) return true;
		if (pcs[3]->piece == PIECE_PAWN && pcs[3]->color == WHITE) return true;
	}
	
	// Check for attacking bishops or queen diagonals
	rows[0] = row + 1, cols[0] = col - 1;
	rows[1] = row + 1, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col - 1;
	rows[3] = row - 1, cols[3] = col + 1;
	
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int i = 0; i < 4; i++)
		{
			pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
			
			// this is a different-color bishop or queen. In check
			if ((pcs[i]->piece == PIECE_BISHOP || pcs[i]->piece == PIECE_QUEEN) && pcs[i]->color != color)
				return true;
			
			// we hit a piece, so stop "shooting a ray" here.
			if (pcs[i]->piece != PIECE_NONE)
				rows[i] = cols[i] = -1;
		}
	}
	
	// Check for attacking rooks or queen axes
	rows[0] = row, cols[0] = col - 1;
	rows[1] = row, cols[1] = col + 1;
	rows[2] = row - 1, cols[2] = col;
	rows[3] = row + 1, cols[3] = col;
	
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int i = 0; i < 4; i++)
		{
			pcs[i] = GetPieceIgnore(rows[i], cols[i], rowIgn, colIgn);
			
			// this is a different-color rook or queen. In check
			if ((pcs[i]->piece == PIECE_ROOK || pcs[i]->piece == PIECE_QUEEN) && pcs[i]->color != color)
				return true;
		}
	}
	
	// no checks found. We're good
	return false;
}

eErrorCode ChessCommitMove(int rowSrc, int colSrc, int rowDst, int colDst)
{
	BoardPiece* pcSrc = GetPiece(rowSrc, colSrc);
	BoardPiece* pcDst = GetPiece(rowDst, colDst);
	
	if (rowSrc == rowDst && colSrc == colDst)
		// don't treat it as a failure if they didn't move
		return ERROR_SUCCESS;
	
	// Check if it even makes sense to move here
	if (pcDst->piece == PIECE_KING)
		return ERROR_CANNOT_CAPTURE_KING;
	
	if (pcDst->piece != PIECE_NONE && pcDst->color == pcSrc->color)
		return ERROR_CANNOT_MOVE_ON_SAME_COLOR;
	
	if (pcSrc->color != g_playingPlayer)
		return ERROR_NOT_YOUR_TURN;
	
	// Check if the move is valid, based on the rank.
	if (!ChessCheckLegalBasedOnRank(rowSrc, colSrc, rowDst, colDst))
		return ERROR_MOVE_ILLEGAL;
	
	// Check if the move would put us in check.
	// There are at least two situations where a move can put the king in check:
	// 1. The king moves directly into check
	// 2. A piece reveals the king to an enemy piece, checking it
	{
		// Do the swap very temporarily
		BoardPiece pcSrcT = *pcSrc;
		BoardPiece pcDstT = *pcDst;
		
		eColor colr = pcSrc->color;
		
		*pcDst = pcSrcT;
		SetPiece(rowSrc, colSrc, PIECE_NONE, BLACK);
		
		BoardPos pos = g_KingPositions[colr];
		bool wouldPutUsInCheck = ChessIsKingInCheck(pos.row, pos.col, colr, -1, -1);
		
		*pcSrc = pcSrcT;
		*pcDst = pcDstT;
		
		if (wouldPutUsInCheck)
			return ERROR_MOVE_WOULD_PUT_US_IN_CHECK;
	}
	
	g_bIsKingInCheck[pcSrc->color] = false;
	
	// Capture whatever was here before.
	if (pcDst->piece != PIECE_NONE)
		AddCapture(pcSrc->color, *pcDst);
	
	SetPiece(rowDst, colDst, pcSrc->piece, pcSrc->color);
	SetPiece(rowSrc, colSrc, PIECE_NONE, BLACK);
	
	// Check if any other players are in check, and update their state.
	for (int i = FIRST_PLAYER; i < NPLAYERS; i++)
	{
		BoardPos pos = g_KingPositions[i];
		g_bIsKingInCheck[i] = ChessIsKingInCheck(pos.row, pos.col, i, -1, -1);
		
		if (g_bIsKingInCheck[i])
		{
			LogMsg("%d's in check!", i);
		}
	}
	
	g_playingPlayer = GetNextPlayer(g_playingPlayer);
	
	
	
	return ERROR_SUCCESS;
}
