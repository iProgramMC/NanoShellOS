#include "chess.h"

const int nPieceValues[] = {
	0,   // None
	999, // King
	9,   // Queen
	3,   // Bishop
	3,   // Knight
	5,   // Rook
	1,   // Pawn
};

int EvaluateBoardState(BoardState* pState)
{
	int State = 0;
	
	for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		int iRow = i % BOARD_SIZE, iCol = i / BOARD_SIZE;
		BoardPiece* pPiece = GetPiece(pState, iRow, iCol);
		
		int value = nPieceValues[pPiece->piece];
		if (pPiece->color == BLACK)
			value = -value;
		
		State += value;
	}
	
	return State;
}

#define MAX_MOVES 256
BoardMove* GetAllMoves(BoardState* pState, int* outMoveCount)
{
#define ADD_MOVE(a, b, c, d) do {         \
	if (nMoves == MAX_MOVES) {            \
		LogMsg("ERROR: too many moves!"); \
	} else {                              \
		BoardMove move = { a, b, c, d };  \
		pMoves[nMoves++] = move;          \
	}                                     \
} while (0)
	
	// backup the current state pointer, we'll modify it
	BoardState state;
	BoardState * pOldCurrentState = g_CurrentState;
	g_CurrentState = &state;
	
	int nMoves = 0;
	BoardMove* pMoves = calloc(MAX_MOVES, sizeof(BoardMove));

	// Enumerate every possible move that the current player can make.
	for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		int iRow = i % BOARD_SIZE, iCol = i / BOARD_SIZE;
		BoardPiece* pPiece = GetPiece(pState, iRow, iCol);
		if (pPiece->color != pState->m_Player)
			continue;
		
		// TODO: Add moves depending on the piece that's being moved. Don't try every move ever, in a dumb way.
		for (int j = 0; j < BOARD_SIZE * BOARD_SIZE; j++)
		{
			int jRow = j % BOARD_SIZE, jCol = j / BOARD_SIZE;
			if (i == j) continue;
			
			// there's a legal move.
			// The explanation for this is:
			// If the king is in check, then ChessCheckMove only returns true if the move would take the king out of check.
			
			UNUSED eCastleType castleType;
			UNUSED bool bEnPassant;
			if (ChessCheckMove(pState, iRow, iCol, jRow, jCol, &castleType, &bEnPassant, false) == ERROR_SUCCESS)
				ADD_MOVE(iRow, iCol, jRow, jCol);
		}
	}
	
	g_CurrentState = pOldCurrentState;
	
	*outMoveCount = nMoves;
	return pMoves;
}

int NegativeMax(BoardState* pState, int Depth)
{
	if (Depth == 0)
		return EvaluateBoardState(pState);
	
	return 0;
}

BoardMove FindBestMove(BoardState* pState)
{
	BoardMove* pMoves = NULL;
	int nMoves = 0;
	
	pMoves = GetAllMoves(pState, &nMoves);
	
	BoardMove bm = { -1, -1, -1, -1 };
	if (nMoves == 0)
		LogMsg("No moves available!");
	else
		bm = pMoves[rand() % nMoves];
	
	free(pMoves);
	return bm;
}


