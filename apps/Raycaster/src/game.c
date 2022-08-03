/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
           Raycaster application

          Game logic source file
******************************************/

#include <nsstandard.h>
#include "game.h"

//include sprites

#define ASPECT_RATIO ((double)ScreenWidth / ScreenHeight)

#define MAP_WIDTH  16
#define MAP_HEIGHT 16
const char* GetGameName()
{
	return "Raycaster";
}

// Definitions
#if 1
char map[MAP_WIDTH * MAP_HEIGHT] = 
	"################"
	"#..............#"
	"#..#...#########"
	"#..#...#.......#"
	"#..#...#...#...#"
	"#..#.......#...#"
	"#..........#...#"
	"############...#"
	"#...#....#.....#"
	"#...#....#.....#"
	"#...#....#.....#"
	"#...#....#.....#"
	"#..............#"
	"#..............#"
	"#..............#"
	"################";

double playerX = 12.0, playerY = 11.0, playerAngle = 0.0;
double fov = PI;
double farPlane = MAP_WIDTH * 1.414; // approx. sqrt(2)

//WORK: the bigger this is, the more coarse the result will be, and the faster the game will run.
//If it's too big, it may skip walls
double rayMoveDistance = 0.05;
#endif

// Basic utilities
#if 1
char GetTile (int x, int y)
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return ' ';
	return map[y * MAP_WIDTH + x];
}
bool IsCollidable (char c)
{
	return c != '.';
}
bool IsRender (char c)
{
	return c != '.';
}
#endif

void Normalize(double *x, double *y)
{
	//calculate the distance
	double dist = ((*x) * (*x)) + ((*y) * (*y));
	
	if (dist < 0.0001) //too small to normalize - would divide by zero
	{
		*x = *y = 0;
		return;
	}
	
	dist = sqrt(dist);
	
	*x /= dist;
	*y /= dist;
}

// returns the length of the casted ray
double CastRay (double rayAngle, char *pTileFoundOut)
{
	double distance  = 0.0;
	
	
	double dirX = cos(rayAngle), dirY = sin(rayAngle);
	Normalize (&dirX, &dirY);
	
	double stepX = sqrt(1 + (dirY / dirX) * (dirY / dirX)),
	       stepY = sqrt(1 + (dirX / dirY) * (dirX / dirY));
	
	int    tileX   = (int)playerX,
	       tileY   = (int)playerY; //tile to check
	double raylenX = 0, raylenY = 0;
	int    istepX  = 0, istepY  = 0;
	
	//perform the starting check
	if (dirX < 0)
	{
		istepX  = -1;
		raylenX = (playerX - tileX) * stepX;
	}
	else
	{
		istepX = 1;
		raylenX = (tileX + 1 - playerX) * stepX;
	}
	
	if (dirY < 0)
	{
		istepY = -1;
		raylenY = (playerY - tileY) * stepY;
	}
	else
	{
		istepY = 1;
		raylenY = (tileY + 1 - playerY) * stepY;
	}
	
	char cTileFound = '\0';
	while (!cTileFound && distance < farPlane)
	{
		//perform a walk
		if (raylenX < raylenY)
		{
			tileX += istepX;
			distance = raylenX;
			raylenX += stepX;
		}
		else
		{
			tileY += istepY;
			distance = raylenY;
			raylenY += stepY;
		}
		
		//check if the tile is solid
		if (tileX >= 0 && tileY >= 0 && tileX <= MAP_WIDTH && tileY <= MAP_HEIGHT)
		{
			if (IsRender(GetTile(tileX, tileY)))
			{
				cTileFound = GetTile(tileX, tileY);
			}
		}
	}
	
	*pTileFoundOut = cTileFound;
	return distance;
}

void DrawColumn (int x)
{
	
	double rayAngle = (playerAngle - fov / 2) + ((double)x / (double)ScreenWidth) * fov;
	
	//Perform a ray cast
	char cTileFound = '\0';
	double distToWall = CastRay(rayAngle, &cTileFound);
	
	//if we didn't hit a tile, just show nothing at all
	if (!cTileFound)
	{
		distToWall = 0;
	}
	else
	{
		double rd = playerAngle - rayAngle;
		if (rd < 0)       rd += 2 * PI;
		if (rd >= 2 * PI) rd -= 2 * PI;
		distToWall *= cos(rd); // fish eye effect fix
	}
	
	int ceiling = (int)((double)(ScreenHeight / 2.0) - ScreenHeight / distToWall);
	int floor = ScreenHeight - ceiling;
	
	int project = 2;
	
	uint32_t colors[4] = { 0x000000, 0x0000FF, 0x00FF00, 0xFF0000 };
	
	int endPoints[5];
	endPoints[0] = ceiling;
	endPoints[4] = floor;
	endPoints[1] = ceiling + (floor - ceiling) * 0.25;
	endPoints[2] = ceiling + (floor - ceiling) * 0.5;
	endPoints[3] = ceiling + (floor - ceiling) * 0.75;
	
	VidDrawVLine(0x008080, 0, ceiling, x);
	
	//VidDrawVLine(colors[project], ceiling, floor, x);
	
	for (int i = 0; i < 4; i++)
	{
		VidDrawVLine(colors[i], endPoints[i], endPoints[i + 1], x);
	}
	
	VidDrawVLine(0x808000, floor, ScreenHeight, x);
}



int fc;
void DisplayDebug(int deltaTime)
{
	VidFillRect(0x000000, 0,0,100, 15);
	
	char buf [50];
	sprintf(buf,"DT: %d  FC: %d", deltaTime, fc++);
	
	VidTextOut(buf, 1, 1, 0xFFFFFF, TRANSPARENT);
}
void Init()
{
	LogMsg("Init!");
	fov = PI / (640.0/240.0);    //TODO: adjust FOV depending on the screen width
}
void OnSize(UNUSED int width, UNUSED int height)
{
	//TODO: Why are you stretching out the walls
	LogMsg("Size!");
	fov = PI / (640.0/240.0);
}
void Update (UNUSED int deltaTime)
{
	double mx = cos (playerAngle) / 10;
	double my = sin (playerAngle) / 10;
	if (IsKeyDown(KEY_W))
	{
		playerX += mx;
		playerY += my;
	}
	if (IsKeyDown(KEY_S))
	{
		playerX -= mx;
		playerY -= my;
	}
	if (IsKeyDown(KEY_A))
	{
		playerX += my;
		playerY -= mx;
	}
	if (IsKeyDown(KEY_D))
	{
		playerX -= my;
		playerY += mx;
	}
	if (IsKeyDown(KEY_ARROW_LEFT))
	{
		playerAngle -= 0.03;
		if (playerAngle < 0)
			playerAngle += 2 * PI;
	}
	if (IsKeyDown(KEY_ARROW_RIGHT))
	{
		playerAngle += 0.03;
		if (playerAngle >= 2 * PI)
			playerAngle -= 2 * PI;
	}
}
void Render (int deltaTime)
{
	for (int x = 0; x < ScreenWidth; x++)
		DrawColumn (x);
	
	// show FPS
	DisplayDebug(deltaTime);
}