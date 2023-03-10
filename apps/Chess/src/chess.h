#ifndef CHESS_H
#define CHESS_H

#include <nanoshell/nanoshell.h>

#define BOARD_SIZE (8)  // most likely will never change
#define PIECE_SIZE (32)
#define SIDE_BAR_WIDTH  (400)
#define TOP_BAR_HEIGHT  (70)
#define MOVE_LIST_WIDTH (50)

#define TILE_FLASH_COUNT (9)

#define BOARD_THICKNESS (2)

typedef enum
{
	PIECE_NONE,
	PIECE_PAWN,
	PIECE_KING,
	PIECE_QUEEN,
	PIECE_ROOK,
	PIECE_KNIGHT,
	PIECE_BISHOP,
	PIECE_MAX,
}
ePiece;

typedef enum
{
	ERROR_SUCCESS,
	ERROR_CANNOT_CAPTURE_KING,
	ERROR_CANNOT_MOVE_ON_SAME_COLOR,
	ERROR_NOT_YOUR_TURN,
	ERROR_MOVE_ILLEGAL,
	ERROR_MOVE_WOULD_PUT_US_IN_CHECK,
	
	ERROR_STALEMATE = 1000,
	ERROR_CHECKMATE = 1001,
}
eErrorCode;

typedef enum
{
	MATE_NONE,
	MATE_STALE, // no legal moves, king is not attacked
	MATE_CHECK, // no legal moves, king is attacked
}
eMateType;

typedef enum
{
	FIRST_PLAYER,
	BLACK = FIRST_PLAYER,
	WHITE,
	NPLAYERS,
}
eColor;

typedef struct
{
	eColor color;
	ePiece piece;
}
BoardPiece;

typedef struct
{
	int row, col;
}
BoardPos;

extern BoardPiece g_pieces[BOARD_SIZE][BOARD_SIZE];
extern Window* g_pWindow;

eErrorCode ChessCommitMove(int rowSrc, int colSrc, int rowDst, int colDst);

// Check if a color is in checkmate or stalemate.
eMateType ChessCheckmateOrStalemate(eColor color);

void PaintTile(int row, int column);
void SetPiece(int row, int column, ePiece pc, eColor col);
BoardPiece* GetPiece(int row, int column);

void FlashTile(int row, int column);

eColor GetNextPlayer(eColor curPlr);

#endif//CHESS_H
