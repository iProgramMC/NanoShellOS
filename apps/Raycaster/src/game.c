/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
           Raycaster application

          Game logic source file
******************************************/

#include <nsstandard.h>
#include "game.h"

//include sprites
#include "sand.h"

#define SCREEN_WIDTH (ScreenWidth * 2 / 4)

#define ASPECT_RATIO ((double)SCREEN_WIDTH / ScreenHeight)

#define TILE_SIZE 12 // for the minimap

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
double CastRay (double rayAngle, char *pTileFoundOut, double *pIntersectionXOut, double *pIntersectionYOut)
{
	double distance  = 0.0;
	
	
	double dirX = cos(rayAngle), dirY = sin(rayAngle);
	//Normalize (&dirX, &dirY);--- no need to normalize, already normalized
	
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
		if (tileX >= 0 && tileY >= 0 && tileX < MAP_WIDTH && tileY < MAP_HEIGHT)
		{
			if (IsRender(GetTile(tileX, tileY)))
			{
				cTileFound = GetTile(tileX, tileY);
			}
		}
	}
	
	*pTileFoundOut = cTileFound;
	
	if (cTileFound)
	{
		//hack: lower the distance so that we don't pierce into the walls)
		*pIntersectionXOut = playerX + dirX * (distance - 0.005);
		*pIntersectionYOut = playerY + dirY * (distance - 0.005);
	}
	
	return distance;
}

uint32_t ColorDarken (uint32_t color, double brightness)
{
	uint32_t result = 0;
	
	result |= (int)(((color >> 16) & 0xFF) * brightness) << 16;
	result |= (int)(((color >>  8) & 0xFF) * brightness) <<  8;
	result |= (int)(((color >>  0) & 0xFF) * brightness) <<  0;
	
	return result;
}

void DrawMmapDot(double x, double y, uint32_t color)
{
	int mmapX = ScreenWidth - MAP_WIDTH * TILE_SIZE, mmapY = ScreenHeight - MAP_HEIGHT * TILE_SIZE;
	
	VidPlotPixel (mmapX + x * TILE_SIZE, mmapY + y * TILE_SIZE, color);	
}

double Trim2 (double d)
{
	double td = Trim(d);
	if (td > 0.5)
		return td - 1;
	return td;
}

//the array's count specifies the resolution
//note. The bigger the texture, the heavier the calculations, so don't make your textures too big
int g_endPoints[33];

void SetupEndPoints(int ceiling, int floor)
{
	int gap =  (floor - ceiling);
	g_endPoints[0] = ceiling;
	g_endPoints[ARRAY_COUNT(g_endPoints)-1] = floor;
	for (size_t i = 1; i < ARRAY_COUNT(g_endPoints) - 1; i++)
	{
		g_endPoints[i] = ceiling + gap * i / ARRAY_COUNT(g_endPoints);
	}
}

void DrawColumn (int x)
{
	
	double rayAngle = (playerAngle - fov / 2) + ((double)x / (double)SCREEN_WIDTH) * fov;
	
	//clamp the ray angle
	rayAngle = fmod(rayAngle, 2*PI);
	
	//Perform a ray cast
	char cTileFound = '\0';
	double intersectX = 0.0, intersectY = 0.0;
	double distToWall = CastRay(rayAngle, &cTileFound, &intersectX, &intersectY);
	
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
	
	// give it a little bit of texturing
	
	VidDrawVLine(0x008080, 0, ceiling, x);
	
	// Determine the progress inside the wall. This will be useful for texturing.
	double texProgress = 0.5;
	
	double shading = 0.7;
	if (Abs(Trim2(intersectX)) <= 0.01) // intersecting on the X axis
	{
		texProgress = Trim(intersectY);
		if (rayAngle > PI/2 && rayAngle < 3*PI/2)
			texProgress = 1.0f - texProgress;
		
		shading = 0.8;
	}
	else if (Abs(Trim2(intersectY)) <= 0.01) // intersecting on the Y axis
	{
		texProgress = Trim(intersectX);
		if (rayAngle < PI)
			texProgress = 1.0f - texProgress;
		
		shading = 1.0f;
	}
	
	//for debugging, draw all the intersections
	DrawMmapDot(intersectX, intersectY, 0xFFFFFF);
	
	//VidDrawVLine(colors[project], ceiling, floor, x);
	SetupEndPoints(ceiling, floor);
	
	int textureHeight = (int)ARRAY_COUNT(g_endPoints) - 1; // the height that's used by this renderer
	
	for (size_t i = 0; i < textureHeight; i++)
	{
		int texX = 32 * texProgress;
		uint32_t pixel;
		
		pixel = g_sand_icon.framebuffer[i * g_sand_icon.width + texX];
		
		VidDrawVLine(ColorDarken(pixel, shading), g_endPoints[i], g_endPoints[i + 1], x);
	}
	
	VidDrawVLine(0x808000, floor, ScreenHeight, x);
}

void DrawMiniMap()
{
	int mmapX = ScreenWidth - MAP_WIDTH * TILE_SIZE, mmapY = ScreenHeight - MAP_HEIGHT * TILE_SIZE;
	
	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{
			char c = GetTile(x, y);
			
			uint32_t color = 0;
			
			if (c == '#') color = 0xFF0000;
			
			VidFillRect(color, mmapX + x * TILE_SIZE, mmapY + y * TILE_SIZE, mmapX + x * TILE_SIZE + TILE_SIZE - 1, mmapY + y * TILE_SIZE + TILE_SIZE - 1);
			
			DrawMmapDot(playerX,playerY,0xFF00);
		}
	}
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
	DrawMiniMap();
	
	for (int x = 0; x < SCREEN_WIDTH; x++)
		DrawColumn (x);
	
	// show FPS
	DisplayDebug(deltaTime);
}