#include "chess.h"

typedef struct
{
	
} x;

BoardMove FindBestMove(BoardState* pState)
{
	BoardState state;
	
#define MAX_MOVES 256
	BoardMove* pMoves = calloc(MAX_MOVES, sizeof(BoardMove));
	int nMoves = 0;
	
	// backup the current state pointer, we'll modify itoa
	BoardState * pCurrentState = g_CurrentState;
	g_CurrentState = &state;

#define ADD_MOVE(a, b, c, d) do {         \
	if (nMoves == MAX_MOVES) {            \
		LogMsg("ERROR: too many moves!"); \
	} else {                              \
		BoardMove move = { a, b, c, d };  \
		pMoves[nMoves++] = move;          \
	}                                     \
} while (0)
	
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
	
	free(pMoves);
	
	g_CurrentState = pCurrentState;
	
	// TODO: evaluate each one
	
	
	if (nMoves == 0)
	{
		LogMsg("No moves available!");
		BoardMove bm = { -1, -1, -1, -1 };
		return bm;
	}
	
	return pMoves[rand() % nMoves];
}


