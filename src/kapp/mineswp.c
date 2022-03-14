/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Scribble Application module
******************************************/

#include <wbuiltin.h>

#define BOARD_WIDTH  9
#define BOARD_HEIGHT 9
#define MINE_NUMBER  15

#define MINESW_WIDTH  ((BOARD_WIDTH  * 16) + 26)
#define MINESW_HEIGHT ((BOARD_HEIGHT * 16) + 78 + 11 + TITLE_BAR_HEIGHT)

typedef struct
{
	bool m_gameOver, m_firstClickDone;
	bool m_mine[BOARD_WIDTH][BOARD_HEIGHT];
	bool m_unco[BOARD_WIDTH][BOARD_HEIGHT];
	bool m_flag[BOARD_WIDTH][BOARD_HEIGHT];
	bool m_clck[BOARD_WIDTH][BOARD_HEIGHT];
}
Minesweeper;

void RenderButtonShapeNoRounding(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);

bool IsMine (Minesweeper* pGame, int x, int y)
{
	if (x < 0 || y < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) return false;
	
	return pGame->m_mine[x][y];
}

const uint32_t g_MinesweeperColors[] = {
	TRANSPARENT,
	0x10000FF,
	0x1007F00,
	0x1FF0000,
	0x100007F,
	0x17F0000,
	0x1007F7F,
	0x1000000,
	0x1404040,
	0x1FFFFFF,
};

void MineDrawTile(Minesweeper* pGame, int tileX, int tileY, int drawX, int drawY)
{
	Rectangle rect;
	
	if (!pGame->m_unco[tileX][tileY])
	{
		if (pGame->m_clck[tileX][tileY])
		{
			RECT(rect, drawX, drawY, 15, 15);
			VidFillRectangle (0xC0C0C0, rect);
			VidDrawRectangle (0x808080, rect);
		}
		else
		{
			RECT(rect, drawX, drawY, 16, 16);
			RenderButtonShapeNoRounding(rect, 0x808080, 0xFFFFFF, 0xC0C0C0);
			
			if (pGame->m_flag[tileX][tileY])
			{
				RenderIconForceSize(ICON_NANOSHELL_LETTERS16, drawX, drawY, 16);
			}
		}
	}
	else
	{
		RECT(rect, drawX, drawY, 15, 15);
		
		//Is there a bomb?
		if (IsMine (pGame, tileX, tileY))
		{
			// Yes.  Draw an icon
			VidFillRectangle (0xFF0000, rect);
			VidDrawRectangle (0x808080, rect);
			
			RenderIconForceSize(ICON_BOMB, drawX, drawY, 16);
		}
		else
		{
			// Count how many mines are around this tile.
			int mines_around = 
				IsMine(pGame, tileX - 1, tileY - 1) + 
				IsMine(pGame, tileX + 0, tileY - 1) + 
				IsMine(pGame, tileX + 1, tileY - 1) + 
				IsMine(pGame, tileX - 1, tileY + 0) + 
				IsMine(pGame, tileX + 1, tileY + 0) + 
				IsMine(pGame, tileX - 1, tileY + 1) + 
				IsMine(pGame, tileX + 0, tileY + 1) + 
				IsMine(pGame, tileX + 1, tileY + 1);
			
			VidFillRectangle (0xC0C0C0, rect);
			VidDrawRectangle (0x808080, rect);
			
			if (mines_around)
			{
				char text[2];
				text[0] = '0' + mines_around;
				text[1] = 0;
				
				VidDrawText (text, rect, TEXTSTYLE_HCENTERED|TEXTSTYLE_VCENTERED, g_MinesweeperColors[mines_around], TRANSPARENT);
			}
		}
	}
}

void GenerateMines(Minesweeper *pGame, int nMines, int avoidX, int avoidY);

void MineFlagTile(Minesweeper* pGame, int x, int y)
{
	if (pGame->m_gameOver) return;
	
	if (x < 0 || y < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) return;
	
	if (!pGame->m_firstClickDone)
		return;
	
	if (pGame->m_unco[x][y])
		return;
	
	pGame->m_flag[x][y] ^= 1;
}
	
void MineUncoverTile(Minesweeper* pGame, int x, int y)
{
	if (pGame->m_gameOver) return;
	
	if (x < 0 || y < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) return;
	
	if(!pGame->m_firstClickDone)
	{
		pGame->m_firstClickDone = true;
		GenerateMines(pGame, MINE_NUMBER, x, y);
	}
	
	if (pGame->m_flag[x][y])
		return;
	
	if (pGame->m_unco[x][y])
		return;
	
	pGame->m_unco[x][y] = true;
	
	if (pGame->m_mine[x][y])
	{
		pGame->m_gameOver = true;
		
		// Uncover other mines
		for (int my = 0; my < BOARD_HEIGHT; my++)
		{
			for (int mx = 0; mx < BOARD_WIDTH; mx++)
			{
				if (pGame->m_mine[mx][my])
					pGame->m_unco[mx][my] = true;
			}
		}
		return;
	}
	
	int mines_around = 
		IsMine(pGame, x - 1, y - 1) + 
		IsMine(pGame, x + 0, y - 1) + 
		IsMine(pGame, x + 1, y - 1) + 
		IsMine(pGame, x - 1, y + 0) + 
		IsMine(pGame, x + 1, y + 0) + 
		IsMine(pGame, x - 1, y + 1) + 
		IsMine(pGame, x + 0, y + 1) + 
		IsMine(pGame, x + 1, y + 1);
	
	// If this tile is fully empty, also uncover the tiles around it:
	if (!mines_around)
	{
		MineUncoverTile(pGame, x - 1, y - 1);
		MineUncoverTile(pGame, x + 0, y - 1);
		MineUncoverTile(pGame, x + 1, y - 1);
		MineUncoverTile(pGame, x - 1, y + 0);
		MineUncoverTile(pGame, x + 1, y + 0);
		MineUncoverTile(pGame, x - 1, y + 1);
		MineUncoverTile(pGame, x + 0, y + 1);
		MineUncoverTile(pGame, x + 1, y + 1);
	}
}

void GenerateMines(Minesweeper *pGame, int nMines, int avoidX, int avoidY)
{
	if (nMines > BOARD_WIDTH * BOARD_HEIGHT - 1)
		nMines = BOARD_WIDTH * BOARD_HEIGHT - 1;
	
	while (nMines)
	{
		while (true)
		{
			int randomX = (GetRandom() % BOARD_WIDTH);
			int randomY = (GetRandom() % BOARD_HEIGHT);
			
			if (!pGame->m_mine[randomX][randomY] && (avoidX != randomX || avoidY != randomY))
			{
				pGame->m_mine[randomX][randomY] = true;
				break;
			}
		}
		
		nMines--;
	}
}

void InstantiateNewGame(Minesweeper *pGame)
{
	memset (pGame, 0, sizeof *pGame);
}

bool WidgetSweeper_OnEvent(UNUSED Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			this->m_dataPtr = MmAllocate (sizeof (Minesweeper));
			
			Minesweeper *pGame = (Minesweeper*)this->m_dataPtr;
			
			InstantiateNewGame(pGame);
			
			break;
		}
		case EVENT_DESTROY:
		{
			if (this->m_dataPtr)
				MmFree(this->m_dataPtr);
			
			this->m_dataPtr = NULL;
			break;
		}
		case EVENT_PAINT:
		{
			Minesweeper *pGame = (Minesweeper*)this->m_dataPtr;
			for (int y = 0; y < BOARD_HEIGHT; y++)
			{
				for (int x = 0; x < BOARD_WIDTH; x++)
				{
					MineDrawTile(pGame, x, y, this->m_rect.left + x * 16, this->m_rect.top + y * 16);
				}
			}
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			Minesweeper *pGame = (Minesweeper*)this->m_dataPtr;
			if (pGame->m_gameOver) break;
			
			for (int y = 0; y < BOARD_HEIGHT; y++)
			{
				for (int x = 0; x < BOARD_WIDTH; x++)
				{
					pGame->m_clck[x][y] = false;
				}
			}
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			if (xcl < 0 || ycl < 0 || xcl >= BOARD_WIDTH || ycl >= BOARD_HEIGHT) return false;
			
			pGame->m_clck[xcl][ycl] = true;
			
			WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			
			break;
		}
		case EVENT_RIGHTCLICKRELEASE:
		{
			Minesweeper *pGame = (Minesweeper*)this->m_dataPtr;
			
			for (int y = 0; y < BOARD_HEIGHT; y++)
			{
				for (int x = 0; x < BOARD_WIDTH; x++)
				{
					pGame->m_clck[x][y] = false;
				}
			}
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			MineFlagTile (pGame, xcl, ycl);
			
			WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Minesweeper *pGame = (Minesweeper*)this->m_dataPtr;
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			MineUncoverTile (pGame, xcl, ycl);
			
			WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			
			break;
		}
	}
	return false;
}

void CALLBACK PrgMineProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle rect;
			
			RECT(rect, 15, 96, BOARD_WIDTH*16, BOARD_HEIGHT*16);
			
			AddControl (pWindow, CONTROL_NONE, rect, NULL, 1000, 0, 0);
			
			SetWidgetEventHandler (pWindow, 1000, WidgetSweeper_OnEvent);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void PrgMineTask (__attribute__((unused)) int argument)
{
	// Full window size: 170x259
	// Client area size: 164x227
	// Game   area size: 164x208
	// HUD area size:    150x37
	// Game FBoard size: 150x150
	// Game  Tiles size: 144x144 (each tile is 16x16) -- Board is 9x9
	
	// create ourself a window:
	Window* pWindow = CreateWindow ("Minesweeper", CW_AUTOPOSITION, CW_AUTOPOSITION, MINESW_WIDTH, MINESW_HEIGHT, PrgMineProc, 0);
	pWindow->m_iconID = ICON_BOMB;
	
	if (!pWindow)
	{
		DebugLogMsg("Hey, the window couldn't be created");
		return;
	}
	
	// setup:
	//ShowWindow(pWindow);
	
	// event loop:
#if THREADING_ENABLED
	while (HandleMessages (pWindow));
#endif
}
