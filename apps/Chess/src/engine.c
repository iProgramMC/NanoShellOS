#include "chess.h"

#define INF (1000000)

const int g_InitialDepth = 4;

const int nPieceValues[] = {
	0,     // None
	99999, // King
	900,   // Queen
	300,   // Bishop
	300,   // Knight
	500,   // Rook
	100,   // Pawn
};

// NOTE: I pulled these values out of my own ass.

const char nKingValueMult[] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

const char nRookValueMult[] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0
};

const char nBishopValueMult[] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const char nKnightValueMult[] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const char nQueenValueMult[] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const char nPawnValueMult[] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 25, 25, 10,  5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5, -5,-10,  0,  0,-10, -5,  5,
    5, 10, 10,-20,-20, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

const char* const nValueMultPiece[] = {
	NULL,             // None
	nKingValueMult,   // King,
	nQueenValueMult,  // Queen
	nBishopValueMult, // Bishop
	nKnightValueMult, // Knight
	nRookValueMult,   // Rook
	nPawnValueMult,   // Pawn
};

int EvaluateBoardState(BoardState* pState)
{
	int eval = 0;
	
	for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		int iRow = i % BOARD_SIZE, iCol = i / BOARD_SIZE;
		BoardPiece* pPiece = GetPiece(pState, iRow, iCol);
		if (pPiece->piece == PIECE_NONE)
			continue;
		
		bool isBlack = pPiece->color == BLACK;
		int index = isBlack ? i : (BOARD_SIZE * BOARD_SIZE - 1 - i);
		
		int value = nPieceValues[pPiece->piece] + nValueMultPiece[pPiece->piece][index];
		if (isBlack)
			value = -value;
		
		eval += value;
	}
	
	return eval;
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
	
	*outMoveCount = nMoves;
	return pMoves;
}

int MinMax(BoardState* pState, int Depth, BoardMove* winningMoveOut, bool isMaxing, int alpha, int beta)
{
	BoardMove winningMove = { -1, -1, -1, -1 };
	
	if (Depth == 0)
		return EvaluateBoardState(pState);
	
	BoardState state;
	int nMoves = 0;
	BoardMove* pMoves = GetAllMoves(pState, &nMoves);
	
	int max = -INF, min = INF;
	
	for (int i = 0; i < nMoves; i++)
	{
		BoardMove bm = pMoves[i];
		
		// copy the state
		state = *pState;
		
		// mutate the state to perform the move
		eErrorCode ec = ChessCommitMove(&state, bm.rowSrc, bm.colSrc, bm.rowDst, bm.colDst);
		
		if (ec != ERROR_SUCCESS)
		{
			if (ec == ERROR_CHECKMATE) {
				// checkmate!! don't hesitate!!!!
				winningMove = pMoves[i];
				break;
			}
			
			if (ec != ERROR_STALEMATE) {
				LogMsg("ERROR: couldn't perform move - MinMax. %d,%d -> %d,%d ==> %d", bm.rowSrc, bm.colSrc, bm.rowDst, bm.colDst, ec);
				continue;
			}
		}
		
		// the move has been performed. control was passed to the other player. call the MinMax again
		
		int score = MinMax(&state, Depth - 1, &bm, !isMaxing, alpha, beta);
		
		bm = pMoves[i];
		
		// For debugging and tracking:
		if (Depth == g_InitialDepth)
			LogMsg(">> Eval predicted after %d,%d -> %d,%d ==> %d [%d/%d]", bm.rowSrc, bm.colSrc, bm.rowDst, bm.colDst, score, i+1, nMoves);
		
		if (isMaxing)
		{
			if (max < score) {
				max = score;
				winningMove = pMoves[i];
			}
			if (alpha < score)
				alpha = score;
		}
		else
		{
			if (min > score) {
				min = score;
				winningMove = pMoves[i];
			}
			if (beta > score)
				beta = score;
		}
		
		if (beta <= alpha)
			break;
	}
	
	free(pMoves);
	*winningMoveOut = winningMove;
	return nMoves == 0 ? 0 : (isMaxing ? max : min);
}

BoardMove FindBestMove(BoardState* pState)
{
	BoardMove bm = { -1, -1, -1, -1 };
	
	BoardState bs = *pState;
	int eval = MinMax(&bs, g_InitialDepth, &bm, bs.m_Player == WHITE, -INF, INF);
	
	LogMsg("Eval after %d,%d -> %d,%d ==> %d", bm.rowSrc, bm.colSrc, bm.rowDst, bm.colDst, eval);
	
	return bm;
}


