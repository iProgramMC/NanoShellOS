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

void DrawColumn (int x)
{
	double rayAngle = (playerAngle - fov / 2) + ((double)x / (double)ScreenWidth) * fov;
	double distToWall  = 0.0;
	bool   hitWall = false;
	char   wallHit = '.';
	//bool   boundary = false;
	double eyeX = sin (rayAngle), eyeY = cos (rayAngle);
	double sampleX = 0;
	int    project = 0;
	
	while (!hitWall)
	{
		if (distToWall > farPlane)
		{
			// Too far out!
			distToWall = 100000000;
			break;
		}
		
		//WORK: You can optimize this by jumping only until you reach a new x/y coordinate instead of
		//doing intermediary steps like these.
		distToWall += rayMoveDistance;
		
		int testX = (int)(playerX + eyeX * distToWall);
		int testY = (int)(playerY + eyeY * distToWall);
		char tileGot = GetTile(testX, testY);
		
		if (IsRender(tileGot))
		{
			hitWall = true;
			wallHit = tileGot;
			
			// Lighting goes here.
			double blockMidX = (double)testX + 0.5;
			double blockMidY = (double)testY + 0.5;

			double testPointX = playerX + eyeX * distToWall;
			double testPointY = playerY + eyeY * distToWall;

			float testAngleF = atan2f((float)(testPointY - blockMidY), (float)(testPointX - blockMidX));
			double testAngle = (double)testAngleF;

			if (testAngle >= -PI * 0.25 && testAngle < PI * 0.25)
			{
				sampleX = testPointY - (double)testY;
				project = 4;
			}
			if (testAngle >= PI * 0.25 && testAngle < PI * 0.75)
			{
				sampleX = testPointX - (double)testX;
				project = 3;
			}
			if (testAngle < -PI * 0.25 && testAngle >= -PI * 0.75)
			{
				sampleX = testPointX - (double)testX;
				project = 2;
			}
			if (testAngle >= PI * 0.75 || testAngle < -PI * 0.75)
			{
				sampleX = testPointY - (double)testY;
				project = 1;
			}
		}
	}
	
	double rd = playerAngle - rayAngle;
	if (rd < 0)       rd += 2 * PI;
	if (rd >= 2 * PI) rd -= 2 * PI;
	distToWall *= cos(rd);
	
	int ceiling = (int)((double)(ScreenHeight / 2.0) - ScreenHeight / distToWall);
	int floor = ScreenHeight - ceiling;
	
	uint32_t colors[4] = { 0x000000, 0x0000FF, 0x00FF00, 0xFF0000 };
	VidDrawVLine(0x8080,          0,       ceiling,       x);
	VidDrawVLine(colors[project], ceiling, floor,         x);
	VidDrawVLine(0x808000,        floor,   ScreenHeight, x);
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
	fov = PI / ASPECT_RATIO;
}
void OnSize(UNUSED int width, UNUSED int height)
{
	//TODO: Why are you stretching out the walls
	LogMsg("Size!");
	fov = PI / ASPECT_RATIO;
}
void Update (UNUSED int deltaTime)
{
	double mx = sin (playerAngle) / 10;
	double my = cos (playerAngle) / 10;
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
		playerX -= my;
		playerY += mx;
	}
	if (IsKeyDown(KEY_D))
	{
		playerX += my;
		playerY -= mx;
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