/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
          Minesweeper application

             Main source file
******************************************/
#include <nsstandard.h>

void SetNumber(Window* win, int cid, int num)
{
	SetIcon(win, cid, num);
	CallControlCallback(win, cid, EVENT_PAINT, 0, 0);
}

bool IsTimerRunning();
void StartTimer();
void TickTimer();
void EndTimer();
void ClearTimer();

//#define BOARD_MAX_WIDTH  9
//#define BoardHeight 9

int BoardWidth = 9, BoardHeight = 9; //can customize easily
int numberMines = 10;

Window* g_pWindow = NULL;

enum
{
	COMBO_SWEEPER = 1000,
	COMBO_MENUBAR,
	COMBO_BUTTON,
	COMBO_LED_TIME,
	COMBO_LED_SCORE,
};

enum
{
	RES_LCD = 1000,
	RES_NUMBERS,
	ICON_SMILEY,
	ICON_WON,
	ICON_CLICK,
	ICON_DEAD,
};

enum
{
	NUM_FLAG = 9,
	NUM_BOMB,
	NUM_MAX,
};

enum
{
	LCD_BLANK = 10,
	LCD_DASH,
	LCD_MAX,
};

int minesweeperX = CW_AUTOPOSITION, minesweeperY = CW_AUTOPOSITION;

Image *g_pLcdImage, *g_pNumbersImage;

int g_tileSize = 18;
int g_nNowMines = 0, g_nTimer = 0;

#define TILE_SIZE (g_tileSize)

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

#define MINESW_WIDTH  ((BoardWidth  * TILE_SIZE) + 16)
#define MINESW_HEIGHT ((BoardHeight * TILE_SIZE) + 54 + MENU_BAR_HEIGHT)

#define HUD_TOP    (MENU_BAR_HEIGHT + 8)
#define HUD_LEFT   (8)
#define HUD_HEIGHT (26)

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

void RenderLCDNumber(int x, int y, int num)
{
	Image img;
	img.width   = g_pLcdImage->width;
	img.height  = g_pLcdImage->height;
	
	int numHeight = img.height / LCD_MAX;
	
	img.framebuffer = &g_pLcdImage->framebuffer[g_pLcdImage->width * numHeight * num];
	img.height = numHeight;
	
	VidBlitImage(&img, x, y);
}

void RenderNumber(int x, int y, int num)
{
	if (num == 0) return;
	num--;
	
	Image img;
	img.width   = g_pNumbersImage->width;
	img.height  = g_pNumbersImage->height;
	
	int numHeight = img.height / (NUM_MAX - 1);
	
	x += (TILE_SIZE - img.width) / 2;
	y += (TILE_SIZE - numHeight) / 2;
	
	img.framebuffer = &g_pNumbersImage->framebuffer[g_pNumbersImage->width * numHeight * num];
	img.height = numHeight;
	
	VidBlitImage(&img, x, y);
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
			RECT(rect, drawX, drawY, TILE_SIZE - 1, TILE_SIZE - 1);
			VidFillRectangle (0xC0C0C0, rect);
			VidDrawRectangle (0x808080, rect);
		}
		else
		{
			RECT(rect, drawX, drawY, TILE_SIZE, TILE_SIZE);
			//RenderButtonShapeNoRounding(rect, 0x808080, 0xFFFFFF, 0xC0C0C0);
			DrawEdge(rect, DRE_RAISED | DRE_FILLED, BUTTON_MIDDLE_COLOR);
			
			if (m_flag[tileX][tileY])
			{
				RenderNumber(drawX, drawY, NUM_FLAG);
			}
		}
	}
	else
	{
		RECT(rect, drawX, drawY, TILE_SIZE - 1, TILE_SIZE - 1);
		
		//Is there a bomb?
		if (IsMine (tileX, tileY))
		{
			// Yes.  Draw an icon
			VidFillRectangle (0xFF0000, rect);
			VidDrawRectangle (0x808080, rect);
			
			RenderNumber(drawX, drawY, NUM_BOMB);
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
			
			VidFillRectangle (WINDOW_BACKGD_COLOR, rect);
			VidDrawRectangle (BUTTON_SHADOW_COLOR, rect);
			
			if (mines_around)
			{
				RenderNumber(drawX, drawY, mines_around);
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
	
	if (!m_flag[x][y]) g_nNowMines++;
	else               g_nNowMines--;
	SetNumber(g_pWindow, COMBO_LED_SCORE, g_nNowMines);
	
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
	
	if (!IsTimerRunning())
		StartTimer();
	
	if (m_flag[x][y])
		return;
	
	if (m_unco[x][y])
		return;
	
	m_unco[x][y] = m_upda[x][y] = true;
	
	if (m_mine[x][y])
	{
		m_gameOver = true;
		
		EndTimer();
		
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
	
	g_nNowMines = nMines;
	SetNumber(g_pWindow, COMBO_LED_SCORE, nMines);
	
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
	if (IsTimerRunning())
		EndTimer();
	
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
	EndTimer();
	ClearTimer();
	
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
				MineDrawTile(x, y, this->m_rect.left + x * TILE_SIZE, this->m_rect.top + y * TILE_SIZE);
			}
		}
	}
}

bool WidgetNumDisplay_OnEvent(Control* this, int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
{
	if (eventType != EVENT_PAINT) return false;
	
	int num = this->m_parm1;
	
	//RenderButtonShapeNoRounding(this->m_rect, 0xFFFFFF, 0x808080, TRANSPARENT);
	DrawEdge(this->m_rect, DRE_SUNKEN, TRANSPARENT);
	
	int digits[3];
	
	if (num < 0)
	{
		digits[0] = LCD_DASH;
		num = abs(num);
		digits[1] = num /  10 % 10;
		digits[2] = num       % 10;
	}
	else
	{
		digits[0] = num / 100 % 10;
		digits[1] = num /  10 % 10;
		digits[2] = num       % 10;
	}
	
	for (int i = 0; i < (int)ARRAY_COUNT(digits); i++)
		RenderLCDNumber(this->m_rect.left + 2 + i * g_pLcdImage->width, this->m_rect.top + 2, digits[i]);
	
	return false;
}

bool g_bRightClicking = false;

void MinePerformChord(int xcl, int ycl)
{
	// Check mines that are around this tile
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

int g_lastClickedTick = 0;

bool WidgetSweeper_OnEvent(Control* this, UNUSED int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
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
			
			if (g_bRightClicking)
			{
				m_leftClickHeld = true;
				break; // don't do anything. Let right click handle it.
			}
			
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
			
			if (xcl < 0 || ycl < 0) return false;
			
			xcl /= TILE_SIZE;
			ycl /= TILE_SIZE;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			m_clck[xcl][ycl] = true;
			m_upda[xcl][ycl] = true;
			
			m_leftClickHeld  = true;
			
			SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_CLICK);
			CallControlCallback(pWindow, COMBO_BUTTON, EVENT_PAINT, 0, 0);
			
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
			
			if (xcl < 0 || ycl < 0) return false;
			
			xcl /= TILE_SIZE;
			ycl /= TILE_SIZE;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			g_bRightClicking = true;
			
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
			
			g_bRightClicking = false;
			
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			if (xcl < 0 || ycl < 0) return false;
			
			xcl /= TILE_SIZE;
			ycl /= TILE_SIZE;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			if (m_leftClickHeld)
				MinePerformChord(xcl, ycl);
			else
				MineFlagTile (xcl, ycl);
			
			MineCheckWin ();
			
			if (m_gameOver)
			{
				SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_DEAD);
			}
			else if (m_gameWon)
			{
				SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_CARET);
			}
			else
			{
				SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_SMILE);
			}
			
			CallControlCallback(pWindow, COMBO_BUTTON, EVENT_PAINT, 0, 0);
			WidgetSweeperPaint(this);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			int xcl = GET_X_PARM(parm1);
			int ycl = GET_Y_PARM(parm1);
			
			m_leftClickHeld  = false;
			
			xcl -= this->m_rect.left;
			ycl -= this->m_rect.top;
			
			if (xcl < 0 || ycl < 0) return false;
			
			xcl /= TILE_SIZE;
			ycl /= TILE_SIZE;
			
			if (xcl < 0 || ycl < 0 || xcl >= BoardWidth || ycl >= BoardHeight) return false;
			
			if (m_unco[xcl][ycl])
			{
				// have to have clicked before in X ms to count as a double click
				if (g_lastClickedTick + 500 >= GetTickCount())
					MinePerformChord(xcl, ycl);
				
				g_lastClickedTick = GetTickCount();
			}
			else
			{
				MineUncoverTile (xcl, ycl);
			}
			
			MineCheckWin ();
			
			if (m_gameOver)
			{
				SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_DEAD);
			}
			else if (m_gameWon)
			{
				SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_CARET);
			}
			else
			{
				SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_SMILE);
			}
			
			CallControlCallback(pWindow, COMBO_BUTTON, EVENT_PAINT, 0, 0);
			WidgetSweeperPaint(this);
			
			break;
		}
	}
	return false;
}

int g_TimerID = -1;

bool IsTimerRunning()
{
	return  g_TimerID >= 0;
}

void StartTimer()
{
	if (g_TimerID >= 0) EndTimer();
	g_TimerID = AddTimer(g_pWindow, 1000, EVENT_USER);
	g_nTimer = -1;
	SetNumber(g_pWindow, COMBO_LED_TIME, 0);
}

void TickTimer()
{
	if (!IsTimerRunning())
	{
		LogMsg("Why are we ticking?");
		return;
	}
	
	g_nTimer++;
	if (g_nTimer > 999) g_nTimer = 999;
	
	SetNumber(g_pWindow, COMBO_LED_TIME, g_nTimer);
}

void EndTimer()
{
	if (!IsTimerRunning()) return;
	DisarmTimer(g_pWindow, g_TimerID);
	g_TimerID = -1;
}

void ClearTimer()
{
	g_nTimer = 0;
	SetNumber(g_pWindow, COMBO_LED_TIME, g_nTimer);
}

void OnBossKey()
{
	if (GetWindowFlags(g_pWindow) & WF_MINIMIZE)
		return;
	
	// Your boss just showed up? Press escape to minimize the game and
	// rename it to look inconspicuous. I mean, your boss doesn't have
	// "fool" written on his head, but still!
	
	SetWindowTitle(g_pWindow, "Not A Game");
	SetWindowIcon(g_pWindow, ICON_JOURNAL);
	RegisterEvent(g_pWindow, EVENT_MINIMIZE, 0, 0);
}

void CALLBACK PrgMineProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_USER:
		{
			TickTimer();
			break;
		}
		case EVENT_UNMINIMIZE:
		{
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			
			// undo the effect of boss key
			SetWindowTitle(g_pWindow, "Minesweeper");
			SetWindowIcon(g_pWindow, ICON_BOMB);
			break;
		}
		case EVENT_KEYRAW:
		{
			if (parm1 == KEY_ESC)
			{
				OnBossKey();
			}
			break;
		}
		case EVENT_CREATE:
		{
			g_pLcdImage     = GetImage(GetResource(RES_LCD));
			g_pNumbersImage = GetImage(GetResource(RES_NUMBERS));
			
			Rectangle rect;
			
			RECT(rect, HUD_LEFT, HUD_TOP + HUD_HEIGHT + MENU_BAR_HEIGHT, BoardWidth*TILE_SIZE, BoardHeight*TILE_SIZE);
			
			AddControl (pWindow, CONTROL_NONE, rect, NULL, COMBO_SWEEPER, 0, 0);
			
			SetWidgetEventHandler (pWindow, COMBO_SWEEPER, WidgetSweeper_OnEvent);
			
			// Add a menu bar
			RECT(rect, 0, 0, 0, 0);
			AddControl (pWindow, CONTROL_MENUBAR, rect, NULL, COMBO_MENUBAR, 0, 0);
			
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 0, 1, "Game");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 0, 2, "Help");
			
			// help menu
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 2, 3, "About Minesweeper...");
			
			// game menu
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1, 4, "New Game");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1, 5, "");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1, 6, "Beginner");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1, 7, "Novice");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1, 8, "Intermediate");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1, 9, "Expert");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1,10, "");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1,14, "Small Tiles");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1,15, "Big Tiles");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1,16, "Huge Tiles");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1,12, "");
			AddMenuBarItem (pWindow, COMBO_MENUBAR, 1,11, "Exit");
			
			RECT(rect, HUD_LEFT + ((MINESW_WIDTH - HUD_LEFT * 2) - HUD_HEIGHT) / 2, HUD_TOP + ((HUD_HEIGHT - (g_pLcdImage->height / LCD_MAX)) / 2), HUD_HEIGHT, HUD_HEIGHT);
			AddControl (pWindow, CONTROL_BUTTON_ICON, rect, "", COMBO_BUTTON, ICON_SWEEP_SMILE, 17);
			
			int dispWidth =  g_pLcdImage->width * 3 + 4, dispHeight = g_pLcdImage->height / LCD_MAX + 4;
			
			RECT(rect, HUD_LEFT, HUD_TOP, dispWidth, dispHeight);
			AddControl(pWindow, CONTROL_NONE, rect, "", COMBO_LED_SCORE, 0, 0);
			SetWidgetEventHandler(pWindow, COMBO_LED_SCORE, WidgetNumDisplay_OnEvent);
			RECT(rect, MINESW_WIDTH - HUD_LEFT - dispWidth, HUD_TOP, dispWidth, dispHeight);
			AddControl(pWindow, CONTROL_NONE, rect, "", COMBO_LED_TIME, 0, 0);
			SetWidgetEventHandler(pWindow, COMBO_LED_TIME, WidgetNumDisplay_OnEvent);
			
			break;
		}
		case EVENT_COMMAND:
		{
			Rectangle rect;
			GetWindowRect(pWindow, &rect);
			
			if (parm1 == COMBO_MENUBAR) switch (parm2)
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
					SetWidgetEventHandler (pWindow, COMBO_SWEEPER, WidgetSweeper_OnEvent);
					
					SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_SMILE);
					CallControlCallback(pWindow, COMBO_BUTTON, EVENT_PAINT, 0, 0);
					CallControlCallback(pWindow, COMBO_SWEEPER, EVENT_PAINT, 0, 0);
					
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
					minesweeperX = rect.left;
					minesweeperY = rect.top;
					DestroyWindow (pWindow);
					break;
				case 7: // "Novice"
					BoardWidth  = NOVICE_BOARD_WIDTH;
					BoardHeight = NOVICE_BOARD_HEIGHT;
					numberMines = NOVICE_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = rect.left;
					minesweeperY = rect.top;
					DestroyWindow (pWindow);
					break;
				case 8: // "Intermediate"
					BoardWidth  = INTERMEDIATE_BOARD_WIDTH;
					BoardHeight = INTERMEDIATE_BOARD_HEIGHT;
					numberMines = INTERMEDIATE_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = rect.left;
					minesweeperY = rect.top;
					DestroyWindow (pWindow);
					break;
				case 9: // "Expert"
					BoardWidth  = EXPERT_BOARD_WIDTH;
					BoardHeight = EXPERT_BOARD_HEIGHT;
					numberMines = EXPERT_NUMBER_MINES;
					//If we destroy the window after this flag gets set then a new window will come in its place!
					bRequestingNewSize = true;
					minesweeperX = rect.left;
					minesweeperY = rect.top;
					DestroyWindow (pWindow);
					break;
					
				case 14:
				case 15:
				case 16:
				{
					static const int tileSizes[] = { 18, 28, 38 };
					g_tileSize = tileSizes[parm2 - 14];
					bRequestingNewSize = true;
					minesweeperX = rect.left;
					minesweeperY = rect.top;
					DestroyWindow (pWindow);
					break;
				}
			}
			else switch (parm1)
			{
				case COMBO_BUTTON:
				{
					SetWidgetEventHandler (pWindow, COMBO_SWEEPER, WidgetSweeper_OnEvent);
					
					SetIcon (pWindow, COMBO_BUTTON, ICON_SWEEP_SMILE);
					CallControlCallback(pWindow, COMBO_BUTTON, EVENT_PAINT, 0, 0);
					CallControlCallback(pWindow, COMBO_SWEEPER, EVENT_PAINT, 0, 0);
					break;
				}
			}
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

int main()
{
	// create ourself a window:
	do {
		bRequestingNewSize = false;
		
		Window* pWindow = g_pWindow = CreateWindow ("Minesweeper", minesweeperX, minesweeperY, MINESW_WIDTH, MINESW_HEIGHT, PrgMineProc, 0);
		SetWindowIcon (pWindow, ICON_BOMB_SPIKEY);
		
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
