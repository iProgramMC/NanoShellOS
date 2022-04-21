//game.c
#include <nsstandard.h>

//include sprites
#include "game_sprites.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

//forward declares
bool IsKeyDown (int keyCode);

int fc = 0;


int playerPosX = 0, playerPosY = 0;

const char* GetGameName()
{
	return "NanoShell Gaming";
}
void Init()
{
	playerPosX = SCREEN_WIDTH / 2;
	playerPosY = SCREEN_HEIGHT / 2;
}
void DisplayDebug(int deltaTime)
{
	VidFillRect(0x000000, 0,0,100, 30);
	
	char buf [50];
	sprintf(buf,"DT: %d  FC: %d\nPX: %d  PY: %d", deltaTime, fc++, playerPosX, playerPosY);
	
	VidTextOut(buf, 1, 1, 0xFFFFFF, TRANSPARENT);
}

void Update (int deltaTime)
{
	if (IsKeyDown(KEY_W))
	{
		playerPosY -= deltaTime/4;
	}
	if (IsKeyDown(KEY_S))
	{
		playerPosY += deltaTime/4;
	}
	if (IsKeyDown(KEY_A))
	{
		playerPosX -= deltaTime/4;
	}
	if (IsKeyDown(KEY_D))
	{
		playerPosX += deltaTime/4;
	}
	/*
	if (playerPosX < 0) playerPosX = 0;
	if (playerPosY < 0) playerPosY = 0;
	if (playerPosX >= SCREEN_WIDTH -32) playerPosX = SCREEN_WIDTH -32;
	if (playerPosY >= SCREEN_HEIGHT-32) playerPosY = SCREEN_HEIGHT-32;
	*/
}
void Render (int deltaTime)
{
	//VidFillScreen (0x81CBFF); //clear screen
	
	for (int y = 0; y < SCREEN_HEIGHT; y += 32)
	{
		for (int x = 0; x < SCREEN_WIDTH; x += 32)
		{
			VidBlitImage(&g_sand_icon, x, y);
		}
	}
	
	
	VidBlitImage (&g_idle_icon, playerPosX, playerPosY);
	
	
	
	// show FPS
	DisplayDebug(deltaTime);
}