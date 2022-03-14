/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       Scribble Application module
******************************************/

#include <wbuiltin.h>

//#define BOARD_WIDTH  9
//#define BOARD_HEIGHT 9
#define BOARD_WIDTH  20
#define BOARD_HEIGHT 15

#define MINE_NUMBER  15

#define MINESW_WIDTH  ((BOARD_WIDTH  * 16) + 26)
#define MINESW_HEIGHT ((BOARD_HEIGHT * 16) + 78 + 11 + TITLE_BAR_HEIGHT)

#define HUD_TOP  50
#define HUD_LEFT 12

typedef struct
{
	bool m_gameOver, m_firstClickDone, m_gameWon;
	bool m_leftClickHeld;
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
bool IsFlag (Minesweeper* pGame, int x, int y)
{
	if (x < 0 || y < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) return false;
	
	return pGame->m_flag[x][y];
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
		if (pGame->m_clck[tileX][tileY] && !pGame->m_flag[tileX][tileY])
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
				//rect.left ++, rect.top ++;
				rect.left ++, rect.top ++;
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
	if (pGame->m_gameOver || pGame->m_gameWon) return;
	
	if (x < 0 || y < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) return;
	
	if (!pGame->m_firstClickDone)
		return;
	
	if (pGame->m_unco[x][y])
		return;
	
	pGame->m_flag[x][y] ^= 1;
}
	
void MineUncoverTile(Minesweeper* pGame, int x, int y)
{
	if (pGame->m_gameOver || pGame->m_gameWon) return;
	
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

void MineCheckWin(Minesweeper *pGame)
{
	bool game_win = true;
	for (int my = 0; my < BOARD_HEIGHT; my++)
	{
		for (int mx = 0; mx < BOARD_WIDTH; mx++)
		{
			if (pGame->m_unco[mx][my] == pGame->m_mine[mx][my])
			{
				SLogMsg("Tile at %d %d different (%b %b)", mx,my,pGame->m_unco[mx][my] , pGame->m_mine[mx][my]);
				game_win = false;
				break;
			}
		}
	}
	
	if (!game_win) return;
	
	pGame->m_gameWon = true;
	
	for (int my = 0; my < BOARD_HEIGHT; my++)
	{
		for (int mx = 0; mx < BOARD_WIDTH; mx++)
		{
			pGame->m_flag[mx][my] = pGame->m_mine[mx][my];
		}
	}
}

void InstantiateNewGame(Minesweeper *pGame)
{
	memset (pGame, 0, sizeof *pGame);
}

// TODO: Improve the "cool move" thing :^)

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
			if (pGame->m_gameOver || pGame->m_gameWon) break;
			
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
			
			pGame->m_leftClickHeld  = true;
			
			
			SetLabelText(pWindow, 2001, ":-O");
			
			//WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
			break;
		}
		case EVENT_RIGHTCLICK:
		{
			Minesweeper *pGame = (Minesweeper*)this->m_dataPtr;
			if (pGame->m_gameOver || pGame->m_gameWon) break;
			
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
			
			if (pGame->m_leftClickHeld)
			{
				for (int x1 = xcl-1; x1 <= xcl+1; x1++)
					for (int y1 = ycl-1; y1 <= ycl+1; y1++)
						if (x1 >= 0 && y1 >= 0 && x1 < BOARD_WIDTH && y1 < BOARD_HEIGHT)
							pGame->m_clck[x1][y1] = true;
			
				//WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
				CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			}
			
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
			
			if (xcl < 0 || ycl < 0 || xcl >= BOARD_WIDTH || ycl >= BOARD_HEIGHT) return false;
			
			if (pGame->m_leftClickHeld)
			{
				// Check mines that're around this tile
				int mines_around = 0, flags_around = 0;
				
				for (int x1 = xcl-1; x1 <= xcl+1; x1++)
					for (int y1 = ycl-1; y1 <= ycl+1; y1++)
					{
						if (IsMine (pGame, x1, y1)) {
							mines_around++;
						}
						if (IsFlag (pGame, x1, y1)) {
							flags_around++;
						}
					}
				
				if (mines_around == flags_around)
				{
					// We "know" all the mine tiles that are around this empty tile; uncover all the unflagged tiles
					for (int x1 = xcl-1; x1 <= xcl+1; x1++)
						for (int y1 = ycl-1; y1 <= ycl+1; y1++)
						{
							if (!IsFlag (pGame, x1, y1))
								MineUncoverTile(pGame, x1, y1);
						}
				}
			}
			else
				MineFlagTile (pGame, xcl, ycl);
			
			MineCheckWin (pGame);
			
			if (pGame->m_gameOver)
			{
				SetLabelText(pWindow, 2001, "X-(");
			}
			else if (pGame->m_gameWon)
			{
				SetLabelText(pWindow, 2001, "B-)");
			}
			else
			{
				SetLabelText(pWindow, 2001, ":-)");
			}
			
			//WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
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
			
			if (xcl < 0 || ycl < 0 || xcl >= BOARD_WIDTH || ycl >= BOARD_HEIGHT) return false;
			
			MineUncoverTile (pGame, xcl, ycl);
			
			pGame->m_leftClickHeld  = false;
			
			
			MineCheckWin (pGame);
			
			if (pGame->m_gameOver)
			{
				SetLabelText(pWindow, 2001, "X-(");
			}
			else if (pGame->m_gameWon)
			{
				SetLabelText(pWindow, 2001, "B-)");
			}
			else
			{
				SetLabelText(pWindow, 2001, ":-)");
			}
			
			//WidgetSweeper_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			CallWindowCallbackAndControls(pWindow, EVENT_PAINT, 0, 0);
			
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
			
			// Add a menu bar
			RECT(rect, 0, 0, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR, rect, NULL, 2000, 0, 0);
			
			AddMenuBarItem (pWindow, 2000, 0, 1, "Game");
			AddMenuBarItem (pWindow, 2000, 1, 2, "New");
			AddMenuBarItem (pWindow, 2000, 1, 3, "");
			AddMenuBarItem (pWindow, 2000, 1, 4, "Exit");
			
			RECT(rect, HUD_LEFT + ((MINESW_WIDTH - 24) - 26) / 2/*74*/, 56, 26, 26);
			AddControl (pWindow, CONTROL_BUTTON, rect, ":-)", 2001, 0, 0);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == 2000) switch (parm2)
			{
				case 2:
					// New Game
					
					// A side effect of SetWidgetEventHandler is that it actually calls EVENT_DESTROY
					// with the old handler, and EVENT_CREATE with the new one.  Nifty if you want
					// to reset a control's defaults
					SetWidgetEventHandler (pWindow, 1000, WidgetSweeper_OnEvent);
					
					SetLabelText(pWindow, 2001, ":-)");
					
					CallWindowCallbackAndControls (pWindow, EVENT_PAINT, 0, 0);
					break;
				case 4:
					// Exit
					PrgMineProc (pWindow, EVENT_CLOSE, 0, 0);
					break;
			}
			else switch (parm1)
			{
				case 2001:
				{
					SetWidgetEventHandler (pWindow, 1000, WidgetSweeper_OnEvent);
					
					SetLabelText(pWindow, 2001, ":-)");
					
					CallWindowCallbackAndControls (pWindow, EVENT_PAINT, 0, 0);
					break;
				}
			}
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
