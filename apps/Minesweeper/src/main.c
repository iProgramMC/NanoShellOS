/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
          Minesweeper application

             Main source file
******************************************/
#include <nsstandard.h>

//#define BOARD_MAX_WIDTH  9
//#define BoardHeight 9

int BoardWidth = 9, BoardHeight = 9; //can customize easily
int numberMines = 10;

int minesweeperX = CW_AUTOPOSITION, minesweeperY = CW_AUTOPOSITION;

#define BEGINNER_BOARD_WIDTH  9
#define BEGINNER_BOARD_HEIGHT 9
#define BEGINNER_NUMBER_MINES 10

#define NOVICE_BOARD_WIDTH  12
#define NOVICE_BOARD_HEIGHT 12
#define NOVICE_NUMBER_MINES 20

#define INTERMEDIATE_BOARD_WIDTH  16
#define INTERMEDIATE_BOARD_HEIGHT 16
#define INTERMEDIATE_NUMBER_MINES 40

#define EXPERT_BOARD_WIDTH  24
#define EXPERT_BOARD_HEIGHT 24
#define EXPERT_NUMBER_MINES 99

#define BOARD_MAX_WIDTH  30
#define BOARD_MAX_HEIGHT 30

#define MINE_NUMBER  15

#define MINESW_WIDTH  ((BoardWidth  * 16) + 16)
#define MINESW_HEIGHT ((BoardHeight * 16) + 91)

#define HUD_TOP  50
#define HUD_LEFT 8

#define MENU_BAR_HEIGHT TITLE_BAR_HEIGHT - 3

bool bRequestingNewSize = false;

bool m_gameOver, m_firstClickDone, m_gameWon;
bool m_leftClickHeld;
bool m_mine[BOARD_MAX_WIDTH][BOARD_MAX_HEIGHT];
bool m_unco[BOARD_MAX_WIDTH][BOARD_MAX_HEIGHT];
bool m_flag[BOARD_MAX_WIDTH][BOARD_MAX_HEIGHT];
bool m_clck[BOARD_MAX_WIDTH][BOARD_MAX_HEIGHT];
bool m_upda[BOARD_MAX_WIDTH][BOARD_MAX_HEIGHT];

void RenderButtonShapeNoRounding(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle)
{
	//draw some lines
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink
	rect.left++, rect.right--, rect.top++, rect.bottom--;
	
	//do the same
	VidDrawVLine (colorLight, rect.top,   rect.bottom-1,   rect.left);
	VidDrawVLine (colorDark,  rect.top,   rect.bottom-1,   rect.right  - 1);
	VidDrawHLine (colorDark,  rect.left,  rect.right -1,   rect.bottom - 1);
	VidDrawHLine (colorLight, rect.left,  rect.right -1,   rect.top);
	
	//shrink again
	rect.left++, rect.right -= 2, rect.top++, rect.bottom -= 2;
	
	//fill the background:
	if (colorMiddle != TRANSPARENT)
		VidFillRectangle(colorMiddle, rect);
}

bool IsMine (int x, int y)
{
	if (x < 0 || y < 0 || x >= BoardWidth || y >= BoardHeight) return false;
	
	return m_mine[x][y];
}
bool IsFlag (int x, int y)
{
	if (x < 0 || y < 0 || x >= BoardWidth || y >= BoardHeight) return false;
	
	return m_flag[x][y];
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

void MineDrawTile(int tileX, int tileY, int drawX, int drawY)
{
	Rectangle rect;
	
	if (!m_unco[tileX][tileY])
	{
		if (m_clck[tileX][tileY] && !m_flag[tileX][tileY])
		{
			RECT(rect, drawX, drawY, 15, 15);
			VidFillRectangle (0xC0C0C0, rect);
			VidDrawRectangle (0x808080, rect);
		}
		else
		{
			RECT(rect, drawX, drawY, 16, 16);
			RenderButtonShapeNoRounding(rect, 0x808080, 0xFFFFFF, 0xC0C0C0);
			
			if (m_flag[tileX][tileY])
			{
				RenderIconForceSize(ICON_NANOSHELL_LETTERS16, drawX, drawY, 16);
				
				//VidFillRect(0xFF0000, drawX+3, drawY+3, drawX+13, drawY+13);
			}
		}
	}
	else
	{
		RECT(rect, drawX, drawY, 15, 15);
		
		//Is there a bomb?
		if (IsMine (tileX, tileY))
		{
			// Yes.  Draw an icon
			VidFillRectangle (0xFF0000, rect);
			VidDrawRectangle (0x808080, rect);
			
			RenderIconForceSize(ICON_BOMB, drawX, drawY, 16);
				
			//VidFillRect(0x000000, drawX+3, drawY+3, drawX+13, drawY+13);
		}
		else
		{
			// Count how many mines are around this tile.
			int mines_around = 
				IsMine(tileX - 1, tileY - 1) + 
				IsMine(tileX + 0, tileY - 1) + 
				IsMine(tileX + 1, tileY - 1) + 
				IsMine(tileX - 1, tileY + 0) + 
				IsMine(tileX + 1, tileY + 0) + 
				IsMine(tileX - 1, tileY + 1) + 
				IsMine(tileX + 0, tileY + 1) + 
				IsMine(tileX + 1, tileY + 1);
			
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

void GenerateMines(int nMines, int avoidX, int avoidY);

void MineFlagTile(int x, int y)
{
	if (m_gameOver || m_gameWon) return;
	
	if (x < 0 || y < 0 || x >= BoardWidth || y >= BoardHeight) return;
	
	if (!m_firstClickDone)
		return;
	
	if (m_unco[x][y])
		return;
	
	m_flag[x][y] ^= 1;
	m_upda[x][y] = true;
}
	
void MineUncoverTile(int x, int y)
{
	if (m_gameOver || m_gameWon) return;
	
	if (x < 0 || y < 0 || x >= BoardWidth || y >= BoardHeight) return;
	
	if(!m_firstClickDone)
	{
		m_firstClickDone = true;
		GenerateMines(numberMines, x, y);
	}
	
	if (m_flag[x][y])
		return;
	
	if (m_unco[x][y])
		return;
	
	m_unco[x][y] = m_upda[x][y] = true;
	
	if (m_mine[x][y])
	{
		m_gameOver = true;
		
		// Uncover other mines
		for (int my = 0; my < BoardHeight; my++)
		{
			for (int mx = 0; mx < BoardWidth; mx++)
			{
				if (m_mine[mx][my])
					m_unco[mx][my] = m_upda[mx][my] = true;
			}
		}
		return;
	}
	
	int mines_around = 
		IsMine(x - 1, y - 1) + 
		IsMine(x + 0, y - 1) + 
		IsMine(x + 1, y - 1) + 
		IsMine(x - 1, y + 0) + 
		IsMine(x + 1, y + 0) + 
		IsMine(x - 1, y + 1) + 
		IsMine(x + 0, y + 1) + 
		IsMine(x + 1, y + 1);
	
	// If this tile is fully empty, also uncover the tiles around it:
	if (!mines_around)
	{
		MineUncoverTile(x - 1, y - 1);
		MineUncoverTile(x + 0, y - 1);
		MineUncoverTile(x + 1, y - 1);
		MineUncoverTile(x - 1, y + 0);
		MineUncoverTile(x + 1, y + 0);
		MineUncoverTile(x - 1, y + 1);
		MineUncoverTile(x + 0, y + 1);
		MineUncoverTile(x + 1, y + 1);
	}
}

void GenerateMines(int nMines, int avoidX, int avoidY)
{
	if (nMines > BoardWidth * BoardHeight - 1)
		nMines = BoardWidth * BoardHeight - 1;
	
	while (nMines)
	{
		while (true)
		{
			int randomX = (GetRandom() % BoardWidth);
			int randomY = (GetRandom() % BoardHeight);
			
			if (!m_mine[randomX][randomY] && (avoidX != randomX || avoidY != randomY))
			{
				m_mine[randomX][randomY] = true;
				break;
			}
		}
		
		nMines--;
	}
}

void MineCheckWin()
{
	bool game_win = true;
	for (int my = 0; my < BoardHeight; my++)
	{
		for (int mx = 0; mx < BoardWidth; mx++)
		{
			if (m_unco[mx][my] == m_mine[mx][my])
			{
				game_win = false;
				break;
			}
		}
	}
	
	if (!game_win) return;
	
	m_gameWon = true;
	
	for (int my = 0; my < BoardHeight; my++)
	{
		for (int mx = 0; mx < BoardWidth; mx++)
		{
			m_flag[mx][my] = m_mine[mx][my];
			m_upda[mx][my] = true;
		}
	}
}

void InstantiateNewGame()
{
	m_gameOver = m_firstClickDone = m_gameWon = m_leftClickHeld = false;
	
	memset (m_mine, 0, sizeof m_mine);
	memset (m_unco, 0, sizeof m_unco);
	memset (m_flag, 0, sizeof m_flag);
	memset (m_clck, 0, sizeof m_clck);
	memset (m_upda, 1, sizeof m_upda);
}

// TODO: Improve the "cool move" thing :^)

void WidgetSweeperPaint(Control* this)
{
	for (int y = 0; y < BoardHeight; y++)
	{
		for (int x = 0; x < BoardWidth; x++)
		{
			if (m_upda[x][y])
			{
				m_upda[x][y] = 0;
				MineDrawTile(x, y, this->m_rect.left + x * 16, this->m_rect.top + y * 16);
			}
		}
	}
}

bool WidgetSweeper_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			InstantiateNewGame();
			break;
		}
		case EVENT_PAINT:
		{
			for (int y = 0; y < BoardHeight; y++)
			{
				for (int x = 0; x < BoardWidth; x++)
				{
					m_upda[x][y] = 1;
				}
			}
			
			WidgetSweeperPaint(this);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			if (m_gameOver || m_gameWon) break;
			
			for (int y = 0; y < BoardHeight; y++)
			{
				for (int x = 0; x < BoardWidth; x++)
				{
					if (m_clck[x][y])
						m_upda[x][y] = true;
					
					m_clck[x][y] = false;
				}
			}
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			m_clck[xcl][ycl] = true;
			m_upda[xcl][ycl] = true;
			
			m_leftClickHeld  = true;
			
			SetIcon (pWindow, 2001, ICON_SWEEP_CLICK);
			CallControlCallback(pWindow, 2001, EVENT_PAINT, 0, 0);
			
			WidgetSweeperPaint(this);
			
			break;
		}
		case EVENT_RIGHTCLICK:
		{
			if (m_gameOver || m_gameWon) break;
			
			for (int y = 0; y < BoardHeight; y++)
			{
				for (int x = 0; x < BoardWidth; x++)
				{
					if (m_clck[x][y])
						m_upda[x][y] = true;
					
					m_clck[x][y] = false;
				}
			}
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			if (m_leftClickHeld)
			{
				for (int x1 = xcl-1; x1 <= xcl+1; x1++)
					for (int y1 = ycl-1; y1 <= ycl+1; y1++)
						if (x1 >= 0 && y1 >= 0 && x1 < BoardWidth && y1 < BoardHeight)
						{
							m_upda[x1][y1] = true;
							m_clck[x1][y1] = true;
						}
				
				WidgetSweeperPaint(this);
			}
			
			break;
		}
		case EVENT_RIGHTCLICKRELEASE:
		{
			for (int y = 0; y < BoardHeight; y++)
			{
				for (int x = 0; x < BoardWidth; x++)
				{
					if (m_clck[x][y])
						m_upda[x][y] = true;
					
					m_clck[x][y] = false;
				}
			}
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			if (m_leftClickHeld)
			{
				// Check mines that're around this tile
				int mines_around = 0, flags_around = 0;
				
				for (int x1 = xcl-1; x1 <= xcl+1; x1++)
					for (int y1 = ycl-1; y1 <= ycl+1; y1++)
					{
						if (IsMine (x1, y1)) {
							mines_around++;
						}
						if (IsFlag (x1, y1)) {
							flags_around++;
						}
					}
				
				if (mines_around == flags_around)
				{
					// We "know" all the mine tiles that are around this empty tile; uncover all the unflagged tiles
					for (int x1 = xcl-1; x1 <= xcl+1; x1++)
						for (int y1 = ycl-1; y1 <= ycl+1; y1++)
						{
							if (!IsFlag (x1, y1))
								MineUncoverTile(x1, y1);
						}
				}
			}
			else
				MineFlagTile (xcl, ycl);
			
			MineCheckWin ();
			
			if (m_gameOver)
			{
				SetIcon (pWindow, 2001, ICON_SWEEP_DEAD);
			}
			else if (m_gameWon)
			{
				SetIcon (pWindow, 2001, ICON_SWEEP_CARET);
			}
			else
			{
				SetIcon (pWindow, 2001, ICON_SWEEP_SMILE);
			}
			
			CallControlCallback(pWindow, 2001, EVENT_PAINT, 0, 0);
			WidgetSweeperPaint(this);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			xcl /= 16;
			ycl /= 16;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			MineUncoverTile (xcl, ycl);
			
			m_leftClickHeld  = false;
			
			
			MineCheckWin ();
			
			if (m_gameOver)
			{
				SetIcon (pWindow, 2001, ICON_SWEEP_DEAD);
			}
			else if (m_gameWon)
			{
				SetIcon (pWindow, 2001, ICON_SWEEP_CARET);
			}
			else
			{
				SetIcon (pWindow, 2001, ICON_SWEEP_SMILE);
			}
			
			CallControlCallback(pWindow, 2001, EVENT_PAINT, 0, 0);
			WidgetSweeperPaint(this);
			
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
			
			RECT(rect, HUD_LEFT, 96-18-HUD_LEFT+MENU_BAR_HEIGHT, BoardWidth*16, BoardHeight*16);
			
			AddControl (pWindow, CONTROL_NONE, rect, NULL, 1000, 0, 0);
			
			SetWidgetEventHandler (pWindow, 1000, WidgetSweeper_OnEvent);
			
			// Add a menu bar
			RECT(rect, 0, 0, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR, rect, NULL, 2000, 0, 0);
			
			AddMenuBarItem (pWindow, 2000, 0, 1, "Game");
			AddMenuBarItem (pWindow, 2000, 0, 2, "Help");
			
			// help menu
			AddMenuBarItem (pWindow, 2000, 2, 3, "About Minesweeper...");
			
			// game menu
			AddMenuBarItem (pWindow, 2000, 1, 4, "New Game");
			AddMenuBarItem (pWindow, 2000, 1, 5, "");
			AddMenuBarItem (pWindow, 2000, 1, 6, "Beginner");
			AddMenuBarItem (pWindow, 2000, 1, 7, "Novice");
			AddMenuBarItem (pWindow, 2000, 1, 8, "Intermediate");
			AddMenuBarItem (pWindow, 2000, 1, 9, "Expert");
			AddMenuBarItem (pWindow, 2000, 1,10, "");
			AddMenuBarItem (pWindow, 2000, 1,11, "Exit");
			
			RECT(rect, HUD_LEFT + ((MINESW_WIDTH - HUD_LEFT * 2) - 26) / 2/*74*/, 56-18-15+MENU_BAR_HEIGHT, 26, 26);
			AddControl (pWindow, CONTROL_BUTTON_ICON, rect, "", 2001, ICON_SWEEP_SMILE, 17);
			
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == 2000) switch (parm2)
			{
				case 3:
					// About
					ShellAbout("Minesweeper", ICON_BOMB_SPIKEY);
					break;
				case 4:
					// New Game
					
					// A side effect of SetWidgetEventHandler is that it actually calls EVENT_DESTROY
					// with the old handler, and EVENT_CREATE with the new one.  Nifty if you want
					// to reset a control's defaults
					SetWidgetEventHandler (pWindow, 1000, WidgetSweeper_OnEvent);
					
					SetIcon (pWindow, 2001, ICON_SWEEP_SMILE);
					CallControlCallback(pWindow, 2001, EVENT_PAINT, 0, 0);
					CallControlCallback(pWindow, 1000, EVENT_PAINT, 0, 0);
					
					break;
				case 11:
					// Exit
					PrgMineProc (pWindow, EVENT_CLOSE, 0, 0);
					break;
					
				case 6: // "Beginner"
					BoardWidth  = BEGINNER_BOARD_WIDTH;
					BoardHeight = BEGINNER_BOARD_HEIGHT;
					numberMines = BEGINNER_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = pWindow->m_rect.left;
					minesweeperY = pWindow->m_rect.top;
					DestroyWindow (pWindow);
					break;
				case 7: // "Novice"
					BoardWidth  = NOVICE_BOARD_WIDTH;
					BoardHeight = NOVICE_BOARD_HEIGHT;
					numberMines = NOVICE_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = pWindow->m_rect.left;
					minesweeperY = pWindow->m_rect.top;
					DestroyWindow (pWindow);
					break;
				case 8: // "Intermediate"
					BoardWidth  = INTERMEDIATE_BOARD_WIDTH;
					BoardHeight = INTERMEDIATE_BOARD_HEIGHT;
					numberMines = INTERMEDIATE_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = pWindow->m_rect.left;
					minesweeperY = pWindow->m_rect.top;
					DestroyWindow (pWindow);
					break;
				case 9: // "Expert"
					BoardWidth  = EXPERT_BOARD_WIDTH;
					BoardHeight = EXPERT_BOARD_HEIGHT;
					numberMines = EXPERT_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = pWindow->m_rect.left;
					minesweeperY = pWindow->m_rect.top;
					DestroyWindow (pWindow);
					break;
			}
			else switch (parm1)
			{
				case 2001:
				{
					SetWidgetEventHandler (pWindow, 1000, WidgetSweeper_OnEvent);
					
					SetIcon (pWindow, 2001, ICON_SWEEP_SMILE);
					CallControlCallback(pWindow, 2001, EVENT_PAINT, 0, 0);
					CallControlCallback(pWindow, 1000, EVENT_PAINT, 0, 0);
					break;
				}
			}
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

int NsMain (UNUSED int argc, UNUSED char **argv)
{
	// create ourself a window:
	do {
		bRequestingNewSize = false;
		
		Window* pWindow = CreateWindow ("Minesweeper", minesweeperX, minesweeperY, MINESW_WIDTH, MINESW_HEIGHT-15+MENU_BAR_HEIGHT, PrgMineProc, 0);
		SetWindowIcon (pWindow, ICON_BOMB);
		
		if (!pWindow)
		{
			LogMsg("Hey, the window couldn't be created");
			return 0;
		}
		
		// event loop:
		while (HandleMessages (pWindow));
	} while (bRequestingNewSize);
	
	return 0;
}
