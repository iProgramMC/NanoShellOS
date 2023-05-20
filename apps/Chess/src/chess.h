#ifndef CHESS_H
#define CHESS_H

#include <nanoshell/nanoshell.h>

#define BOARD_SIZE (8)  // most likely will never change
#define PIECE_SIZE (48)

#define EVENT_UPDATE_FLASHING  (EVENT_USER)
#define EVENT_UPDATE_MOVE_LIST (EVENT_USER + 1)

#define RIGHT_BAR_WIDTH (140)
#define LEFT_BAR_WIDTH  (60)
#define SIDE_BAR_WIDTH  (LEFT_BAR_WIDTH + RIGHT_BAR_WIDTH)
#define TOP_BAR_HEIGHT  (70)
#define MOVE_LIST_WIDTH (50)

#define TILE_FLASH_COUNT (9)

#define BOARD_THICKNESS (2)

typedef enum
{
	CASTLE_NONE,
	CASTLE_KINGSIDE,
	CASTLE_QUEENSIDE,
	CASTLE_TYPES,
}
eCastleType;

typedef enum
{
	PIECE_NONE,
	PIECE_KING,
	PIECE_QUEEN,
	PIECE_BISHOP,
	PIECE_KNIGHT,
	PIECE_ROOK,
	PIECE_PAWN,
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
	ERROR_OUT_OF_BOUNDS,
	ERROR_CANT_OVERWRITE_HISTORY,
	
	ERROR_STALEMATE = 1000,
	ERROR_CHECKMATE,
	ERROR_RESIGNATION_BLACK,
	ERROR_RESIGNATION_WHITE,
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

typedef struct
{
	bool m_bCastleAllowed[CASTLE_TYPES];
	
	BoardPiece m_Captures[BOARD_SIZE * BOARD_SIZE];
	int m_nCaptures;
	
	int m_nEnPassantColumn;
	
	bool m_bKingInCheck;
	
	BoardPos m_KingPos;
}
PlayerState;

typedef struct
{
	char pgn[32];
}
MoveInfo;

typedef struct
{
	// the move that led here
	MoveInfo m_MoveInfo;
	
	// the board's actual state
	PlayerState m_PlayerState[NPLAYERS];
	
	BoardPiece m_Pieces[BOARD_SIZE][BOARD_SIZE];
	
	// the current player's turn
	eColor m_Player;
}
BoardState;

extern BoardState* g_CurrentState;


extern BoardPiece g_pieces[BOARD_SIZE][BOARD_SIZE];
extern Window* g_pWindow;

eErrorCode ChessCommitMove(int rowSrc, int colSrc, int rowDst, int colDst);
eErrorCode ChessCheckMove(BoardState* pState, int rowSrc, int colSrc, int rowDst, int colDst, eCastleType* castleType, bool * bWouldDoEP, bool bFlashTiles);

// Check if a color is in checkmate or stalemate.
eMateType ChessCheckmateOrStalemate(BoardState* pState, eColor color);

void PaintTile(int row, int column);
void SetPiece(BoardState* pState, int row, int column, ePiece pc, eColor col);
BoardPiece* GetPiece(BoardState* pState, int row, int column);

void FlashTile(int row, int column);

void ResetGame();

eColor GetNextPlayer(eColor curPlr);

void ChessAddMoveToUI(const char* moveList);
void ChessClearGUI();

void SetupBoard(BoardState* pState);

int ChessMessageBox(const char* text, const char* caption, int flags);

void SetGameOver(bool);

#endif//CHESS_H
